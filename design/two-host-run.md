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

## 0. A correction to the brief, made before anything else

The brief that commissioned this design described the effect to be resolved as "60 to 85
microseconds on a classification-off median of about 1.4 ms establishment or 0.09 ms request
latency". Read against the paper and against the filed records, that sentence conflates two
different effects in two different cells, and neither of them lives where the first draft of this
design put its machine time. The correction is stated here rather than buried, because every
section below depends on it and because the two-reader rule requires the disagreement to be on the
record.

Paper 2 reports **two** demultiplexing costs, and both are in the `churn` design, one request per
connection:

- **Cleartext: the cost is in request latency, not in establishment.** `sections/results.tex`
  line 158: the request median is 0.028 to 0.030 ms higher with classification on at all four
  rates, 28.57 to 32.95 percent of a classification-off median of 0.088 to 0.105 ms. All eight
  cells reportable.
- **TLS: the cost moves to establishment**, because that is where the first octet is.
  `sections/results.tex` line 164: 0.085 ms at 100/s on a 1.396 ms classification-off median,
  0.061 ms at 150/s on 1.383 ms.

So "60 to 85 microseconds" is the TLS **establishment** pair, 85 and 61; "0.09 ms request latency"
is the **cleartext** baseline, whose effect is 28 to 30 microseconds, not 60 to 85. The brief
joined the numerator of one cell to the denominator of the other.

The consequence that costs machine time: **the `transport` design cannot carry the claim at all.**
It is keep-alive, sixty-four connections opened in the warmup and held for the whole window, so
the once-per-connection classification path is divided by tens of thousands of requests before it
reaches a percentile. Paper 2 calls that null "arithmetic before it is a measurement"
(`sections/results.tex` line 98) and names the design that can fail as "the one that divides by
one". Recomputing the filed loopback `transport` records confirms it: the classification-on minus
classification-off difference of per-run medians is 0.0 to 1.7 percent of a 0.057 to 0.076 ms
baseline, which is zero to one microsecond, in every one of the ten cells. A real path does not
change that arithmetic. Spending 640 runs and 4.6 hours asking whether a wire can resolve a
60-microsecond difference in cells where the difference is one microsecond would have answered
nothing.

The first draft of this design named ten `transport` cells and four `churn` establishment cells as
the cells the claim rests on. Both halves were wrong. Section 1 states the six cells that do.

## 1. The one question, and the rule that answers it

**The question.** Paper 2's two demultiplexing costs were measured over loopback. This run answers
one question: can a real path resolve differences of those sizes? Not whether it reproduces the
numbers.

**The quantity.** What decides resolution is not the per-packet spread of the path. We compare
per-run medians, and the standard error of a median over the requests of one run is small if the
noise is independent; what enters the comparison is the run-to-run spread of those medians.
`stats.compare` (`benchmark/harness/stats.py`) bootstraps the difference of medians between the
two arms at 95 percent. **The half-width of that interval, in microseconds, is the resolution: how
small a difference the cell could have seen.** Call it **h**.

The share of the baseline that `results2tex.hypothesis_x1` calls resolution — h divided by the
classification-off median — is reported beside h and decides nothing. It cannot decide, for two
reasons found in review and both confirmed. Its denominator is measured in the same run over the
same medium, and prediction 8 of section 8 predicts the wire inflates exactly that denominator, so
a cell would pass a threshold on the share with two to four times the absolute spread of a
loopback cell that failed it, purely because the path added latency to both arms. And the 5
percent it would be compared against is `stats.MIN_RELATIVE_DIFFERENCE` (`stats.py` line 148),
which gates `large_enough = abs(relative) >= MIN_RELATIVE_DIFFERENCE` (`stats.py` line 204) — a
floor on the **effect size**, never on the resolution. Borrowed for a resolution it fails in both
directions: 5 percent of a 1.4 ms establishment baseline is a 70-microsecond half-width against a
61-microsecond effect, so a cell could pass and still provably not have seen the difference; 5
percent of a 0.09 ms request baseline is 4.5 microseconds, so a cell with a 6-microsecond
half-width would be called unresolved on a 28-microsecond effect. The rule below is stated in
microseconds against the effect each cell was run to see.

**Per cell.** A cell is one offered rate, one transport (cleartext or TLS) and one statistic. Its
two arms are classification off, the baseline, and classification on. h is computed from accepted
runs only. The path is not `hypothesis_x1`: that function reads `record.latency_ms[percentile]`
and nothing else (`results2tex.py` lines 302, 305), so no argument to it can return an
establishment figure, and it emits only a minimum and a maximum across the cells it was handed,
never a per-cell value. Worse, it fails silently — churn runs populate `latency_ms` too, so
calling it on churn records returns a request-latency resolution and the reader gets a plausible
number for the wrong quantity. The recipe instead, per cell, is:

> split the accepted records of one rate, one `tls` value and `max_requests_per_connection = 1`
> into two arms by `protocol_detection`; call `stats.compare(off, on)` on the arm lists of
> `connect_ms["p50"]` for establishment or `latency_ms["p50"]` for request latency; then
> h = `(interval.high - interval.low) / 2`, in milliseconds, times 1000.

A cell with fewer than three accepted runs in either arm has no interval (`stats.py` lines
183-187), is UNRESOLVED by construction, and its counts are reported.

**The six cells the claim rests on.** Both statistics come from the same `churn` runs; no extra
machine time buys the second one.

| # | Cell | Statistic | Loopback baseline | Effect to resolve |
| --- | --- | --- | --- | --- |
| 1 | 25/s cleartext | `latency_ms.p50` | 0.105 ms | +30 us |
| 2 | 50/s cleartext | `latency_ms.p50` | 0.100 ms | +30 us |
| 3 | 100/s cleartext | `latency_ms.p50` | 0.089 ms | +28 us |
| 4 | 150/s cleartext | `latency_ms.p50` | 0.088 ms | +29 us |
| 5 | 100/s TLS | `connect_ms.p50` | 1.396 ms | +85 us |
| 6 | 150/s TLS | `connect_ms.p50` | 1.383 ms | +61 us |

Cells 1 to 4 are **half A**, the cleartext request-latency half; cells 5 and 6 are **half B**, the
TLS establishment half.

