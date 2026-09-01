# Coordinator handoff

Written 2026-09-02 by the coordinating session (Fable); last refreshed 02:45 EEST. A new session takes over
by reading this file first, then the memory notes `benchmark-machines`, `coordinator-role`
and `handoff-before-limit`, then `ListAgents` to find the remote sessions named below.
Run with ultracode on. Update this file at every milestone; it is the only place the
cross-machine state is written down.

## The job

Coordinate the PhD thesis in this repo (`doc/thesis/`, builds with `latexmk -pdf Main.tex`
in that directory, 148 pages) and the four papers in
`~/Documents/GitHub/Publications/paper-*` (each its own git repo; `INDEX.md` is shared
byte-for-byte across the four, `BLOCKERS.md` is per paper). Remote sessions do machine
work; this session writes, reviews, merges. Method rules that are never relaxed: one
binary with runtime flags, never two builds; not loopback for anything load-bearing;
pre-declared validity gates, unknown is not clean; no paper drafts ahead of data
(the papers were reset to zero measurements on 2026-09-01 for exactly that reason).

## Machines and sessions

| Session name | Host | What it is for | State |
| --- | --- | --- | --- |
| `Windows machine` | the desktop: Ryzen 5 3600, Windows 11, WSL2 Ubuntu-24.04 hosts the generator; repo path and addresses are in the local memory note `benchmark-machines` | IOCP measurements, the campaign nights | released for tonight; running steps 0-6 below |
| `ArchLinux` | the laptop: Ryzen 7 5800H, Arch on a hardened kernel, repo `~/GitHub/PhD-WebFrame`, presets `linux-dual` (validated) and `linux-epoll`; since late 2 Sep it has perf, strace, ninja, git-lfs, passwordless sudo and a wired link | Linux code work and, once the harness branch lands, netns runs and the perf syscall counter; io_uring runs only as root on this kernel (restricted, not off); governor still powersave | brief 3 (preset check + perf feasibility probe) running |
| `Dispatch background conversation` | unknown | never answered a probe | ignore |

Both hosts are Alex's daily machines. No timed run without a window Alex names.

**Machine sessions must never block on a question.** Alex is often unavailable, and a
session that calls AskUserQuestion stops processing everything, including messages from
here. Every brief repeats: never ask Alex or wait on him; send the question to the
coordinator with the default you will take and continue with it after 10 minutes; push only
to the branches the brief names; never rebase, force-push or modify `phase0-foundation`;
never change the system or enter a sudo password (skip and report); a blocked permission
prompt is reported and skipped; when unsure, stop that step and report. Both sessions were told; the desktop session keeps its own
channel to Alex on principle, which is fine because Alex pre-authorised its whole sequence.

**This repository is public.** Nothing that identifies a machine beyond its hardware class
belongs here or in any commit, branch or inbox file on it: no hostnames, addresses, SSIDs,
login state or security posture. Those live in the local memory notes. Measurement result
branches (whose environment records carry the host name) go to the private paper
repositories, never here.

## Repository state

