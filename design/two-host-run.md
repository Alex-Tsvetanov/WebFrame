# The two-host run: what the harness can do for a generator on another host, and the design fixed before it runs

Both papers state loopback as a limitation. One measurement can replace that sentence with a
number or show that loopback was the right medium: the framework's own load generator against the
framework's own benchmark server across one gigabit Ethernet segment between two hosts, a Linux
laptop and a Windows desktop, instead of over loopback, a namespace pair or a virtual switch inside
one host.

This document does three things. It establishes, from the code and with line references, what the
harness can and cannot do when the generator is on another host, so the design promises nothing the
tool cannot deliver. It fixes the design: arrangement, rates, repetitions, order, and the decision
rule with both outcomes worded in advance. And it is the runbook: the laptop and the desktop run it
from this text alone. Anything that identifies a machine beyond its hardware class is deliberately
absent; addresses, user names and paths on the two hosts are exchanged by message and appear here
as placeholders in angle brackets.

## 1. The one quantity that decides it

Paper 2's demultiplexing cost is 60 to 85 microseconds on a classification-off median of about
1.4 ms establishment or 0.09 ms request latency. Whether a real path can resolve that is not decided
by the per-packet spread of the path (a ping statistic; see the last two entries in
`.claude/HANDOFF.md` under "NEW CAPABILITY"). We compare per-run medians, and the standard error of
a median over a hundred thousand requests is a few microseconds if the noise is independent. What
enters the bootstrap is the run-to-run spread of those medians, and the harness already reports it:
`stats.compare` (`benchmark/harness/stats.py:161-216`) bootstraps the difference of medians between
the two arms, and `results2tex` reports the half-width of that interval as a share of the
classification-off median (`benchmark/harness/results2tex.py:296-316`), with 5 percent as the
reportability floor (`stats.py:148`). That share is the resolution, and the decision rule in section
7 is stated on it and on nothing else.

## 2. What the harness does today for a generator that is not on this host

### 2.1 How the generator is launched

`run_campaign.py` finds the build's own generator at `<build>/benchmark/loadgen[.exe]`
(`benchmark/run_campaign.py:826-830`) and constructs one of three launchers:

- no prefix: the local binary (`adapters.py:1176`);
- `--wsl-distro D --wsl-loadgen P`: `["wsl.exe", "-d", D, "--", P]` (`run_campaign.py:865`), with the
  result path translated to the other side of the WSL boundary (`adapters.py:1109-1120, 1171-1172`);
- `--generator-command PREFIX --generator-location LABEL`: `shlex.split(PREFIX) + [str(gen_bin)]`
  (`run_campaign.py:877-879`), the namespace arrangement, where the prefix enters a namespace that
  shares the filesystem.

`LoadgenGenerator._argv` appends `--host`, `--port`, `--connections`, `--threads`, `--duration`,
`--warmup` and `--out <work_dir>/loadgen-<port>.json` (`adapters.py:1174-1204`), runs it with
`subprocess.run(..., capture_output=True, text=True, timeout=duration+warmup+120)`
(`adapters.py:1240-1243`), and reads the result back **from the file**, not from stdout: a missing
file is the only failure it recognises (`adapters.py:1249-1258`). The generator writes the same JSON
to stdout and, when `--out` was given, to the file (`benchmark/generator/loadgen.cpp:1821-1830`);
stdout carries nothing else (`usage()` at `loadgen.cpp:1394` prints only on `--help`). A failed
`fopen` of the `--out` path is silent (`loadgen.cpp:1825`).

### 2.2 What breaks if the prefix is `ssh user@host`

Passing `--generator-command 'ssh <target>'` fails at four places, in this order:

