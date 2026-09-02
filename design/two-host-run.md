# The two-host run: the design, fixed before the first run

Both papers state loopback as a limitation. One measurement can replace that sentence with a
number or show that loopback was the right medium: the framework's own load generator against the
framework's own benchmark server across one gigabit Ethernet segment between two hosts, a Linux
laptop and a Windows desktop, instead of over loopback, a namespace pair or a virtual switch
inside one host. Alex approved it on 2 September for an overnight window, on the condition that
the design is pre-declared and committed before it runs.

This is that design. The laptop and the desktop execute it from this text. Every value the text
cannot carry (addresses, the user name, paths on each host, the wired interface's name) is a
placeholder in angle brackets, exchanged by message and recorded, if at all, in the private paper
repository. Nothing that identifies a machine beyond its hardware class and operating system
appears here. Facts about the link and the hosts cited below are from the coordinator's record of
2 September and are not re-measured by this run.

## 1. The one question, and the rule that answers it

**The question.** Paper 2's demultiplexing cost is 60 to 85 microseconds on a classification-off
median of about 1.4 ms establishment or 0.09 ms request latency, measured over loopback. This run
answers one question: can a real path resolve a difference of that size? Not whether it
reproduces the number.

**The quantity.** What decides resolution is not the per-packet spread of the path. We compare
per-run medians, and the standard error of a median over the hundred thousand requests of one run
is a few microseconds if the noise is independent; what enters the comparison is the run-to-run
spread of those medians. The harness already reports it. `stats.compare`
(`benchmark/harness/stats.py`) bootstraps the difference of medians between the two arms at 95
percent, and `results2tex` reports the half-width of that interval as a share of the
classification-off median (`benchmark/harness/results2tex.py`, `hypothesis_x1`), with 5 percent
as the reportability floor (`stats.MIN_RELATIVE_DIFFERENCE`). Call that share **r**. It is the
resolution: how small a difference the cell could have seen. The rule below is stated on r and on
nothing else. No ping statistic enters it. The Linux end's ping to the gateway on the same wire
(median 722 microseconds, 502 with cores held awake, standard deviation about 300, an upper bound
because a router answers ICMP on a low-priority path) is a fact about the path, not about r.

**Per cell.** A cell is one offered rate, one transport (cleartext or TLS) and one shape
(keep-alive, or one request per connection). Its two arms are classification off, the baseline,
and classification on. The statistic is the per-run median: `connect_ms.p50` for establishment,
`latency_ms.p50` for request latency. r is computed from accepted runs only, per arm, by
`stats.compare` applied per full cell. `hypothesis_x1` as written keys cells by offered rate
alone, which would pool TLS with cleartext at one rate, and reads the per-run p99 by default; it
is called per full cell with `p50`, and the same figure on p99 is reported beside, never in place
of, the rule's. A cell with fewer than three
accepted runs in either arm has no interval, is UNRESOLVED by construction, and its counts are
reported.

**The cells the claim rests on.** Establishment at 100 and 150 connections a second, cleartext and
TLS: four cells. Request latency under keep-alive at 5 000, 10 000, 15 000, 25 000 and 35 000 a
second, cleartext and TLS: ten cells. Establishment at 25 and 50 a second is in the design and
reported but is diagnostic only (section 3.1) and does not enter the rule.

**The rule.**

1. A cell RESOLVES if r is under 5 percent at the repetition count taken.
2. The arrangement carries the demultiplexing claim for establishment if all four establishment
   cells at 100 and 150 resolve, and for request latency if all ten request-latency cells
   resolve. The two halves are judged separately, and a half that resolves is written up as
   resolved whatever the other did.
3. The verdict is on r, never on the sign or size of the difference. r is a function of spread;
   reading it does not choose an outcome.
4. Where a cell resolves, the difference is reported as it comes: an interval excluding zero is a
   measured cost of classification over a real path; an interval containing zero is a null result
   at resolution r.
5. Beside every cell: r, the relative difference, whether the interval excludes zero, the
   run-to-run coefficient of variation of the medians per arm, the accepted count per arm, the
   pacing p99 covariate per arm, and the median against repetition index, which is where
   autocorrelated network noise or thermal drift would show as structure.

**If the cells resolve, this is written:** over one gigabit Ethernet segment between two hosts,
the demultiplexing difference was resolved to r in every cell the claim rests on, and its value
there is whatever was measured, interval included. The loopback caveat is replaced by this
measurement in both papers. The loopback figures stand as the figures for the medium in which the
claim was first made; they are not revised by it.

**If they do not resolve, this is written:** over a real path the run-to-run spread of per-run
medians is r in the cells the claim rests on, and at this n the design could not have seen a
difference of 60 to 85 microseconds there. Loopback was the right medium for the demultiplexing
claim: the claim is a difference between two arms that share one path, the medium adds spread and
no information about the arms, and loopback's own r for the same cells was under 5 percent. That
last clause is a condition, not a flourish. The sentence is written for a cell only where the
filed loopback campaign's r, recomputed from its first n accepted repetitions in file order, was
under 5 percent; where loopback did not resolve the cell either, neither medium did and the text
says so. The real path is then named as the right medium for the questions in which the path is a
factor rather than noise: the establishment ceiling of the arrangement, which loopback set at
about 330 a second with the generator sharing the server's cores and the virtual-switch arm moved
past 800; the idle-state mediation of low-load latency at the receiving end, which a generator
sharing the host masks; the tail, p99 and above, under a real driver and interrupt path; and
paper 3's shape comparison of epoll against io_uring over a real path (direction B).

Any other pattern is reported per cell, and no sentence is written that the pattern does not
support. Two caveats apply to either outcome and are reported, not adjudicated: network noise
autocorrelated over a run's length collapses the effective sample count toward the number of
runs, which the repetition plot shows as structure; and variation correlated with the arm rather
than added to it biases the comparison at any n, which the interleaved shuffle addresses for
time-correlated variation and nothing addresses for arm-correlated variation.

## 2. Arrangement: direction A

### 2.1 Who does what

- **The Windows desktop serves.** `benchmark_server` from `build/windows-tls`, IOCP, four
  workers, affinity `0ff` as in every filed desktop campaign
  (`run_campaign._WINDOWS_SERVER_AFFINITY`). The four logical CPUs that hosted the generator on
  loopback sit idle. That is a difference from the loopback arrangement and is stated.
- **The Linux laptop generates.** `loadgen` built from the same commit, two threads, sixty-four
  connections, no affinity mask: a mask names the driver host's cores, and the WSL branch
  already drops it for the same reason.
- **The driver runs on the desktop**, where the server runs, and reaches the generator over ssh.
  With the driver beside the server, every host gate (clock drift, power, governor,
  virtualisation, dirty tree) and the fingerprint describe the server host, which is the host
  whose throttling and idle state matter, and the server adapter is unchanged. The other
  placement, driver on the laptop with the server remote, would have the Windows server adapter
  rewritten in five places (the held-port check runs `ss`, readiness runs the local interpreter,
  pid discovery walks `/proc`, stop signals with `sudo -n kill`, and a server under a prefix must
  report a uid Windows has no notion of) and would leave the server host's gates unread. Same
  rule for direction B: the driver runs where the server runs.

### 2.2 The commit both hosts build

The desktop's 2 September campaign is at `b4a01e8c7`: schema 7, IOCP with no `TCP_NODELAY`,
Nagle on. Mainline from `a4519ada2` has the shared socket policy (Nagle off on every backend,
`TCP_QUICKACK` where the platform has it, `SO_SNDBUF` 256 KiB; `src/net/socket_options.cpp`) and
schema 8, which added the interface fields.

**Steer, open to challenge: the desktop is rebuilt at mainline for this run.** Two reasons; the
second is sufficient alone. Nagle on at the server over a real path is a known confound: a small
response can wait on an acknowledgement and the measurement becomes one of Nagle, while the
generator has had Nagle off throughout (`loadgen.cpp`, `connect_to`). And the interface field does
not exist in a schema 7 record, so the dual-homing check of 2.4 cannot be made by the harness at
all without the rebuild.

The cost is that the two-host cells are a separate population from paper 2's loopback data at
`b4a01e8c7`. They are a separate population anyway: the harness refuses to pool records whose
`transport_path` differs (`run_campaign.transport_mismatch`), and a rebuild is a campaign boundary
that changes the fingerprint. Nothing measured here is pooled with anything filed.

**The single commit.** Both hosts build one commit, `<commit>` below. It must contain: schema 8
or later with the interface fields; the shared socket policy; the pacing-as-covariate change
(schema 9, on `harness/pacing-covariate`, not on mainline as this file is committed; in this tree
`validity.py` still refuses a pacing p99 above 1 ms, which is not the admission rule of section
4.1); and the ssh launcher of 2.3. If any of the four is missing from `<commit>`, the run does
not start. There is no fallback to `b4a01e8c7`: a run with Nagle on, without the interface field
and under the old admission rule is a different campaign, and this document does not pre-declare
it.

### 2.3 The launch

The harness launches the generator locally, inside WSL, or under a free-form prefix that shares
the driver's filesystem. None of those reaches another host: the generator's binary path and its
`--out` result path are the driver host's, and the affinity mask names the driver host's cores.
The change this run needs mirrors the WSL branch and is small:

- `--ssh-generator <user>@<generator-address>`, `--ssh-loadgen <loadgen>`, `--ssh-repo <repo>`,
  with the existing `--generator-location lan:linux`. The launcher is
  `ssh -o BatchMode=yes -o ConnectTimeout=5 <target> <loadgen>` followed by the harness's own
  generator arguments; `BatchMode` makes a missing key a refusal rather than a prompt, for the
  reason `netns.py` uses `sudo -n`. No affinity mask. The result JSON is read from the
  generator's stdout, where the generator already writes it, rather than from a file. `--samples`
  is refused in this mode: the histogram percentiles in the record are the data.
- Once per campaign, before the first run, the driver captures the generator host's environment
  over ssh with the same `environment.capture()` the fingerprint uses and stores it in the
  manifest as `generator_environment`, with `tc qdisc show dev <iface>` beside it. It refuses to
  start if that capture's `build.git_commit` is not `<commit>`, because the staleness gate cannot
  see a remote binary.

Every record then carries `generator = coroute-loadgen-lan` and a `transport_path` of
`loopback=false, generator_location=lan:linux, server_location=host, netem_profile=none`, which no
filed campaign shares. Pooling is refused by the harness, not by discipline. `netem_profile=none`
is truthful and incomplete: it says no impairment was applied and records no qdisc, which is why
the qdisc is captured beside `generator_environment`.

### 2.4 `--expect-interface` on the generator end

The Linux laptop is dual-homed, wired and wireless on one subnet with only a route metric between
them. The generator reads the interface its established socket actually used (`getsockname`,
matched to an interface name; speed, duplex and MTU from `/sys/class/net`) and writes it into the
record, and the driver refuses a run whose interface is absent or not the expected one
(`driver.py`, `expect_interface`). Every command in this design passes `--expect-interface
<iface>`, the wired interface's name. A refusal is a rejected record, not an abort, so the smoke
run is read before anything else runs: a wrong route metric would otherwise cost 26 seconds per
refused run all night.

### 2.5 Preconditions, checked by command

`<repo>` is the checkout on the laptop, `<loadgen>` its generator binary, `<iface>` its wired
interface (from `ip -br link`; a name, not an address), `<server-address>` the desktop's wired
address as the laptop sees it, `<ssh-target>` `<user>@<generator-address>`, `<clocklog>` a file
on the laptop outside `<repo>`, `<dir>` a directory on the desktop outside the framework checkout
(so the tree stays clean), for instance the private paper repository's
`measurements/<yyyy-mm-dd>-two-host/`.

**Laptop.**

```
git -C <repo> fetch && git -C <repo> checkout <commit> && git -C <repo> status -sb
cmake --build <repo>/build/linux-dual --target loadgen
<loadgen> --help
cd <repo> && python3 -c 'from benchmark.harness import environment as e; print(e._power_source(), e._governor())'
cat /sys/devices/system/clocksource/clocksource0/current_clocksource
ip route get <server-address>
cat /sys/class/net/<iface>/speed /sys/class/net/<iface>/duplex /sys/class/net/<iface>/mtu
tc qdisc show dev <iface>
systemctl is-active sshd
```

Refuse if: the tree is not clean at `<commit>`; the power source is not mains; the governor is
not `performance`; the clocksource is not `tsc`; `ip route get` names any interface but
`<iface>`; speed, duplex and MTU are not `1000`, `full`, `1500`; sshd is not active. The route
metric is fixed or the wireless interface is taken down for the night, the operator's call,
written in the results README if done. Then start the clock sampler of section 4.4 and leave the
laptop idle: no editor, no browser, no agent session doing work.

```
nohup nice -n 19 sh -c 'while :; do printf "%s " "$(date +%s)"; grep "cpu MHz" /proc/cpuinfo | sort -t: -k2 -rn | head -1; sleep 5; done' > <clocklog> 2>/dev/null &
echo $! > <clocklog>.pid
```

It is stopped after the last block with `kill $(cat <clocklog>.pid)`.

**Desktop.**

```
git fetch; git checkout <commit>; git status -sb
cmake --build build/windows-tls --config Release
ssh -o BatchMode=yes <ssh-target> true; $LASTEXITCODE
ssh -o BatchMode=yes <ssh-target> <loadgen> --help
powercfg /getactivescheme | Out-File <dir>\desktop-power.txt
Get-NetAdapter -Name "<wired adapter>" | Select-Object LinkSpeed, FullDuplex
Get-NetAdapterAdvancedProperty -Name "<wired adapter>" | Select-Object DisplayName, DisplayValue | Out-File <dir>\desktop-adapter.txt
```

Refuse if: the tree is not clean at `<commit>`; the build fails; ssh prompts or exits non-zero;
the generator does not print its usage; the link is not 1 Gbps full duplex. The power plan and the
adapter's advanced properties (interrupt moderation, receive-side scaling, offloads) are recorded
and none is changed.

**Reachability, before anything is timed.** The server is started once by hand on port 18080
(`<server-exe>`, the executable of `build/windows-tls` the harness launches):

```
<server-exe> --port 18080
   (laptop)   curl -s -o /dev/null -w '%{http_code}\n' http://<server-address>:18080/
   (desktop)  curl.exe -s -o NUL -w '%{http_code}\n' http://127.0.0.1:18080/
```

Both print 200; the server is stopped; then, and only then, anything is timed. Refuse otherwise.
No firewall rule is changed on either machine.

### 2.6 What the record carries about the other host

The fingerprint, the per-run clock, power and governor gates and the drift gate all describe the
driver host, which here is the server host. Nothing in the harness gates the generator host:
nothing refuses a laptop generator on battery or on the `powersave` governor, and the laptop's
governor is set by hand and does not survive a reboot. The smallest honest fix is the one in 2.3:
`generator_environment` captured once per campaign into the manifest by the same parser, so both
hosts are described by one code path. If that capture is not in `<commit>` on the night, the same
facts are taken by hand from the laptop and saved to `<dir>`:

```
ssh <ssh-target> "cd <repo> && python3 -c 'import json; from benchmark.harness import environment as e; print(json.dumps(e.capture(), indent=1))'" | Out-File <dir>\generator-environment.json
ssh <ssh-target> "tc qdisc show dev <iface>" | Out-File <dir>\generator-qdisc.txt
```

and the results README says they were taken by hand. Either way the stated limitation is the
same: **the generator host is described once per campaign, not per run, and is not
fingerprinted**; a generator host that changed state part-way through would not be refused. Its
per-run witnesses are the achieved share, which is an admission rule, the pacing p99 covariate,
and the clock log of section 4.4.

## 3. Cells, rates, repetitions, time

### 3.1 The cells

**Request latency: `transport`** (`run_campaign.design_transport`). Five rates × cleartext and TLS
× classification off and on: twenty cells. Keep-alive, sixty-four connections, four workers,
payload 0. The rates are `TLS_OFFERED_RATES`, 5 000, 10 000, 15 000, 25 000 and 35 000, in both
halves, so the halves are comparable to each other.

**Establishment: `churn`** (`design_churn`). Four rates × cleartext and TLS × classification off
and on: sixteen cells, one request per connection, so every request pays a fresh accept and a
fresh classification and `connect_ms` measures it directly. The rates are `CHURN_OFFERED_RATES`,
25, 50, 100 and 150. **100 and 150 carry the claim.** 25 and 50 are diagnostic only: on loopback
those two rates are bimodal at the run level, an entire 20-second run either fast (0.3 to 1.2 ms)
or slow (8.7 to 9.7 ms), thirty of forty-seven runs slow at 25 cleartext and none of 198 slow at
100 and 150, and the cause is not established. A bootstrapped difference of medians over a
bimodal per-run distribution reports the mode probability, not the path, so those cells are not
judged by the rule. They are kept because the wire tests whether the bimodality belongs to the
loopback arrangement or to the server host (section 8).

`churn-net`'s table (50, 150, 400, 800) is not used. 400 and 800 are rates the paper does not
rest on, and rates shared with the loopback table are what make shape comparison possible. The
churn ladder still runs (3.4) so the arrangement's establishment ceiling is measured and read
against section 8, but the table does not depend on it.

`h1-deep` is not run. Its cleartext cells at 5 000 to 35 000 are inside `transport`; its 40 000
to 70 000 cells are the ladder's business at one repetition (3.2); the machine time goes to the
awake control of section 4.3 instead.

### 3.2 The link ceiling, computed

The figure of about 1 100 bytes on the wire per request and response, saturating the link near
70 000 a second, was an estimate. Computed from the request the generator builds and the response
the server serialises, keep-alive, payload 0:

| Direction | Payload | On the wire |
| --- | --- | --- |
| request (`GET /`, `Host`, `Connection`, `User-Agent`) | 92 B | 170 B per frame |
| response (`200 OK`, four headers, 13-byte body) | 126 B | 204 B per frame |
| pure acknowledgement | 0 | 84 B per frame |

Per frame: 8 preamble, 14 Ethernet, 4 FCS, 12 inter-frame gap, 20 IPv4, 20 TCP with no options
(timestamps are negotiated only if both ends enable them; Windows does not by default; if on, add
12 per segment). The heavier direction is server to generator: 204 bytes per request with the
request's acknowledgement carried on the response, 288 if a pure acknowledgement precedes it.

**Link ceiling for payload 0: 1e9 / (288 × 8) = 434 000 requests a second worst case; 613 000
with acknowledgements piggybacked.** At 70 000 the link carries 114 to 161 Mbit/s, 11 to 16
percent of capacity, 70 000 to 140 000 frames a second per direction. At the cleartext ladder's
top of 120 000 it carries 28 percent. TLS adds 22 bytes of record overhead per message: 87 Mbit/s
worst case at 35 000. Establishment at 800 a second is about ten frames and a kilobyte per
cleartext connection, a few kilobytes with a TLS handshake, under 40 Mbit/s either way.

**No table in this design is bound by the link.** The binding ceilings are the server's own,
about 75 000 on loopback with a generator sharing its cores, and the laptop generator's pacing
over the wire, which nothing has yet measured. The ladders of 3.4 measure both at one repetition
before any timed run, so the ceilings are pre-declared by measurement rather than discovered as
refusals. Cut rule: if a ladder refuses a table rate by achieved share, non-2xx or socket errors,
the table is cut at the last admitted rate below it and the cut is written in the results README
before the design starts. A cut table is a stated limitation; there is no re-laddering.

Where the link would bind, stated so nobody runs it unchanged: the `h1` design's payload sweep at
40 000. Payload 8 192 is six segments and about nine kilobytes on the wire per request, a ceiling
near 13 800 a second against an offered 40 000. `h1` is out of scope for this run; if it is ever
run over the wire, the 8 192 cell is capped at 10 000.

### 3.3 Repetitions

**n = 25 per cell**, the number the X1 resolution analysis found necessary on loopback
(`benchmark/README.md`). The filed loopback campaigns ran more; "at the same n" in section 1 is
made literal by recomputing their r from their first n accepted repetitions.

The power calculation, from the desktop's filed loopback transport records (fifty accepted runs
per cell, so a real estimate of run-to-run spread): at n = 7 the half-width of the difference
interval as a share of the cell median is 1.2 to 4.7 percent in nine of ten cells; the tenth,
25 000 with TLS, is 6.7 percent (run-to-run coefficient of variation 6.4 percent against 1 to 4
elsewhere) and would reach 3.6 percent at n = 25. So seven repetitions resolves nine of ten cells
on loopback. Assumed: a normal interval on a difference of means as a stand-in for the harness's
bootstrapped difference of medians, and independence of runs. It is a prior from a different
medium: it says what n = 25 buys if the wire's run-to-run spread is like loopback's, resolution
with a margin of √(25/7) = 1.9 on the spread, and it says nothing if the wire's spread is not like
loopback's, which is the unknown this run measures.

