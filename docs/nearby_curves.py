#!/usr/bin/env python3
"""nearby_curves.py -- verify every claim in docs/nearby_curves.md

Deliberately standalone: shares no code with hunt.c, mod_obstruct.c or
converse_pairs.py, so agreement between them is evidence rather than a
shared bug.  Same policy as docs/brute_validate.py.

   python3 nearby_curves.py            # run all checks
   python3 nearby_curves.py --census   # also the (slow) 1e9 emirp census

The family is  p = n^2 + (n+k)^2  for odd k; k=1 is the project's curve.
"""
import sys
from math import isqrt

LIM_FAST = 10**8          # palindrome / converse-pair checks
LIM_CENSUS = 10**9        # full emirp census (minutes)


# ---------------------------------------------------------------- helpers

def is_prime(v):
   """Deterministic Miller-Rabin; exact for all 64-bit v."""
   if v < 2:
      return False
   small = (2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37)
   for q in small:
      if v % q == 0:
         return v == q
   d, s = v - 1, 0
   while d % 2 == 0:
      d //= 2
      s += 1
   for a in small:
      x = pow(a, d, v)
      if x in (1, v - 1):
         continue
      for _ in range(s - 1):
         x = x * x % v
         if x == v - 1:
            break
      else:
         return False
   return True


def rev(v):
   return int(str(v)[::-1])


def curve_index(v, k):
   """n with v = n^2 + (n+k)^2, else None.  Uses 2v = a^2 + k^2."""
   t = 2 * v - k * k
   if t < 0:
      return None
   a = isqrt(t)
   if a * a != t or (a - k) % 2:
      return None
   n = (a - k) // 2
   return n if n >= 0 else None


SQ11 = sorted({s * s % 11 for s in range(11)})     # {0,1,3,4,5,9}
INV2 = pow(2, -1, 11)                              # 6


def allowed_residues(k):
   """p mod 11 permitted for an even-d converse pair on curve k."""
   t = (-2 * k * k) % 11
   a_ok = {x for x in SQ11 for y in SQ11 if (x + y) % 11 == t}
   return sorted({(INV2 * (x + k * k)) % 11 for x in a_ok})


# ----------------------------------------------------------------- checks

def check_membership_identity():
   """2p = a^2 + k^2 with a = 2n+k."""
   bad = []
   for k in range(0, 12):
      for n in range(0, 400):
         p = n * n + (n + k) ** 2
         if 2 * p != (2 * n + k) ** 2 + k * k:
            bad.append((k, n))
   assert not bad, "membership identity failed: %r" % bad[:5]
   # and the k=1 specialization the project actually uses
   for n in range(0, 400):
      p = 2 * n * n + 2 * n + 1
      assert 2 * p - 1 == (2 * n + 1) ** 2
      assert curve_index(p, 1) == n
   return "2p = a^2 + k^2 holds (k<=11, n<400); k=1 gives 2p-1 = a^2"


def check_parity():
   """Only odd k can produce odd p."""
   for k in range(0, 10):
      vals = [n * n + (n + k) ** 2 for n in range(1, 40)]
      all_even = all(v % 2 == 0 for v in vals)
      assert all_even == (k % 2 == 0), "parity claim wrong at k=%d" % k
   return "k even -> p always even (dead); k odd -> p can be prime"


def check_mod11_table():
   """The published allowed-residue table."""
   expect = {1: [3, 5, 6, 8], 3: [1, 5, 6, 10], 5: [2, 4, 7, 9],
             7: [3, 4, 7, 8], 9: [1, 2, 9, 10], 11: [0],
             13: [1, 2, 9, 10]}
   for k, want in expect.items():
      got = allowed_residues(k)
      assert got == want, "k=%d: got %r want %r" % (k, got, want)
   return "mod-11 allowed-residue table matches the doc for k in %s" % (
          sorted(expect))


