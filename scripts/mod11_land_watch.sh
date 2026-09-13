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

# wait for BOTH legs: hunt gone AND no resweep_leg wrapper still queued
while pgrep -x hunt >/dev/null || pgrep -f '[r]esweep_leg.sh' >/dev/null; do
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

if grep -q 'ALL CLEAN' "$OUT"; then
   notify-send -u normal "mod-11 gap CLOSED" "d=16/24/26 all clean. See $OUT"
else
   notify-send -u critical "mod-11 VIOLATION" "LOOK NOW: $OUT"
fi
