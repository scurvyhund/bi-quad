# Glossary — bi-quad / BigFermat

Terms used across this project, collected 2026-08-30.
Until now the definitions lived scattered across ~20 documents; the fullest single source was the *Definitions* section of `mod11_converse_constraint.md`.

⚠ **Read `k` carefully — it means two different things.** See [Symbol collisions](#symbol-collisions).

---

## The object

**the curve** — `p = 2n² + 2n + 1 = n² + (n+1)²`.
The sum of two *consecutive* squares. OEIS **A027862**.
Geometrically: lattice points where the line `y = x+1` crosses the circle `x² + y² = p` (`curve_geometry.md`).
It is a **conic** (degree 2, genus 0), *not* an elliptic curve — no group law.

**n** — the curve index. The `n` in `n² + (n+1)²`. Every curve point has exactly one.

**membership test** — `p` is on the curve **iff `2p − 1` is an odd perfect square**.
Implemented as `curve_index()` in the Python tools.

**a, b** — `a = 2n + 1` and `b = 2m + 1`, so `a² = 2p − 1` and `b² = 2q − 1`.
The variables the mod-11 proof works in.

**d** — the number of decimal digits of `p`. The search proceeds d by d.

---

## The search targets

**rev(p)** — decimal digit reversal. `rev(12641) = 14621`. Not an algebraic operation; this is the source of the project's difficulty.

**converse pair** — `p` together with `q = rev(p)` where **both lie on the curve**.
Says nothing about primality. The scarce resource: only 2–16 exist below 10⁹ on any curve in the family.

**bi-quadratic emirp** — a converse pair that is **prime on both sides**.
The only one known on our curve: **12641 ↔ 14621** (d=5, n=79/85).
(On the nearby curve k=5 there is also **37 ↔ 73** — see `nearby_curves.md`.)

**emirp** (general usage) — a prime whose digit reversal is a *different* prime. The bi-quadratic version additionally demands both lie on the curve.

**prime palindrome (on the curve)** — the degenerate converse pair `m = n`, i.e. `p = rev(p)`.
**Jim's conjecture:** `3187813` (d=7) is the largest. Held since 1997, confirmed to d=27.

**the unification** — the emirp sieve *is* the palindrome sieve: a palindrome is a survivor with `m = n`. A modular obstruction kills both. See `unification` notes and `mod11_converse_constraint.md`.

---

## Sieve machinery

**survivor** — ⚠ means different things in the two engines:
- in **`hunt.c`** — an exact converse pair, valid at every d
- in **`mod_obstruct.c`** — a *residue class* matching first-k/last-k digits, which covers all d digits only when `d ≤ 2k`

This distinction is why the mod-11 assertion is unguarded in `hunt.c` but guarded by `d <= 2*k` in `mod_obstruct.c`.

**obstruction** — a digit-length `d` for which the sieve returns **zero** survivors, proving no emirp *and* no palindrome at that d.

**valid endings / VE** — the set of achievable `p mod 10^k` values whose last digit is not 0 or 5.

**the cliff / `range < mod`** — `mod_obstruct` is valid and tractable only while `range < mod` (at most one n-value per residue).
k=10 → valid to d≤20, cliff at d=21; k=6 → valid to d≤12, cliff at d=13.
Past the cliff the old interval method **over-counts** and is slow. Fixed in 22a7121 by enumerating actual n-values.

**div-5 filter / `VALID_NMOD`** — skips `n` values whose `p` is divisible by 5.
⚠ **Lossy**: it drops div-5 *composites*, so survivor and palindrome counts are undercounts. Emirp results are unaffected. See `skip_optimization.md`.

**mod-110 wheel** — fuses the div-5 skip with the mod-11 even-d constraint. `110 = lcm(10,11)`, and since `gcd(10,11) = 1`, CRT guarantees `j ↦ (j mod 10, j mod 11)` is a bijection on ℤ/110. ~1.53× at even d.

---

## Results and constraints

**reversal law (mod 11)** — since `10 ≡ −1 (mod 11)`:

    rev(p) ≡ (−1)^(d−1) · p   (mod 11)

Odd d → `q ≡ p`; even d → `q ≡ −p`.
**The only known bridge between base-10 and the algebra.**

**even-d constraint** — PROVEN. At even d, a converse pair forces `p mod 11 ∈ {3, 5, 6, 8}` — 7 of 11 classes excluded.

**palindrome corollary** — PROVEN, and stronger than classical.
Classical: even-digit palindromes are divisible by 11.
Ours: **no even-digit palindrome lies on the curve at all.**

**mod 9 (why it is too weak)** — `10 ≡ 1 (mod 9)`, so reversal preserves the digit sum and `q ≡ p (mod 9)` always. It cannot distinguish reversal from any other digit permutation.

**frontier formula** — `log₁₀(n_max) = (d − 0.30103) / 2`. See `density_heuristics.md`.

---

## Nearby curves

**the family** — `p = n² + (n+k)²` for **odd** k. Even k makes `p` always even. Our curve is `k = 1`.

**generalized membership** — `2p = a² + k²` with `a = 2n + k`. At k=1 this is `2p − 1 = a²`.

**the 11 | k exception** — for `11 ∤ k` no even-digit palindrome is on the curve; for `11 | k` they *are* (e.g. `4114 = 33² + 55²` at k=22) but are automatically composite. Full treatment in `nearby_curves.md`.

---

## Ruled out — do not revisit

**MITM** — splitting `n = a·10^t + b`. No √-speedup; the cross-term couples the middle digits.

**two-ended digit-DP** — midpoint survivors ≈ `0.023·range` ≈ `10^(d/2)`. Brute force in disguise.

**congruence obstruction** — 50 moduli, d ∈ [8,30]: **zero** obstructions. Real obstructions are non-congruential and sporadic.

All three in `structural_attacks_2026-06-04.md`. Do not re-propose without a genuinely new idea.

---

## Symbol collisions

⚠ **`k` is overloaded.** Both uses are entrenched; check the context.

| context | meaning |
|---|---|
| sieve (`mod_obstruct.c`) | digit count; modulus `10^k`. k=10 → mod 10¹⁰ |
| curve family | the **gap** in `n²+(n+k)²`. k=5 → `37 = 1²+6²` |

**`n` vs `m`** — `n` indexes `p`; `m` indexes `q = rev(p)`. A palindrome is `m = n`.

**`d` vs `k`(sieve)** — `d` is the digit length being searched; `k`(sieve) is how many digits the sieve pins at each end. Validity needs `d ≤ 2k`.

---

## Tools

| file | what it does |
|---|---|
| `hunt.c` | exhaustive GMP brute; every n. Ground truth. |
| `mod_obstruct.c` | fast modular sieve. Has had real bugs. |
| `palhunt.c` | prime palindromes. Markers: 5, 181, 313, 3187813 |
| `brute_validate.py` | independent Python ground truth |
| `converse_pairs.py` | converse pairs; `--check` tests the mod-11 law |
| `nearby_curves.py` | verifies `nearby_curves.md` (7 checks, <1s) |

(Python tools live in `docs/`.)

**Verification rule:** never declare "no emirp at d=X" from the fast engines alone. Always cross-check against `hunt.c` *and* `brute_validate.py`.
