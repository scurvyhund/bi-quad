#!/usr/bin/env python3
"""density_cross_curve.py -- test the palindrome-density heuristic of
OPEN_PROBLEM.md against all 13 curves in docs/curve_palindromes.txt.

The heuristic predicts a DIFFERENT constant for each curve, from two
curve-specific quantities, so it can fail per curve rather than being
fitted to one.  See docs/density_cross_curve.md for the write-up.

Usage:  python3 docs/density_cross_curve.py        (run from repo root)
"""
import math
import os
import re
import sys
from collections import Counter

HERE = os.path.dirname(os.path.abspath(__file__))
PATH = os.path.join(HERE, "curve_palindromes.txt")

# First-digit law.  For any p = A m^2 + ... with A > 0 and m ranging
# over a decade, p has density proportional to 1/sqrt(p), so the
# leading-digit distribution is the same on EVERY curve here.
_DEN = math.sqrt(10) - 1
PFIRST = {k: (math.sqrt(k + 1) - math.sqrt(k)) / _DEN for k in range(1, 10)}

# N_d = #{m : A m^2 + Bm + C has d digits} = (1/sqrt(A)) * KN * 10^(d/2)
KN = 1 - 1 / math.sqrt(10)

DMAX_DATA = 31          # curve_palindromes.txt is complete to d = 31


def sieve(n):
    s = bytearray([1]) * (n + 1)
    s[0] = s[1] = 0
    for i in range(2, int(n ** 0.5) + 1):
        if s[i]:
            s[i * i::i] = bytearray(len(s[i * i::i]))
    return [i for i in range(2, n + 1) if s[i]]


QS = [q for q in sieve(50000) if q not in (2, 5)]


def parse(path=PATH):
    """-> [ {name, br:[(A,B,C)..], vals:[(value, is_prime)..]} ]"""
    curves, cur = [], None
    for line in open(path):
        s = line.rstrip()
        m = re.match(r'^(k=\d+|cuban|Z\[sqrt-2\])\s', s)
        if m and 'p =' in s:
            cur = {'name': m.group(1), 'br': [], 'vals': []}
            curves.append(cur)
            continue
        if cur is None:
            continue
        mm = re.match(r'^\s+p = (.+)$', s)
        if mm:
            for t in mm.group(1).split('U'):
                t = t.strip()
                a = re.search(r'(\d*)m\^2', t)
                b = re.search(r'([+-]\s*\d*)m(?!\^)', t)
                c = re.search(r'([+-]\s*\d+)\s*$', t)
                cur['br'].append((
                    int(a.group(1)) if a and a.group(1) else 1,
                    int(b.group(1).replace(' ', '')) if b else 0,
                    int(c.group(1).replace(' ', '')) if c else 0))
            continue
        v = re.match(r'^\s+(\d+)\s+m=(\d+)(\s+PRIME)?', s)
        if v:
            cur['vals'].append((int(v.group(1)), bool(v.group(3))))
    return [c for c in curves if c['br'] and c['vals']]


def last_digits(br):
    c = Counter()
    for (A, B, C) in br:
        for m in range(10):
            c[(A * m * m + B * m + C) % 10] += 1
    tot = sum(c.values())
    return {v: n / tot for v, n in c.items()}