**Half A includes 25 and 50 per second, and the bimodality does not touch it.** The bimodality
that makes those two rates unusable is in `connect_ms`, where an entire 20-second run is either
fast or slow and the per-run medians span 0.1 to 9.7 ms. It is not in `latency_ms`: over the filed
loopback campaign the classification-off `latency_ms.p50` at 25/s cleartext spans 0.102 to 0.109
ms across 24 accepted runs, and the bootstrapped half-width on the difference is 1.0 microsecond
at n = 25. The two statistics come from the same runs and behave nothing alike, because
`latency_ms` is issue-to-response and excludes both connect and handshake: the generator stamps
`c.issued` in the issue loop, past the guard that skips a connection whose handshake is still in
flight (`loadgen.cpp`, `if (c.fd == kInvalidSocket || !c.ready || c.awaiting) continue;`), and
takes the sample as `at - c.issued` (`loadgen.cpp`, the `latencies_us.push_back` block). Whatever
state produces the slow establishment mode is paid before the clock the request median runs on
starts.

**The rule.**

1. A cell RESOLVES if h is under half the effect it was run to see: **h < 14 microseconds** for
   half A, **h < 30 microseconds** for half B. Two thresholds and not one, because the two effects
   differ by a factor of two to three and a single threshold would be slack for one half and
   impossible for the other.
2. The arrangement carries the demultiplexing claim for **cleartext request latency** if all four
   half-A cells resolve, and for **TLS establishment** if both half-B cells resolve. The two
   halves are judged separately, and a half that resolves is written up as resolved whatever the
   other did.
3. The verdict is on h, never on the sign or size of the difference. Reading h does not choose an
   outcome.
4. Where a cell resolves, the difference is reported as it comes, and the flag that is read is
   `interval.excludes(0.0)` and **not** `Comparison.reportable`. The two are not the same:
   `reportable` requires exclusion of zero **and** a relative difference at or above
   `MIN_RELATIVE_DIFFERENCE` (`stats.py` lines 204-215), and half B's 150/s cell already fails that
   second bar on loopback — 0.061 ms on 1.383 ms is 4.41 percent, and paper 2 says so in as many
   words: "the interval excludes zero and the size is under the floor, so it is not reported as a
   difference". A cell whose interval excludes zero with a relative difference under 5 percent is
   written as **resolved, with a measured difference below the project's reporting floor**, which
   is the sentence the harness's own `reason` field will print. An interval containing zero is a
   null result at resolution h.
5. Beside every cell: h in microseconds, the half-width as a share of the baseline for continuity
   with the papers, the relative difference, whether the interval excludes zero, whether
   `reportable` was true, the run-to-run coefficient of variation of the medians per arm, the
   accepted and rejected counts per arm with reasons, the pacing p50 and p99 covariates per arm,
   and the median against repetition index, which is where autocorrelated network noise or thermal
   drift would show as structure.

**If a half resolves, this is written:** over one gigabit Ethernet segment between two hosts, the
demultiplexing difference in [cleartext request latency / TLS establishment] was resolved to h in
every cell of that half, and its value there is whatever was measured, interval included. Where
only one half resolves, the other half is reported at its measured h and carries no replacement of
the caveat, and the sentence names which half it is. Where both resolve, the loopback caveat is
replaced by this measurement **in paper 2**, and in paper 2 only: paper 3's loopback caveat is over
epoll and io_uring, which is direction B (section 6), and is replaced only by direction B's
addendum. The sentence replaced is `sections/limitations.tex` line 14, which begins "Loopback.
Generator and server are joined by 127.0.0.1." The loopback figures stand as the figures for the
medium in which the claim was first made; they are not revised by this run.

**If a half does not resolve, this is written:** over a real path the run-to-run spread of per-run
medians gives a half-width of h microseconds in the cells of that half, and at this n the design
could not have seen the difference it was run to see. Loopback was the right medium for that half
of the demultiplexing claim: the claim is a difference between two arms that share one path, and
loopback's own h for the same cells was under the same threshold. Two things about that sentence.
The first clause is a condition, not a flourish — it is written for a cell only where the filed
loopback campaign's h, recomputed from its first n accepted repetitions in file order, was under
the threshold, and section 3.3 tabulates that it was for all six. The second clause rests on an
assumption this run cannot test: **that the medium adds spread and no information about the arms.**
A run that failed to resolve cannot tell that apart from classification costing something different
on a real socket than on loopback, which is one of the things a real path was run to check. The
assumption is named as an assumption wherever the sentence is written.

Whichever way it goes, the real path is named as the right medium for the questions in which the
path is a factor rather than noise: the establishment ceiling of the arrangement; the idle-state
mediation of low-load latency at the receiving end, which a generator sharing the host masks; the
tail, p99 and above, under a real driver and interrupt path; and paper 3's shape comparison of
epoll against io_uring over a real path.

Any other pattern is reported per cell, and no sentence is written that the pattern does not
support. Two caveats apply to either outcome and are reported, not adjudicated: network noise
autocorrelated over a run's length collapses the effective sample count toward the number of runs,
which the repetition plot shows as structure; and variation correlated with the arm rather than
added to it biases the comparison at any n, which the interleaved shuffle addresses for
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

`<commit>` must also not have changed either of two constants that section 4.3's spin argument
turns on: `GENERATOR_THREADS = 2` (`run_campaign.py`) and `kSpinBelowUs = 20000` (`loadgen.cpp`).
The reason is in 4.3, and the margin there is zero.

### 2.3 The launch

The harness launches the generator locally, inside WSL, or under a free-form prefix that shares
the driver's filesystem. None of those reaches another host: the generator's binary path and its
`--out` result path are the driver host's, and the affinity mask names the driver host's cores.
The change this run needs mirrors the WSL branch and is small:

- `--ssh-generator <user>@<generator-address>`, `--ssh-loadgen <loadgen>`, `--ssh-repo <repo>`,
  with the existing `--generator-location lan:linux`. The launcher is
  `ssh -n -o BatchMode=yes -o ConnectTimeout=5 <target> <loadgen>` followed by the harness's own
  generator arguments. `BatchMode` makes a missing key a refusal rather than a prompt, for the
  reason `netns.py` uses `sudo -n`. **`-n` is not optional.** `LoadgenGenerator.run` calls
  `subprocess.run(argv, capture_output=True, text=True, timeout=...)` with no `stdin=`
  (`benchmark/adapters.py`), and ssh with a command and an inherited stdin reads the console: from
  a PowerShell session every keystroke during the night would be swallowed by whichever ssh is
  current, and under a non-console launch stdin is an invalid handle rather than a terminal.
  `BatchMode=yes` stops password prompts; it does not detach stdin. `-n` redirects stdin from
  /dev/null, which is the flag's entire purpose. No affinity mask. The result JSON is read from
  the generator's stdout, where the generator already writes it, rather than from a file.
  `--samples` is refused in this mode: the histogram percentiles in the record are the data.
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
address as the laptop sees it, `<ssh-target>` `<user>@<generator-address>`, `<clocklog>` and
`<deskclocklog>` files outside `<repo>`, `<dir>` a directory on the desktop outside the framework
checkout (so the tree stays clean), for instance the private paper repository's
`measurements/<yyyy-mm-dd>-two-host/`.

