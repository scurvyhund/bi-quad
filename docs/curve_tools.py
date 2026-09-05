"""curve_tools.py -- shared primitives for ad-hoc work on the curve.

The C tools have curve.h. Python explorations kept re-deriving the same
four helpers by hand -- on_curve, a primality test, digit reversal, and
the negative-Pell family -- four separate times in one session on
2026-09-04, which is three times too many.

    from curve_tools import curve, on_curve, is_prime, reverse, ...

Nothing here is fast. It is for exploring, not for sweeps; the sweeps
live in hunt.c, palsplit.c and palbrute.c.

Self-test:   python3 docs/curve_tools.py
"""

import random
from math import isqrt

__all__ = ["curve", "on_curve", "is_prime", "reverse", "square_root",
           "curve_squares", "ENDINGS"]

# curve values end ONLY in these digits -- see ending_constraint_proof.md
ENDINGS = (1, 3, 5)


def curve(n):
    """p = 2n^2 + 2n + 1 = n^2 + (n+1)^2"""
    return 2 * n * n + 2 * n + 1


def on_curve(p):
    """n with curve(n) == p, else None.  Exact: 2p-1 must be an odd square."""
    if p < 1:
        return None
    t = 2 * p - 1
    s = isqrt(t)
    if s * s != t:
        return None
    n = (s - 1) // 2
    return n if curve(n) == p else None


def is_prime(n, k=25):
    """Miller-Rabin.  Deterministic for the small primes, probabilistic above."""
    if n < 2:
        return False
    for p in (2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37):
        if n % p == 0:
            return n == p
    d, r = n - 1, 0
    while d % 2 == 0:
        d //= 2
        r += 1
    for _ in range(k):
        a = random.randrange(2, n - 1)
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


def reverse(n):
    """decimal digit reversal.  Involutive only when n has no trailing zero --
    curve values end in 1, 3 or 5, so it is safe on them."""
    return int(str(n)[::-1])


def square_root(n):
    """s with s*s == n, else None"""
    s = isqrt(n)
    return s if s * s == n else None


def curve_squares(count=20):
    """Values that are BOTH a perfect square AND on the curve.

    curve(m) = s^2  <=>  (2m+1)^2 - 2s^2 = -1, the NEGATIVE PELL equation.
    Solutions are the convergents of sqrt(2): (u,s) = (1,1), (7,5), (41,29)...
    Each term is ~34x the last, so this family is exponentially sparse --
    which is why conditions requiring membership in it almost never hold.

    Yields (m, value, s) with value == curve(m) == s*s.
    """
    u, s = 1, 1
    for _ in range(count):
        yield (u - 1) // 2, s * s, s
        u, s = 3 * u + 4 * s, 2 * u + 3 * s


if __name__ == "__main__":
    fails = 0

    def check(label, got, want):
        global fails
        if got != want:
            print(f"  FAIL {label}: got {got}, want {want}")
            fails += 1

    check("curve(30)", curve(30), 1861)
    check("on_curve(1861)", on_curve(1861), 30)
    check("on_curve(1862)", on_curve(1862), None)
    check("on_curve(1)", on_curve(1), 0)
    check("is_prime(1861)", is_prime(1861), True)
    check("is_prime(1681)", is_prime(1681), False)
    check("reverse(1861)", reverse(1861), 1681)
    check("square_root(1681)", square_root(1681), 41)
    check("square_root(1862)", square_root(1862), None)
    check("41 on curve", on_curve(41), 4)

    # the emirp: both members prime, both on the curve, mutual reversals
    check("12641 on curve", on_curve(12641), 79)
    check("14621 on curve", on_curve(14621), 85)
    check("rev(12641)", reverse(12641), 14621)

    # the negative-Pell family
    sq = list(curve_squares(5))
    check("curve_squares values", [v for _, v, _ in sq], [1, 25, 841, 28561, 970225])
    for m, v, s in sq:
        check(f"pell m={m}", (curve(m), s * s), (v, v))

    # 1861: reversal is the square of a curve prime -- unique below 10^14
    r = reverse(1861)
    check("1861 reversal square of prime", (square_root(r), is_prime(square_root(r))), (41, True))
    check("root 41 on curve", on_curve(41), 4)

    print("  all checks passed" if not fails else f"  {fails} FAILED")
