"""Records QUIC datagrams on an interface: when, from where, and how big.

Written because tcpdump is not installed and installing it is a system change; AF_PACKET
gives the same bytes. Used to measure f_after independently of the server's own counters,
so it must not lose packets: a lossy capture would understate traffic after a migration
and turn an instrument artefact into a model disagreement. A first version did json.dumps
and a flush per packet and captured 62 of 118 datagrams under a 4 MB download, which is
exactly that failure. So:

  - a large receive buffer, because the default is a few hundred kilobytes and a bulk
    transfer's acknowledgements arrive faster than Python can format them;
  - plain text and batched writes rather than JSON per packet;
  - the kernel's own drop counter read at the end, so a capture that lost packets says so
    instead of being quietly believed.

Output: one line per datagram, "t src_ip:port dst_ip:port len", then a final line
"#dropped N" that the analysis refuses to ignore.
"""
import socket, struct, sys, time

iface, port, seconds, out = sys.argv[1], int(sys.argv[2]), float(sys.argv[3]), sys.argv[4]
# Batched writes mean a killed capture loses everything it has not flushed, and the
# harness deletes the namespace this runs in when the cell ends. So the caller asks it to
# stop by creating this file, and it flushes and records its drop count on the way out.
# Signals are not usable for it: this runs behind `ip netns exec` under sudo, so the pid
# the caller knows is a wrapper's rather than this process's.
stop_file = out + ".stop"
import os
s = socket.socket(socket.AF_PACKET, socket.SOCK_RAW, socket.ntohs(0x0003))
try:
    s.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 64 * 1024 * 1024)
except OSError:
    pass
s.bind((iface, 0))
s.settimeout(0.5)

end = time.time() + seconds
buf = []
handle = open(out, "w", buffering=1 << 20)
while time.time() < end:
    try:
        frame = s.recv(2048)
    except socket.timeout:
        if buf:
            handle.write("".join(buf)); buf.clear()
        if os.path.exists(stop_file):
            break
        continue
    if len(frame) < 34 or frame[12] != 0x08 or frame[13] != 0x00:
        continue
    ip = frame[14:]
    if ip[9] != 17:
        continue
    ihl = (ip[0] & 0x0F) * 4
    sport, dport, ulen = struct.unpack("!HHH", ip[ihl:ihl + 6])
    if port not in (sport, dport):
        continue
    src = f"{ip[12]}.{ip[13]}.{ip[14]}.{ip[15]}:{sport}"
    dst = f"{ip[16]}.{ip[17]}.{ip[18]}.{ip[19]}:{dport}"
    buf.append(f"{time.time():.6f} {src} {dst} {ulen - 8}\n")
    if len(buf) >= 512:
        handle.write("".join(buf)); buf.clear()
        # Also checked here, not only on a receive timeout: any packet on the interface
        # resets the timeout, so a busy link means the timeout branch may never run and
        # the stop request would never be seen.
        if os.path.exists(stop_file):
            break

handle.write("".join(buf))
# PACKET_STATISTICS: (received, dropped) since the last read. A capture that dropped is
# not evidence about how much traffic there was.
# SOL_PACKET (263) and PACKET_STATISTICS (6) are not exposed by Python's socket module,
# so they are given numerically. tpacket_stats is {tp_packets, tp_drops}.
try:
    stats = s.getsockopt(263, 6, 8)
    _, dropped = struct.unpack("II", stats)
except (OSError, AttributeError):
    dropped = -1
handle.write(f"#dropped {dropped}\n")
handle.close()
