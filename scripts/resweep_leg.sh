#!/bin/bash
# resweep_leg.sh -- run hunt over a range of d, one length at a time,
# gating on EMIRPS=0.  Supersedes logs/resweep_tail.sh.
#
# Usage:  scripts/resweep_leg.sh <d_lo> <d_hi> [summary-log]
#
# WHY THIS EXISTS.  The 2026-09-05 wrapper piped hunt's stdout through
#   grep -E "range=|wall=|EMIRP"
# and appended only that.  hunt prints each hit as
#   "  palindrome     n=... p=..."   -- which matches NONE of those.
# So every palindrome line of the d=13..27 resweep was discarded at the
# pipe.  Nothing was ultimately lost (pals.txt already held the d=27
# values) and the emirp result was never at risk ("*** EMIRP ***" does
# match), but the SHAPE was wrong: a whitelist filter on a program's
# only output stream throws away precisely what you failed to predict.
#
# The rule this encodes: NEVER let a filter be the only consumer of a
# stream you cannot regenerate.  tee the full stream to disk first,
# then filter for the human-readable view.  Filtering the VIEW is fine;
# filtering the RECORD is not.

set -u
cd /home/jim/programming/c/BigFermat/bi-quad || exit 1

D_LO=${1:?usage: resweep_leg.sh <d_lo> <d_hi> [summary-log]}
D_HI=${2:?usage: resweep_leg.sh <d_lo> <d_hi> [summary-log]}
LOG=${3:-logs/hunt_resweep_$(date +%F).log}
RAWDIR=logs/raw
# HUNT_BIN exists ONLY so the halt gates below can be exercised with a
# stub.  Production leaves it at ./hunt.  An untested gate is not a
# gate -- both halts were unreachable under test without this.
HUNT="${HUNT_BIN:-./hunt}"
mkdir -p "$RAWDIR" || exit 1

# never launch on top of a running hunt (never pgrep -f: it matches self)
while pgrep -x hunt >/dev/null; do sleep 15; done

echo "=== resweep d=$D_LO..$D_HI on $(git rev-parse --short HEAD) ===" \
   >> "$LOG"

for d in $(seq "$D_LO" "$D_HI"); do
   RAW="$RAWDIR/hunt_d${d}_$(date +%F).raw"

   # FULL stream to disk, filtered copy to the summary log.
   /usr/bin/time -f "  wall=%es" "$HUNT" "$d" "$d" 2>&1 \
      | tee -a "$RAW" \
      | grep --line-buffered -E "range=|wall=|EMIRP" >> "$LOG"

   # The raw capture is the record; prove it actually captured.
   if [ ! -s "$RAW" ]; then
      echo "!!! HALT: d=$d raw capture $RAW is EMPTY" >> "$LOG"
      exit 4
   fi

   # A result line must exist at all -- absence is not success.
   if ! grep -qE "^  d= *$d .*EMIRPS=" "$LOG"; then
      echo "!!! HALT: d=$d produced NO result line -- crash or kill?" \
         >> "$LOG"
      exit 5
   fi

   # The gate.  A nonzero EMIRPS is the entire point of the search:
   # stop and make noise rather than roll on past it.
   if ! grep -qE "^  d= *$d .*EMIRPS=0" "$LOG"; then
      echo "!!! HALT: d=$d did NOT report EMIRPS=0 -- LOOK AT THIS" \
         >> "$LOG"
      exit 3
   fi

   printf '  --- d=%s clean (raw: %s) ---\n' "$d" "$RAW" >> "$LOG"
done

echo "=== resweep d=$D_LO..$D_HI complete, all EMIRPS=0 ===" >> "$LOG"
