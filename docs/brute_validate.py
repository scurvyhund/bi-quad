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
    t = 2*10**(d-1) - 1
    sq = isqrt(t) - 1
    nmin = sq//2 + 1
    if 2*nmin*nmin + 2*nmin + 1 < 10**(d-1):
        nmin += 1
    t = 2*10**d - 1
    sq = isqrt(t) - 1
    nmax = sq//2
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
    print("  d=%2d  GROUND-TRUTH survivors = %d   (range=%d, range/mod=%.3f)" % (d, s, rng, ratio))