1. **The binary path.** The appended `gen_bin` is the driver host's path (`run_campaign.py:826,
   878`). Driven from the desktop that is a Windows path to a Windows executable, executed on the
   laptop; driven from the laptop it is the laptop's own build path, which happens to be right only
   if the remote checkout is at the same absolute path.
2. **The result file.** `--out` names a directory on the driver host (`adapters.py:1184, 1208`).
   On the remote host the `fopen` fails silently, the JSON goes to stdout, and the driver refuses
   the run with "generator produced no result file" (`adapters.py:1249-1253`). Every run fails the
   same way. `--samples` (`adapters.py:1200-1201`) would likewise write on the remote host and
   record a path that exists nowhere.
3. **The affinity mask.** `GENERATOR_AFFINITY = "f00"` is passed unless the generator is in WSL
   (`run_campaign.py:71-72, 1087`). On the laptop that pins the generator to logical CPUs 8 to 11 of
   a different machine, and `isolation_problem` (`run_campaign.py:99-130`) checks the mask against
   the driver host's sibling map, not the generator host's.
4. **The environment.** Every gate and every fingerprinted field describes the driver host
   (section 4).

What does **not** break, and is why the change in 2.3 is small:

- **The euid gate.** The three refusals under a launch prefix fire only when the *server* has one
  (`driver.py:328-351`). With a local server, `result.euid` is recorded (`driver.py:352`) and the
  asymmetry check at `driver.py:398-410` is skipped whenever `server_euid` is None, which it is on
  Windows (`adapters.py:783-784`). A Linux generator's euid is read by the generator itself
  (`loadgen.cpp:1750`) and survives remoting.
- **Readiness.** `wait_until_ready` probes the server's own loopback when the server has no prefix
  (`adapters.py:678-683`). That is the correct host for readiness. It does not prove the server is
  reachable from the generator host; the runbook's one-request-from-each-end precondition does.
- **The clock and the host gates.** `probes.cpu_mhz`, `power_source`, `speed_limit`, `governor`
  and `counters` are host-level, not pid-level (`driver.py:163-167, 243-245, 289-291, 494-499`).
  They read the driver host, which in the recommended placement is the server host.
- **Generator CPU.** Self-reported by the generator from `getrusage` (`loadgen.cpp:1355-1379`),
  so it needs no local pid.
- **The interface field, pacing, achieved share.** All in the generator's JSON
  (`loadgen.cpp:1756-1759, 1801-1806, 1863`), read by `adapters.py:1294-1295, 1311-1314`.
- **Quoting.** The launcher is a list handed to `subprocess.run`; `ssh` joins it with spaces and
  the remote shell splits it again. Every argument the harness passes is a bare word or a number;
  the one path, the remote generator, is chosen without spaces.
- **Timeouts.** `duration + warmup + 120` seconds (`adapters.py:1242`) covers ssh session setup.

### 2.3 The change: mirror the WSL branch, read the JSON from stdout

Two files, about thirty lines, no new dependency. The WSL branch is the template
(`run_campaign.py:848-866`).

`run_campaign.py`:

- `--ssh-generator USER@HOST`, `--ssh-loadgen PATH` (the generator binary on that host),
  `--ssh-repo PATH` (the repository checkout on that host), used together with the existing
  `--generator-location LABEL`. Refused: `--wsl-distro`, `--generator-command`, `--server-command`
  and `--samples` in combination with it.
- `gen_command = ["ssh", "-o", "BatchMode=yes", "-o", "ConnectTimeout=5", target, loadgen]`;
  `location = LABEL`. `BatchMode` is what makes a missing key a refusal rather than a prompt, the
  same reason `netns.py` uses `sudo -n` (`benchmark/netns.py:128-134`).
- `affinity_mask=None` for the ssh generator, on the WSL branch's reasoning
  (`run_campaign.py:1083-1087`): a mask names the driver host's cores.
- Once, before `Campaign.open_or_create`: `ssh target "cd <repo> && python3 -c 'import json;
  from benchmark.harness import environment as e; print(json.dumps(e.capture()))'"`, stored in the
  manifest as `env["generator_environment"]`, with the output of `tc qdisc show dev <iface>` added
  under `generator_environment["qdisc"]` when `--expect-interface` names one. Refuse to start if
  `generator_environment["build"]["git_commit"]` differs from `env["build"]["git_commit"]`: both
  binaries from one commit is the README's rule (`benchmark/README.md:44-52`), and
  `build_staleness` (`run_campaign.py:838`) cannot see a remote binary.

`adapters.py`, `LoadgenGenerator`:

- `result_from_stdout: bool = False`. When set, `_argv` omits `--out`, and `run` parses
  `json.loads(proc.stdout)`; unparseable stdout is `RunFailed` quoting stderr and the exit code.
  Exit 3, the generator's own inadmissibility verdict (`loadgen.cpp:1865-1883`), still has the JSON
  on stdout, because the write at `loadgen.cpp:1821` precedes it, and is parsed: the verdict belongs
  to `validity.py`, once (`adapters.py:1245-1248`). Exit 255 is ssh failing and has no JSON.

One self-check: a fake `subprocess.run` returning JSON on stdout with exit 3 produces a
`GeneratorResult`; one returning empty stdout with exit 255 raises `RunFailed`.

Nothing changes in `driver.py`, `schema.py`, `validity.py` or `environment.py`. The schema stays
at 8. `generator_environment` is a manifest key, not a record field and not fingerprinted; that
limitation is stated in section 4.

### 2.4 Placement: the driver runs on the server host

