#!/usr/bin/env python3
"""
palfirst.py -- palindrome-FIRST verifier for the curve p = 2n^2+2n+1.

Independent cross-check for hunt.c (which is n-FIRST). Instead of looping
over n, this loops over d-digit palindromes P and asks whether each is on
the curve. P is on the curve  <=>  2P-1 is a perfect (odd) square, because
p = 2n^2+2n+1  <=>  2p-1 = (2n+1)^2. The test is exact (integer isqrt), no
float. Two unrelated methods agreeing (this vs hunt.c) is the ground truth.

Primality is flagged with Miller-Rabin: deterministic for P < 3.3e24
(d <= 24), robustly probabilistic beyond. Definitive COMPOSITENESS of a
large candidate is confirmed by factoring instead -- see CREDITS.md / qs.

Practical to ~d=15 (Python enumerates ~9*10^((d-1)/2) palindromes); for
larger d use palhunt_gmp, the GMP palindrome-first hunter.

Even d yield 0: every even-length palindrome is divisible by 11, and no
curve value is EVER divisible by 11 (p = n^2+(n+1)^2 is a sum of coprime
squares, and 11 = 3 mod 4). So the genuine tests are the ODD d only.

Usage:   python3 palfirst.py [D_MIN] [D_MAX]        (defaults 1 15)
Example: python3 palfirst.py 1 13
         d= 7  palindromes=9000  on-curve=5  primes=1
               ... 3187813 PRIME ...
"""
import sys
from math import isqrt

# deterministic MR up to 3.317e24; effectively certain beyond
_MR_WITNESSES = (2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37)

def is_prime(n):
   if n < 2:
      return False
   for p in _MR_WITNESSES:
      if n % p == 0:
         return n == p
   d = n - 1
   r = 0
   while d % 2 == 0:
      d //= 2
      r += 1
   for a in _MR_WITNESSES:
      x = pow(a, d, n)
      if x == 1 or x == n - 1:
         continue
      for _ in range(r - 1):
         x = x * x % n
         if x == n - 1:
            break
      else:
         return False
   return True

def on_curve(P):
   """True iff P = 2n^2+2n+1 for some integer n >= 0 (2P-1 an odd square)."""
   t = 2 * P - 1
   s = isqrt(t)
   return s * s == t and s % 2 == 1

def palindromes(d):
   """Yield every d-digit palindrome in increasing order."""
   k = (d + 1) // 2
   for half in range(10 ** (k - 1), 10 ** k):
      s = str(half)
      tail = s[:-1] if d % 2 else s      # drop the middle digit for odd d
      yield int(s + tail[::-1])

def main():
   d_min = int(sys.argv[1]) if len(sys.argv) > 1 else 1
   d_max = int(sys.argv[2]) if len(sys.argv) > 2 else 15
   print("palindrome-first curve check   p = 2n^2+2n+1 = n^2+(n+1)^2")
   print(f"{'d':>3} {'palindromes':>12} {'on-curve':>9} {'primes':>7}")
   for d in range(d_min, d_max + 1):
      hits = [P for P in palindromes(d) if on_curve(P)]
      primes = [P for P in hits if is_prime(P)]
      npal = 9 * 10 ** ((d - 1) // 2)
      tag = "  (even d: 0 by /11 dodge)" if d % 2 == 0 and not hits else ""
      print(f"{d:>3} {npal:>12} {len(hits):>9} {len(primes):>7}{tag}")
      for P in hits:
         print(f"      {P}{'  PRIME' if P in primes else ''}")

if __name__ == "__main__":
   main()
