# Campaign runbook

How a measurement night is run, in the order it has to happen. Everything here is one
command per step so that a window that opens at midnight is spent measuring and not
deciding. The method behind each rule is in `doc/thesis/chapters/05_methodology.tex`;
the reasons behind each design are in the docstrings of `run_campaign.py`,
`run_routing.py` and `run_routing_e2e.py`.

## Preconditions, all of them, before anything is timed

| Check | How | Refuse if |
| --- | --- | --- |
| Host idle | close the editor, the game, the browser, the agent session's own busy work | anything else is using CPU |
| Mains power | `pmset -g ps` / `/sys/class/power_supply` / `Win32_Battery` | on battery |
| Clean tree at the commit you will cite | `git status -sb` | any modified or untracked source file |
| Release build of that commit | `cmake --build build/<preset> --config Release`; the only preset that also builds `route_bench` and the router arms is `bench` (`cmake --preset bench && cmake --build --preset bench` gives `build/bench`) | binary older than HEAD |
| TLS material | `python -m benchmark.make_cert` once; produces `benchmark/certs/bench.{crt,key}` | missing and a TLS design is planned |
| Off-host generator | Windows: `wsl -d Ubuntu-24.04 -- ls -la /home/alex/loadgen`; Linux: the netns pair, entered with `--generator-command "sudo -n ip netns exec gen runuser -u $USER --" --generator-location netns:gen` | not built, or reaches the server over loopback |
| Off-host generator at the same commit | rebuild it from the cited commit before the night: `wsl -d Ubuntu-24.04 -- cmake --build <its build dir>`; then check the smoke record's `requests_total_whole_run` is a number | `requests_total_whole_run` is null, or the two binaries are from different commits: `bytes_per_second` and the whole-run count are defined by the generator, and a night with two generator builds is a night with two definitions of both |
| Fresh results directory | `benchmark/results/<yyyy-mm-dd>-<host>/` | appending to a file whose `.env.json` fingerprint differs; the driver refuses this itself |

The generator inside WSL reaches the Windows host at the vEthernet address, which changes
between reboots. Read it from inside the distribution rather than from an old note:

```powershell
$wslHost = ([regex]::Match((wsl -d Ubuntu-24.04 -- ip route show default) -join ' ', '\bvia\s+(\d{1,3}(?:\.\d{1,3}){3})\b')).Groups[1].Value
if (-not $wslHost) { throw "could not resolve the WSL default gateway" }
```

No nested shell, so nothing expands before the pattern sees it, and a route line without
a `via` address stops loudly instead of handing `--host` an interface name. If the output
ever looks space-separated per character, that is `wsl.exe` emitting UTF-16 for its own
subcommands; a command run inside the distribution should not do that.

That address is `--host` for every off-host design below. If the WSL generator is
missing or older than the commit you will cite, build it inside the distribution from
`benchmark/generator` with CMake. It is a Linux binary and lives in the WSL filesystem,
so a search of the Windows drives will not find it.

## The quiet-host gate is measured first, and the night stops if it fails

```
python -m benchmark.run_campaign --design smoke --repetitions 1 --build build/windows-tls --results benchmark/results/<dir>/smoke.jsonl
```

Read `generator_pacing_p99_us` in the two records. On this host it is about 40
microseconds when the machine is idle and about 2000 with an editor and an agent session
open. **If it is not in the tens of microseconds, stop.** Nothing measured afterwards
would be admissible, and the gate would refuse it only after the hours were spent.

## Ladders: re-run them whenever the binary or the host changed

The offered rates hard-coded in `run_campaign.py` (`TLS_OFFERED_RATES`,
`CHURN_OFFERED_RATES`, `CHURN_NET_OFFERED_RATES`) were measured on `alex-pc` at an
earlier commit. They are this host's numbers and nobody else's, and a rebuilt binary can
move them. The three ladders cost about eight minutes together and turn a guess into a
measurement:

```
python -m benchmark.run_campaign --design tls-smoke    --repetitions 1 --build build/windows-tls --results benchmark/results/<dir>/tls-smoke.jsonl
python -m benchmark.run_campaign --design tls-ladder   --repetitions 1 --build build/windows-tls --results benchmark/results/<dir>/tls-ladder.jsonl
python -m benchmark.run_campaign --design churn-ladder --repetitions 1 --build build/windows-tls --results benchmark/results/<dir>/churn-ladder.jsonl
python -m benchmark.run_campaign --design churn-ladder --repetitions 1 --build build/windows-tls --results benchmark/results/<dir>/churn-ladder-net.jsonl --wsl-distro Ubuntu-24.04 --wsl-loadgen /home/alex/loadgen --host <vEthernet address>
```

If the admissible boundary moved, change the rate tables in `run_campaign.py`, commit
that change on its own before the campaigns, and cite the new commit. A campaign run at
rates the ladder no longer supports is refused by `validity.py` one run at a time.

## Night 1: the socket-demultiplexing claim, establishment first

Each run is about 26 seconds (20 measured, 3 warmup, 3 process turnover). Twenty-five
repetitions is what the X1 resolution analysis found necessary; seven was not enough to
resolve the 5 percent threshold at four of five rates.

