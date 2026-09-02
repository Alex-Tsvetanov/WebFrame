#!/usr/bin/env bash
# Run a command and wait for it by pid, never by pattern.
#
#   procwait.sh <logfile> <command> [args...]
#
# Why this exists. The obvious way to wait for a background job is to poll for it:
#
#   while pgrep -f 'bash /tmp/run.sh' >/dev/null; do sleep 30; done
#
# That is wrong, and wrong in both directions. `pgrep -f` matches against full command
# lines, and the shell running the check has 'bash /tmp/run.sh' in its own command line,
# so it matches itself: the loop never ends, and the job looks like it is still running
# forever after it has finished. The same match also reports a job as running when it
# was never started at all, because some other shell mentioning the name is enough.
#
# This cost three separate stalls in one session. Once a measurement script was reported
# as running for twenty minutes after it had exited; once a chained job never fired and
# the check said it had; once a build directory was reported busy when nothing was
# building, because a waiting shell had the word cmake in its arguments.
#
# A pid cannot self-match. `wait` on a known pid is exact, costs nothing, and returns
# the moment the process ends.
#
# For processes this script did not start, prefer `pgrep -x <exact-name>` over a
# substring: `pgrep -x cc1plus` finds a compiler, `pgrep -f cmake` finds every shell
# that ever mentioned cmake.
#
# The same class of mistake has a second face worth naming here, because it looks
# unrelated and is not: a measurement script and an interactive diagnostic both using
# one build directory. Two builds in one tree race, one wins, and the artefacts that
# come out belong to neither. If a script is building, do not build beside it; either
# wait for it by pid, as this script does, or give the diagnostic its own build tree.
set -u
if [ "$#" -lt 2 ]; then
    echo "usage: procwait.sh <logfile> <command> [args...]" >&2
    exit 2
fi
log="$1"; shift
"$@" > "$log" 2>&1 &
pid=$!
echo "procwait: pid $pid running: $*"
wait "$pid"
rc=$?
echo "procwait: pid $pid exited $rc"
cat "$log"
exit "$rc"