`phase0-foundation` = `aee6cea19`, pushed. Contains today's: campaign runbook
(`benchmark/README.md`), thesis gate table (chapter V) and a build that survives missing
`data/` (chapter VI guards), Linux memory peak in `route_bench.cpp` (merged from the
laptop's `linux/route-bench-memory`, branch deleted).

Branches in flight:

- `origin/linux/io-backend-runtime` at `137a1686c` (laptop; two commits on `fef3a212e`).
  Both Linux backends in one binary, `--io-backend` on benchmark_server, banner
  `(multi-accept, backend X)` read back by the harness (regex shared as
  `environment.BANNER_BACKEND`), presets `linux-epoll`/`linux-dual`, ctest registered per
  arm, census takes `--io-backend` and refuses a dual build without it, a plain epoll tree
  is accepted on Linux. Review (12 confirmed findings) fully addressed in the second commit.
  Verified on Linux (epoll 178/178, uring 178 with 6 skipped, dual 184 with 6 skipped) and on
  macOS here (build clean, 178/178, selfchecks 169, census kqueue rows correct). **Merge is
  gated only on the desktop's MSVC compile check (step 1 of its sequence).** Then
  `git merge --no-ff origin/linux/io-backend-runtime` into `phase0-foundation`, push,
  delete the branch. Worktree checkout for re-verification:
  `.../848236b3-ca09-48d3-bad5-f698c7808b0e/scratchpad/wt-iob`.
- `origin/harness/linux-readiness` at `9ee8de784`, pushed (23 commits on `fef3a212e`, written
  by a local workflow, worktree `.../848236b3-ca09-48d3-bad5-f698c7808b0e/scratchpad/wt-harness`).
  Implements the 21 verified harness gaps: `--generator-command`/`--generator-location`
  launcher (netns), generator output under a harness-owned dir, loopback detection by
  launcher, transport_path compared on campaign open, port preflight bind before every
  server start, `UdpRcvbufErrors` key, governor gate (run-level, unknown refused on Linux),
  clock reading on run_routing, power-supply scope filter, sibling topology recorded and the
  two masks checked, whole-run response counter in loadgen (schema v4), systemd-detect-virt
  exit 1 handled, `git_dirty` None refused, server death before listening reported with
  stderr, `--build` required, presets and README rows. Selfcheck 211 (was 162). Reviewed by
  three lenses; the one HIGH and five MEDIUMs were fixed in the last six commits. Fifteen
  LOW findings remain, listed in
  `.../848236b3-ca09-48d3-bad5-f698c7808b0e/tasks/wel4atrqm.output` (`low_findings_for_coordinator`);
  fold the cheap ones (run_routing `git_commit[:12]` guard, vacuous port selfcheck, temp-dir
  leak, census held-port traceback, README `...` elisions now that `--build` is required,
  duplicated port probe in the two integration tests) during the merge.
  **In progress 02:45: a local workflow (run `wf_89d61877-739`, task `wno89410k`) is merging
  `origin/linux/io-backend-runtime` INTO this branch in its worktree, resolving the conflicts,
  folding the cheap lows, verifying on macOS (full ctest, census, loopback smoke) and pushing.
  Once that lands, this branch merges into `phase0-foundation` cleanly after io-backend does.**
  Original note: both touch `adapters.py`, `driver.py`, `environment.py`,
  `run_campaign.py`, `descriptor_census.py`, `tests/integration/*.py`; expect conflicts and
  resolve by hand (or with a workflow: merge, resolve, fold lows, selfchecks, build loadgen
  on macOS, macOS loopback smoke). NOTE for the desktop: after this merge the WSL
  generator must be rebuilt from the same commit as the server (bytes_read window and
  `responses_total` changed); tonight's campaign runs on `aee6cea19` deliberately, with the
  harness the earlier Windows records were produced by, so its records are schema v3.
- Excluded from both branches, still to do on the laptop (root is now available via passwordless sudo): perf-based syscalls-per-request counter (audit items B2-B4: `perf stat
  -e raw_syscalls:sys_enter -p <pid>` around a non-timed "mechanism" run, opt-in flag,
  schema field), netns pair creation and a real off-loopback Linux run, the QUIC UDP-seam
  reconciliation between `feature/http3-quic` and HEAD (paper 4, cheapest next step per
  its BLOCKERS).

## Desktop sequence sent 02:20 (message id 5461cf03)

0 pull to `aee6cea19`; 1 MSVC compile check of `linux/io-backend-runtime` in `..\wt-iob`;
2 rebuild `build\windows-tls` and `build\windows-routing`, check WSL loadgen, resolve the
vEthernet gateway, certs, idleness; 3 `smoke` quiet-host gate (pacing p99 tens of µs or
stop); 4 four ladders with a decision rule (all table rates accepted below 500 µs pacing,
else stop and report); 5 `churn` → `churn-net` → `transport` → `h1-deep` at n=25, results
in `benchmark\results\2026-09-02-desktop\`, one message per design, stop at a design
boundary if Alex wants the machine or if a design rejects >10 %; 6 commit results LOCALLY on
`measure/desktop-2026-09-02` with `git add -f`; the desktop session pushes nothing (its own policy, and Alex's
wording was that the coordinator places records in the paper repos). **Morning step for Alex: push that
local branch to the private `paper-socket-demux` repository**, then the coordinator copies the files into
`measurements/`. About 13 hours. The desktop pulled `4645e5e03` (one docs commit past `aee6cea19`,
code identical), so its records carry that hash. It reports by message only and declined the
`coord/inbox` practice; the laptop follows it.

When results arrive: copy each `.jsonl` + `.env.json` into
`Publications/paper-socket-demux/measurements/` with a README entry (machine, commit,
command, what it does not show), following the existing entries there. Chapter VI of the
thesis regenerates from them: `python -m benchmark.harness.results2csv <runs.jsonl>
doc/thesis/data` and `results2tex <runs.jsonl> doc/thesis/generated/results.tex`; the
`\R{}` keys the chapter expects are `campaign.sweeps.*`, `campaign.x1.*`,
`h1detect.*`, `tail.x1.*` (grep the chapter). Routing (night 2 in the runbook) has not
been scheduled yet.

## Decisions taken today, with reasons

- Campaign tree is the desktop's tracking checkout at HEAD, not its `-measure` checkout
  (detached at `b14002aab`, which predates the three gate fixes of 1 Sep).
- Laptop timing numbers are pipeline validation only until Alex boots stock `linux`
  7.2.2 with the `performance` governor; the harness will get a run-level governor gate.
- Same-hardware Linux vs Windows does not exist (two machines); cross-platform is shape
  only. Recorded in all four `INDEX.md` ("Update, 2 September 2026").
- The stale wrk-era `benchmark/README.md` was replaced by the runbook; the old text is in
  history at `068431b37`.
- `Compile-time-Protobuf` has uncommitted rebuilt PDF/DOCX/presentation; not touched.

## Open questions for Alex

1. How long is tonight's desktop window; morning stop is fine at a design boundary.
2. Laptop: may a campaign set the governor to `performance` for its duration
   (`cpupower frequency-set -g performance`, restored afterwards)? Everything else on the
   old list is done: tools installed, passwordless sudo, Ethernet up; Alex declined a reboot
   and the io_uring sysctl, so io_uring arms run as root and the record must say so.
3. (done: the laptop is wired) A two-host validity check desktop<->laptop over the LAN is now possible; worth a night later.
4. Protobuf rebuilds: commit or discard.

## Next steps, in order

1. Desktop reports MSVC result for `137a1686c` → merge `linux/io-backend-runtime` into
   `phase0-foundation` (merge commit), delete branch.
2. Merge `harness/linux-readiness` onto the new HEAD, resolve conflicts, fold the cheap lows,
   selfchecks, macOS build + loopback smoke, push, delete branch.
3. Desktop reports per design → ingest into paper-socket-demux, regenerate chapter VI,
   check the red `[?key]` markers shrink, commit thesis data snapshots (`doc/thesis/data`
   CSVs are meant to be committed; `generated/` is not).
4. Schedule routing night (runbook night 2) with Alex; dispatch-only can also run on the
   laptop once the governor is `performance`.
5. With Alex's root decision: laptop netns pair + first admissible Linux run of `churn`
   and `h1-deep` on epoll (and io_uring once permitted); perf syscall counter (the
   "mechanism" both papers 2 and 3 rest on).
6. Then paper 4's UDP-seam reconciliation on a scratch branch (design work, Linux).

## Succession, automated (set up 02:30 EEST)

The switch is mechanical, so it needs nobody awake:

- The primary session refreshes
  `~/.claude/projects/-Users-Alex-Tsvetanov-Documents-GitHub-PhD-WebFrame/coordinator-alive`
  every 20 minutes (a session cron). Its first line is a UTC timestamp.
- A scheduled task `coordinator-successor` (`~/.claude/scheduled-tasks/coordinator-successor/SKILL.md`,
  `model: opus`, every 30 minutes while the desktop app is open) reads that file. Younger
  than 75 minutes: it stops. Older: it takes over — writes `coordinator-successor-lock`
  (2 h TTL) beside it, reads this file and the memory notes, reads new reports on
  `origin/coord/inbox`, tells `ArchLinux` and `Windows machine` its session name, works
  the next steps for at most about an hour, refreshes this file, pushes, deletes the lock.
  Each run is stateless; this file is the state.
- Machine sessions write every report as a file on the orphan branch `coord/inbox`
  (`coordination/inbox/<host>-<UTC>.md`) in addition to messaging, so a report that
  arrives between coordinator sessions is not lost. The laptop does this (full text, at Alex's choice); the desktop reports by message only, so a successor must ask it to re-send.
- A probe on 02:21 confirmed a scheduled session runs as `claude-opus-5` with that
  frontmatter and can message peers.

If a human takes over instead: open a session on Opus with ultracode on, say "read
.claude/HANDOFF.md and continue", and disable the scheduled task in the sidebar.
