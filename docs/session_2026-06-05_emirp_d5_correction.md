# Session — 2026-06-05: The d=5 emirp, and a headline correction

> Jim noticed the README never differentiated *palindrome* from *emirp*, and
> offered `12641 ⟷ 14621` as an emirp example. Verifying it overturned the
> project's headline result. This doc records the correction and the
> brute-force re-verification. See `PROJECT_OVERVIEW.md` for method & glossary.

---

## The finding: a bi-quadratic emirp EXISTS

**`12641 ⟷ 14621` is a genuine bi-quadratic emirp at d=5.**

- `12641 = 79² + 80²`, prime, ≡ 1 (mod 4)
- `14621 = 85² + 86²`, prime, ≡ 1 (mod 4), and `= rev(12641)`
- not a palindrome, `q ≠ p` → satisfies the definition exactly

This was **never hidden** — the project's own tools always reported it
(`check_d5` prints `BOTH PRIME — CONVERSE PAIR`; the sieve logs record
`d=5 survivors=6`; `hunt.c` prints `EMIRPS=2 *** EMIRP *** n=79, n=85`). The
old headline — *"no bi-quadratic emirp ≤ 22 digits / existence unknown"* — was a
**prose misread**: the small-d range was asserted "OBSTRUCTION (no candidates)"
without reading the sieve's own d=5 output, and the d=5 survivors were never
primality-tested in the writeup.

## Corrected result (brute-force verified through d=24)

**`12641 ⟷ 14621` is the ONLY bi-quadratic emirp through 24 digits.**
`EMIRPS = 0` for every d = 6..24, confirmed by exhaustive `hunt.c` (every n
tested, GMP-exact). The next emirp, if any, has ≥ 25 digits (d=25 search running).

Authoritative landscape (`hunt.c`, raw = n with p & rev(p) both on curve;
elig = prime-eligible after small-prime filter):

```
d :  5   6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24
raw:  6   0  7  2  6  0  5  2  4  6  6  2  1  0  3  0  7  0  3  2
elig: 6*  0  5  2  0  0  0  0  4  6  5  0  1  0  0  0  6  0  2  0
                                                       (* = the EMIRP, d=5)
```
True obstructions (zero curve-reversal pairs at all): **d = 6, 10, 18, 20, 22**.
All other survivors are composite — except the d=5 pair.

## Two tool issues uncovered (verify with the brute-force pair!)

1. **`check_survivors.c` UNDERCOUNTS — unreliable.** It reported d=13 → 2
   structural matches; `hunt.c` (canonical) finds 4. It disagrees at d=7,11,13,15,16
   too. Do NOT trust its counts. Use `hunt.c` (and `docs/brute_validate.py`) as
   ground truth. A warning was added to its header.
2. **The modular sieve over-claimed d=21 as an obstruction** (the `range<mod`
   cliff artifact). `hunt.c` shows d=21 has 7 raw / 6 prime-eligible candidates
   (all composite, no emirp). The earlier "d=18–21 sieve obstruction" was only
   cleanly true at d=18; d=19,20,21 needed brute force (d=19,20 raw-survivor /
   saturated, d=21 cliff).

## The cvpipe "10^28" claim is NOT backed by logs

The only real `cvpipe` run log (`gmp-cvo/logs/cvpipe_nitroIII_10e25-10e26_zones1-6.log`)
covers just **d ≤ 12** (n_max=291547), finished in 0.02 s, and its "100% eliminated
/ 6.28 BILLION× zone skip" is the illusory speedup Jim flagged. `converse.dat` is
empty. "10^23/24/27/28" appear only as time-estimates in planning docs. Our
exhaustive `hunt.c` (d ≤ 24) is the real frontier and supersedes cvpipe's true
reach by 12 digits.

## Artifacts updated
- `README.md` — headline, objects, heuristic, "Why BigFermat" (odd prime / *if
  and only if*; palindrome 3187813 vs emirp 12641⟷14621). Pushed (13b49ca).
- `generate_graph.py` + `docs/biquad_curve_landscape.png` — regenerated with d=5
  emirp marked and authoritative raw `hunt.c` counts; obstructions d=6,10,18,20,22.
- Recovered 9 unique reference docs from the old Codeberg line (commit 99e5506).
- `PROJECT_OVERVIEW.md` corrected; `session_2026-06-04_milestone.md` and
  `_archive.md` and `structural_attacks_2026-06-04.md` annotated as superseded.

## Open
- d=25 brute running (~ETA mid-day 2026-06-05) → extends "through 24" to "through 25".
- Density heuristic (Σ C/d² → expected total ≈ 1) is consistent with 12641⟷14621
  being the *only* emirp that exists. Whether a second exists at d ≥ 25 is open.