**Laptop.**

```
git -C <repo> fetch && git -C <repo> checkout <commit> && git -C <repo> status -sb
cmake --build <repo>/build/linux-dual --target loadgen
<loadgen> --help
ldd <loadgen> | grep -q libssl && echo TLS-OK || echo REFUSE-NO-TLS
cd <repo> && python3 -c 'from benchmark.harness import environment as e; print(e._power_source(), e._governor())'
cat /sys/devices/system/clocksource/clocksource0/current_clocksource
ip route get <server-address>
cat /sys/class/net/<iface>/speed /sys/class/net/<iface>/duplex /sys/class/net/<iface>/mtu
tc qdisc show dev <iface>
systemctl is-active sshd
```

Refuse if: the tree is not clean at `<commit>`; the power source is not mains; the governor is
not `performance`; the clocksource is not `tsc`; `ip route get` names any interface but
`<iface>`; speed, duplex and MTU are not `1000`, `full`, `1500`; sshd is not active; **or the
`ldd` line prints REFUSE-NO-TLS.** That last check is not decoration. `COROUTE_LOADGEN_TLS` is
defined only if `find_package(OpenSSL)` succeeded
(`benchmark/generator/CMakeLists.txt`); without it `--tls` prints one line to stderr and returns 2
(`loadgen.cpp`), so under the stdout path of 2.3 every TLS run is a `RunFailed` — and `usage()`
prints the `--tls` entry unconditionally, so `--help` succeeding proves nothing. The first failure
would be block 3, sixteen minutes in, and blocks 5 and 6 would then lose eight of sixteen churn
cells, including both cells of half B. The route metric is fixed or the wireless interface is
taken down for the night, the operator's call, written in the results README if done. Then start
the clock sampler and leave the laptop idle: no editor, no browser, no agent session doing work.

```
nohup nice -n 19 sh -c 'while :; do printf "%s " "$(date +%s)"; grep "cpu MHz" /proc/cpuinfo | sort -t: -k2 -rn | head -1; sleep 5; done' > <clocklog> 2>/dev/null &
echo $! > <clocklog>.pid
```

It is stopped after the last block with `kill $(cat <clocklog>.pid)`.

**Desktop.**

```
git fetch; git checkout <commit>; git status -sb
cmake --build build/windows-tls --config Release
python -m benchmark.make_cert
ssh -n -o BatchMode=yes <ssh-target> true; $LASTEXITCODE
ssh -n -o BatchMode=yes <ssh-target> <loadgen> --help
powercfg /getactivescheme | Out-File <dir>\desktop-power.txt
Get-NetAdapter -Name "<wired adapter>" | Select-Object LinkSpeed, FullDuplex
Get-NetAdapterAdvancedProperty -Name "<wired adapter>" | Select-Object DisplayName, DisplayValue | Out-File <dir>\desktop-adapter.txt
```

Refuse if: the tree is not clean at `<commit>`; the build fails; ssh prompts or exits non-zero;
the generator does not print its usage; the link is not 1 Gbps full duplex. `make_cert` is in the
list because `run_campaign` refuses any design with a TLS cell outright when
`benchmark/certs/bench.crt` or `.key` is missing, and `benchmark/.gitignore` ignores `certs/`, so
`git checkout <commit>` neither creates them nor guarantees they survived. It is idempotent, and
safe because the generator does not verify (`tls_verify: none`). The power plan and the adapter's
advanced properties (interrupt moderation, receive-side scaling, offloads) are recorded and none
is changed.

The desktop's own clock sampler is started here and runs for the whole campaign, for the reason in
4.3:

```
Start-Job -Name deskclock -ScriptBlock {
  while ($true) {
    $c = Get-CimInstance Win32_PerfFormattedData_Counters_ProcessorInformation |
         Where-Object { $_.Name -ne '_Total' }
    $m = ($c | ForEach-Object { $_.ProcessorFrequency * $_.PercentProcessorPerformance / 100 } |
          Measure-Object -Maximum).Maximum
    "{0} {1}" -f [DateTimeOffset]::UtcNow.ToUnixTimeSeconds(), $m | Out-File -Append <deskclocklog>
    Start-Sleep -Seconds 5
  }
}
```

