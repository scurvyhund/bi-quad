# Two Open Problems on the Curve p = 2n² + 2n + 1

**A computational investigation, framed as open questions.**
BigFermat project · last revised 2026-06-27

---

## The curve

Every integer n gives a value on the *centered-square* curve

    p(n) = n² + (n+1)² = 2n² + 2n + 1.

These are the numbers that are a sum of **two consecutive squares**.
The primes among them are OEIS **A027862**: 5, 13, 41, 61, 113, 181,
313, 421, 613, 761, … Every value p(n) is ≡ 1 (mod 4), so by Fermat's
two-square theorem each *prime* p(n) is a sum of two squares in exactly
one way — and here those two squares are forced to be consecutive.

This note states two questions about this curve, summarizes the
computational evidence gathered for each, and gives a density heuristic
that — honestly — predicts opposite answers for the two. Nothing here
is a theorem. The point is to state the questions cleanly and show why
they are interesting, in the hope that someone with the right machinery
finds them worth an attack.

---

## Problem 1 — The bi-quadratic emirp

Call a prime p **bi-quadratic emirp** if:

1. p lies on the curve: p = 2n² + 2n + 1;
2. its decimal reversal q = rev(p) also lies on the curve:
   q = 2m² + 2m + 1 for some integer m;
3. q is prime; and
4. q ≠ p (so p is not a palindrome).

Exactly **one** is known:

    12641  =  79² + 80²        (n = 79)
    14621  =  85² + 86²        (m = 85)     12641 ⟷ 14621

both prime, both on the curve, mutual digit-reversals. It has 5 digits.

> **Open question.** Is 12641 ⟷ 14621 the *only* bi-quadratic emirp?

**Evidence.** An exhaustive search (enumerate every n, form p, reverse,
test all four conditions) finds no other example through 27 decimal
digits, with an independent modular-obstruction sieve confirming the
same "no candidate of any kind" obstruction. The result has been
cross-validated four ways (two independent brute forces in different
languages, the sieve, and a separate pipeline). See *Reproducibility*
below.

---

## Problem 2 — The prime-palindrome conjecture

A prime p(n) is a **palindromic prime on the curve** if its decimal
digits read the same forwards and backwards. Four are known, all small:

    5  (n=1),   181 (n=9),   313 (n=12),   3187813 (n=1262).

The largest, **3187813**, was conjectured (J., 1997) to be the *last* —
the largest prime palindrome anywhere on the curve.

> ⚠ **ANSWERED — NO (2026-09-04).** A fifth term exists at 59 digits,
> `n = 91732095351342012927350087594` (De Geest, 2026-04-24). Problem 2
> below is therefore closed; **Problem 1, the emirp, remains open** and
> is the live question. See [`session_2026-09-04_palindrome_conjecture_refuted.md`](session_2026-09-04_palindrome_conjecture_refuted.md).

**Evidence.** No further palindromic prime on the curve has been found
for 8 through 27 digits — direct search to 19 digits, extended to 27 by
the same modular sieve used for Problem 1. (The sieve unifies the two
problems: a palindrome is the degenerate emirp case m = n, so a
digit-length with *no* curve-survivors rules out both an emirp and a
prime palindrome at once.)

---

## Why the two problems likely have opposite answers

A heuristic, clearly labeled as such. For a d-digit window there are

    N_d ≈ 10^(d/2) / √2

candidate values of n. Model "p is prime" as an independent event of
probability ≈ 1/ln p ≈ 1/(d·ln 10), and "a d-digit number lands on the
curve" as probability ≈ 10^(−d/2). Then:

**Prime palindrome at digit-length d.** Needs (i) p is a palindrome
[≈ 10^(−d/2)] and (ii) p is prime [≈ 1/(d·ln10)]; reversal is free
since rev(p) = p. So

    E[palindromes at d]  ≈  N_d · 10^(−d/2) · 1/(d·ln10)  =  C′/d.

**Bi-quadratic emirp at digit-length d.** Needs (i) rev(p) on the curve
[≈ 10^(−d/2)], (ii) p prime and (iii) rev(p) prime — *two* independent
primality events. So

    E[emirps at d]  ≈  N_d · 10^(−d/2) · [1/(d·ln10)]²  =  C/d².

| object              | expected count at d | sum over all d        |
|---------------------|---------------------|-----------------------|
| prime palindrome    | C′/d                | **diverges** (∞)      |
| bi-quadratic emirp  | C/d²                | **converges** (≈ 1)   |

The single extra factor 1/(d·ln10) — the cost of a *second* prime —
turns a divergent harmonic sum into a convergent p-series. So the same
back-of-envelope model predicts **infinitely many** prime palindromes
on the curve yet only a **finite** number of emirps, with expected
total ≈ 1.

This is the crux, and it cuts against intuition: the palindrome
conjecture (Problem 2, "3187813 is the last") is the one the heuristic
calls *false in spirit* — it expects more, none yet found — while the
emirp uniqueness (Problem 1) is exactly what the heuristic predicts.
Neither is settled. A convergent expectation does not forbid a second
emirp; a divergent one does not produce a fifth palindrome on demand.

