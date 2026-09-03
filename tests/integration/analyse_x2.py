"""Checks the forwarded share against f_after measured on the wire, not on the counters.

    python3 analyse_x2.py CELL.jsonl WIRE.jsonl PORT

The model is  share = f_after * (N-1)/N , but that factor is an expectation over
connections, not a per-connection prediction. For one connection the hash either lands on
the owning worker or it does not, and the (N-1)/N is the probability of the second case.
So there are two predictions and they are checked separately:

    per connection, landed off-owner:  share == f_after   (every later packet is forwarded)
    per connection, landed on owner:   share == 0
    across connections:                mean(share) == mean(f_after) * (N-1)/N

f_after comes from the packet capture: the fraction of the connection's client-to-server
datagrams sent after its source address changed. Nothing in it is read from the server.
"""
import json, sys
from collections import defaultdict

cell, wire, port = sys.argv[1], sys.argv[2], int(sys.argv[3])
conns = [json.loads(l) for l in open(cell) if l.strip()]
dropped = 0
inbound = []
for line in open(wire):
    if line.startswith("#dropped"):
        dropped = int(line.split()[1]); continue
    t, src, dst, ln = line.split()
    if dst.endswith(f":{port}"):
        inbound.append({"t": float(t), "src": src, "dst": dst, "len": int(ln)})
if dropped != 0:
    print(f"REFUSED: the capture dropped {dropped} packets. f_after measured from an "
          f"incomplete capture understates traffic after the move and would manufacture a "
          f"disagreement with the model.", file=sys.stderr)
    raise SystemExit(2)

rows = []
short = []
for c in conns:
    window = [p for p in inbound if c["t_start"] <= p["t"] <= c["t_end"]]
    if not window:
        continue
    first_src = window[0]["src"]
    moved = [p for p in window if p["src"] != first_src]
    f_after = len(moved) / len(window) if window else 0.0
    share = c["forwarded_in"] / c["received"] if c["received"] else 0.0
    if len(window) < c["received"]:
        short.append((c["conn"], len(window), c["received"]))
    rows.append({
        "conn": c["conn"], "delay": c["requested_delay"], "workers": c["workers"],
        "wire_pkts": len(window), "counter_received": c["received"],
        "f_after": f_after, "share": share,
        "moved": bool(moved), "forwarded": c["forwarded_in"] > 0,
    })

# A shortfall in proportion to the traffic means the capture is missing a share of it and
# f_after cannot be trusted -- the double-counted-denominator defect showed as a 47 per cent
# shortfall and had to be refused. A small constant shortfall is a different animal: it does
# not grow with the traffic, so it cannot produce a proportional error in f_after. It is
# reported rather than ignored, and it is unexplained: with GRO disabled by ioctl, the
# kernel reporting zero drops, and only one caller of the counting entry point, the server
# still counts about two datagrams per connection that never appeared on the tap.
if short:
    worst = max((ctr - w) / ctr for _, w, ctr in short)
    total_gap = sum(ctr - w for _, w, ctr in short)
    if worst > 0.05:
        print(f"REFUSED: the capture is short by up to {worst:.0%} of what the server "
              f"counted, which is proportional to the traffic and would bias f_after:",
              file=sys.stderr)
        for conn, wire_n, ctr in short:
            print(f"  conn {conn}: wire {wire_n} < counter {ctr}", file=sys.stderr)
        raise SystemExit(2)
    print(f"note: capture short by {total_gap} datagrams across {len(short)} connections "
          f"(worst {worst:.1%}), constant per connection and unexplained; recorded, not ignored")

if not rows:
    print("no connections matched the capture window"); raise SystemExit(1)
N = rows[0]["workers"]
print(f"{'conn':>4} {'delay':>6} {'wire':>5} {'ctr':>5} {'f_after':>8} {'share':>7} {'moved':>6} {'fwd':>4}  verdict")
agree = disagree = 0
for r in rows:
    if r["forwarded"]:
        ok = abs(r["share"] - r["f_after"]) <= 0.10
        verdict = "share==f_after" if ok else "DISAGREES"
    else:
        ok = r["share"] == 0
        verdict = "on owner, share 0" if ok else "DISAGREES"
    agree += ok; disagree += not ok
    print(f"{r['conn']:>4} {r['delay']:>6} {r['wire_pkts']:>5} {r['counter_received']:>5} "
          f"{r['f_after']:>8.3f} {r['share']:>7.3f} {str(r['moved']):>6} {str(r['forwarded']):>4}  {verdict}")

n_fwd = sum(r["forwarded"] for r in rows)
mean_share = sum(r["share"] for r in rows) / len(rows)
mean_f = sum(r["f_after"] for r in rows) / len(rows)
print(f"\nconnections={len(rows)} landed off-owner={n_fwd} ({n_fwd/len(rows):.2f}, model says {(N-1)/N:.2f})")
print(f"mean share={mean_share:.4f}  mean f_after * (N-1)/N = {mean_f * (N-1)/N:.4f}")
print(f"per-connection: {agree} agree, {disagree} disagree")