`[DateTimeOffset]` rather than `[double]::Parse((Get-Date -UFormat %s))` for the reason
`environment.py` records against `Get-Counter`: this project's own Windows host formats a decimal
with a comma, and a probe that fails on a decimal separator fails silently. It is stopped after the
last block with `Stop-Job -Name deskclock; Remove-Job -Name deskclock`, and `<deskclocklog>` is
copied into `<dir>`.

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
ssh -n <ssh-target> "cd <repo> && python3 -c 'import json; from benchmark.harness import environment as e; print(json.dumps(e.capture(), indent=1))'" | Out-File <dir>\generator-environment.json
ssh -n <ssh-target> "tc qdisc show dev <iface>" | Out-File <dir>\generator-qdisc.txt
```

and the results README says they were taken by hand. Either way the stated limitation is the
same: **the generator host is described once per campaign, not per run, and is not
fingerprinted**; a generator host that changed state part-way through would not be refused. Its
per-run witnesses are the achieved share, which is an admission rule, the pacing p50 and p99
covariates, and the clock log of section 4.4.

## 3. Cells, rates, repetitions, time

### 3.1 The cells

**`churn` carries the claim** (`run_campaign.design_churn`). Four rates x cleartext and TLS x
classification off and on: sixteen arms, one request per connection, so every request pays a fresh
accept and a fresh classification. The rates are `CHURN_OFFERED_RATES`, 25, 50, 100 and 150. Two
statistics are read from the same runs, and section 1's table says which cell each belongs to:
`latency_ms.p50` carries half A at all four rates, cleartext; `connect_ms.p50` carries half B at
100 and 150, TLS.

Everything else `churn` produces is diagnostic and is reported without entering the rule:

- **`connect_ms` at 25 and 50, both transports.** Bimodal at the run level: an entire 20-second
  run is either fast or slow, and the filed loopback campaign's bootstrapped half-widths there are
  4 206 to 8 156 microseconds. A bootstrapped difference of medians over a bimodal per-run
  distribution reports the mode probability, not the path. The cells are kept because the wire
  tests whether the bimodality belongs to the loopback arrangement or to the server host
  (prediction 4).
- **`connect_ms` at 100 and 150 cleartext.** Not bimodal, but the effect there is about 10
  microseconds on a 0.25 ms baseline and loopback's own half-width was 27.8 to 40.0 microseconds
  at n = 25 — loopback did not resolve it either, and paper 2 does not claim it: "The cost is
  there, and it is in request latency, not in establishment time."
- **`latency_ms` on the TLS arm.** The measured effect is -1 to +1 microsecond in all four cells,
  because on TLS the classification is paid inside the handshake and lands in `connect_ms`.

**`transport` runs once, at n = 7, as a diagnostic, and is outside the rule** (section 0). It
exists for two things only: to confirm over a real path that the keep-alive null still holds, and
to give prediction 8 its request-latency magnitudes for the shape comparison. Twenty arms, five
rates (`TLS_OFFERED_RATES`: 5 000, 10 000, 15 000, 25 000, 35 000) x cleartext and TLS x two arms,
keep-alive, sixty-four connections, four workers, payload 0.

`churn-net`'s table (50, 150, 400, 800) is not used. 400 and 800 are rates the paper does not rest
on, and rates shared with the loopback table are what make shape comparison possible. The churn
ladder still runs (3.4) so the arrangement's establishment ceiling is measured. `h1-deep` is not
run: it is keep-alive at 40 000 to 70 000, which is `transport`'s null at higher rates, and by
section 0 it can carry nothing.

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

**Link ceiling for payload 0: 1e9 / (288 x 8) = 434 000 requests a second worst case; 613 000
with acknowledgements piggybacked.** At 70 000 the link carries 114 to 161 Mbit/s, 11 to 16
percent of capacity. At the cleartext ladder's top of 120 000 it carries 28 percent. TLS adds 22
bytes of record overhead per message: 87 Mbit/s worst case at 35 000. Establishment at 800 a
second is about ten frames and a kilobyte per cleartext connection, a few kilobytes with a TLS
handshake, under 40 Mbit/s either way.

**No table in this design is bound by the link, and that is arithmetic, not a prediction.** The
binding ceilings are the server's own, about 75 000 on loopback with a generator sharing its
cores, and the laptop generator's pacing over the wire, which nothing has yet measured. The
ladders of 3.4 measure both at one repetition before any timed run, so the ceilings are
pre-declared by measurement rather than discovered as refusals. Cut rule: if a ladder refuses a
table rate by achieved share, non-2xx or socket errors, the table is cut at the last admitted rate
below it and the cut is written in the results README before the design starts. A cut table is a
stated limitation; there is no re-laddering.

Where the link would bind, stated so nobody runs it unchanged: the `h1` design's payload sweep at
40 000. Payload 8 192 is six segments and about nine kilobytes on the wire per request, a ceiling
near 13 800 a second against an offered 40 000. `h1` is out of scope for this run; if it is ever
run over the wire, the 8 192 cell is capped at 10 000.

### 3.3 Repetitions, and the loopback prior for both halves

**n = 25 per cell.** The prior below is recomputed from the desktop's filed loopback `churn`
campaign (`measurements/2026-09-02-desktop-reevaluated/churn.jsonl`, 24 or 25 accepted runs per
arm) by exactly the recipe of section 1: `stats.compare` per cell, h in microseconds, and again
over each arm's first seven accepted runs in file order. This is a prior from a different medium.
It says what n buys if the wire's run-to-run spread is like loopback's, and says nothing if it is
not, which is the unknown this run measures.

| Cell | Statistic | Effect | h at n = 25 | h at n = 7 | Threshold |
| --- | --- | --- | --- | --- | --- |
| 25/s cleartext | `latency_ms.p50` | +30.0 us | 1.00 us | 2.50 us | 14 us |
| 50/s cleartext | `latency_ms.p50` | +30.0 us | 1.00 us | 1.50 us | 14 us |
| 100/s cleartext | `latency_ms.p50` | +28.0 us | 0.50 us | 1.00 us | 14 us |
| 150/s cleartext | `latency_ms.p50` | +29.0 us | 1.00 us | 2.50 us | 14 us |
| 100/s TLS | `connect_ms.p50` | +85.0 us | 14.8 us | 26.0 us | 30 us |
| 150/s TLS | `connect_ms.p50` | +61.0 us | 12.5 us | 20.5 us | 30 us |

Read it as the two halves it is. **Half A has fourteen to twenty-eight times the margin it needs
at n = 25 and five to fourteen times at n = 7**: it is not the half that decides n. **Half B has a
margin of 2.0 to 2.4 at n = 25 and 1.15 to 1.46 at n = 7.** Seven repetitions would put half B's
loopback resolution within half a factor of its own threshold before the wire has added anything,
which is why n = 25 and not 7, and why the number is chosen against the establishment half rather
than the request half. The earlier draft of this design justified n on `transport`, where the
effect is 67 to 94 percent of the baseline, and applied it to the half an order of magnitude
harder; that is the objection this table answers.

**The clock rule.** n is chosen **after block 4 and before block 5**, from measured wall clock per
run only, and never afterwards: 25 if the measured per-run wall clock times 977 fits in the
remaining window, otherwise 7. Blocks 1 to 4 run at one repetition whatever n is and produce 37
timed runs in about sixteen minutes, which is exactly the measurement the choice needs, including
the ssh session setup the table below cannot predict. "Wall clock only" is part of the rule: the
choice must not be readable as outcome-dependent, so no number from a block's records enters it.
No block runs at a third count, and no block's n is changed after a number from it has been read.

If a 7-repetition block is later extended, the extension is a second file with a second seed
(`--seed 20260904`; `ordering.plan` seeds each pass from `seed:repetition`, so the same seed would
replay the same orders), pooled with the first for h, with the repetition index read as pass
within file. This is the one exception to section 5's rule against pooling files, and section 5
names it. At n = 7 the design reports h as measured and, beside it and marked as a projection, h
scaled by the square root of 7/25.

### 3.4 The blocks, in order, with the commands

One run is 26 seconds: 20 measured, 3 warmup, 3 turnover, plus ssh session setup. The "arms"
column counts `Cell` objects, which are arms and not the cells of section 1; runs are arms times n.

| Order | Block | Arms | n | Runs | Time | Stopping here licenses |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | `smoke` | 2 | 1 | 2 | 1 min | the interface and the launch are right |
| 2 | `ladder` (10 000 to 120 000) | 12 | 1 | 12 | 5 min | the cleartext keep-alive ceiling |
| 3 | `tls-ladder` (5 000 to 60 000) | 12 | 1 | 12 | 5 min | the TLS keep-alive ceiling |
| 4 | `churn-ladder` (25 to 800, TLS only) | 11 | 1 | 11 | 5 min | the TLS establishment ceiling |
| 5 | `churn`, cores idle | 16 | 25 | 400 | 2.9 h | **both halves of the rule** |
| 6 | `churn`, cores awake | 16 | 25 | 400 | 2.9 h | the idle-state control (4.3) |
| 7 | `transport`, cores idle | 20 | 7 | 140 | 1.0 h | the keep-alive null and prediction 8 |
| | **total at n = 25** | | | **977** | **7.1 h** | |
| | total at n = 7 | | | 401 | 2.9 h | |

**Block 5 alone licenses the whole rule.** That is the point of the restructure: if the window
closes after block 5, both halves are answered and the night was not wasted. Block 7 stays at n = 7
whatever the clock rule chose, because it tests a null and not a resolution.

Block 4 is the **TLS** establishment ceiling and only that: `design_churn_ladder` is
`_base(tls=True, max_requests_per_connection=1)` with `protocol_detection=True`
(`run_campaign.py`), so no ladder in this run measures the cleartext establishment ceiling and it
is inferred from the TLS one. Said here so nobody reads block 4 as more than it is.

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
number, `git_commit` `<commit>`, and `admission_rules` reading exactly
`v2: achieved-share, non-2xx, socket-errors, environment; pacing recorded not gated`; and in
`smoke.env.json`, `generator_environment.build.git_commit` `<commit>` and
`generator_environment.cpu.governor` `performance`. Stop at the first field that is not so.

```
python -m benchmark.run_campaign --design ladder       --repetitions 1  --results <dir>/ladder.jsonl       <options>
python -m benchmark.run_campaign --design tls-ladder   --repetitions 1  --results <dir>/tls-ladder.jsonl   <options>
python -m benchmark.run_campaign --design churn-ladder --repetitions 1  --results <dir>/churn-ladder.jsonl <options>
```

The ladders are read by the admission rules of 4.1 and the cut rule of 3.2; the ceilings and the
per-run wall clock are written in the results README, and n is chosen, before the next command.

```
python -m benchmark.run_campaign --design churn --repetitions <n> --results <dir>/churn-idle.jsonl <options>
```

Then the spinners of 4.3 are started, two minutes pass, and:

```
python -m benchmark.run_campaign --design churn --repetitions <n> --results <dir>/churn-awake.jsonl <options>
```

Then the spinners are stopped by recorded PID, and:

```
python -m benchmark.run_campaign --design transport --repetitions 7 --results <dir>/transport.jsonl <options>
```

**Stop only at a block boundary**: a finished block is citable, half of one is not. If the window
closes before block 6 or 7, the blocks not run are the pre-declared next window and run unchanged.
Both clock samplers are stopped after the last block and their logs copied into `<dir>`.

## 4. Gates

### 4.1 Admission

As decided on 2 September: achieved share at or above 0.99; non-2xx at or below 0.1 percent; zero
socket errors; **zero TLS handshake failures**; **a TLS record must carry a negotiated version**;
and the environment gates: virtualisation, dirty tree, governor where the platform has one, clock
drift within 2 percent across the run on the server host, mains power, and the interface
expectation of 2.4. **Pacing p99 is a recorded covariate reported beside every cell, not an
admission rule**, and every record names the rule set that judged it (`admission_rules`, schema 9).
No threshold on any latency quantity. Rejections are counted and their reasons read before any
number is.

The two TLS gates are in `validity.check_run` and were omitted from the earlier draft's list. They
matter here in a way they did not on loopback, where a handshake essentially cannot fail: a TLS
churn cell at 150/s is roughly 3 000 handshakes per run and 75 000 per cell at n = 25, and any one
failure discards a 26-second run. The reading is pre-declared: **a handshake-failure refusal is a
property of the path or the server's accept queue, not of the arm**, is counted per cell, and is
never re-run. A cell whose accepted count falls below three in either arm is UNRESOLVED by section
1's own clause. Half B is the half exposed to this, and if it loses cells this way the results
README says how many and the half is reported as unresolved for want of runs rather than for want
of resolution — a different sentence, and the counts are what tell them apart.

### 4.2 The link

1. **Interface.** Every run reports `<iface>`, 1000 Mbit, full duplex, MTU 1500, or is refused.
2. **Where the reading "the link saturated" applies.** For payload 0 the link cannot bind below
   434 000 requests a second, and no table in this design offers a fifth of that. So **no refusal
   in this design is read as the link's.** This is a consequence of 3.2's arithmetic, not a
   finding, and it is not among the predictions of section 8 for that reason. The reading applies
   only at or above that rate, or in the `h1` payload sweep, which is not run.
3. **What a refusal by achieved share is read as.** Three things, told apart by fields already in
   the record. Pacing separates none of them and is not used for this: `next_due` advances only
   inside the issue loop, past the guard that skips a connection still awaiting a response
   (`loadgen.cpp`, `intended = next_due; next_due += period;` reached only past the `c.awaiting`
   test), so when every connection is awaiting, `next_due` stalls in the past, the connection that
   frees up takes that stale slot as its `intended`, and `pacing_us` is `Clock::now() - c.issued`.
   A server that holds connections therefore drives share **down and pacing up**, without bound
   over a 20-second window. The earlier draft's "server signature" — share falling while pacing
   stays in the tens of microseconds — cannot occur, and under it every refusal would have been
   charged to the laptop generator. (The generator's own comment, "advances by the period whether
   or not anything is free to send it", contradicts the code nine lines below it and is what the
   earlier draft trusted.) The three readings:
   - **Generator-bound**: `generator_cpu_fraction` at both threads' full share. Visible on the
     ladder, before any timed run.
   - **Closed loop saturated**: `achieved_rate x latency_ms.p50 / 1000` at or near 64, the
     connection count. The generator issues only on a connection not awaiting a response, so
     achievable throughput is capped at connections over mean per-request latency whatever the
     server does; over the wire the per-request floor rises, which puts that cap inside the window
     where the ladder's first refusal is expected. Then **path-bound** if `latency_ms.p50` at the
     refused rate is near its low-rate floor, **server-bound** if it is sharply above it. p50
     stands in for the mean the identity wants, and the substitution is stated wherever the
     reading is used.
   - **Server-bound outright**: share falling with the closed-loop identity well under 64.

   Each is a **ceiling finding about the arrangement**, written as such, and is a property of the
   server only in the last case and in the sharply-above branch of the second. The results README
   says which, and names the fields it read.
4. **Drops.** Read at neither end: `validity.read_counters` reads `/proc/net/*` only, so
   `counter_deltas` is empty on Windows and the zero-delta gate never fires, and the generator
   host's counters are not read. Socket errors, non-2xx and handshake failures remain admission
   rules and would carry a drop's consequences. Stated limitation.

### 4.3 Idle state

CPU idle state mediates low-load latency by hundreds of microseconds at the receiving end (the
laptop's 495 against 74 microseconds at 100 requests a second; its ping's 722 against 502). The
two ends differ in whether they can pay that cost:

- **The generator end spins.** For the last 20 ms before every due slot the generator spins rather
  than sleeps (`loadgen.cpp`, `kSpinBelowUs = 20000`). The periods are **per thread**, not per
  campaign: `rate_per_thread = opt.rate / opt.threads` with `GENERATOR_THREADS = 2`, so the
  inter-slot periods at 25, 50, 100 and 150 a second are 80, 40, 20 and 13.3 ms, twice the
  aggregate figures. At 150/s it spins with room. **At 100/s the margin is zero**: the per-thread
  period is 20 ms and the constant is 20 000 microseconds, and it spins only because `until` is
  computed after the issue loop from a `next_due` already advanced past a `now` that has moved on,
  so the comparison is strictly less than. A change to either constant flips the claim-carrying
  100/s cell into sleeping, which is why 2.2 refuses `<commit>` if either has moved. At 25 and 50
  a second it sleeps in 1 ms polls between slots. That sleep does **not** reach the pacing
  covariate: the last 20 ms before the slot is spun either way, so the slot itself is issued from
  an awake core and `pacing_us` never sees it. What it does add is a generator-side *receive* wake
  — after issuing, the thread polls with a 1 ms timeout, its core enters a shallow state, and the
  response arrives some tenths of a millisecond later into a core that has to come back. That cost
  lands in `latency_ms` in **both** arms equally, is a property of the generator host, and is not
  removed by the desktop's spinners, which is consistent with loopback's 25 and 50 baselines
  sitting 12 to 17 microseconds above 100 and 150. It is why prediction 5 expects a residual at 25
  and 50 in the awake file, and why that residual is not the control failing.
- **The server end may idle.** The desktop runs under the power plan `powercfg /getactivescheme`
  reports, recorded and unchanged, and between connections at 25 to 150 a second its cores have
  6.7 to 40 ms to park, past the 5 ms at which the measured wake cost plateaus.

So **`churn` is taken twice**, once as-is with cores idle and once with the server's cores held
awake, in two files, both reported. The idle file is the arrangement as the filed loopback
campaigns ran; the awake file is the control that says how much of the idle file is the idle
governor. It is the control for **both halves at once**, which is the second dividend of the
restructure: half A's cells at 25 to 150 a second are the low-load keep-alive-free condition the
earlier draft went looking for in `transport` and could not find there, because the harness has no
timed keep-alive cell in the designs this run uses below 5 000 a second. **On this direction the
control costs the generator nothing**: the spinners run on the server host and the generator is on
the other machine.

The Windows form of the documented control (the Linux form is sixteen `nice -n 19` spinners,
`design/socket-options-inventory.md` section 4): twelve PowerShell processes, one per logical
processor of the Ryzen 5 3600, each spinning, priority class Idle, started before block 6 and
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

**Two things the control confounds, and the witnesses for each.**

*Contention.* The earlier draft asserted that the server at Normal class preempts an Idle-class
thread immediately, "which is what makes the block a test of idleness and not of contention". That
is not established for this arrangement. Priority preemption is per logical processor. The server
is pinned to `0ff`, which the mask's own comment says is logical 0-7 and therefore physical cores
0-3, siblings adjacent; the twelve spinners are unpinned. On logical 8-11 they are unobjectionable
because no server thread is ever ready there. On 0-7 an Idle-class spinner on logical 1 is not
preempted by a Normal-class thread on logical 0, and those two share one physical core's front
end. The Linux precedent earned its claim empirically, 495 against 74 microseconds, on an
arrangement where the server was unpinned; nothing has earned it on Windows. So the claim is
replaced by a bound read from two fields already in every record and both live on Windows:
`server_cpu_seconds` (from `GetProcessTimes`, `benchmark/adapters.py`; set in `driver.py`) and
`requests_per_second`, compared idle against awake per cell. If the server's CPU seconds per
request rise materially in the awake file, the block measured contention as well as idleness and
prediction 5 is reported with that bound rather than as a clean control.

*Boost clock.* Twelve spinning processes hold the package at all-core load, which lowers the boost
clock available to the server's eight logical processors. Prediction 5 predicts the awake medians
fall; a boost-clock drop pushes them up, and the two are not separable from the record. The field
that looks like the witness is not one: `cpu_mhz_start` and `cpu_mhz_end` exist per run, but on
Windows they come from `_cpu_mhz_windows` (`environment.py`), which reads
`Win32_PerfFormattedData_Counters_ProcessorInformation` where `Name -eq '_Total'` — the **average
over all twelve logical processors**. Under the spinners the four idle CPUs go from parked to full,
so `_Total` *rises* while the server's own cores may be slower. The Linux probe is not the same
statistic: `_cpu_mhz_linux` takes `max(values)`, the fastest core, which is what the laptop
sampler's `sort -rn | head -1` also reads. And the drift gate compares start against end within
one run and cannot see a level shift between blocks at all. So 2.5 starts a desktop sampler that
takes the **maximum** over the per-processor instances, the same statistic as the laptop's, every
five seconds for the whole campaign. **Pre-declared: prediction 5 is not judged for a cell whose
awake-block sampled maximum clock is more than 2 percent below the idle block's**, the same 2
percent the drift gate uses; such a cell is reported with the clock figures beside it and no
idle-state sentence is written from it.

The spinners start two minutes before block 6's first run so the package clock is at its sustained
level before the drift gate's first sample; drift refusals in the awake block are counted and not
re-run. The awake condition is carried by no record field, so the two churn files are never
concatenated, and the results README names the condition of each.

### 4.4 The generator host's clock

The laptop throttles under sustained TLS: as a server on 2 September it fell from 4 067 to 3 322
MHz inside a twenty-second run and its TLS half was declared unusable. As direction A's TLS
generator it does the symmetric record-layer work on two spinning threads, and no gate reads its
clock. The witness is the sampler of 2.5: every five seconds, a timestamp and the fastest core's
`cpu MHz`, at nice 19, for the whole campaign, beside the pacing covariate per run.

Reading rule, fixed now: **for every run of every block**, the lowest sample inside its window
against the campaign's first sample, at the same 2 percent the drift gate applies to the server.
The earlier draft applied this to TLS `transport` runs only, which is a sixth of the campaign and
none of block 5; the sampler runs all night regardless, the laptop's governor is set by hand and
survives no reboot, and nothing else gates the generator host per run, so restricting the rule
gave away the only per-run governor witness the design has for free. The consequence is still
reported per half: if any TLS cell has a run below the 2 percent bar, or pacing p99 rises with
repetition in the TLS cells and not in the cleartext ones, half B is reported with that caveat and
no TLS-against-cleartext sentence is written from it, by the standard the laptop's own re-run
applied to itself. Half A is cleartext and the generator does no cryptography there.

## 5. What may and may not be compared

**Within this arrangement:** arms against each other. Classification on against off in every cell,
with its interval; TLS against cleartext at a shared rate; cores awake against idle at the same
cell. These are the only magnitudes this run produces.

**Against loopback and against the virtual-switch arm:** the resolution and the shape only.
Whether h is under its threshold in the same cell; the sign of the difference; its trend with
rate; whether the bimodality at 25 and 50 is present in `connect_ms`. **Compare h, in
microseconds, and not the share of the baseline** — the share's denominator is the thing the
change of medium moves, so two shares from two media are two ratios whose levels differ by exactly
what is under study. Never magnitudes, for the same reason and three more: the medium changed, the
commit changed, and the server host's spare cores changed from hosting the generator to idling.

**One carved exception, admissibility ceilings.** The highest rate a ladder admits is a yes-or-no
per rate, not a latency, so it does not carry the medium's added level the way a median does. The
ceilings may be compared across media — loopback's establishment ceiling of about 330 a second,
the virtual-switch arm's past 800, and whatever this run's block 4 admits — and that comparison is
the design's stated reason for running the ladder at all. It is still written with a hedge and not
a causal verb: "consistent with the loopback ceiling having been the arrangement's", never
"confirms".

**Across platforms:** never. Direction A's Windows server against direction B's Linux server, or
either against paper 3's filed Linux arms, is shape only, because the two ends are different
hardware.

**Across files:** never, with one exception declared in 3.3 — a same-`<commit>`, same-arrangement
extension of a 7-repetition block under a second seed, pooled with the block it extends. The idle
and awake churn files are not concatenated; `transport` is not pooled with `churn`; nothing
measured here is pooled with anything filed, and the harness refuses that last attempt where it
can see it (`transport_mismatch` on `loopback`, `generator_location`, `server_location`,
`netem_profile`). The harness refuses none of the others — eight files are eight campaigns to it —
so those are discipline, and this paragraph is where the discipline is written down.

## 6. Direction B, later

Linux server, epoll and io_uring, for paper 3's arms over a real path; Windows generator; driver
on the laptop, by the placement rule of 2.1, reaching the generator over ssh. That needs the
OpenSSH Server feature on Windows and a key: a system change and Alex's decision, so direction B
is deferred until it is made.

It is second because **the generator end sleeps.** The Windows generator's 1 ms poll is quantised
to the raised system timer. Per thread and not per campaign, by the same divisor as 4.3: at 25 and
50 establishments a second the per-thread periods are 80 and 40 ms, well past `kSpinBelowUs`, so
it sleeps between slots and charges its own wake to the system under test; at 100 a second the
per-thread period is 20 ms and the margin is zero, as on Linux but with a coarser tick behind it;
at 150 and above it spins. Three more things follow from the code and are stated now:
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
the fingerprinted machine key, and every record's `generator_argv` carries the ssh target. The
directory holds every `.jsonl` and `.env.json`, `generator-environment.json`,
`generator-qdisc.txt`, both clock logs, `spinners.txt`, `desktop-power.txt`,
`desktop-adapter.txt`, and a README that names: `<commit>`, one commit for both hosts, with the
note that the run is refused if they differ, and that neither `GENERATOR_THREADS` nor
`kSpinBelowUs` moved in it; both builds, `build/windows-tls` Release on the desktop and the
laptop's build directory and generator path, with the `ldd` TLS check's result; the fingerprint
from `.env.json`; the interface fields as recorded, `local_interface`, speed, duplex and MTU; the
power plan and the adapter's advanced properties; the qdisc; whether the wireless interface was
taken down; the n chosen, the measured per-run wall clock it was chosen from, and the clock time
at block 4's end; any table cut and why; the rejection count and every reason per file, handshake
failures called out separately; and, for each churn file, whether cores were allowed to idle.

Into the framework repository, which is public, go this design, the harness change of 2.3 and its
documentation, all with placeholders. No hostname, address, MAC, network name or router detail,
and no description of the network beyond one gigabit Ethernet segment, two hosts.

## 8. Pre-registered predictions

Stated so the outcome is judged rather than explained. The claim that no rate is refused for the
link is not among them: it is 3.2's arithmetic, no measurement in this design could contradict it,
and a design constant restated as a result is not a prediction.

1. **The ceilings.** The cleartext keep-alive ladder admits 70 000, and its first refusal lies
   between 80 000 and 120 000, read by one of the three readings of 4.2(3) and most likely the
   closed-loop one, since 64 connections over a wire-inflated per-request floor puts that cap
   inside the window. The TLS keep-alive ladder admits 35 000. The TLS churn ladder admits through
   800. Consistent with the loopback establishment ceiling of about 330 a second having been the
   arrangement's rather than the server's, since the generator here has its own cores; written
   with that hedge, per section 5.
2. **Half B resolves at n = 25.** Both TLS establishment cells, 100 and 150, come in under 30
   microseconds of half-width. The path's contribution to a per-run median is small against a 1.4
   ms baseline (a per-packet spread of 300 microseconds over 2 000 to 3 000 connections a run
   gives a standard error near 7 microseconds), so h over the wire is set by run-level state, and
   at 100 and 150 loopback showed none in 198 runs. If this is wrong, the failure is a second mode
   in the medians, not a wide unimodal spread, and the repetition plot shows it. This is the half
   with the smaller margin and it is the one to watch.
3. **Half A resolves in all four cells at n = 25.** Loopback's h there is 0.5 to 1.0
   microseconds against a 14-microsecond threshold, and the wire would have to inflate the
   run-to-run spread of a request median fourteenfold to break it at 100 and 150. **25 a second is
   the half-A cell to watch**, and not for the bimodality, which is in the other statistic: it is
   500 requests a run against 2 000 to 3 000 at 100 and 150. Prediction 2's own arithmetic applied
   there — a 300-microsecond per-packet upper bound over 500 requests — gives a within-run standard
   error of the median near 17 microseconds and h near 9 to 10 microseconds at n = 25 from sample
   count alone. Under the threshold, but by a factor of about 1.5 and not fourteen, so the word for
   that cell is not "comfortably".
4. **`connect_ms` stays bimodal over the wire at 25 and 50**, with the slow-mode share in
   loopback's range. The state lives in the server host, not in a loopback generator sharing its
   cores. `latency_ms` at the same two rates stays unimodal, as it is on loopback. If instead no
   slow mode appears in `connect_ms` over the wire, the state was the loopback arrangement's,
   which is the more useful outcome for paper 2 and is written as such.
5. **The idle mediator shows at the server end unless cores are held awake, in both statistics,
   and does not fall across this rate range.** In the awake file the classification-off
   `connect_ms.p50` is lower than in the idle file at every rate from 25 to 150, and so is the
   classification-off cleartext `latency_ms.p50`; the demultiplexing difference itself is
   unchanged within its interval in both halves, the idle cost being added to both arms. The shift
   does not shrink between 25 and 150 because every per-thread gap there, 80 down to 13.3 ms, is
   past the 5 ms at which the wake cost plateaus. No magnitude is predicted: the plateau figures
   in hand (1 ms gives 9 microseconds, 2 ms 26, 5 ms and beyond 63) and the 495-against-74 pair
   are the laptop's, and the desktop's wake ladder has never been measured. Judged only for cells
   that pass 4.3's boost-clock bar.
6. **The awake control costs the generator nothing.** The awake block's **median** pacing p99 lies
   inside the idle block's interquartile range, per cell, and both stay in the tens of
   microseconds. A range test over 400 idle runs is a minimum-to-maximum span that any plausible
   shift hides inside, so the check the design leans on to show the spinners did not reach the
   generator is stated on quartiles and cannot come out true by construction.
7. **The laptop's clock falls under the TLS churn arm** by less than 2 percent, and half B carries
   no clock caveat. TLS establishment at 150 a second is 150 handshakes a second on two threads,
   two orders below the sustained record-layer work at which the laptop throttled as a server, so
   the throttling that made its TLS half unusable there is not expected here. If it falls anyway,
   half B carries 4.4's caveat and the fact is recorded.
8. **The keep-alive null holds over the wire, and magnitudes for shape only.** In block 7 the
   `transport` difference of medians stays under 2 percent of the classification-off median in all
   ten cells at n = 7, as the arithmetic of section 0 requires: the once-per-connection cost is
   still divided by tens of thousands of requests, and no medium changes that. Request-latency
   medians over the wire are two to four times loopback's, the path's round trip added;
   establishment medians are within a factor of two of loopback's fast mode. Neither magnitude is
   a comparison the papers may draw; they are recorded so the medium's effect is judged and not
   explained.

## 9. Objections considered

Every finding from the two red-team passes was taken except as noted here. Two were taken in a
different form than proposed, and the reasoning belongs on the record rather than in a message.

**A single absolute threshold for both halves, proposed as "half-width < 60 microseconds, or < 30
for a factor-two margin, for both statistics".** The diagnosis was right and is section 1's rule;
the single number was not taken. The two claim-carrying effects are 28 to 30 microseconds and 61
to 85, so one threshold is either slack for half B or unreachable for half A: at 30 microseconds
half A would pass with a half-width larger than the whole effect it is looking for. Two
thresholds, each half the effect of its own half. The cost is that the rule cannot be quoted as one
number, and section 1's table carries both.

**"If the filed records will not support a power figure for the establishment half, say so."**
Not taken, because they did support one. The prior in 3.3 is computed from
`measurements/2026-09-02-desktop-reevaluated/churn.jsonl` by the same recipe the run will use, and
it is what moved n from a number justified on `transport` to a number justified on the half with
the smaller margin. The disclaimer would have been the honest fallback; the table is better.

**"Pre-declare that prediction 4 is not judged for a cell whose awake-block median differs from its
idle-block median by more than 2 percent."** Taken, but restated on the clock and not on the
median. As written the bar would void the prediction precisely when the prediction came true: the
awake-minus-idle shift in the median *is* what prediction 5 (prediction 4 in the draft reviewed)
measures. What the finding's own argument needs is a bar on the confound, so 4.3 states it on the
sampled maximum clock, which is the quantity the boost-clock objection is about.

Two further departures are not objections but corrections, recorded here for the same reason.
Section 0 states the first: the brief's description of the effect conflated two cells, and the
`transport` design cannot carry the claim. Section 3.1 states the second: 25 and 50 a second are
claim-carrying for `latency_ms` and diagnostic only for `connect_ms`, where the earlier draft made
them diagnostic for both.

## 10. Why this file exists

A design declared in advance and then reported afterwards is only as good as the evidence that it
really was declared in advance. Alex's decision of 2 September was that the two-host run is
approved for overnight and its design is pre-declared and committed before it runs. Every rule
above is fixed now: the six cells, the two statistics, the two thresholds in microseconds, the
rates, n and the clock that chooses it, the admission rules, the reading of every refusal, the
conclusions in both directions and the predictions. A rule added after the records exist is a
filter on the outcome, and this file's timestamp is the evidence that none was.