**The clock rule.** n is chosen from the clock before the smoke run and never afterwards: 25 if
eleven hours remain in the window, otherwise 7 for every block except block 8, which is 7 in
either case, and the results README says which.
No block runs at a third count, and no block's n is changed after a number from it has been read.
If a 7-repetition block is later extended, the extension is a second file with a second seed
(`--seed 20260903`; `ordering.plan` seeds each pass from `seed:repetition`, so the same seed would
replay the same orders), pooled with the first for r, with the repetition index read as pass
within file. At n = 7 the design reports r as measured and, beside it and marked as a projection,
r scaled by √(7/25).

### 3.4 The blocks, in order, with the commands

One run is 26 seconds: 20 measured, 3 warmup, 3 turnover, plus ssh session setup.

| Order | Block | Cells | n | Runs | Time | Stopping here licenses |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | `smoke` | 2 | 1 | 2 | 1 min | the interface and the launch are right |
| 2 | `ladder` (10 000 to 120 000) | 12 | 1 | 12 | 5 min | the cleartext ceiling |
| 3 | `tls-ladder` (5 000 to 60 000) | 12 | 1 | 12 | 5 min | the TLS ceiling |
| 4 | `churn-ladder` (25 to 800) | 11 | 1 | 11 | 5 min | the establishment ceiling |
| 5 | `churn`, cores idle | 16 | 25 | 400 | 2.9 h | the establishment half of the rule |
| 6 | `transport`, cores idle | 20 | 25 | 500 | 3.6 h | both halves of the rule |
| 7 | `churn`, cores awake | 16 | 25 | 400 | 2.9 h | the idle-state control (4.3), establishment |
| 8 | `transport`, cores awake | 20 | 7 | 140 | 1.0 h | the idle-state control (4.3), request latency |
| | **total at n = 25** | | | **1 477** | **10.7 h** | |
| | total at n = 7 | | | 541 | 3.9 h | |