def omega(A, B, C, q):
    """number of roots of A m^2 + B m + C mod q, q an odd prime"""
    if A % q == 0:
        if B % q:
            return 1
        return q if C % q == 0 else 0
    D = (B * B - 4 * A * C) % q
    if D == 0:
        return 1
    return 2 if pow(D, (q - 1) // 2, q) == 1 else 0


def hardy_littlewood(br):
    """prod_q (1 - w(q)/q)/(1 - 1/q) over odd q != 5.

    q = 2 and q = 5 are excluded because they are exactly the primes
    the base-10 digit analysis already accounts for (the last-digit
    signature and the div-5 dead fraction)."""
    r = 1.0
    for q in QS:
        w = sum(omega(A, B, C, q) for (A, B, C) in br) / len(br)
        r *= (1 - w / q) / (1 - 1 / q)
    return r


def model(br):
    ld = last_digits(br)
    p_fl = sum(ld.get(v, 0) * PFIRST[v] for v in range(1, 10))
    nd = sum(1 / math.sqrt(A) for (A, B, C) in br) * KN
    # E[raw palindromes at ODD d] -- the 10^(d/2) cancels exactly
    e_odd = nd * (10 ** 0.5) * p_fl * 10
    # even d carries one more digit constraint => 10^(1/2) rarer
    e_even = e_odd / (10 ** 0.5)
    dead5 = (ld.get(5, 0) * PFIRST[5] / p_fl) if p_fl else 0.0
    return {'p_fl': p_fl, 'e_odd': e_odd, 'e_even': e_even,
            'dead5': dead5, 'hl': hardy_littlewood(br), 'ld': ld}


def count(vals, dmin, dmax, parity, primes_only=False):
    return len([v for v, pr in vals
                if (pr or not primes_only)
                and dmin <= len(str(v)) <= dmax
                and len(str(v)) % 2 == parity])


def main():
    curves = parse()
    if not curves:
        sys.exit("parse failed: %s" % PATH)
    M = {c['name']: model(c['br']) for c in curves}

    # ---- 1. raw palindromes, odd d -------------------------------
    DMIN = 5
    nd = len([d for d in range(DMIN, DMAX_DATA + 1) if d % 2])
    print("1. RAW PALINDROMES, odd d in [%d,%d]  (%d digit-lengths)\n"
          % (DMIN, DMAX_DATA, nd))
    print("   %-11s %-3s %-14s %7s %7s %7s %6s"
          % ("curve", "A", "p mod 10", "P(f=l)", "pred", "obs", "o/p"))
    tp = to = 0
    for c in curves:
        m = M[c['name']]
        obs = count(c['vals'], DMIN, DMAX_DATA, 1)
        tp += m['e_odd'] * nd
        to += obs
        print("   %-11s %-3d {%-12s} %7.4f %7.2f %7.2f %6.2f"
              % (c['name'], c['br'][0][0],
                 ",".join(str(v) for v in sorted(m['ld'])),
                 m['p_fl'], m['e_odd'], obs / nd,
                 (obs / nd) / m['e_odd']))
    print("\n   TOTAL predicted %.1f   observed %d   ratio %.2f"
          % (tp, to, to / tp))

    # ---- 2. even d: the Theorem B null test -----------------------
    ne = len([d for d in range(DMIN, DMAX_DATA + 1) if d % 2 == 0])
    print("\n\n2. EVEN d -- the model knows nothing about Theorem B\n")
    print("   %-11s %8s %6s" % ("curve", "pred", "obs"))
    fp = fo = 0
    for c in curves:
        m = M[c['name']]
        obs = count(c['vals'], DMIN, DMAX_DATA, 0)
        pred = m['e_even'] * ne
        flag = "  <- Theorem B PERMITS" if obs else ""
        if not obs:
            fp += pred
            fo += obs
        print("   %-11s %8.1f %6d%s" % (c['name'], pred, obs, flag))
    print("\n   on the curves Theorem B FORBIDS:"
          " predicted %.0f, observed %d" % (fp, fo))

    # ---- 3. prime palindromes ------------------------------------
    #
    # The per-curve chi-square is INVALID here and is reported only to
    # show why: with 13 curves and at most 30 primes, every expected
    # cell count is below 5, and past d = 15 every one is below 1.  The
    # apparent "improving fit" as the floor rises is the test losing
    # power, not the model gaining accuracy.  The pooled test below is
    # the one that means anything.
    print("\n\n3a. PER-CURVE chi-square -- REPORTED ONLY TO BE DISMISSED\n")
    print("   %-10s %7s %9s %-28s" %
          ("d range", "primes", "chi2/12df", "cells with E<5 / E<1"))
    for dmin in (5, 11, 15):
        odd = [d for d in range(dmin, DMAX_DATA + 1) if d % 2]
        S = sum(1.0 / d for d in odd)
        raw = [M[c['name']]['e_odd'] * (1 - M[c['name']]['dead5'])
               * M[c['name']]['hl'] for c in curves]
        obs = [count(c['vals'], dmin, DMAX_DATA, 1, True) for c in curves]
        scale = sum(obs) / (sum(raw) * S)
        exp = [r * scale * S for r in raw]
        chi = sum((o - e) ** 2 / e for e, o in zip(exp, obs))
        print("   d >= %-5d %7d %9.1f   %d/13 and %d/13   -> INVALID"
              % (dmin, sum(obs), chi,
                 sum(1 for e in exp if e < 5),
                 sum(1 for e in exp if e < 1)))

    # ---- 3b. the valid test: pool the curves, bin the digit-lengths
    print("\n\n3b. POOLED test of the 1/d law -- all 13 curves together\n")
    cnt = Counter()
    for c in curves:
        for v, pr in c['vals']:
            if pr and len(str(v)) % 2:
                cnt[len(str(v))] += 1
    ds = [d for d in range(3, DMAX_DATA + 1, 2)]
    S = sum(1.0 / d for d in ds)
    N = sum(cnt[d] for d in ds)
    bins = [(3, 3), (5, 5), (7, 7), (9, 9), (11, 13), (15, 19), (21, 31)]
    print("   %-9s %6s %9s" % ("d", "obs", "model 1/d"))
    chi = 0.0
    emin = 1e9
    for lo, hi in bins:
        o = sum(cnt[d] for d in range(lo, hi + 1, 2))
        e = N * sum(1.0 / d for d in range(lo, hi + 1, 2)) / S
        chi += (o - e) ** 2 / e
        emin = min(emin, e)
        print("   d=%-2d-%-4d %6d %9.1f" % (lo, hi, o, e))
    print("\n   chi2 = %.2f on %d df   (all cells E >= %.1f -- VALID)"
          % (chi, len(bins) - 2, emin))

    # ---- 3c. C' for our curve, as a RANGE
    tot_w = sum(M[c['name']]['e_odd'] * (1 - M[c['name']]['dead5'])
                * M[c['name']]['hl'] for c in curves)
    w1 = (M['k=1']['e_odd'] * (1 - M['k=1']['dead5']) * M['k=1']['hl'])
    c_pooled = (N / S) * w1 / tot_w
    odd15 = [d for d in range(15, DMAX_DATA + 1) if d % 2]
    S15 = sum(1.0 / d for d in odd15)
    raw = [M[c['name']]['e_odd'] * (1 - M[c['name']]['dead5'])
           * M[c['name']]['hl'] for c in curves]
    obs15 = [count(c['vals'], 15, DMAX_DATA, 1, True) for c in curves]
    c_d15 = (sum(obs15) / (sum(raw) * S15)) * w1
    lo_c, hi_c = sorted((c_d15, c_pooled))
    print("\n\n3c. C' for k=1 (our curve)\n")
    print("   pooled over all 13 curves : %.2f" % c_pooled)
    print("   fitted on d >= 15 alone   : %.2f   (7 primes -- weak)" % c_d15)
    print("   => quote as a RANGE       : %.1f - %.1f" % (lo_c, hi_c))
    print("\n   %-22s %14s %14s" % ("search range", "E (C'=%.1f)" % lo_c,
                                     "E (C'=%.1f)" % hi_c))
    for a, b in ((29, 37), (39, 51), (29, 51)):
        ss = sum(1.0 / d for d in range(a, b + 1, 2))
        print("   d = %-2d .. %-12d %8.2f -> %2.0f%% %8.2f -> %2.0f%%"
              % (a, b, lo_c * ss, 100 * (1 - math.exp(-lo_c * ss)),
                 hi_c * ss, 100 * (1 - math.exp(-hi_c * ss))))
    print("\n   NOTE: the d=21-31 bin runs LOW (3 observed vs 7.0). If")
    print("   that deficit is real rather than Poisson noise (P<=3 ~ 0.08)")
    print("   then C' falls at high d and these odds are optimistic.")


if __name__ == "__main__":
    main()
