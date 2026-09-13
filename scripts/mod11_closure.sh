#!/bin/bash
# mod11_closure.sh -- wait for a hunt leg, then CLOSE the mod-11 gap
# for it: extract every survivor from the raw capture, recompute
# p mod 11 independently, and assert the even-d invariant.
#
# Why this exists: the 2026-09-05 wrapper filtered hunt's stderr, so
# the mod-11 alarm could not be seen for d=16/24/26.  Re-running under
# the tee'd runner restores the channel -- but "no BUG line" is a NULL
# result, and a null result must be earned.  This recomputes the
# residues from the survivor values rather than trusting silence.
#
#   scripts/mod11_closure.sh <d> [--wait]
set -u
cd /home/jim/programming/c/BigFermat/bi-quad || exit 1
D=${1:?usage: mod11_closure.sh <d> [--wait]}
WAIT=${2:-}

if [ "$WAIT" = "--wait" ]; then
   while pgrep -x hunt >/dev/null; do sleep 30; done
   sleep 10
fi

RAW=$(ls -t logs/raw/hunt_d${D}_*.raw 2>/dev/null | head -1)
[ -n "$RAW" ] && [ -s "$RAW" ] || { echo "NO RAW CAPTURE for d=$D"; exit 4; }

# the leg must actually have finished
grep -qE "^  d= *$D .*EMIRPS=" "$RAW" || { echo "d=$D: NO result line in $RAW"; exit 5; }

python3 - "$D" "$RAW" <<'PY'
import re,sys
d=int(sys.argv[1]); raw=sys.argv[2]
txt=open(raw,errors="replace").read()
bugs=[l for l in txt.splitlines() if "BUG" in l]
hits=re.findall(r'^  (\*\*\* EMIRP \*\*\*|palindrome|survivor)\s+n=(\d+)\s+p=(\d+)',
                txt, re.M)
print(f"  d={d}  raw={raw}")
print(f"  survivors found in raw: {len(hits)}")
print(f"  BUG lines in raw      : {len(bugs)}")
for b in bugs: print("    "+b)
bad=0
for lbl,n,p in hits:
    n=int(n); p=int(p)
    oncurve = (2*n*n+2*n+1 == p)
    pal = (str(p)==str(p)[::-1])
    r11 = p % 11
    ok = True; why=[]
    if not oncurve: ok=False; why.append("NOT ON CURVE")
    if d%2==0:
        # even-d: no palindrome may lie on the curve at all
        if pal: ok=False; why.append("EVEN-d PALINDROME (theorem says impossible)")
        elif r11 not in (3,5,6,8): ok=False; why.append(f"p mod 11 = {r11} not in {{3,5,6,8}}")
    if not ok: bad+=1
    print(f"    [{'OK ' if ok else 'BAD'}] {lbl:<13} n={n} p mod 11={r11} pal={pal}"
          + (("  <- "+"; ".join(why)) if why else ""))
print(f"  VERDICT: {'CLEAN' if (bad==0 and not bugs) else 'VIOLATION'}"
      f"  (independently recomputed, not inferred from silence)")
sys.exit(0 if (bad==0 and not bugs) else 3)
PY
