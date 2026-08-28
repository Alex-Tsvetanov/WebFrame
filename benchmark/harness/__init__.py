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

Not built yet:

    the run schema and its Parquet writer
    the driver: fresh server and generator process per run, randomised order
    the netns pair with veth and per-direction netem
    cgroup v2 accounting for CPU and memory
    h2load, wrk2 and k6 invocation and output parsing
    results2tex.py, which turns accepted runs into the thesis' \\R{} keys

Run the self-check with:

    python3 benchmark/harness/selfcheck.py
"""
