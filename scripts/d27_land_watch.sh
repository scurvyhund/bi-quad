#!/bin/bash
# d27_land_watch.sh -- shout when the d=27 resweep leg lands.
#
# DETACHED on purpose: survives the Claude session, the terminal, and
# a logout.  It can only shout, not analyse -- it writes a marker file
# and fires a desktop notification, nothing more.
#
# Exits on EITHER terminal state.  Silence is not success: a crashed
# hunt must break the loop too, or a crash looks like "still running".
#
#   nohup scripts/d27_land_watch.sh >/dev/null 2>&1 &

cd /home/jim/programming/c/BigFermat/bi-quad || exit 1
LOG=logs/hunt_resweep_2026-09-05.log
MARK=logs/d27_LANDED.txt
# process name is overridable ONLY so the death branch can be tested
# (WATCH_PROC=nosuchproc).  Production leaves it at hunt.
PROC=${WATCH_PROC:-hunt}

while true; do
   if grep -qE '^  d=27 ' "$LOG"; then
      line=$(grep -E '^  d=27 ' "$LOG" | head -1)
      { date '+%F %T  d=27 LANDED'; echo "$line"; } > "$MARK"
      case "$line" in
         *EMIRPS=0*) u=normal; t="d=27 clean (EMIRPS=0)" ;;
         *)          u=critical; t="d=27 NONZERO EMIRPS -- LOOK NOW" ;;
      esac
      notify-send -u "$u" "$t" "$line"
      exit 0
   fi
   # terminal failure: hunt gone with no d=27 line written
   if ! pgrep -x "$PROC" >/dev/null; then
      # hunt is gone.  The result line is written by the wrapper AFTER
      # the process exits, so do NOT call it death on one look -- poll
      # the grace window and bail out the moment the line appears.
      # A single-shot grace check reports a false DEATH whenever the
      # write lands a moment late.
      for _g in $(seq 1 "${GRACE_TRIES:-24}"); do
         grep -qE '^  d=27 ' "$LOG" && break
         sleep "${GRACE_SLEEP:-5}"
      done
      grep -qE '^  d=27 ' "$LOG" && continue
      { date '+%F %T  d=27 DIED with no result line'; } > "$MARK"
      notify-send -u critical "d=27 DIED" \
         "hunt is gone and no d=27 line was written -- investigate"
      exit 2
   fi
   sleep 60
done
