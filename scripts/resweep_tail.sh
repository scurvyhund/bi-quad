#!/bin/bash
# Continue the emirp resweep past d=23, one length at a time.
# Gate: proceed only while EMIRPS=0.  A nonzero EMIRPS is the whole
# point of the search -- stop and make noise rather than roll on.
cd /home/jim/programming/c/BigFermat/bi-quad || exit 1
LOG=logs/hunt_resweep_2026-09-05.log

# wait out whatever hunt is currently running (never -f: it matches self)
while pgrep -x hunt >/dev/null; do sleep 15; done

# gate on d=23 from the existing sweep
if ! grep -qE "d=23 .*EMIRPS=0" $LOG; then
   echo "!!! GATE FAILED: d=23 not clean, halting before d=24" >> $LOG
   exit 2
fi
echo "  gate: d=23 clean, continuing to d=24..27" >> $LOG

for d in 24 25 26 27; do
   /usr/bin/time -f "  wall=%es" ./hunt $d $d 2>&1 \
      | grep -E "range=|wall=|EMIRP" >> $LOG
   if ! grep -qE "d=$d .*EMIRPS=0" $LOG; then
      echo "!!! HALT: d=$d did NOT report EMIRPS=0 -- look at this" >> $LOG
      exit 3
   fi
   echo "  --- d=$d clean ---" >> $LOG
done
echo "=== resweep d=13..27 complete, all EMIRPS=0 ===" >> $LOG
