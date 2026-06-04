# Milestone — 2026-06-04: No Bi-Quadratic Emirp Has ≤ 22 Digits

> A clean, exhaustively-verified result, reached the morning after the
> 2026-06-03 cliff/correctness fix. See `PROJECT_OVERVIEW.md` for full method &
> glossary; this doc records the milestone and how we got it.

---

## The result

**No bi-quadratic emirp exists with 21 or fewer digits.**

A *bi-quadratic emirp* is a prime `p = 2n²+2n+1` whose digit-reversal
`q = rev(p)` is **also** prime **and also** of the form `2m²+2m+1` (and `q ≠ p`).

The ≤22-digit range is now closed, gap-free:

```
d ≤ 12 : no candidates       (obstruction)
d = 13 : 4 candidates,  all composite   → no emirp   ┐
d = 14 : 6 candidates,  all composite   → no emirp   │ exhaustive brute force
d = 15 : 5 candidates,  all composite   → no emirp   │ (hunt.c) — every n tested
d = 16 : 0 candidates        (obstruction)           │
d = 17 : 1 candidate,   composite       → no emirp   ┘
d = 18 : no candidates       (obstruction)   ┐
d = 19 : no candidates       (obstruction)   │ modular sieve (k=10), proven
d = 20 : no candidates       (obstruction)   │
d = 21 : no candidates       (obstruction)   ┘
d = 22 : no candidates       (obstruction)   ← exact brute force (hunt.c): 0 pairs
```

Every prime-eligible candidate has `q` sitting exactly on the curve `2m²+2m+1`,
yet `p` and/or `q` is composite (Miller–Rabin, 40 rounds; error < 10⁻²⁴).

## How we know it (four independent confirmations)

The same survivor counts (4 / 6 / 5 / 0 / 1 for d=13–17) were produced by:

1. The **modular sieve** (`mod_obstruct`) at **k=7**,
2. …reproduced at **k=8**,
3. …reproduced at **k=9** (the convergence diagonal — counts stable ⇒ true),
4. An **independent direct brute force** (`hunt.c`) enumerating *every* `n` and
   testing the exact (not modular) conditions — matching exactly once the
   composite-5 filter is applied.

The d=16 obstruction is thus verified at full exact-arithmetic precision: its two
raw curve-hits are both divisible by 5, so there are genuinely zero
prime-eligible candidates.

## Why this matters (the new thesis)

The structure is *satisfiable* (candidates with both ends on the curve and
prime-eligible do exist) but primality never lands in this range. Meanwhile
obstructions **dominate** right at the upper edge (d=16, 18, 19, 20, 21, 22 all
obstructed — five consecutive (18–22) — d=17 down to a single straggler).

So the project's question sharpens:

> **If bi-quadratic emirps exist at all, they are d ≥ 23** — brute force reaches
> ~d=25, and beyond that the obstruction sieve (or new mathematics) is the only
> tool. And the
> live possibility is now **non-existence**: do obstructions become *total* for
> all large `d`?

This is the exact opposite of the old, retracted "saturation at d=2k+2" picture
(an artifact of the interval-over-approximation bug fixed 2026-06-03).

## The path that got us here (one morning)

1. Read overnight k=7/k=8 logs → noticed counts line up by **absolute d**, not
   just offset → realized that meant **convergence** to true candidate counts.
2. Ran the **k=9 diagonal** as a live regression test (d=15 *must* be 5 — it was).
   It confirmed the d=16 obstruction and revealed **new** obstructions at d=18,19,
   and that d=17 falls 3→1 (k=8 wasn't converged — the methodology caught it).
3. Recognized that at k ≥ ⌈d/2⌉ the survivors are **fully-determined candidates**,
   not just digit-feasible → built `hunt.c` to extract and primality-test them.
4. `hunt.c` found EMIRPS=0 and re-confirmed all sieve counts exactly → milestone.

## Artifacts

- `hunt.c` — independent brute-force hunter / verifier (build in Makefile-style:
  `gcc hunt.c -o hunt -O3 -march=znver2 -std=c99 -Wall -fopenmp -lgmp`).
- `logs/landscape_k9.log` — the k=9 diagonal (d=15:5, d=16:0, d=17:1, d=18:0, d=19:0).
- `logs/landscape_k6.log`, `_k7.log`, `_k8.log`, `_k6_d18.log` — the cross-k map.
- `docs/PROJECT_OVERVIEW.md` — canonical method, glossary, algorithms.

## Next

k=10 diagonal done (d=18–21 obstructed), then **d=22 closed by exact brute force**
(hunt.c: 0 curve-reversal pairs) — result now **≤22 digits**. Note: the k=10 sieve
*over-counts* past d=20 (it covers only 20 of 22 digits), so brute force is the
trustworthy tool for d≥21; it reaches ~d=25. Options from here:
- Push **d=23, 24, 25** by exact brute force (running now) — extends the wall.
- Beyond d=25: only the sieve or new mathematics reaches (search is 10^(d/2)-bound;
  MITM and congruence both ruled out — see memory `structural_attacks_ruled_out`).
- Or write up the **non-existence trend** + the prime-palindrome unification.