**The model has since been tested on 13 curves** — see
[`density_cross_curve.md`](density_cross_curve.md). Three results bear
on the above. The *raw* palindrome density is confirmed across all 13
curves to 4%, with each curve's own last-digit signature predicting its
own constant. The even-d prediction fails on exactly the eleven curves
Theorem B forbids — 77 predicted, 0 observed — which is the strongest
confirmation of that theorem so far. And the 1/d shape for primes
survives a pooled test (χ² = 3.95 on 5 df), though the per-curve
constants cannot be tested: there are at most 30 primes across 13
curves, so every expected cell count is below 5.

For this curve C′ ≈ 1.9–3.3, a factor-of-two spread. That makes the
d = 29..37 sweep an expected yield of 0.28–0.50 (so the null result
there is weak evidence), and the reachable d ≈ 51 ceiling a 25–40%
chance of an actual hit. The silence after 3187813 is statistically
ordinary — 3 observed against 4.5 predicted on this curve, and twelve
siblings range from none at all (k=7, k=13) to d = 31 (k=19).

### Putting a number on Problem 1 (added 2026-09-05)

The `C/d²` shape above gives the trend but no magnitude. Calibrating
the constant from *observed* survivor counts rather than back-solving
it from the d = 5 hit — the `hunt` logs give a mean of **3.47**
curve-reversal pairs per digit-length over d = 13..27, ranging 0 to 7 —
and taking `P(p and rev(p) both prime) ≈ 1/(d·ln10)²`:

| range | expected number of emirps |
|---|---|
| d = 28 … 40 | 0.0076  (**0.76 %**) |
| d = 28 … 100 | 0.0173  (1.73 %) |
| **d = 28 … ∞** | **0.0238  (2.4 %)** |
| d = 1 … ∞ (model total) | 1.076 — against 1 observed |

> **A second, independent calibration exists** and is roughly a factor
> of two lower: `gmp-cvo/docs/biquadratic_emirps_analysis.md` §5 sums
> P(n) over each decade and gets ~0.012 for everything past 10^27,
> against the 0.0238 above. The difference is calibration, not model —
> that one back-solves its constant from the d = 5 hit, this one takes
> it from *observed* survivor counts over d = 13…27, which is the
> better-grounded of the two. Quote **2.4 %**, and note the spread.
> Neither figure changes the conclusion: the expected yield past the
> frontier is small enough that further search buys bound strength,
> not a realistic chance of a second example.

So continuing the emirp search **to infinity** carries a ~2.4 % chance
of a second example, and the model's total expectation over all
digit-lengths, 1.08, sits almost exactly on the one pair we have.
The search is not stalled for want of a better algorithm; it is at the
point where the heuristic says there is most likely nothing left to
find.

Two caveats, and the second is the serious one:

1. This is **TREND, not PROVEN**. A convergent expectation forbids
   nothing — it makes no statement about any particular d.
2. **The same model's other prediction was right, and the silence
   around it was misread for 29 years.** It calls the palindrome side
   divergent, and a fifth prime palindrome duly turned up at d = 59
   (De Geest, 2026-04-24) after decades of absence that had been taken
   for evidence of a last term. Whatever weight that earns the model,
   it earns equally as a warning against over-trusting the emirp side's
   convergence.

This bears only on searching **past** d = 27. It says nothing about the
coverage below that, which is exhaustive and independent of any
heuristic.

---

## What has been ruled out

Four attempts to do better than O(10^(d/2)) brute force, or to prove
non-existence, were tested and abandoned (2026):

- **Meet-in-the-middle** (split n = a·10^t + b): the cross term 4ab
  couples the middle ~d/2 digits — no √-speedup.
- **Two-ended digit DP**: midpoint survivor count ~ 10^(d/2); brute
  force in disguise.
- **Congruence obstruction**: 50 moduli, d ∈ [8, 30] — *zero* fixed-
  modulus obstructions. The real obstructions are non-congruential.
- **Reversal-axis change of variables** (2026-09-05): pairing digits
  as `s_i = a_i + a_{d-1-i}` and `t_i = a_i − a_{d-1-i}` does decouple
  the two ends — `p+q` depends only on `s`, `p−q` only on `t` — but the
  resulting system reconstructs `2p−1 = x²` exactly. An invertible
  change of variables, not a constraint. See
  [`structural_attacks_2026-06-04.md`](structural_attacks_2026-06-04.md) §3b.

The takeaway: digit-reversal of a quadratic is *anti-structural* —
reversal does not commute with arithmetic — which defeats both
algebraic and modular machinery and is presumably why such problems are
hard. A genuine attack likely needs analytic/additive number theory or
a reformulation that sidesteps the reversal, not more computation.

---

## Reproducibility

All code and logs are in the bi-quad repository. The exhaustive
checkers (`hunt.c`, `brute_validate.py`) enumerate every n directly;
the modular sieve (`mod_obstruct.c`) proves digit-length obstructions;
`palhunt.c` searches palindromic primes directly. The four-way
cross-validation of Problem 1 and the sieve/brute agreement underlying
Problem 2 are documented in `docs/`.

*Corrections and attacks welcome. The questions are the contribution;
the answers are open.*
