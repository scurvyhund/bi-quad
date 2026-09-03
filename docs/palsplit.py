#!/usr/bin/env python3
"""
palsplit.py -- prototype of the head/tail split search for palindromic
curve-values p = 2n^2+2n+1.

Idea under test:
  A d-digit palindrome's high t digits are the reverse of its low t
  digits.  So one choice of "n mod 10^t" fixes BOTH:
     - the low t digits of p      (directly, p = f(n) mod 10^t)
     - the high t digits of p     (= reverse of the low t digits)
  and the high digits pin n to a narrow band via n ~ sqrt(p/2).
  Intersecting "n in band" with "n = r mod 10^t" leaves ~10^(d/2-2t)
  candidates per residue, over 10^t residues.  Balanced at t ~ d/4
  this is O(10^(d/4)) instead of the O(10^(d/2)) n-enumeration.

Validation: does it recover every known palindromic curve-value?
"""
import sys, time
from math import isqrt

def f(n):
    return 2*n*n + 2*n + 1

def search(d, t):
    """All palindromic curve-values with exactly d digits."""
    M = 10**t
    lo10 = 10**(d-1)          # smallest d-digit number
    hi10 = 10**d              # one past largest
    found = []
    ops = 0
    for r in range(M):
        L = f(r) % M                       # low t digits of p
        s = str(L).zfill(t)                # "p_{t-1} ... p_0"
        if s[-1] == '0':                   # p_0 is p's LEADING digit
            continue
        H = int(s[::-1])                   # high t digits of p
        Plo = H * 10**(d-t)
        Phi = Plo + 10**(d-t)
        if Plo < lo10 or Phi > hi10:
            continue
        # n band from the magnitude of p
        nlo = (isqrt(2*Plo - 1) - 1)//2 - 2
        nhi = (isqrt(2*Phi - 1) - 1)//2 + 2
        if nlo < 0:
            nlo = 0
        # first n >= nlo with n = r (mod M)
        n = nlo + ((r - nlo) % M)
        while n <= nhi:
            ops += 1
            p = f(n)
            if Plo <= p < Phi:
                ps = str(p)
                if ps == ps[::-1]:
                    found.append((p, n))
            n += M
    return sorted(found), ops, M

def brute(d):
    """Ground-truth n-enumeration (only feasible for small d)."""
    lo10 = 10**(d-1); hi10 = 10**d
    n = (isqrt(2*lo10 - 1) - 1)//2
    while f(n) < lo10:
        n += 1
    out = []
    while True:
        p = f(n)
        if p >= hi10:
            break
        ps = str(p)
        if ps == ps[::-1]:
            out.append((p, n))
        n += 1
    return out

KNOWN = {
 13: [1635446445361, 3166046406613],
 15: [124852060258421, 149988757889941, 310433303334013],
 21: [108348382545283843801, 129052205999502250921,
      129776662212266677921, 316234169939961432613],
 25: [1427056470511150746507241, 3130000999262629990000313,
      3153012324698964232103513, 3169345937085807395439613,
      5054998070382830708994505],
 27: [108491007414868414700194801, 318216440234333432044612813,
      318288756988131889657882813],
}

if __name__ == "__main__":
    print("=== brute-force cross-check that the .txt files are complete ===")
    for d in (11, 13, 15):
        b = brute(d)
        print(f"  d={d:2d} brute: {[p for p,_ in b]}")

    print()
    print("=== head/tail split search vs known values ===")
    for d, t in ((13,3), (15,4), (21,5), (25,6), (27,7)):
        t0 = time.time()
        got, ops, M = search(d, t)
        el = time.time() - t0
        gotp = [p for p,_ in got]
        known = KNOWN[d]
        missing = [p for p in known if p not in gotp]
        extra   = [p for p in gotp if p not in known]
        status = "OK " if not missing else "MISS"
        print(f"  d={d:2d} t={t}  found={len(gotp):2d}  known={len(known)}  "
              f"outer=10^{t}  inner_ops={ops:,}  {el:6.1f}s  [{status}]")
        if missing:
            print(f"        MISSING: {missing}")
        for p in extra:
            tag = "div-5 (unlisted in file)" if p % 5 == 0 else "UNEXPECTED"
            print(f"        extra: {p}  n={dict(got)[p] if False else ''} {tag}")
        sys.stdout.flush()