Block 8 runs at n = 7 whatever the clock rule chose, because it tests a predicted shift and not
the 5 percent resolution: on the loopback prior of 3.3, seven repetitions bound a shift to within
1.2 to 4.7 percent of the cell median, which is enough to judge prediction 4.

`<options>` in every command below is:

```
--build build/windows-tls --host <server-address> --ssh-generator <ssh-target> --ssh-loadgen <loadgen> --ssh-repo <repo> --generator-location lan:linux --expect-interface <iface>
```

```
python -m benchmark.run_campaign --design smoke        --repetitions 1  --results <dir>/smoke.jsonl        <options>
```

The two smoke records are read before anything else runs: `accepted` true on both,
`local_interface` `<iface>`, `local_interface_speed_mbit` 1000, `local_interface_duplex` `full`,
`local_interface_mtu` 1500, `generator` `coroute-loadgen-lan`, `generator_euid` a non-zero
number, `git_commit` `<commit>`, `admission_rules` naming the covariate rule set; and in
`smoke.env.json`, `generator_environment.build.git_commit` `<commit>` and
`generator_environment.cpu.governor` `performance`. Stop at the first field that is not so.

```
python -m benchmark.run_campaign --design ladder       --repetitions 1  --results <dir>/ladder.jsonl       <options>
python -m benchmark.run_campaign --design tls-ladder   --repetitions 1  --results <dir>/tls-ladder.jsonl   <options>
python -m benchmark.run_campaign --design churn-ladder --repetitions 1  --results <dir>/churn-ladder.jsonl <options>
```

