"""brute_validate.py -- independent check of mod_obstruct's SIEVE MODEL.

What it is: a from-scratch Python re-implementation of the survivor
test that mod_obstruct.c computes at depth k, evaluated by enumerating
EVERY n in the d-digit window.  A residue r = n mod 10^k survives when
some n == r has p = 2n^2+2n+1 with
  - a prime-eligible ending e = p mod 10^k (last digit not 0 or 5),
  - a prefix f = first k digits of p with rev(f) a valid ending, and
  - SOME real n' in the window whose p' has first k digits rev(e) and
    last k digits rev(f) -- i.e. q = rev(p) could be on the curve as
    far as its first and last k digits can tell.
It shares no code with mod_obstruct (different language, no Hensel
lift, no residue arithmetic): a common bug cannot fool both.

What it is NOT: an emirp ground truth.  It tests no primality and
checks only the outer k digits, so it says nothing about whether an
emirp exists at d.  The brute force for emirp claims is hunt.c.

Hardcoded to k = 6, d = 12, 13, 14 -- the k=6 sanity markers, chosen
around the range<mod cliff (range/mod = 0.48, 1.53, 4.84).

Usage:  python3 -I docs/brute_validate.py        (~10 s)

Expected output (must equal ./mod_obstruct 50 6 6 11 at d=12..14):
    building valid-ending set (mod 1000000)...
      |VE| = 54176
      d=12  GROUND-TRUTH survivors = 0   (range=483500, range/mod=0.483)
      d=13  GROUND-TRUTH survivors = 8   (range=1528961, range/mod=1.529)
      d=14  GROUND-TRUTH survivors = 30   (range=4835000, range/mod=4.835)
"""
from math import isqrt

k = 6
mod = 10**k
half = 10**(k-1)

def rev_k(v, k):
    r = 0
    for _ in range(k):
        r = r*10 + v % 10
        v //= 10
    return r

# valid endings: achievable as (2n^2+2n+1) mod 10^k AND last digit not 0/5
print("building valid-ending set (mod %d)..." % mod)
VE = set()
for n in range(mod):
    e = (2*n*n + 2*n + 1) % mod
    if e % 10 not in (0, 5):
        VE.add(e)
print("  |VE| =", len(VE))

def n_bounds(d):
    """Least n with p >= 10^(d-1), greatest n with p < 10^d -- exact.
    The isqrt only seeds; both edges are then walked to their true
    position by evaluating p.  The old closed form's unconditional +1
    returned nmin = 1 at d = 1, dropping n = 0 (p = 1) -- the same bug
    curve_gmp.h's compute_n_bounds retired.  2026-11-19."""
    f = lambda n: 2*n*n + 2*n + 1
    nmin = max(0, (isqrt(2*10**(d-1)) - 1)//2)
    while f(nmin) < 10**(d-1):
        nmin += 1
    while nmin > 0 and f(nmin - 1) >= 10**(d-1):
        nmin -= 1
    nmax = (isqrt(2*10**d) + 1)//2
    while f(nmax) >= 10**d:
        nmax -= 1
    while f(nmax + 1) < 10**d:
        nmax += 1
    return nmin, nmax

def ground_truth(d):
    nmin, nmax = n_bounds(d)
    div = 10**(d-k)
    # S_real: realized (first_k, last_k) pairs over actual n in range
    S = set()
    n = nmin
    while n <= nmax:
        p = 2*n*n + 2*n + 1
        S.add((p//div, p % mod))
        n += 1
    # survivor residues
    surv = set()
    n = nmin
    while n <= nmax:
        p = 2*n*n + 2*n + 1
        e = p % mod
        f = p // div
        r = n % mod
        if e % 10 not in (0, 5):
            if f >= half and rev_k(f, k) in VE:
                if (rev_k(e, k), rev_k(f, k)) in S:
                    surv.add(r)
        n += 1
    rng = nmax - nmin + 1
    return len(surv), rng, rng/mod

for d in (12, 13, 14):
    s, rng, ratio = ground_truth(d)
    print("  d=%2d  GROUND-TRUTH survivors = %d   (range=%d, "
          "range/mod=%.3f)" % (d, s, rng, ratio))