def check_mod11_empirical(limit=LIM_FAST):
   """No even-d converse pair violates its curve's allowed set."""
   out = []
   for k in (1, 3, 5, 7, 9, 11):
      ok = set(allowed_residues(k))
      pairs = viol = 0
      n = 0
      while True:
         p = n * n + (n + k) ** 2
         if p > limit:
            break
         if len(str(p)) % 2 == 0:
            q = rev(p)
            if q != p and curve_index(q, k) is not None:
               pairs += 1
               if p % 11 not in ok:
                  viol += 1
         n += 1
      assert viol == 0, "k=%d: %d violations" % (k, viol)
      out.append("k=%d:%d pairs" % (k, pairs))
   return "no mod-11 violations below %.0e  [%s]" % (limit, ", ".join(out))


def check_palindrome_theorem(limit=LIM_FAST):
   """No even-digit palindrome on the curve unless 11 | k."""
   assert (-1) % 11 not in SQ11, "-1 must be a non-residue mod 11"
   found = {}
   for k in (1, 3, 5, 7, 9, 11, 13, 22, 33):
      pals = []
      n = 0
      while True:
         p = n * n + (n + k) ** 2
         if p > limit:
            break
         s = str(p)
         if len(s) % 2 == 0 and s == s[::-1]:
            pals.append(p)
         n += 1
      found[k] = pals
      if k % 11:
         assert not pals, "k=%d should have NO even-digit palindrome, got %r" % (
                          k, pals[:3])
      else:
         assert pals, "k=%d should permit them, found none below %e" % (k, limit)
   # and they are all composite (classical: 11 divides them)
   for k in (11, 22, 33):
      for p in found[k]:
         assert not is_prime(p) and p % 11 == 0, "%d unexpectedly prime" % p
   ex = {k: v for k, v in found.items() if v}
   return "11|k exception confirmed, all composite: %r" % ex


def check_known_emirps():
   """The two emirp pairs the doc names."""
   for (p, q, k) in ((12641, 14621, 1), (37, 73, 5)):
      assert curve_index(p, k) is not None, "%d not on curve k=%d" % (p, k)
      assert curve_index(q, k) is not None, "%d not on curve k=%d" % (q, k)
      assert rev(p) == q and rev(q) == p
      assert is_prime(p) and is_prime(q)
   # 37<->73 is even-d, so it must satisfy the mod-11 rule
   ok = allowed_residues(5)
   assert 37 % 11 in ok and 73 % 11 in ok
   return "12641<->14621 (k=1, d=5) and 37<->73 (k=5, d=2) verified"


def check_palhunt_positive_controls():
   """Prime palindromes that already exist on our own curve."""
   want = [5, 181, 313, 3187813]
   got = []
   n = 0
   while True:
      p = 2 * n * n + 2 * n + 1
      if p > 10**10:
         break
      s = str(p)
      if s == s[::-1] and is_prime(p):
         got.append(p)
      n += 1
   assert got == want, "got %r want %r" % (got, want)
   return "palhunt regression markers on k=1: %r" % want


def census(limit=LIM_CENSUS):
   print("\n  family census below %.0e (slow)" % limit)
   print("  %-4s %-13s %-16s %s" % ("k", "curve primes", "converse pairs",
                                    "emirp pairs"))
   for k in (1, 3, 5, 7, 9, 11):
      prim = cp = em = 0
      ex = []
      n = 0
      while True:
         p = n * n + (n + k) ** 2
         if p > limit:
            break
         pp = is_prime(p)
         prim += pp
         q = rev(p)
         if q != p and curve_index(q, k) is not None:
            cp += 1
            if pp and is_prime(q):
               em += 1
               ex.append((p, q))
         n += 1
      print("  %-4d %-13d %-16d %d %s" % (k, prim, cp, em // 2,
                                          ex[:1] if ex else ""))


# ------------------------------------------------------------------- main

def main(argv):
   checks = [check_membership_identity, check_parity, check_mod11_table,
             check_mod11_empirical, check_palindrome_theorem,
             check_known_emirps, check_palhunt_positive_controls]
   print("\n  verifying docs/nearby_curves.md\n")
   for fn in checks:
      try:
         msg = fn()
      except AssertionError as e:
         print("  FAIL  %-28s %s" % (fn.__name__, e))
         return 1
      print("  ok    %-28s %s" % (fn.__name__, msg))
   print("\n  all %d checks passed" % len(checks))
   if "--census" in argv:
      census()
   print()
   return 0


if __name__ == "__main__":
   sys.exit(main(sys.argv[1:]))