The ladders are read by the admission rules of 4.1 and the cut rule of 3.2, and the ceilings are
written in the results README before the next command.

```
python -m benchmark.run_campaign --design churn        --repetitions <n> --results <dir>/churn-idle.jsonl  <options>
python -m benchmark.run_campaign --design transport    --repetitions <n> --results <dir>/transport.jsonl   <options>
```

Then the spinners of 4.3 are started, two minutes pass, and:

```
python -m benchmark.run_campaign --design churn        --repetitions <n> --results <dir>/churn-awake.jsonl     <options>
python -m benchmark.run_campaign --design transport    --repetitions 7   --results <dir>/transport-awake.jsonl <options>
```

Then the spinners are stopped by recorded PID. **Stop only at a block boundary**: a finished
block is citable, half of one is not. If the window closes before block 7 or 8, the blocks not
run are the pre-declared next window and run unchanged. The clock sampler on the laptop is
stopped after the last block, and `<clocklog>` is copied into `<dir>`.

## 4. Gates

### 4.1 Admission

As decided on 2 September: achieved share at or above 0.99; non-2xx at or below 0.1 percent;
zero socket errors; and the environment gates: virtualisation, dirty tree, governor where the
platform has one, clock drift within 2 percent across the run on the server host, mains power,
and the interface expectation of 2.4. **Pacing p99 is a recorded covariate reported beside every
cell, not an admission rule**, and every record names the rule set that judged it
(`admission_rules`, schema 9). No threshold on any latency quantity. Rejections are counted and
their reasons read before any number is.

