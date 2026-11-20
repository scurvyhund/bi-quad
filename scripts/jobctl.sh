#!/bin/bash
# jobctl.sh -- the ONE correct way to launch, find, verify and stop a
# long compute job in this project.  Sourced or called; no job should
# be started or killed by hand again.
#
#   scripts/jobctl.sh launch <name> <args...>   # nohup + verify singleton
#   scripts/jobctl.sh pid    <name>             # resolve FRESH, exact
#   scripts/jobctl.sh count  <name>             # never self-matches
#   scripts/jobctl.sh stop   <name>             # kill + VERIFY it landed
#   scripts/jobctl.sh wait   <name>             # block until gone
#
# WHY THIS EXISTS -- four traps, all of which have bitten this project:
#
#   1. `$!` after `nohup ... &` can report an intermediate wrapper, not
#      the worker.  A d=27 hunt ran TWICE for 3.7 days because a kill
#      was aimed at a PID that was never doing the work.  So: never
#      report or remember $!.  Resolve fresh at every use.
#   2. `pgrep -f` MATCHES ITSELF -- the shell running it has the
#      pattern in its own command line.  Observed returning 3 with ZERO
#      target processes running.  So it is never used here to count or
#      to loop.
#   3. `pkill -f` kills the very shell that invoked it.  Observed
#      2026-09-12: exit 144, and only a filename coincidence kept it
#      from orphaning a 22-hour run.
#
# Everything below matches on the exact process NAME (pgrep -x) or on
# exact argv, never on a cmdline substring.

set -u
# NB: no braces or angle brackets in these messages -- a "}" inside
# ${x:?...} closes the expansion early and the remainder leaks out
# as shell code (a stray redirect). Bit us on 2026-09-12.
cmd=${1:?usage: jobctl.sh launch/pid/count/stop/wait NAME [args]}
name=${2:?need a process name}
shift 2 || true

# 4. The kernel truncates a process name (comm) to 15 characters, and
#    pgrep -x matches against that.  A longer name NEVER matches: count
#    reads 0 while the job runs, so launch starts a SECOND copy and stop
#    reports "not running".  pgrep warns on stderr, but jc_count discards
#    stderr, so it was silent.  Demonstrated 2026-11-19 with a 19-char
#    name: two launches, two instances.  Refuse such names outright --
#    matching the truncated prefix could hit an unrelated process.
if [ "${#name}" -gt 15 ]; then
   echo "REFUSING: name '$name' is ${#name} chars; the kernel truncates" \
        "process names to 15, so pgrep -x can never see it. Rename the" \
        "binary to 15 chars or fewer." >&2
   exit 65
fi
case "$name" in
   */*) echo "REFUSING: give the binary NAME, not a path: '$name'" >&2
        exit 65 ;;
esac

# pgrep -xc PRINTS 0 and EXITS 1 when nothing matches, so a
# `|| echo 0` fallback yields "0\n0" and every [ -eq ] then fails
# with "integer expected". Take the first line, default empty to 0.
jc_count() { local n; n=$(pgrep -xc "$name" 2>/dev/null | head -1);
             [ -n "$n" ] || n=0; echo "$n"; }
jc_pids()  { pgrep -x  "$name" 2>/dev/null; }

case "$cmd" in
  count) jc_count ;;

  pid)
     n=$(jc_count)
     if [ "$n" -eq 0 ]; then echo "no '$name' running" >&2; exit 1
     elif [ "$n" -gt 1 ]; then
        echo "REFUSING: $n instances of '$name' -- look before acting:" >&2
        pgrep -ax "$name" >&2; exit 2
     fi
     jc_pids ;;

  wait)
     while [ "$(jc_count)" -gt 0 ]; do sleep 15; done ;;

  launch)
     if [ "$(jc_count)" -gt 0 ]; then
        echo "REFUSING: '$name' already running:" >&2; pgrep -ax "$name" >&2; exit 3
     fi
     nohup "./$name" "$@" > "logs/${name}_$(date +%F_%H%M%S).log" 2>&1 &
     sleep 2
     # deliberately IGNORE $! -- resolve fresh
     n=$(jc_count)
     if [ "$n" -ne 1 ]; then
        echo "LAUNCH FAILED or not singleton: count=$n" >&2; pgrep -ax "$name" >&2; exit 4
     fi
     echo "launched '$name', singleton verified, pid $(jc_pids)" ;;

  stop)
     n=$(jc_count)
     [ "$n" -eq 0 ] && { echo "'$name' not running"; exit 0; }
     [ "$n" -gt 1 ] && { echo "REFUSING: $n instances -- look first:" >&2
                         pgrep -ax "$name" >&2; exit 2; }
     p=$(jc_pids)
     echo "stopping '$name' pid $p"
     kill "$p" 2>/dev/null
     for _ in $(seq 1 20); do
        [ "$(jc_count)" -eq 0 ] && { echo "kill VERIFIED: '$name' gone"; exit 0; }
        sleep 1
     done
     echo "KILL DID NOT LAND -- '$name' still running:" >&2
     pgrep -ax "$name" >&2; exit 5 ;;

  *) echo "unknown: $cmd" >&2; exit 64 ;;
esac
