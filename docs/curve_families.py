#!/usr/bin/env python3
"""curve_families.py -- verify every claim in docs/curve_families.md

Standalone: shares no code with hunt.c, mod_obstruct.c, converse_pairs.py
or nearby_curves.py, so agreement is evidence rather than a shared bug.

   python3 curve_families.py

A curve here is  alpha*p + beta = gamma*a^2  with a linear in n.
Theorem A (even d):  a^2 + b^2 = 2*beta/gamma   (mod 11)
Theorem B (palin.):  a^2       =   beta/gamma   (mod 11)
"""
import sys
from math import isqrt

SQ11 = sorted({s * s % 11 for s in range(11)})       # {0,1,3,4,5,9}


def inv11(z):
   return pow(z % 11, -1, 11)


class Curve:
   """p = poly(n), with membership  alpha*p + beta = gamma*a^2, a = an*n+a0."""

   def __init__(self, name, poly, alpha, beta, gamma, an, a0):
      self.name, self.poly = name, poly
      self.alpha, self.beta, self.gamma = alpha, beta, gamma
      self.an, self.a0 = an, a0

   def p(self, n):
      return self.poly(n)

   def index(self, v):
      """n with v on the curve, else None.  Inverts alpha*v+beta = gamma*a^2."""
      t = self.alpha * v + self.beta
      if t < 0 or self.gamma == 0 or t % self.gamma:
         return None
      s = t // self.gamma
      a = isqrt(s)
      if a * a != s:
         return None
      if (a - self.a0) % self.an:
         return None
      n = (a - self.a0) // self.an
      return n if n >= 0 and self.p(n) == v else None

   # ---- the two theorems ------------------------------------------------
   def target_A(self):
      return (2 * self.beta * inv11(self.gamma)) % 11

   def allowed_p(self):
      t = self.target_A()
      a_ok = {x for x in SQ11 for y in SQ11 if (x + y) % 11 == t}
      ia = inv11(self.alpha)
      return sorted({(ia * (self.gamma * x - self.beta)) % 11 for x in a_ok})

   def target_B(self):
      return (self.beta * inv11(self.gamma)) % 11

   def palindrome_possible(self):
      return self.target_B() in SQ11


def mk(k):
   return Curve("k=%d  Z[i], y=x+%d" % (k, k),
                lambda n, k=k: n * n + (n + k) ** 2,
                2, -k * k, 1, 2, k)


CURVES = [
   Curve("ours  Z[i], y=x+1", lambda n: n * n + (n + 1) ** 2, 2, -1, 1, 2, 1),
   mk(3), mk(5), mk(7), mk(9), mk(11),
   Curve("cuban Z[w], y=x+1", lambda n: 3 * n * n + 3 * n + 1, 4, -1, 3, 2, 1),
   Curve("Z[sqrt-2], y=x+1", lambda n: n * n + 2 * (n + 1) ** 2, 3, -2, 1, 3, 2),
]


def rev(v):
   return int(str(v)[::-1])


# --------------------------------------------------------------- checks

def check_membership():
   """alpha*p + beta = gamma*a^2 holds, and index() inverts p()."""
   for c in CURVES:
      for n in range(300):
         p = c.p(n)
         a = c.an * n + c.a0
         assert c.alpha * p + c.beta == c.gamma * a * a, \
             "%s fails at n=%d" % (c.name, n)
         assert c.index(p) == n, "%s index() wrong at n=%d" % (c.name, n)
   return "alpha*p+beta = gamma*a^2 and index() invert for all %d curves" % len(
          CURVES)


def check_cuban_is_cube_difference():
   for n in range(300):
      assert 3 * n * n + 3 * n + 1 == (n + 1) ** 3 - n ** 3
   return "cuban curve 3n^2+3n+1 == (n+1)^3 - n^3"


def check_reversal_law():
   """rev(p) = (-1)^(d-1) * p  (mod 11)."""
   for p in list(range(1, 4000)) + [12641, 14621, 3187813, 919]:
      d = len(str(p))
      want = (p if d % 2 else -p) % 11
      assert rev(p) % 11 == want, "reversal law fails at %d" % p
   return "rev(p) = (-1)^(d-1)*p (mod 11) on 4000+ values"