### 4.2 The link

1. **Interface.** Every run reports `<iface>`, 1000 Mbit, full duplex, MTU 1500, or is refused.
2. **Where the reading "the link saturated" applies.** For payload 0 the link cannot bind below
   434 000 requests a second, and no table in this design offers a fifth of that. So **no
   refusal in this design is read as the link's.** The reading applies only at or above that
   rate, or in the `h1` payload sweep, which is not run.
3. **What a refusal by achieved share is read as.** One of two things, told apart by fields
   already in the record. The generator: pacing p99 rising with rate and `generator_cpu_fraction`
   at both threads' full share, on the ladder, before any timed run. The server: achieved share
   falling while pacing stays in the tens of microseconds, because the generator hands a due slot
   only to a connection not awaiting a response, so a server that holds connections starves the
   schedule. Either is a **ceiling finding about the arrangement**, written as such, and is a
   property of the server only where the second pattern shows. The results README says which.
4. **Drops.** Read at neither end: Windows has no counter the harness reads and the generator
   host's are not read. Socket errors and non-2xx remain admission rules and would carry a drop's
   consequences. Stated limitation.

### 4.3 Idle state

CPU idle state mediates low-load latency by hundreds of microseconds at the receiving end (the
laptop's 495 against 74 microseconds at 100 requests a second; its ping's 722 against 502). The
two ends differ in whether they can pay that cost:

