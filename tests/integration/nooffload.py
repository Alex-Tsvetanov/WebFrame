"""Turns generic receive offload off on an interface, without ethtool.

    python3 nooffload.py IFACE

GRO merges datagrams before an AF_PACKET tap sees them, so a capture reports one frame
where the server received several. Capping gro_max_size through `ip link` removes the
large merges but not merges of small packets, which for a QUIC connection means the
acknowledgements -- and those are most of the inbound traffic during a download. The
residue was one to two datagrams per connection, which is small until it is the thing
being measured.

ethtool is not installed here and installing it would be a system change, so this issues
the same ioctl ethtool would: SIOCETHTOOL with the legacy ETHTOOL_SGRO command, whose
argument is a struct ethtool_value {u32 cmd; u32 data;}. Applied inside the test's own
network namespace, on an interface deleted with it.
"""
import array, fcntl, socket, struct, sys

SIOCETHTOOL = 0x8946
ETHTOOL_SGRO = 0x0000002C

iface = sys.argv[1]
value = array.array("I", [ETHTOOL_SGRO, 0])          # 0 = off
addr, _ = value.buffer_info()
ifreq = struct.pack("16sP", iface.encode()[:15], addr)
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
try:
    fcntl.ioctl(s.fileno(), SIOCETHTOOL, ifreq)
    print(f"gro off on {iface}")
except OSError as exc:
    print(f"could not turn gro off on {iface}: {exc}", file=sys.stderr)
    raise SystemExit(1)
