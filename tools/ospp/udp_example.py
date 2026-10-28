#!/usr/bin/env python3
"""Build an explicit UDP-only variant of the upstream HelloWorld example.

Preserve upstream source files. The generated main changes only the two
participant QoS arguments, removing SHM from both publisher and subscriber.
"""
import argparse
from pathlib import Path
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--source", type=Path, required=True)
parser.add_argument("--prefix", type=Path, required=True)
parser.add_argument("--output", type=Path, required=True)
parser.add_argument("--cxx", default="/usr/bin/g++")
args = parser.parse_args()
example = args.source.resolve() / "example/cpp/hello_world"
prefix = args.prefix.resolve()
output = args.output.resolve()
output.mkdir(parents=True, exist_ok=True)
original = (example / "main.cpp").read_text()
needle = "114, PARTICIPANT_QOS_DEFAULT,"
if original.count(needle) != 2:
    raise SystemExit("Upstream participant setup changed; review UDP-only variant generation")
replacement = """114, [] {
            DomainParticipantQos qos = PARTICIPANT_QOS_DEFAULT;
            qos.transport.removeTransport(TRANSPORT_SHM_QOS);
            return qos;
        }(),"""
generated = output / "main_udp_only.cpp"
generated.write_text("// Generated UDP-only test variant; see tools/ospp/udp_example.py.\n" +
                     original.replace(needle, replacement))
subprocess.run([
    args.cxx, "-std=c++17", "-O2", "-pthread",
    "-I", str(prefix / "include/travodds"), "-I", str(example),
    str(generated), str(example / "HelloWorld.cpp"),
    str(example / "HelloWorldTypeSupport.cpp"),
    "-L", str(prefix / "lib"), f"-Wl,-rpath,{prefix / 'lib'}",
    "-ltravodds", "-o", str(output / "udp_only"),
], check=True)
