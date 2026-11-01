#!/usr/bin/env python3
import os,sys,socket,struct,subprocess,select,json,fcntl,ctypes,time,signal,pathlib
ROOT='/tmp/xhost-20261101'
NS='xh20261101'
TAP='xhtap0'

def command(*args):
    subprocess.run(args,check=True,stdout=sys.stderr,stderr=sys.stderr)

def offloads(name):
    s=socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
    result={}
    for label,code in [('tx_checksum',0x17),('rx_checksum',0x15),('scatter_gather',0x19),('tso',0x1f),('gso',0x24),('gro',0x2c)]:
        data=ctypes.create_string_buffer(struct.pack('II',code,0))
        req=struct.pack('16sP',name.encode(),ctypes.addressof(data)).ljust(40,b'\0')
        try: fcntl.ioctl(s.fileno(),0x8946,req);result[label]='disabled'
        except OSError as e:result[label]=str(e)
    data=ctypes.create_string_buffer(struct.pack('II',0x16,0))
    req=struct.pack('16sP',name.encode(),ctypes.addressof(data)).ljust(40,b'\0')
    fcntl.ioctl(s.fileno(),0x8946,req)
    result['tx_checksum_enabled']=struct.unpack('II',data.raw[:8])[1]
    if result['tx_checksum_enabled']:raise RuntimeError('TX checksum offload remains enabled '+name)
    s.close();return result

def remote():
    os.sched_setaffinity(0,{min(os.sched_getaffinity(0))})
    ip=ROOT+'/ip'
    command(ip,'link','add','v0','type','veth','peer','name','v1')
    off={n:offloads(n) for n in ['v0','v1']}
    for n in ['lo','v0','v1']:command(ip,'link','set',n,'up')
    command(ip,'link','set','v0','mtu','1400')
    command(ip,'link','set','v1','mtu','1400')
    command(ip,'addr','add','10.99.0.2/24','dev','v0')
    command(ip,'route','add','default','dev','v0')
    raw=socket.socket(socket.AF_PACKET,socket.SOCK_RAW,socket.htons(3));raw.bind(('v1',0))
    ctl=socket.socket(socket.AF_UNIX,socket.SOCK_STREAM);ctl.bind(ROOT+'/ctl.sock');ctl.listen(8)
    udp=socket.socket(socket.AF_INET,socket.SOCK_DGRAM);udp.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1);udp.bind(('',39999))
    udp.setsockopt(socket.IPPROTO_IP,socket.IP_ADD_MEMBERSHIP,socket.inet_aton('239.255.0.1')+socket.inet_aton('10.99.0.2'))
    info={'event':'remote_ready','pid':os.getpid(),'netns':os.readlink('/proc/self/ns/net'),'boot_id':pathlib.Path('/proc/sys/kernel/random/boot_id').read_text().strip(),'offloads':off}
    pathlib.Path(ROOT+'/network-ready.json').write_text(json.dumps(info,indent=2)+'\n')
    print(json.dumps(info),file=sys.stderr,flush=True)
    children={};clients={};buffer=b'';frames_in=frames_out=0
    def respond(c,obj):c.sendall((json.dumps(obj)+'\n').encode());c.close()
    def cleanup(*_):raise SystemExit(0)
    signal.signal(signal.SIGTERM,cleanup)
    try:
      while True:
        ready,_,_=select.select([0,raw,ctl,udp,*clients],[],[],1)
        for item in ready:
          if item==0:
            data=os.read(0,1048576)
            if not data:return
            buffer+=data
            while len(buffer)>=2:
              size=struct.unpack('!H',buffer[:2])[0]
              if len(buffer)<2+size:break
              raw.send(buffer[2:2+size]);buffer=buffer[2+size:];frames_in+=1
          elif item is raw:
            frame,addr=raw.recvfrom(65535)
            if addr[2]==socket.PACKET_OUTGOING:continue
            os.write(1,struct.pack('!H',len(frame))+frame);frames_out+=1
          elif item is udp:
            data,addr=udp.recvfrom(65535);udp.sendto(b'XHOST_ECHO:'+data,addr)
          elif item is ctl:
            c,_=ctl.accept();clients[c]=b''
          else:
            c=item;data=c.recv(65536)
            if not data:c.close();del clients[c];continue
            clients[c]+=data
            if b'\n' not in clients[c]:continue
            data=clients.pop(c).split(b'\n',1)[0]
            try:
              q=json.loads(data);op=q['op']
              if op=='ping':out={**info,'frames_in':frames_in,'frames_out':frames_out}
              elif op=='start':
                args=q['args'];name=q['name']
                if not(name.replace('-','').replace('_','').isalnum() and len(args)==8 and args[0] in ['writer','reader']):raise ValueError('invalid task arguments')
                log=open(ROOT+'/'+name+'.log','wb')
                child=subprocess.Popen([ROOT+'/xhost',*map(str,args)],stdout=log,stderr=subprocess.STDOUT,start_new_session=True);log.close()
                children[child.pid]={'p':child,'name':name};out={'pid':child.pid,'name':name}
              elif op=='poll':
                child=children[int(q['pid'])];out={'pid':int(q['pid']),'name':child['name'],'returncode':child['p'].poll()}
                if out['returncode'] is not None:out['log_text']=pathlib.Path(ROOT+'/'+child['name']+'.log').read_text(errors='replace')
              elif op=='list':out={'children':[{'pid':k,'name':v['name'],'returncode':v['p'].poll()} for k,v in children.items()]}
              elif op=='stop':
                child=children[int(q['pid'])]['p']
                if child.poll() is None:child.terminate()
                out={'pid':child.pid,'returncode':child.poll()}
              else:raise ValueError('unknown operation')
              respond(c,{'ok':True,**out})
            except Exception as e:respond(c,{'ok':False,'error':repr(e)})
    finally:
      for v in children.values():
        if v['p'].poll() is None:v['p'].terminate()
      for v in children.values():
        try:v['p'].wait(timeout=4)
        except subprocess.TimeoutExpired:v['p'].kill();v['p'].wait()
      pathlib.Path(ROOT+'/network-final.json').write_text(json.dumps({'pid':os.getpid(),'frames_in':frames_in,'frames_out':frames_out,'children':[{'pid':k,'name':v['name'],'returncode':v['p'].returncode} for k,v in children.items()]},indent=2)+'\n')
      ctl.close();raw.close();udp.close()
      try:os.unlink(ROOT+'/ctl.sock')
      except FileNotFoundError:pass

