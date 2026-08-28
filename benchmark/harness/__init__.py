"""The measurement harness.

Being rebuilt rather than patched. The scripts in the parent directory have defects
that produce plausible numbers rather than errors, which is the worst kind: netem
applied to the loopback root qdisc so a configured 50 ms delay delivers about 100 ms,
error counts never parsed so a server that fails fast wins on throughput, a CPU figure
that means lifetime-average percent on one platform and cumulative seconds on the
other, and five repetitions sharing one server process so n=5 is really n=1.

Built so far, and self-checked:

    environment.py   what the machine was, and a fingerprint that refuses to let a
                     campaign mix two populations
    validity.py      the pre-declared criteria under which a run is discarded, and the
                     kernel counters that must not move during one
    schema.py        one record per run, every factor a field, appended as JSONL so an
                     interrupted campaign keeps everything that finished
    ordering.py      the run order, walked in passes with systems interleaved so a
                     warming machine does not hand one system the cold half

Not built yet:

    the driver, which ties these together: fresh server and generator process per run
    the netns pair with veth and per-direction netem
    cgroup v2 accounting for CPU and memory
    h2load, wrk2 and k6 invocation and output parsing
    results2tex.py, which turns accepted runs into the thesis' \\R{} keys

Nothing here can produce a citable number on the machine it was written on: WSL
reports itself as virtualised and validity.py refuses it, which is the guard working.
The load generators are not installed either. Both are Phase B concerns, waiting on the
Linux partition.

Run the self-check with:

    python3 benchmark/harness/selfcheck.py
"""