- **The generator end spins.** For the last 20 ms before every due slot the generator spins
  rather than sleeps (`loadgen.cpp`, `kSpinBelowUs`), so at every keep-alive rate and at 100 and
  150 establishments a second it never sleeps and cannot charge a wake to the server. At 25 and
  50 a second (periods 40 and 20 ms) it sleeps in 1 ms polls between slots, one more reason those
  cells are diagnostic.
- **The server end may idle.** The desktop runs under the power plan `powercfg /getactivescheme`
  reports, recorded and unchanged, and between connections at 25 to 150 a second its cores have
  6.7 to 40 ms to park, past the 5 ms at which the measured wake cost plateaus.

So **the establishment cells are taken twice**, once as-is with cores idle and once with the
server's cores held awake, in two files, both reported. The idle file is the arrangement as the
filed loopback campaigns ran; the awake file is the control that says how much of the idle file
is the idle governor. **On this direction the control costs the generator nothing**: the spinners
run on the server host and the generator is on the other machine. In the namespace arrangement
the same control raised the generator's pacing p50 from 5 to 52 microseconds because they shared
cores; here the pacing covariate in the awake file is expected to match the idle file's, and
section 8 says so.

The Windows form of the documented control (the Linux form is sixteen `nice -n 19` spinners,
`design/socket-options-inventory.md` section 4): twelve PowerShell processes, one per logical
processor of the Ryzen 5 3600, each spinning, priority class Idle, started before block 7 and
stopped by recorded PID after it.