Two placements were compared. **Driver on the server host, generator remote** needs the change in
2.3 and nothing else. **Driver on the generator host, server remote** would need `CorouteServer`
rewritten in five places: `refuse_held_port` runs `ss -ltnH` through the prefix
(`adapters.py:288-305`), which Windows does not have; `_connect_once` runs `sys.executable -c`
through the prefix (`adapters.py:695-709`), a Linux interpreter path on Windows; `server_pid` walks
`/proc` and falls back to `ss` (`adapters.py:711-757`), answers None, and `stop` then refuses every
run as unlocatable (`adapters.py:1039-1062`); `stop` signals with `sudo -n kill`
(`adapters.py:1072-1073`); `server_euid` returns None under a prefix, which `driver.py:335-339`
refuses; and every host gate would judge the generator host while the server host, the one whose
throttling matters, went unread. The cost estimate is roughly one to two hundred lines against
thirty.

So the rule, for both directions: **the driver runs where the server runs, and the generator is
reached over ssh.** Direction A (Windows server, Linux generator) is driven from the desktop and
needs an ssh client on Windows, key authentication that succeeds under `BatchMode`, and sshd on the
laptop. Direction B (Linux server, Windows generator) is driven from the laptop and needs an sshd on
Windows, which is an optional Windows feature and a system change; it is deferred to section 9.

## 3. The arrangement, and what records it

**One gigabit Ethernet segment, two hosts, no routed hop.** Both ends negotiate 1 Gbit/s full
duplex, MTU 1500. The Linux end's egress qdisc is `fq_codel` and is part of the path. The Linux end
is dual-homed, wired and wireless on one subnet with only a route metric between them. Windows'
ICMP stack reports whole milliseconds, so no ping baseline exists from that end; the Linux end
pinging the gateway on the same wire gave a median of 722 microseconds, 502 with cores held awake,
standard deviation about 300, an upper bound because a router answers ICMP on a low-priority path.
(All of the above from the coordinator's record; not re-measured here.)

**What the records say about where the load came from.** `transport_path` in the campaign manifest
carries `host`, `loopback`, `generator_location`, `server_location` and `netem_profile`
(`run_campaign.py:988-1000`). `transport_mismatch` compares every key except `host` against the
manifest on disk and refuses the append on any difference (`run_campaign.py:179-210`). A two-host
campaign records `loopback=False` (the address is not loopback, `run_campaign.py:897`),
`generator_location="lan:linux"`, `server_location="host"`, `netem_profile="none"`. That refuses
pooling with loopback (`loopback=True, generator_location="host"`), with the namespace pair
(`generator_location="netns:gen"`, `server_location` the netns prefix) and with WSL
(`generator_location="wsl:<distro>"`). Every record also carries `generator="coroute-loadgen-lan"`,
derived from the label's prefix (`adapters.py:1164-1169`). The label is a fact about the
arrangement, not about a machine, and the manifest's `host` key and every record's
`generator_argv` carry the server's address (`run_campaign.py:989`, `adapters.py:1178, 1315`) and
now the ssh target, exactly as the WSL records already carry `--host <gateway>`. Results therefore go
to the private paper repositories, as they do today (`README.md:156-160`).

**`netem_profile="none"` is truthful and incomplete.** It says no netem impairment was applied
(`run_campaign.py:912-913`); `observed_profile` parses only netem lines (`netns.py:94-111`), so
`fq_codel` is recorded nowhere by the harness. `generator_environment["qdisc"]` (section 2.3)
records it.

**The interface field.** Read by the generator from its own established socket with
`getsockname`, matched to an interface name, the address discarded, and the link's speed, duplex
and MTU read from `/sys/class/net` (`loadgen.cpp:383-426, 349-374`). It therefore describes the
**generator host's** interface, which is what a dual-homed generator host needs checked. The driver
records it before judging (`driver.py:356-359`) and, when `--expect-interface` is given, refuses a run
whose interface is absent or different (`driver.py:366-379`). A refusal is a rejected record with
its reason, the server is stopped, and the campaign continues (`driver.py:445-446, 478-480,
505-506`); it is not an abort, which is why the smoke run in section 8 comes first: a route metric
that puts traffic on the wireless interface would otherwise cost 26 seconds per refused run for the
whole night. The Windows generator reports no interface (`loadgen.cpp:427-430`), so
`--expect-interface` refuses every run of direction B (`driver.py:367-372`); see section 9.

**Socket policy and the commit.** The desktop's 2 September campaign is at `b4a01e8c7`, schema 7,
IOCP with no `TCP_NODELAY`: Nagle was on (`include/coroute/net/socket_options.hpp:5-8`). Mainline
has the shared policy, Nagle off everywhere, `TCP_QUICKACK` where present, `SO_SNDBUF` 256 KiB
(`src/net/socket_options.cpp:38-48`), and schema 8 with the interface field. The generator has had
Nagle off throughout (`loadgen.cpp:256-260`). **The desktop is rebuilt at mainline for this run.**
Two reasons, the second sufficient on its own: Nagle on the server over a real path is a known
confound, a request's small response waiting on an ack; and a schema 7 adapter does not read the
interface field at all, so the dual-homing check is impossible without it. The cost is that the
two-host cells are a separate population from paper 2's loopback data at `b4a01e8c7`, which they
are anyway by `transport_mismatch`. The comparisons that remain licensed are within this campaign
(classification on against off, over the wire) and, for shape only, against the loopback and WSL
tables. **Single-commit precondition:** the run needs, in one commit that both hosts check out,
schema 8 and the socket policy (mainline), the pacing-as-covariate change (landing on
`harness/pacing-covariate`; `validity.py:132-137` in this tree still refuses on pacing over 1 ms),
and the ssh change of 2.3. The commit is named in the results directory's README and is the commit both binaries carry.

**Server affinity stays `0ff`** (`run_campaign.py:70, 1094`). The server's configuration is the
one paper 2's loopback data used; only the path changes. The desktop's remaining cores sit idle
rather than hosting the generator, which is itself a difference from the loopback arrangement and
is stated.

**Idle state.** CPU idle state mediates low-load latency by hundreds of microseconds at the
receiving end (coordinator's record; the laptop's 495 against 74 microseconds at 100 requests a
second). In direction A the receiving end is the Windows server and its cores are **allowed to
idle**, under the power plan `powercfg /getactivescheme` reports, recorded in the results README.
No awake control is taken in direction A: the question is within-arm (classification on against
off under one idle regime), the keep-alive rates of 5000 and above keep the server busy every
hundred microseconds or so, and the churn rates are the ones paper 2's loopback data was taken at
under the same regime. What idle state can do to this design is inflate the run-to-run spread of
the churn medians, which is exactly what the resolution figure reports; section 7 says what follows
if it does. The generator's own waiting is the same code on both platforms: it spins when the next
due slot is under 20 ms away and otherwise waits in 1 ms polls (`loadgen.cpp:1126-1153`,
`kSpinBelowUs` at 1150-1151). So at churn rates of 25 and 50 per second either generator sleeps
between slots, at 100 and above either spins, and every keep-alive rate spins. The platform
difference is the accuracy of the 1 ms poll's wake (Windows raises the timer to 1 ms,
`loadgen.cpp:1509-1515`), not sleeping against spinning.