| Order | Design | Cells | n | Machine time | Why this order |
| --- | --- | --- | --- | --- | --- |
| 1 | `churn` | 16 | 25 | 2.9 h | the cell where the hypothesis can fail |
| 2 | `churn-net` (WSL) | 16 | 25 | 2.9 h | the same cell across a network interface |
| 3 | `transport` | 20 | 25 | 3.6 h | the headline four-arm comparison |
| 4 | `h1-deep` | 10 | 25 | 1.8 h | the cleartext X1 table chapter VI quotes |

```
python -m benchmark.run_campaign --design churn     --repetitions 25 --build build/windows-tls --results benchmark/results/<dir>/churn.jsonl
python -m benchmark.run_campaign --design churn-net --repetitions 25 --build build/windows-tls --results benchmark/results/<dir>/churn-net.jsonl --wsl-distro Ubuntu-24.04 --wsl-loadgen /home/alex/loadgen --host <vEthernet address>
python -m benchmark.run_campaign --design transport --repetitions 25 --build build/windows-tls --results benchmark/results/<dir>/transport.jsonl
python -m benchmark.run_campaign --design h1-deep   --repetitions 25 --build build/windows-tls --results benchmark/results/<dir>/h1-deep.jsonl
```

That is eleven hours. If the window is shorter, run them in this order and stop at the
boundary between designs; a finished design is citable, half of one is not. Loopback and
WSL arrangements go in separate files and are never merged; the driver records
`transport_path` in the environment so nothing downstream can merge them by accident.

## Night 2: routing, dispatch level then end to end

Dispatch-only starts no server and needs no network. It is x86-only because it times with
`rdtsc`. The 10 000-route DFA cells are split off because the parameterised table does
not fit in this host's memory and a run that pages would contaminate whatever the shuffle
placed after it. `build/windows-routing` is the tree alex-pc already has with the router arms
compiled in; on Linux and macOS the equivalent is `build/bench` from the `bench` preset.

```
python -m benchmark.run_routing --design main        --repetitions 5 --build build/windows-routing --results benchmark/results/<dir>/routing
python -m benchmark.run_routing --design depth       --repetitions 5 --build build/windows-routing --results benchmark/results/<dir>/routing
python -m benchmark.run_routing --design static      --repetitions 5 --build build/windows-routing --results benchmark/results/<dir>/routing
python -m benchmark.run_routing --design large-cheap --repetitions 5 --build build/windows-routing --results benchmark/results/<dir>/routing
python -m benchmark.run_routing --design large-dfa   --repetitions 1 --build build/windows-routing --results benchmark/results/<dir>/routing
```

End to end runs one worker (the DFA matcher's repeat counters are shared and not
thread-safe upstream, stated as a limitation) and refuses a loopback `--host`:

```
python -m benchmark.run_routing_e2e --design main        --repetitions 5 --build build/windows-routing --results benchmark/results/<dir>/routing-e2e --wsl-distro Ubuntu-24.04 --wsl-loadgen /home/alex/loadgen --host <vEthernet address>
python -m benchmark.run_routing_e2e --design bracket     --repetitions 5 ... same generator flags
python -m benchmark.run_routing_e2e --design bracket-low --repetitions 5 ... same generator flags
python -m benchmark.run_routing_e2e --design large       --repetitions 5 ... same generator flags --readiness-timeout 600
```

About two hours in total. The `large` design has never completed: the 10 000-route DFA
table did not come up inside the default 180-second readiness timeout, and the runs were
rejected for delivering nothing. Whether a longer timeout is a legitimate fix or a thumb
on the scale is decided before the run, not after; the flag is there so the decision is
visible in the command line.

## After the night

1. Do not read the numbers first. Count rejections: `grep -c '"accepted": false'` per
   file, and read every rejection reason. A campaign that rejected a third of its runs is
   a different campaign.
2. Commit the results directory, including every `.env.json`, on a branch named
   `measure/<host>-<yyyy-mm-dd>`, force-adding if the directory is ignored, and push.
   The coordinator moves the files into the paper repository's `measurements/` with a
   README entry that names the machine, commit, command and what the table does not show.
3. Tables and scalars for the thesis are generated, never typed:
   `python -m benchmark.harness.results2csv <runs.jsonl> doc/thesis/data` and
   `python -m benchmark.harness.results2tex <runs.jsonl> doc/thesis/generated/results.tex`.

## Structural counts need no quiet host

`descriptor_census.py` counts listening descriptors, event ports and threads by asking
the kernel. Every column is a count and none is a clock, so battery, governor and load do
not enter into it. It may run any time:

```
python -m benchmark.descriptor_census build/windows-tls --workers 1,2,4,8
```

## What this host cannot measure

Windows has no `strace` and no per-process syscall counter the harness can read, so
`counter_deltas` is empty on every Windows record and the syscalls-per-request
explanation the socket-demultiplexing and I/O-portability papers rest on has to come from
Linux. The Windows host also has no thermal-throttle gate; the frequency-drift gate covers
part of that. Both are stated in the results, not discovered by a reviewer.