def check_published_tables():
   """The allowed-residue and palindrome columns printed in the doc."""
   want = {
      "ours  Z[i], y=x+1": ([3, 5, 6, 8], False),
      "k=3  Z[i], y=x+3": ([1, 5, 6, 10], False),
      "cuban Z[w], y=x+1": ([3, 4, 7, 8], False),
      "Z[sqrt-2], y=x+1": ([0, 2, 9], True),
      "k=11  Z[i], y=x+11": ([0], True),
   }
   for c in CURVES:
      if c.name in want:
         wa, wp = want[c.name]
         assert c.allowed_p() == wa, "%s: %r != %r" % (c.name, c.allowed_p(), wa)
         assert c.palindrome_possible() == wp, "%s palindrome flag" % c.name
   return "allowed-residue + palindrome columns match the doc"


def check_theorems_empirically(limit=2 * 10**7):
   """Theorem A: no even-d converse pair outside the allowed set.
      Theorem B: even-digit palindromes appear iff permitted."""
   notes = []
   for c in CURVES:
      ok = set(c.allowed_p())
      may = c.palindrome_possible()
      pairs = viol = 0
      epals = []
      n = 0
      while True:
         p = c.p(n)
         if p > limit:
            break
         s = str(p)
         if len(s) % 2 == 0:
            if s == s[::-1]:
               epals.append(p)
               if p % 11 not in ok:
                  viol += 1
            else:
               q = rev(p)
               if c.index(q) is not None:
                  pairs += 1
                  if p % 11 not in ok:
                     viol += 1
         n += 1
      assert viol == 0, "%s: %d Theorem-A violations" % (c.name, viol)
      if not may:
         assert not epals, "%s forbids even-d palindromes but found %r" % (
                           c.name, epals[:3])
      notes.append("%s:%dp%s" % (c.name.split()[0], pairs,
                                 "/pal%d" % len(epals) if epals else ""))
   return "Theorem A: 0 violations; Theorem B consistent  [%s]" % ", ".join(notes)


def check_falsification(limit=2 * 10**8):
   """Where Theorem B PERMITS palindromes, they must actually occur --
      otherwise the theorem is only ever predicting absences."""
   found = {}
   for c in CURVES:
      if not c.palindrome_possible():
         continue
      pals = []
      n = 0
      while True:
         p = c.p(n)
         if p > limit:
            break
         s = str(p)
         if len(s) % 2 == 0 and s == s[::-1]:
            pals.append(p)
         n += 1
      assert pals, "%s permits even-d palindromes but none below %.0e" % (
                   c.name, limit)
      found[c.name.split(",")[0]] = pals[:3]
   return "permitted curves really do contain them: %r" % found


def check_odd_d_is_toothless():
   """Odd d gives only a^2 = b^2, i.e. a = +-b -- and our known objects
      (the d=5 emirp, the d=7 palindrome) live there."""
   c = CURVES[0]
   for p, q in ((12641, 14621),):
      assert len(str(p)) % 2 == 1
      a = 2 * c.index(p) + 1
      b = 2 * c.index(q) + 1
      assert (a * a - b * b) % 11 == 0
   assert len(str(3187813)) % 2 == 1 and c.index(3187813) is not None
   return "odd d yields only a^2 = b^2; d=5 emirp and d=7 palindrome sit there"


# ------------------------------------------------------------------ main

def main():
   checks = [check_membership, check_cuban_is_cube_difference,
             check_reversal_law, check_published_tables,
             check_theorems_empirically, check_falsification,
             check_odd_d_is_toothless]
   print("\n  verifying docs/curve_families.md\n")
   for fn in checks:
      try:
         msg = fn()
      except AssertionError as e:
         print("  FAIL  %-28s %s" % (fn.__name__, e))
         return 1
      print("  ok    %-28s %s" % (fn.__name__, msg))
   print("\n  all %d checks passed\n" % len(checks))
   return 0


if __name__ == "__main__":
   sys.exit(main())
