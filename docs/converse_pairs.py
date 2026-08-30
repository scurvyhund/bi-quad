#!/usr/bin/env python3
"""
converse_pairs.py -- enumerate converse pairs on the curve
p = 2n^2 + 2n + 1 = n^2 + (n+1)^2, independently of the C engines.

A CONVERSE PAIR is p together with q = rev(p) where BOTH lie on the
curve (equivalently, 2p-1 and 2q-1 are both odd perfect squares).
An emirp is a converse pair that is prime on both sides.

This is deliberately a dumb, self-contained brute force: it exists to
cross-check mod_obstruct / hunt under the project's verification rule,
so it shares no code with them.

is_prime() and curve_index() are INTENTIONALLY duplicated from
~/programming/python_gold/{miller_rabin,perfect_square}.py rather than
imported. A cross-check tool that depends on a shared library is only
as independent as that library; keeping this file standalone means it
runs anywhere with nothing but python3. Do not "de-duplicate" it.

Usage:
   python3 converse_pairs.py [limit]     # default 10**12
   python3 converse_pairs.py 1e14        # scientific notation ok
   python3 converse_pairs.py --check     # verify the mod-11 law and
                                         # the even-d exclusion

Even-d exclusion (see docs/mod11_converse_constraint.md): when d is
even a converse pair requires p mod 11 in {3,5,6,8}. Violations
reported by --check would mean the doc is wrong.

Origin: BigFermat/bi-quad, 2026-08-29.
"""
import sys
from math import isqrt

EVEN_D_ALLOWED = frozenset((3, 5, 6, 8))
_WITNESSES = (2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37)

def is_prime(n):
   """Deterministic Miller-Rabin below 3.3e24, probabilistic above."""
   if n < 2:
      return False
   for w in _WITNESSES:
      if n % w == 0:
         return n == w
   d, r = n - 1, 0
   while d % 2 == 0:
      d //= 2
      r += 1
   for a in _WITNESSES:
      x = pow(a, d, n)
      if x in (1, n - 1):
         continue
      for _ in range(r - 1):
         x = x * x % n
         if x == n - 1:
            break
      else:
         return False
   return True

def curve_index(v):
   """Return m with v = 2m^2+2m+1, or None if v is not on the curve."""
   t = 2 * v - 1
   a = isqrt(t)
   if a * a != t or a % 2 == 0:
      return None
   return (a - 1) // 2

def converse_pairs(limit):
   """Yield (n, p, m, q) for each converse pair with p <= limit,
   reported once per pair (n < m)."""
   n = 1
   while True:
      p = 2 * n * n + 2 * n + 1
      if p > limit:
         return
      s = str(p)
      rv = s[::-1]
      if rv[0] != '0' and rv != s:
         q = int(rv)
         m = curve_index(q)
         if m is not None and n < m:
            yield (n, p, m, q)
      n += 1

def _check(limit):
   bad = 0
   for n, p, m, q in converse_pairs(limit):
      d = len(str(p))
      law = (q - p) % 11 == 0 if d % 2 else (q + p) % 11 == 0
      if not law:
         print("VIOLATION reversal law: p=%d" % p)
         bad += 1
      if d % 2 == 0 and p % 11 not in EVEN_D_ALLOWED:
         print("VIOLATION even-d mod 11: p=%d  p%%11=%d" % (p, p % 11))
         bad += 1
   print("checked converse pairs up to %d: %d violation(s)" % (limit, bad))
   return 1 if bad else 0

def _main(argv):
   if argv and argv[0] == "--check":
      lim = int(float(argv[1])) if len(argv) > 1 else 10 ** 12
      return _check(lim)
   lim = int(float(argv[0])) if argv else 10 ** 12
   print("%-3s %-12s %-18s %-12s %-18s %-6s %-6s"
         % ("d", "n", "p", "m", "q", "p_pr", "q_pr"))
   count = emirps = 0
   for n, p, m, q in converse_pairs(lim):
      pp, qp = is_prime(p), is_prime(q)
      count += 1
      emirps += pp and qp
      print("%-3d %-12d %-18d %-12d %-18d %-6s %-6s"
            % (len(str(p)), n, p, m, q, pp, qp))
   print("\n%d converse pair(s), %d emirp(s), p <= %d"
         % (count, emirps, lim))
   return 0

if __name__ == "__main__":
   sys.exit(_main(sys.argv[1:]))
