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

Authoritative landscape (`hunt.c`, every n). A curve-reversal survivor is either an
**emirp candidate** (non-palindrome) or a **palindrome** (`p=rev(p)`); raw = both:

```
d :  5   6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24
cand: 6*  0  2  2  6  0  4  2  2  6  2  2  0  0  0  0  2  0  2  2   emirp candidates
pal:  0   0  5  0  0  0  1  0  2  0  4  0  1  0  3  0  5  0  1  0   palindromes
raw:  6   0  7  2  6  0  5  2  4  6  6  2  1  0  3  0  7  0  3  2   total
                                                       (* = the EMIRP, d=5)
```
True obstructions (zero survivors of any kind): **d = 6, 10, 18, 20, 22**. Every
survivor through d=24 is composite EXCEPT two: the d=5 emirp and the d=7 palindrome
`3187813`.

## Tool notes (verify with the brute-force pair!)

1. **`check_survivors.c` is CORRECT — earlier "undercounts" call was my error.** It
   counts NON-PALINDROME emirp candidates (it skips palindromes); `hunt.c`'s "raw"
   count also includes palindromes, which is why they differed (e.g. d=13: 2 vs 4 =
   2 palindromes). Both right. Only real limit: 64-bit overflow at d≥19 → use `hunt.c`.
2. **The modular sieve over-claimed d=21 as an obstruction** (the `range<mod`
   cliff artifact). `hunt.c` shows d=21 has 7 raw / 6 prime-eligible candidates
   (all composite, no emirp). The earlier "d=18–21 sieve obstruction" was only
   cleanly true at d=18; d=19,20,21 needed brute force (d=19,20 raw-survivor /
   saturated, d=21 cliff).

## cvpipe — one bogus log, but the real runs ARE real (corrected 2026-06-05)

Initial over-reaction: I found one cvpipe log
(`gmp-cvo/logs/cvpipe_nitroIII_10e25-10e26_zones1-6.log`) — d ≤ 12 only, 0.02 s, with
an illusory "100% eliminated / 6.28 BILLION× zone skip" — and wrongly concluded
cvpipe never ran past d≤12. **That was an over-generalization.** There is a full set
of real per-decade run logs (`gmp-cvo/logs/run_10e22 … run_10e28`, `10e25_to_10e30.txt`)
and a verified d=24 run (`bi-quad/logs/run-10e23-to-24.txt`): 32.06 B candidates →
2.75 B primes → 211 M emirps → **CON-VERSE = 0, otto = 0**. That run is internally
consistent (candidates = n-range ÷ 15.08 = the proven 6-of-90 digit filter) and
**cross-checks `hunt.c` (EMIRPS=0) and `palhunt_gmp` (found=0)** at d=24.

So: the *one* zone-skip log was bogus (that critique stands); cvpipe's actual
pipeline runs are sound. **Status:** d ≤ 25 is independently corroborated by
`hunt.c` + `palhunt_gmp`; d ≥ 26 currently rests on cvpipe's own logs (not yet
brute-verified, since our brute tools only reach ~d=25–26).

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
