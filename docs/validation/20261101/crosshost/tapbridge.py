#!/usr/bin/env python3
"""Carry Ethernet frames between two hosts over one TCP stream.

Usage: tapbridge.py listen|connect HOST PORT TAPNAME NETNS CIDR
Opens a TAP device in the current namespace, moves it into NETNS with only lo
and the TAP, then forwards length-prefixed frames. Multicast and broadcast
travel as ordinary frames, so RTPS discovery works without product changes.
"""
import fcntl
import os
import select
import socket
import struct
import subprocess
import sys

TUNSETIFF = 0x400454ca
IFF_TAP, IFF_NO_PI = 0x0002, 0x1000


def run(*argv):
    subprocess.run(argv, check=True)


role, host, port, tap_name, netns, cidr = sys.argv[1:7]
tap = os.open("/dev/net/tun", os.O_RDWR)
fcntl.ioctl(tap, TUNSETIFF, struct.pack("16sH", tap_name.encode(), IFF_TAP | IFF_NO_PI))
run("ip", "netns", "add", netns)
run("ip", "link", "set", tap_name, "netns", netns)
for args in (["link", "set", "lo", "up"],
             ["link", "set", tap_name, "mtu", "1400", "up"],
             ["addr", "add", cidr, "dev", tap_name],
             ["route", "add", "default", "dev", tap_name]):
    run("ip", "netns", "exec", netns, "ip", *args)

if role == "listen":
    server = socket.socket()
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind((host, int(port)))
    server.listen(1)
    print("listening", flush=True)
    stream, _ = server.accept()
else:
    stream = socket.create_connection((host, int(port)), timeout=30)
    stream.settimeout(None)
stream.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
print("connected", flush=True)

buffered = b""
while True:
    ready, _, _ = select.select([tap, stream], [], [])
    if tap in ready:
        frame = os.read(tap, 65535)
        stream.sendall(struct.pack("!H", len(frame)) + frame)
    if stream in ready:
        data = stream.recv(1 << 20)
        if not data:
            break
        buffered += data
        while len(buffered) >= 2:
            size = struct.unpack("!H", buffered[:2])[0]
            if len(buffered) < 2 + size:
                break
            os.write(tap, buffered[2:2 + size])
            buffered = buffered[2 + size:]