def local(out):
    out=pathlib.Path(out);tap=os.open('/dev/net/tun',os.O_RDWR)
    fcntl.ioctl(tap,0x400454ca,struct.pack('16sH',TAP.encode(),0x1002))
    command('ip','netns','add',NS);command('ip','link','set',TAP,'netns',NS)
    for args in [('link','set','lo','up'),('link','set',TAP,'mtu','1400','up'),('addr','add','10.99.0.1/24','dev',TAP),('route','add','default','dev',TAP)]:command('ip','netns','exec',NS,'ip',*args)
    err=open(out/'ssh-bridge.stderr.log','wb')
    cmd=['runuser','-u','ubuntu','--','ssh','-o','BatchMode=yes','-o','ConnectTimeout=10','-o','ServerAliveInterval=10','-o','ServerAliveCountMax=2','openlet-eval4090','nice','-n','19','unshare','-Urn','env','LD_LIBRARY_PATH='+ROOT+'/ip-lib', 'python3',ROOT+'/ssh_l2_bridge.py','remote']
    ssh=subprocess.Popen(cmd,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=err)
    state={'pid':os.getpid(),'ssh_pid':ssh.pid,'netns':NS,'tap':TAP,'ssh_command':cmd}
    (out/'network-local.json').write_text(json.dumps(state,indent=2)+'\n')
    def cleanup(*_):raise SystemExit(0)
    signal.signal(signal.SIGTERM,cleanup);buffer=b'';frames_in=frames_out=0
    try:
      while True:
        ready,_,_=select.select([tap,ssh.stdout],[],[],1)
        if tap in ready:
          frame=os.read(tap,65535);ssh.stdin.write(struct.pack('!H',len(frame))+frame);ssh.stdin.flush();frames_out+=1
        if ssh.stdout in ready:
          data=os.read(ssh.stdout.fileno(),1048576)
          if not data:break
          buffer+=data
          while len(buffer)>=2:
            size=struct.unpack('!H',buffer[:2])[0]
            if len(buffer)<size+2:break
            os.write(tap,buffer[2:size+2]);buffer=buffer[size+2:];frames_in+=1
    finally:
      ssh.stdin.close()
      try:ssh.wait(timeout=5)
      except subprocess.TimeoutExpired:ssh.terminate();ssh.wait(timeout=5)
      os.close(tap);err.close()
      command('ip','netns','delete',NS)
      (out/'network-local-final.json').write_text(json.dumps({**state,'ssh_returncode':ssh.returncode,'frames_in':frames_in,'frames_out':frames_out},indent=2)+'\n')

if __name__=='__main__':
    if sys.argv[1]=='remote':remote()
    elif sys.argv[1]=='local':local(sys.argv[2])