## 4. Environment and fingerprint: which host is described

`environment.capture` runs on the driver host (`run_campaign.py:972-974`) and the fingerprint hashes
that host's node, kernel, CPU, governor, memory, tuning, toolchain and build
(`environment.py:54-75, 1010-1019`). In placement A that is the **server host**, which is the right
host for every question the fingerprint and the per-run gates ask: the drift gate compares two
clock samples taken on the driver host inside the measured window (`driver.py:286-291, 420, 494`;
Windows reads base frequency times performance percentage, `environment.py:253-278`) and asks
whether the machine under test throttled; the power and governor gates ask whether the machine
under test is on mains and at a fixed clock (`validity.py:154-177, 245-259`).

What the manifest then says nothing about is the generator host: its kernel, CPU, governor, power
source, clocksource, its qdisc, and whether it was on battery. **Nothing in the harness refuses a
laptop generator on battery or on the powersave governor.** The laptop's governor was set by hand
and does not persist across a reboot. Two measures, both required:

1. A manual precondition on the laptop, verified by command (section 8.1): mains, `performance`,
   `tsc`.
2. `generator_environment` captured once at campaign start into the manifest (section 2.3), by the
   same `environment.capture()` the fingerprint uses, so the two hosts are described by one parser.

Stated limitation: the generator host is described once per campaign, not per run, and is not
fingerprinted, so a generator host that changed governor partway through would not be refused. Its
per-run evidence is the pacing p99 covariate, recorded beside every cell, and its achieved share,
which is an admission rule.