```
$spin = 1..12 | ForEach-Object { Start-Process powershell.exe -ArgumentList '-NoProfile','-Command','while ($true) {}' -WindowStyle Hidden -PassThru }
$spin | ForEach-Object { $_.PriorityClass = 'Idle' }
$spin.Id -join ' ' | Set-Content <dir>\spinners.txt
Get-Process -Id (Get-Content <dir>\spinners.txt).Split(' ') | Select-Object Id, PriorityClass, CPU
```

```
(Get-Content <dir>\spinners.txt).Split(' ') | ForEach-Object { Stop-Process -Id $_ }
```

The server runs at Normal class and Windows preempts an Idle-class thread immediately, which is
what makes the block a test of idleness and not of contention. That it did is read in the
result, as on Linux: an establishment median that falls under the control is not a server starved
of CPU. The cost of the control shows in the tail and is a property of the control. The spinners
start two minutes before the block's first run so the package clock is at its sustained level
before the drift gate's first sample; drift refusals in the awake block are counted and not
re-run. The awake condition is carried by no record field, so the two churn files are never
concatenated, and the results README names the condition of each.

`transport` is taken awake too, as block 8, at seven repetitions. At 5 000 a second and above
the per-worker gap is under a millisecond, where the measured wake cost is about 9 microseconds
(the idle ladder: 1 ms gives 9, 2 ms 26, 5 ms and beyond 63), so the predicted shift is small and
the block exists to bound it, not to resolve the demultiplexing difference; the awake transport
file is never the file the rule of section 1 is applied to. The harness has no keep-alive cell
below 5 000 a second, so the 100-requests-a-second condition at which the 495-against-74
microsecond mediator was measured has no cell in this run; 5 000 a second is the nearest
keep-alive test of it.

### 4.4 The generator host's clock

The laptop throttles under sustained TLS: as a server on 2 September it fell from 4 067 to 3 322
MHz inside a twenty-second run and its TLS half was declared unusable. As direction A's TLS
generator it does the symmetric record-layer work at up to 35 000 a second on two spinning
threads, and no gate reads its clock. The witness is the sampler of 2.5: every five seconds, a
timestamp and the fastest core's `cpu MHz`, at nice 19, for the whole campaign, beside the pacing
covariate per run.

Reading rule, fixed now: for each TLS transport run, the lowest sample inside its window against
the campaign's first sample, at the same 2 percent the drift gate applies to the server. If any
TLS cell has a run below that, or pacing p99 rises with repetition in the TLS cells and not in
the cleartext ones, the TLS half of transport is reported with that caveat and no
TLS-against-cleartext sentence is written from it, by the standard the laptop's own re-run applied
to itself. The cleartext half is unaffected, the generator doing no cryptography there, and churn
is unaffected at its rates.

## 5. What may and may not be compared

**Within this arrangement:** arms against each other. Classification on against off in every
cell, with its interval; TLS against cleartext at a shared rate; cores awake against idle at the
same establishment cell. These are the only magnitudes this run produces.

**Against loopback and against the virtual-switch arm:** the resolution figure and the shape
only. Whether r is under 5 percent in the same cell; the sign of the difference; its trend with
rate; whether the bimodality at 25 and 50 is present. Never magnitudes: the medium changed, the
commit changed, and the server host's spare cores changed from hosting the generator to idling.

**Across platforms:** never. Direction A's Windows server against direction B's Linux server, or
either against paper 3's filed Linux arms, is shape only, because the two ends are different
hardware.

**Across files:** never. The idle and awake churn files are not concatenated; nothing measured
here is pooled with anything filed, and the harness refuses the attempt where it can see it.

## 6. Direction B, later

Linux server, epoll and io_uring, for paper 3's arms over a real path; Windows generator; driver
on the laptop, by the placement rule of 2.1, reaching the generator over ssh. That needs the
OpenSSH Server feature on Windows and a key: a system change and Alex's decision, so direction B
is deferred until it is made.

It is second because **the generator end sleeps**. The Windows generator's 1 ms poll is quantised
to the raised system timer, so at 25 and 50 establishments a second it sleeps between slots and
charges its own wake to the system under test; at 100 and above, and at every keep-alive rate, it
spins as the Linux one does. Three more things follow from the code and are stated now:
`--expect-interface` cannot be used, because the Windows generator reports no interface and the
driver would refuse every run, so the dual-homed host, now the server, has its wireless interface
taken down for the night and the fact recorded; the idle control at the Linux server is the
documented sixteen `nice -n 19` spinners; and the laptop's TLS throttling as a server is gated by
the drift gate as before, so direction B's TLS cells are not promised. Cells, rates, n and the
rule are those of this document, re-declared in a dated addendum to this file before direction B
runs.

