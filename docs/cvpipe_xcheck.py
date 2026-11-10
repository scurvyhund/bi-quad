#!/usr/bin/env python3
"""
cvpipe_xcheck.py -- check gmp-cvo/cvpipe run logs against hunt's results.

Predicts what each cvpipe run should have counted, from the survivor
n values in bi-quad/logs, and compares with the log's summary:

   consec-sq    = non-palindrome survivors passing cvpipe's filter
   palindromes  = palindromic survivors passing cvpipe's filter

Also confirms each run's zones bracket every (d, leading-pair) range and
that every zone n-value was walked. Works from n alone, sharing no code
with either C engine. See docs/cvpipe_resweep_xcheck.md.

Usage:
   python3 docs/cvpipe_xcheck.py [cvpipe_logs_dir]

Exit: 0 = all agree, 1 = a mismatch or coverage problem.
"""
import os
import re
import sys
from math import isqrt

HERE = os.path.dirname(os.path.abspath(__file__))
BIQUAD_LOGS = os.path.join(HERE, "..", "logs")
LEADING = {10, 12, 14, 16, 18, 31}
BRUTE_LIMIT = 7 * 10**6      # every n below this is brute-forced here

# (raw survivors, palindromes) from docs/resweep_d13-27_provenance.md
RESWEEP = {13: (4, 2), 14: (6, 0), 15: (5, 3), 16: (1, 0), 17: (1, 1),
           18: (0, 0), 19: (0, 0), 20: (0, 0), 21: (6, 4), 22: (0, 0),
           23: (2, 0), 24: (1, 0), 25: (5, 4), 26: (4, 0), 27: (4, 3)}

# log file, (lowest d, highest d)
RUNS = [("10e25_run_with_bugFix.txt", (2, 25)),
        ("run_10e26.log", (2, 26)),
        ("run_10e23.log", (23, 23)),
        ("run_10e24.log", (24, 24)),
        ("run_10e25.log", (25, 25)),
        ("run_10e27.log", (27, 27)),
        ("run_10e28.log", (28, 28))]

def curve_value(n):
   return 2 * n * n + 2 * n + 1

def on_curve(v):
   t = 2 * v - 1
   a = isqrt(t)
   return a * a == t and a % 2 == 1

def passes_filter(p):
   s = str(p)
   return int(s[:2]) in LEADING and s[-1] in "13"

def is_pal(p):
   return str(p) == str(p)[::-1]

def harvest():
   """Every n reported as a hit in the hunt logs, plus a local brute."""
   found = set()
   for root, _, files in os.walk(BIQUAD_LOGS):
      for name in files:
         with open(os.path.join(root, name), errors="replace") as f:
            found |= {int(x) for x in re.findall(
               r'(?:survivor|palindrome|EMIRP \*\*\*)\s+n=(\d+)', f.read())}
   bad = sorted(n for n in found
                if not on_curve(int(str(curve_value(n))[::-1])))
   if bad:
      print("excluded, not a real converse n (stray log lines):", bad)
   found -= set(bad)
   for n in range(1, BRUTE_LIMIT):
      s = str(curve_value(n))
      if s[-1] != '0' and on_curve(int(s[::-1])):
         found.add(n)
   return found

def parse_log(path):
   with open(path, errors="replace") as f:
      text = f.read()
   def field(key):
      return int(re.search(key + r':?\s+(\d+)', text).group(1))
   zones = [tuple(map(int, m)) for m in re.findall(
      r'^\s+\d+\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+\d+\s*$', text, re.M)]
   return {"zone_n": field("Zone n-vals"),
           "walked": field("Candidates generated"),
           "pal": field("Palindromes found"),
           "consec": field("Consec-sq pre-filter"),
           "zones": zones}

def coverage_problems(zones, lo_d, hi_d):
   """Does a zone bracket the true n-range of each (d, leading pair)?"""
   out = []
   for d in range(lo_d, hi_d + 1):
      for lead in sorted(LEADING):
         lo = lead * 10**(d - 2)
         hi = (lead + 1) * 10**(d - 2) - 1
         first = (isqrt(2 * lo - 1) - 1) // 2
         while curve_value(first) < lo:
            first += 1
         last = (isqrt(2 * hi - 1) - 1) // 2
         match = [z for z in zones if z[0] == lead and z[1] == d]
         if len(match) != 1 or match[0][2] > first or match[0][3] < last:
            out.append((d, lead))
   return out

def main():
   logs = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
      HERE, "..", "..", "gmp-cvo", "logs")
   by_d = {}
   for n in harvest():
      by_d.setdefault(len(str(curve_value(n))), []).append(curve_value(n))

   rc = 0
   print(" d  resweep  found | filter: non-pal pal")
   non_pal, pals = {}, {}
   for d in range(2, 29):
      vals = by_d.get(d, [])
      kept = [p for p in vals if p % 5]
      got = (len(kept), sum(is_pal(p) for p in kept))
      hits = [p for p in vals if passes_filter(p)]
      pals[d] = sum(is_pal(p) for p in hits)
      non_pal[d] = len(hits) - pals[d]
      want = RESWEEP.get(d)
      note = ""
      if want is not None and want != got:
         note = "  <-- MISMATCH"
         rc = 1
      print("%2d  %-7s  %-6s | %6d %4d%s" % (d, want or "-", got,
            non_pal[d], pals[d], note))

   print("\nlog                         d       consec  pal  coverage")
   for name, (lo, hi) in RUNS:
      r = parse_log(os.path.join(logs, name))
      want = (sum(non_pal[d] for d in range(lo, hi + 1)),
              sum(pals[d] for d in range(lo, hi + 1)))
      problems = coverage_problems(r["zones"], lo, hi)
      ok = (r["consec"], r["pal"]) == want and not problems \
           and r["walked"] == r["zone_n"]
      rc |= 0 if ok else 1
      print("%-27s %2d..%2d  %d/%d  %d/%d  %s%s" % (name, lo, hi,
            r["consec"], want[0], r["pal"], want[1],
            "complete" if not problems and r["walked"] == r["zone_n"]
            else "INCOMPLETE %s" % problems[:3], "" if ok else "  <-- DIFF"))

   print("\nVERDICT:", "AGREE" if rc == 0 else "DISAGREE")
   return rc

if __name__ == "__main__":
   sys.exit(main())