**A known risk under that limitation: the laptop throttles under sustained TLS.** As a server in
its own transport re-run it fell from its start clock to about 3300 MHz inside a twenty-second
run and the TLS half was declared unusable (coordinator's record of 2 September, late evening). As
the TLS generator of direction A it encrypts every request and decrypts every response at up to
35 000 a second on two spinning threads, and no gate reads its clock. Two measures, fixed now:
the clock the drift gate would read (`environment.py:224-250`, the fastest core's `cpu MHz`) is
taken over ssh immediately before and immediately after the transport design and written into the
results README; and the pacing covariate is plotted against repetition index for the TLS cells.
If the laptop's clock fell by more than the drift gate's 2 percent across transport, or pacing
rises with repetition in the TLS cells and not in the cleartext ones, the TLS half of transport is
reported with that caveat and no TLS-against-cleartext sentence is written from it, by the same
standard the laptop's own re-run applied to itself. The cleartext half and every other design are
unaffected: the generator does no cryptography there.

Clock: latency is measured at the generator from `std::chrono::steady_clock` (`loadgen.cpp:115`),
from intended send time to response, so no clock synchronisation between the hosts is needed and
none is assumed. The drift gate gates the server host's clock; the generator host's clock stability
has no gate and is described by `generator_environment.cpu.governor` and
`generator_environment.tuning.clocksource`.

Kernel drop counters: `counter_deltas` reads `/proc/net/snmp` on the driver host
(`driver.py:243, 499`; `validity.py:275-306`), which on Windows yields nothing (`README.md:176-178`),
and the generator host's counters are not read. Drops on the real path are unobserved at both ends;
socket errors and non-2xx remain admission rules and would show a drop's consequences.

## 5. The link ceiling, computed from the bytes the h1 design sends

The brief's estimate of 1100 bytes on the wire per request and response saturating the link near
70 000 requests a second was a guess. Computed from the request the generator builds
(`loadgen.cpp:1558-1561`, path `/` at 123) and the response the server serialises
(`include/coroute/core/response.hpp:105-115`, `src/core/app.cpp:796-797`, timeout 30 s at
`include/coroute/core/app.hpp:199`, body `Hello, World!` at
`examples/Samples/benchmark_server/main.cpp:293, 439`):

| Keep-alive, payload 0 | bytes |
| --- | --- |
| `GET / HTTP/1.1\r\n` | 16 |
| `Host: <13-character address>\r\n` | 21 |
| `Connection: keep-alive\r\n` | 24 |
| `User-Agent: coroute-loadgen\r\n` | 29 |
| blank line | 2 |
| **request payload** | **92** |
| `HTTP/1.1 200 OK\r\n` | 17 |
| `Content-Type: text/plain\r\n` | 26 |
| `Content-Length: 13\r\n` | 20 |
| `Connection: keep-alive\r\n` | 24 |
| `Keep-Alive: timeout=30\r\n` | 24 |
| blank line, body | 2 + 13 |
| **response payload** | **126** |

Per frame: 8 preamble, 14 Ethernet, 4 FCS, 12 inter-frame gap, 20 IPv4, 20 TCP with no options
(timestamps are negotiated only if both ends enable them; Windows does not by default; if on, add
12 per segment). Request frame 170 bytes, response frame 204, a pure acknowledgement 84 (the
64-byte minimum frame plus 20 of preamble and gap). The heavier direction is server to generator:
204 bytes per request with the request's acknowledgement carried on the response, 288 if a pure
acknowledgement precedes it. **Link ceiling for payload 0: 1e9 / (288 × 8) = 434 000 requests a
second, worst case; 613 000 with acknowledgements piggybacked.** At the ladder's top of 70 000 the
link carries 114 to 161 Mbit/s, 11 to 16 percent of capacity, and 70 000 to 140 000 frames a second
per direction. The link is not the binding ceiling for any cell of transport, churn or h1-deep. The
binding ceilings are the server's own, measured at about 75 000 on loopback with a generator
sharing its cores (`run_campaign.py:147-153`), and the laptop generator's pacing over the wire,
which no ladder has yet measured. Both are found by the ladders in section 8.3, at one repetition,
before any timed design, so the ceiling is pre-declared by measurement rather than discovered as
refusals.

TLS (one record per message, 22 bytes of record overhead): 226-byte response frame; at 35 000 a
second, 87 Mbit/s worst case. Churn at 800 connections a second: about ten frames and under two
kilobytes per connection, 13 Mbit/s; the constraint there is the accept path and the generator's
two-second connect timeout (`loadgen.cpp:236-246`), not the link.

Where the link **would** bind, stated so nobody runs it unchanged: the `h1` design's payload sweep
at 40 000 (`run_campaign.py:298-303`). Payload 1024 is 418 Mbit/s worst case, admissible. Payload
8192 is 6 segments, about 9 kilobytes on the wire per request, a ceiling of about 13 800 a second
against an offered 40 000. `h1` is out of scope for this run; if it is ever run over the wire the
8192 cell is capped at 10 000.

**Rate tables, pre-declared.** The existing ones, because they are the loopback tables and shared
rates are what make shape comparison possible: `_WINDOWS_RATES` 10k, 25k, 40k, 55k, 70k for
h1-deep (`run_campaign.py:153`); `TLS_OFFERED_RATES` 5k, 10k, 15k, 25k, 35k for transport
(`run_campaign.py:369`); for churn, `CHURN_NET_OFFERED_RATES` 50, 150, 400, 800
(`run_campaign.py:465`) if the churn ladder admits 800 by the admission rules of section 6, else
`CHURN_OFFERED_RATES` 25, 50, 100, 150 (`run_campaign.py:389`). That choice is made by the ladder
and by that rule, before the first timed run, and is written into the results README.

## 6. Admission

As decided on 2 September: achieved share at or above 0.99 (`validity.py:52, 138-143`), non-2xx at
or below 0.1 percent (`validity.py:34, 110-118`), zero socket errors (`validity.py:202-207`), and
the environment gates: virtualisation, dirty tree, governor, clock drift, power, interface
(`validity.py:87-108, 154-177, 245-259`; `driver.py:366-379`). Pacing p99 is a recorded covariate
reported beside every cell, not an admission rule. No threshold on any latency quantity. Rejections
are counted and their reasons read before any number is (`README.md:154-156`).

## 7. Design, repetitions, and the decision rule worded in advance

**Direction A, in this order, stopping only at a design boundary** (`README.md:115-116`): churn at
25 repetitions, transport at 25, h1-deep at 7. Churn first because it is the cell where the
hypothesis can fail (`run_campaign.py:427-447`). Twenty-five is the number the X1 resolution
analysis found necessary (`README.md:97-99`); the desktop's own loopback spread says seven resolves
nine cells in ten, but the wire's spread is the unknown this run exists to measure, and a design
that could not resolve its own question at the end of the night has spent the night. h1-deep at
seven, because its cleartext cells at 5k to 35k are already inside transport.

**Machine time.** One run is 26 seconds: 20 measured, 3 warmup, 3 turnover (`README.md:97`;
`run_campaign.py:1069`), plus a fraction of a second of ssh session setup.

| Block | Cells | n | Runs | Time |
| --- | --- | --- | --- | --- |
| smoke | 2 | 1 | 2 | 1 min |
| ladder (10k to 120k) | 12 | 1 | 12 | 5 min |
| tls-ladder (5k to 60k) | 12 | 1 | 12 | 5 min |
| churn-ladder (25 to 800) | 11 | 1 | 11 | 5 min |
| churn or churn-net | 16 | 25 | 400 | 2.9 h |
| transport | 20 | 25 | 500 | 3.6 h |
| h1-deep | 10 | 7 | 70 | 0.5 h |
| **direction A** | | | **1007** | **7.3 h** |

At seven repetitions throughout: 112 + 140 + 70 = 322 timed runs, 2.3 h, 2.6 h with the ladders.
Extending a seven-repetition file to twenty-five afterwards is possible but the second invocation
must use a different `--seed`: `plan` seeds each pass from `f"{seed}:{repetition}"`
(`benchmark/harness/ordering.py:78`), so the same seed would replay the same orders under the same
repetition indices, and the repetition column would then no longer mean pass number.

**The decision rule.** For every cell of churn and transport (rate × TLS), the baseline is
classification off and the other arm classification on; the statistic is the per-run median,
establishment (`connect_ms.p50`) for churn and request latency (`latency_ms.p50`) for transport;
the interval is `stats.compare`'s bootstrapped difference of medians at 95 percent; the resolution
r is its half-width divided by the baseline median. Reported for every cell: r, the relative
difference, whether the interval excludes zero, the run-to-run coefficient of variation of the
medians per arm, the count of accepted runs per arm, and the pacing p99 covariate per arm. Also
reported: median against repetition index per cell, which the interleaved shuffle provides for
free and which is where autocorrelated network noise or thermal drift would show.

Two outcomes, worded now:

- **r ≤ 5 percent in a cell.** The wire resolves the difference in that cell at this n. The
  difference is then reported as it comes: an interval excluding zero is a measured cost of
  classification over a real path, an interval containing zero is a null result over a real path,
  and each replaces the loopback caveat for that cell.
- **r > 5 percent in a cell.** The wire does not resolve the difference in that cell at this n.
  The loopback caveat stands for it, restated as a number: the run-to-run spread over the wire is
  r. The sentence "loopback was the right medium for this claim" may be written for a cell only
  where the loopback campaign's r for the same cell was at or under 5 percent at the same n; where
  loopback did not resolve it either, neither medium did and the text says so.

Two caveats that apply to either outcome and are reported, not adjudicated: network noise that is
autocorrelated over a run's length collapses the effective sample count toward the number of runs,
which the repetition plot would show as structure; and any variation correlated with the arm
rather than added to it biases the comparison at any n, which the shuffle addresses for
time-correlated variation and nothing addresses for arm-correlated variation.

**Follow-up, pre-declared.** If churn's r exceeds 5 percent in half or more of its cells, the next
window runs churn again at the same n with the server host's cores held awake by idle-priority
spinners, to a separate results file; the awake condition is written in the results README because
no record field carries it. That is the only condition under which an awake control is taken in
direction A.

**What is not licensed.** No cross-platform magnitude. The two ends are different hardware; the
Linux server of direction B against the Windows server of direction A is shape only.

## 8. Runbook

Executable from this text. Placeholders: `<server-address>` the desktop's wired address as the
laptop sees it; `<ssh-target>` `<user>@<generator-address>`, the laptop's wired address;
`<loadgen>` the generator binary on the laptop; `<repo>` the repository on the laptop; `<iface>`
the laptop's wired interface name (from `ip -br link`; a name, not an address); `<commit>` the
single commit of section 3; `<dir>` `benchmark/results/<yyyy-mm-dd>-two-host/` on the desktop.
The values are exchanged by message and never written to this repository.

### 8.1 Laptop, the generator host

```
git -C <repo> fetch && git -C <repo> checkout <commit> && git -C <repo> status -sb
cmake --build <repo>/build/linux-dual --target loadgen
<loadgen> --help
cd <repo> && python3 -c 'from benchmark.harness import environment as e; print(e._power_source(), e._governor())'
cat /sys/devices/system/clocksource/clocksource0/current_clocksource
ip route get <server-address>
systemctl is-active sshd
```

Refuse if: the tree is not clean at `<commit>`; the power source is not mains; the governor is not
`performance`; the clocksource is not `tsc`; `ip route get` names any interface but `<iface>`
(fix the route metric, or take the wireless interface down for the night, which is the operator's
call and is written in the results README if done); sshd is not active. Then leave the laptop idle:
no editor, no browser, no agent session doing work.

### 8.2 Desktop, the server host and the driver

```
git fetch && git checkout <commit> && git status -sb
cmake --build build/windows-tls --config Release
ssh -o BatchMode=yes <ssh-target> true; echo $LASTEXITCODE
ssh -o BatchMode=yes <ssh-target> <loadgen> --help
powercfg /getactivescheme
Get-NetAdapterAdvancedProperty -Name "<wired adapter>" | Select-Object DisplayName, DisplayValue
```

Refuse if: the tree is not clean at `<commit>`; the build fails (see the fallback below); ssh
prompts or exits non-zero; the generator does not print its usage. Record the power plan and the
adapter's advanced properties (interrupt moderation, RSS, offloads) in the results README; change
none of them.

Reachability, before anything is timed, with the server started once by hand on port 18080:

```
.\build\windows-tls\examples\Samples\benchmark_server\benchmark_server.exe --port 18080
   (on the laptop)  curl -s -o /dev/null -w '%{http_code}\n' http://<server-address>:18080/
   (on the desktop) curl.exe -s -o NUL -w '%{http_code}\n' http://127.0.0.1:18080/
```

Both print 200, then stop the server. Refuse otherwise; no firewall rule is changed on either
machine.

Fallback if mainline does not build on the desktop: the run stays at `b4a01e8c7`, schema 7,
Nagle on, and the interface field does not exist; the wire is then checked out of band (the
wireless interface down for the night) and both facts are stated as limitations. That is a
different and weaker campaign and the results README says which one was run.

### 8.3 The campaign, from the desktop

```
python -m benchmark.run_campaign --design smoke        --repetitions 1  --build build/windows-tls --results <dir>/smoke.jsonl        --host <server-address> --ssh-generator <ssh-target> --ssh-loadgen <loadgen> --ssh-repo <repo> --generator-location lan:linux --expect-interface <iface>
```

Read the two records before continuing: `accepted` true on both, `local_interface` `<iface>`,
`local_interface_speed_mbit` 1000, `local_interface_duplex` full, `local_interface_mtu` 1500,
`generator` `coroute-loadgen-lan`, `generator_euid` a non-zero number, `git_commit` `<commit>`,
and in `smoke.env.json` a `generator_environment` whose `build.git_commit` is `<commit>` and
`cpu.governor` is `performance`. Stop at the first field that is not so.

```
python -m benchmark.run_campaign --design ladder       --repetitions 1  --build build/windows-tls --results <dir>/ladder.jsonl       <same options>
python -m benchmark.run_campaign --design tls-ladder   --repetitions 1  --build build/windows-tls --results <dir>/tls-ladder.jsonl   <same options>
python -m benchmark.run_campaign --design churn-ladder --repetitions 1  --build build/windows-tls --results <dir>/churn-ladder.jsonl <same options>
```

Read the ladders by the admission rules of section 6. If any rate in `_WINDOWS_RATES` or
`TLS_OFFERED_RATES` is refused by achieved share, non-2xx or socket errors, the design is not run
at that rate: the table is cut at the last admitted rate below it and that cut is written in the
results README before the design starts. Churn's table is chosen by the rule in section 5.

```
python -m benchmark.run_campaign --design churn-net    --repetitions 25 --build build/windows-tls --results <dir>/churn.jsonl       <same options>
python -m benchmark.run_campaign --design transport    --repetitions 25 --build build/windows-tls --results <dir>/transport.jsonl   <same options>
python -m benchmark.run_campaign --design h1-deep      --repetitions 7  --build build/windows-tls --results <dir>/h1-deep.jsonl     <same options>
```

`churn` in place of `churn-net` if the ladder chose the loopback table. A finished design is
citable, half of one is not; stop at the boundary if the window closes.

### 8.4 After

1. Count rejections per file and read every reason before reading a number
   (`README.md:154-156`).
2. Write `<dir>/README.md`: the commit, the two hosts by hardware class and operating system, the
   power plan, the adapter properties, the qdisc, the churn table chosen and why, any table cut,
   whether the wireless interface was taken down, and that cores were allowed to idle.
3. Commit `<dir>` including every `.env.json` on `measure/two-host-<yyyy-mm-dd>` and push it to the
   private paper repository, never here: the manifest carries the host name and the records carry
   the addresses.
4. `results2tex` produces the per-cell resolutions; section 7's rule is applied to them as written.

## 9. Direction B: Linux server, Windows generator

For paper 3's epoll and io_uring arms over a real path. By the placement rule the driver runs on
the laptop and reaches the Windows generator over ssh, which needs the OpenSSH Server feature on
Windows and a key: a system change that is Alex's decision, so direction B is deferred until it is
made. When it runs:

- `--expect-interface` cannot be used: the Windows generator reports no interface
  (`loadgen.cpp:427-430`) and the driver refuses every run with an unmet expectation
  (`driver.py:367-372`). The dual-homed host is then the server, whose interface nothing reads; the
  wireless interface is taken down for the night and the fact is recorded.
- The generator reports no euid (`loadgen.cpp:1743-1751`); with no server prefix nothing refuses
  it (`driver.py:328, 398-403`).
- The receiving end is the Linux server and its idle state mediates low-load latency by hundreds
  of microseconds. Direction B takes the awake control the laptop already uses, sixteen `nice -n
  19` spinners, as its own arm of every low-rate design, or states the idle regime as a caveat on
  every low-rate cell.
- The Windows generator's 1 ms poll is quantised to the raised system timer; at churn rates of 25
  and 50 a second it sleeps between slots and the pacing covariate is read with that in mind.
- `generator_environment` from a Windows host has no governor, no clocksource, no qdisc; the
  power plan is recorded by hand.

## 10. Limitations stated before the run

1. The generator host is described once per campaign in the manifest and is not fingerprinted;
   its per-run evidence is the pacing covariate and the achieved share.
2. Kernel drop counters are read at neither end.
3. `fq_codel` on the laptop's egress and the desktop adapter's interrupt moderation are part of
   the path, recorded, and not controlled.
4. Cores at the server are allowed to idle; no awake control in direction A unless the pre-declared
   follow-up condition in section 7 is met.
5. The server keeps its loopback-campaign affinity while the desktop's other cores are idle, a
   difference from the loopback arrangement in which they hosted the generator.
6. Raw per-request samples (`--samples`) are not taken over ssh; the histogram percentiles in the
   record are the data.
7. The two-host cells are a separate population from every filed campaign and are compared with
   them for shape only.
8. Windows ICMP resolution makes a ping baseline from the desktop impossible; none is reported, and
   the design does not need one.
9. No cross-platform magnitude between directions A and B.

## 11. Why this file exists

Alex's decision of 2 September: the two-host run is approved for overnight and its design is
pre-declared and committed before it runs. Every rule above is fixed now. A rule that is added after
the records exist is a filter on the outcome, and this file's timestamp is the evidence that none
was.