## 7. Provenance

Results go to the private paper repository, `measurements/<yyyy-mm-dd>-two-host/`, on a branch
`measure/two-host-<yyyy-mm-dd>`, and nowhere else: the manifest carries the server's address and
every record's `generator_argv` carries the ssh target. The directory holds every `.jsonl` and
`.env.json`, `generator-environment.json`, `generator-qdisc.txt`, the clock log, `spinners.txt`,
`desktop-power.txt`, `desktop-adapter.txt`, and a README that names: `<commit>`, one commit for
both hosts, with the note that the run is refused if they differ; both builds, `build/windows-tls`
Release on the desktop and the laptop's build directory and generator path; the fingerprint from
`.env.json`; the interface fields as recorded, `local_interface`, speed, duplex and MTU; the
power plan and the adapter's advanced properties; the qdisc; whether the wireless interface was
taken down; the n chosen and from what clock; any table cut and why; the rejection count and every
reason per file; and, for each churn and transport file, whether cores were allowed to idle.

Into the framework repository, which is public, go this design, the harness change of 2.3 and its
documentation, all with placeholders. No hostname, address, MAC, network name or router detail,
and no description of the network beyond one gigabit Ethernet segment, two hosts.

## 8. Pre-registered predictions

Stated so the outcome is judged rather than explained.

1. **The link ceiling.** No rate in any table is refused for the link. The cleartext ladder
   admits 70 000, and its first refusal lies between 80 000 and 120 000, by pacing at the laptop
   generator or by the server, not by the link, which binds at 434 000 to 613 000. The TLS ladder
   admits 35 000. The churn ladder admits through 800, as the virtual-switch arm did, because the
   generator has its own cores; the loopback ceiling of about 330 a second is thereby confirmed to
   have been the arrangement's.
2. **Establishment at 100 and 150 resolves**, cleartext and TLS, at n = 25. The path's
   contribution to a per-run median is under one percent of the 1.4 ms baseline (a per-packet
   spread of 300 microseconds over 2 000 to 3 000 connections a run gives a standard error near
   7 microseconds), so r over the wire is set by run-level state, and at 100 and 150 loopback
   showed none in 198 runs. If this is wrong, the failure is a second mode in the medians, not a
   wide unimodal spread, and the repetition plot shows it.
3. **25 and 50 stay bimodal over the wire**, with the slow-mode share in loopback's range (thirty
   to thirty-seven runs in about fifty). The state lives in the server host, not in a loopback
   generator sharing its cores. If instead no slow mode appears over the wire, the state was the
   loopback arrangement's, which is the more useful outcome for paper 2 and is written as such.
4. **The idle mediator shows in low-load latency at the server end unless cores are held
   awake, and by an amount that falls with the rate.** Establishment, judged by blocks 5 and 7:
   in the awake file the classification-off establishment median is lower than in the idle file
   by 100 to 500 microseconds at every rate from 25 to 150; the demultiplexing difference itself
   is unchanged within its interval, the idle cost being added to both arms. Request latency,
   judged by blocks 6 and 8: at 5 000 a second the awake-minus-idle shift in the
   classification-off median is under 30 microseconds; at 35 000 a second it is indistinguishable
   from zero within block 8's own interval. No keep-alive cell below 5 000 a second exists in the
   harness, so the hundreds-of-microseconds form of the mediator is predicted for establishment
   only.
5. **The awake control costs the generator nothing.** Pacing p50 and p99 in the awake churn and
   transport files lie within the idle files' range.
6. **Request latency resolves in every cleartext cell.** TLS at 25 000 and 35 000 may not: the
   loopback cell at 25 000 TLS was the one that failed at n = 7, and over the wire the TLS
   generator's clock is the unknown of 4.4.
7. **The laptop's clock falls under TLS transport**, and by more than 2 percent within a run at
   25 000 and 35 000, the rates at which it throttled as a server, so the TLS half at those two
   rates is expected to carry the caveat of 4.4. If it does not fall, the generator's TLS work is
   lighter than the server's at the same rate, and that is recorded.
8. **Magnitudes, for shape only.** Request-latency medians over the wire are two to four times
   loopback's, the path's round trip added; establishment medians are within a factor of two of
   loopback's fast mode. Neither is a comparison the papers may draw; they are recorded so the
   medium's effect is judged and not explained.

## 9. Why this file exists

A design declared in advance and then reported afterwards is only as good as the evidence that it
really was declared in advance. Alex's decision of 2 September was that the two-host run is
approved for overnight and its design is pre-declared and committed before it runs. Every rule
above is fixed now: the cells, the rates, n and the clock that chooses it, the admission rules,
the reading of every refusal, the two conclusions and the predictions. A rule added after the
records exist is a filter on the outcome, and this file's timestamp is the evidence that none
was.
