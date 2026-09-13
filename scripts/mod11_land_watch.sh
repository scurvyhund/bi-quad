#!/bin/bash
# mod11_land_watch.sh -- detached. Waits out the d=24 and d=26
# verification legs, then closes the mod-11 gap for d=16/24/26 and
# preserves every artifact. Survives the Claude session, the terminal
# and a logout; it can only shout and preserve, never analyse further.
#
#   nohup scripts/mod11_land_watch.sh >/dev/null 2>&1 &
set -u
cd /home/jim/programming/c/BigFermat/bi-quad || exit 1
OUT=logs/mod11_CLOSURE.txt
STAMP=$(date +%F_%H%M%S)

# wait for BOTH legs: hunt gone AND no resweep_leg wrapper still queued.
# The wrapper test matches EXACT argv rather than a cmdline substring:
# `pgrep -f` matches the shell running it, and the project rule is that
# it never appears in a loop condition -- the [r]esweep bracket trick
# works but relies on being clever, which is how this class of bug
# returns.
wrappers_running() {
   ps -eo args --no-headers \
     | awk '$1=="/bin/bash" && $2=="scripts/resweep_leg.sh"' \
     | grep -q .
}
while pgrep -x hunt >/dev/null || wrappers_running; do
   sleep 60
done
sleep 20   # let the final write land

{
  echo "=== mod-11 closure  $(date '+%F %T') ==="
  echo "binary: $(md5sum hunt_2d1948a 2>/dev/null | cut -c1-32)  (2d1948a chain artifact)"
  echo
  rc=0
  for d in 16 24 26; do
     scripts/mod11_closure.sh $d || rc=1
     echo
  done
  echo "OVERALL: $([ $rc -eq 0 ] && echo 'ALL CLEAN -- gap closed' || echo '*** VIOLATION -- LOOK AT THIS ***')"
} > "$OUT" 2>&1

# preserve: snapshot every artifact under one timestamp
mkdir -p logs/preserved/$STAMP
cp -p logs/mod11_verify_*.log logs/raw/hunt_d16_*.raw logs/raw/hunt_d24_*.raw \
      logs/raw/hunt_d26_*.raw "$OUT" logs/preserved/$STAMP/ 2>/dev/null
sha256sum logs/preserved/$STAMP/* > logs/preserved/$STAMP/SHA256SUMS 2>/dev/null

# ---- retention -------------------------------------------------
# ASYMMETRIC BY DESIGN, agreed with Jim 2026-09-12.
#
# CLEAN   -> commit and push all three remotes unattended.  The result
#            is confirmatory, matches the theorem, and ships with the
#            raw capture anyone can re-derive it from.  This box has a
#            single disk, so the remotes are the only off-machine copy.
# VIOLATION -> preserve and SHOUT, push NOTHING.  That would be a claim
#            that our own results carry a bug, landing unreviewed in
#            three permanent histories -- and this verifier is exactly
#            the kind of thing that can be wrong (its own test harness
#            was, once, on 2026-09-12).  A wrong VIOLATION in public
#            history is worse than a delayed CLEAN.  Same instinct as
#            resweep_leg.sh halting on a nonzero EMIRPS instead of
#            rolling on.
if grep -q 'ALL CLEAN' "$OUT"; then
   notify-send -u normal "mod-11 gap CLOSED" "d=16/24/26 all clean -- pushing"
   # add paths INDIVIDUALLY: `git add a b c` aborts adding ANYTHING if
   # any one path is gitignored, which silently disabled this whole
   # block until it was caught in a throwaway clone on 2026-09-12.
   PATHS="$OUT logs/mod11_verify_2026-09-12.log logs/raw"
   for pth in $PATHS; do
      [ -e "$pth" ] || continue
      git add "$pth" || notify-send -u critical "mod-11: git add failed" "$pth"
   done
   if git diff --cached --quiet; then
      notify-send -u normal "mod-11: nothing new to commit" "$OUT"
   else
      git -c commit.gpgsign=false commit --only $PATHS -q -F - <<'MSG'
mod-11 closure: d=16/24/26 verified clean, gap closed

The 2026-09-05 wrapper filtered hunt's stderr, so the even-d mod-11
alarm could not have been seen for these three lengths.  Re-run under
the tee'd runner on the unchanged 2d1948a binary -- the same artifact
the rest of the d=13..27 chain rests on, so the single-binary property
is preserved rather than traded away to close the gap.

Not inferred from silence: mod11_closure.sh reads each survivor back
out of the raw capture and recomputes p mod 11 itself.

Committed automatically by scripts/mod11_land_watch.sh on a CLEAN
result only; a violation would have been preserved and escalated
instead of pushed.

Co-Authored-By: Claude Opus 5 <noreply@anthropic.com>
MSG
      ok=1
      for r in origin github codeberg; do
         git push -q "$r" master_dev 2>/dev/null || ok=0
      done
      if [ $ok -eq 1 ]; then
         notify-send -u normal "mod-11 evidence pushed" "all three remotes"
      else
         notify-send -u critical "mod-11 PUSH FAILED" \
            "committed locally, NOT on remotes -- push by hand"
      fi
   fi
else
   notify-send -u critical "mod-11 VIOLATION -- nothing pushed" "LOOK NOW: $OUT"
fi
