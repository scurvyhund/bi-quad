# Glossary — bi-quad / BigFermat

Terms used across this project, collected 2026-08-30.
Until now the definitions lived scattered across ~20 documents; the fullest single source was the *Definitions* section of `mod11_converse_constraint.md`.

⚠ **Read `k` carefully — it means two different things.** See [Symbol collisions](#symbol-collisions).

---

## Symbols — the formula variables

Every variable that appears in a formula anywhere in this project.
Where two engines use a symbol differently, both rows are given.

| sym | meaning | example |
|---|---|---|
| `n` | curve index of `p`; `p = n²+(n+1)²` | 12641 → n=79 |
| `m` | curve index of `q = rev(p)` | 14621 → m=85 |
| `p` | the curve value under test | 12641 |
| `q` | `rev(p)`, the digit reversal | 14621 |
| `a` | `2n+1`, so `a² = 2p−1` | n=79 → a=159 |
| `b` | `2m+1`, so `b² = 2q−1` | m=85 → b=171 |
| `d` | decimal digits of `p` | 12641 → d=5 |
| `k` **sieve** | digits pinned at each end | k=10 → mod 10¹⁰ |
| `k` **family** | the gap in `n²+(n+k)²` | k=5 → `37 = 1²+6²` |
| `mod` | sieve modulus, `10^k` | k=6 → 1000000 |
| `range` | n-values giving a d-digit `p` | see *the cliff* |
| `r` | residue loop var, `0 ≤ r < mod` | `r % 100000000` |
| `e` | an *ending*, `p mod 10^k` | last k digits |
| `t` | split point in abandoned MITM | `n = a·10^t + b` |

### Words used as if defined, but never were

**residue** — a value mod something. In `mod_obstruct.c` it means one candidate `r` in `[0, mod)`, standing for *all* n with `n ≡ r (mod 10^k)`. "Checking 10¹⁰ residues" = testing every such class once.

**residue class** — the whole set `{ r, r+mod, r+2·mod, … }`. The sieve's leverage: one test decides a whole class. This is also why `mod_obstruct` survivors are classes, not points — see *survivor*.

**allowed / excluded residues** — in the mod-11 work, which of `0..10` a curve value may occupy. Even d allows only `p mod 11 ∈ {3,5,6,8}`; the other 7 are excluded.

**saturated** — a d at which *every* residue survives, so the sieve has proved nothing. The opposite of an obstruction. Marked `sat` in the tables; `OBS` marks an obstruction.

**range < mod** — the validity condition: at most one n-value per residue. Once `range ≥ mod` a residue holds 2+ n-values and the old interval method over-counted. See *the cliff*.

**ending** — the last `k` digits of `p`, i.e. `p mod 10^k`. *Valid* endings are the achievable ones not ending in 0 or 5.

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

**valid_firsts** — the achievable first-`k`-digit patterns, = `reverse_k(valid endings)`. Used to test `p`'s leading digits.

**composite-5 filter** — dropping endings whose last digit is 0 or 5. Half of all achievable endings end in 5; without this they masquerade as survivors and hide real obstructions.

**Hensel lifting** (`solve_residues`) — given a target ending mod `10^k`, find which residues solve `2m²+2m+1 ≡ target`, by lifting solutions mod 10 → mod 100 → … → mod `10^k`. This is how the `q` side is checked.

**range/mod** — average number of `n`-values per residue class, `≈ 2.16 × 10^((d−1)/2 − k)`. The single most important quantity: it decides which regime the search is in.

**converged count** — for fixed `d`, the survivor count stabilizes once `k` is large enough that `d` sits at or below `k`'s cliff (`k ≳ d/2`). That stable value is the true candidate count for length `d`. The real obstruction landscape lives on this converged diagonal.

**saturation** ⚠️ **retracted** — the theoretical maximum survivor count is `3·mod/5`. An old bug made counts falsely hit that ceiling and stop the search early. **Not a real phenomenon.** Kept here only so the word is recognised in old logs.

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

## Theorems A and B

Stated and proven in `curve_families.md` §4. They apply to **any** curve whose membership test can be written

    α·p + β = γ·a²          (a linear in n)

| curve | α | β | γ | a |
|---|---|---|---|---|
| ours | 2 | −1 | 1 | 2n+1 |
| k-family | 2 | −k² | 1 | 2n+k |
| cuban | 4 | −1 | 3 | 2n+1 |
| Z[√−2] | 3 | −2 | 1 | 3n+2 |
| centered k-gon | 8 | k−8 | k | 2n+1 |

**Theorem A — the even-d constraint.** For a converse pair at even d:

    a² + b² ≡ 2β/γ   (mod 11)

Since only `{0,1,3,4,5,9}` are squares mod 11, this pins `p mod 11` to a short list. For our curve it gives `{3,5,6,8}` — 7 of 11 classes eliminated.

**Theorem B — the palindrome corollary.** A palindrome is `b = a`, so:

    a² ≡ β/γ   (mod 11)

An even-digit palindrome can lie on the curve **only if `β/γ` is a square mod 11**. For our curve `β/γ = −1 ≡ 10`, a non-residue, so **no even-digit palindrome lies on our curve at all**.

⚠ **Theorem B is necessary, not sufficient.** "Possible" means *not forbidden*. The centered octagon (k=8) permits even-digit palindromes and has none below 10¹².

⚠ **Both bind even d only.** At odd d the argument collapses to `a² ≡ b²`, which eliminates almost nothing — and both known objects, the d=5 emirp and the d=7 palindrome, live at odd d.

**α, β, γ** — the membership coefficients above. Not to be confused with `a`, `b`, which are the per-point square roots.

---

## Nearby curves

**the family** — `p = n² + (n+k)²` for **odd** k. Even k makes `p` always even. Our curve is `k = 1`.

**generalized membership** — `2p = a² + k²` with `a = 2n + k`. At k=1 this is `2p − 1 = a²`.

**the 11 | k exception** — for `11 ∤ k` no even-digit palindrome is on the curve; for `11 | k` they *are* (e.g. `4114 = 33² + 55²` at k=22) but are automatically composite. Full treatment in `nearby_curves.md`.

---

## Lattice — the full definition

Used constantly in `curve_geometry.md`, `curve_families.md` and `curve_lattices.md`, so it is worth pinning down properly.

### What it is

A **lattice** is the set of *all* whole-number combinations of two independent direction vectors `u` and `v`:

    L = { a·u + b·v  :  a, b whole numbers }

Three consequences, and they are the whole content of the word:

1. **Infinite and regular.** It goes on forever in every direction, and it looks identical from every one of its points. Shift the whole lattice by any of its own vectors and you get the lattice back.
2. **Discrete.** There is a smallest non-zero distance between points. A lattice is not a continuum — you cannot get arbitrarily close to a point without landing on it.
3. **Not a shape.** A lattice is a *set of points*, not a grid drawing, and not a curve. The lines people draw between lattice points are a visual aid only.

### The three used in this project

Each is the natural home of a number ring, and **the ring's norm form is exactly the squared distance from the origin** — verified for every point within radius 6:

| lattice | ring | basis `u`, `v` | norm form | nearest neighbours |
|---|---|---|---|---|
| square | `Z[i]` | (1,0), (0,1) | x² + y² | **4** at distance 1 |
| triangular | `Z[ω]` | (1,0), (½, √3/2) | x² + xy + y² | **6** at distance 1 |
| rectangular | `Z[√−2]` | (1,0), (0, √2) | x² + 2y² | **2** at distance 1 |

The nearest-neighbour count *is* the rotational symmetry — 4-fold, 6-fold, 2-fold. That is the number the crystallographic restriction constrains.

### The basis is not unique

`(1,0), (0,1)` and `(1,0), (1,1)` generate **the same** square lattice — verified. Any two bases related by an integer matrix of determinant ±1 describe the same point set. So "the basis" is a *description*, and the lattice is the thing described. Do not read meaning into a particular choice.

### What is *not* a lattice

- **A Penrose tiling / quasicrystal.** It has genuine 5-fold symmetry, which no lattice can have (see *crystallographic restriction*). This is exactly why the centered **pentagon** curve has no lattice behind it.
- **The curve itself.** Our curve is a *sequence of points on a ray*, picked out of a lattice. The lattice is the ambient set; the curve is a thin selection from it.
- **The set of primes.** No regularity, no basis.

### Why it matters here

Every curve in the family is the same construction:

> the lattice points lying on a fixed ray, at squared distance `p` from the origin.

The **quadratic form** chooses *which lattice*; the **linear condition** chooses *which ray*. Both are geometric. Primality and digit reversal are not — see `curve_lattices.md`.

⚠ **Not the order-theory sense.** In algebra "lattice" also means a partially ordered set with meets and joins. Unrelated. Everything in this project means the geometric sense above.

---

## Number-theory vocabulary

Terms borrowed from outside the project. Defined here so no one has to guess.

**Gaussian integers, `Z[i]`** — complex numbers `x + yi` with x, y whole. Their **norm** (squared size) is `x² + y²` — our quadratic form. Geometrically a **square lattice**.

**Eisenstein integers, `Z[ω]`** — same idea with `ω` a cube root of 1 instead of `i`. Norm `x² + xy + y²`. Geometrically a **triangular lattice**. Named for Gotthold **Eisenstein** (1823–1852) — *not* Einstein.

**norm form** — the polynomial giving the squared distance from the origin in a ring's natural lattice. Every curve family in this project comes from one.

**quadratic residue** — a number that *is* a square modulo something. Mod 11 the residues are `{0,1,3,4,5,9}`; the **non-residues** are `{2,6,7,8,10}`. Theorem B is exactly the question "is `β/γ` a residue mod 11?"

**modular inverse** `γ⁻¹` — the number with `γ·γ⁻¹ ≡ 1`. Mod 11, `3⁻¹ = 4` because `3×4 = 12 ≡ 1`. It is how you "divide" in modular arithmetic.

**Fermat's two-square theorem** — every prime `p ≡ 1 (mod 4)` is a sum of two squares in **exactly one** way. Our curve primes are those whose unique representation happens to be *consecutive*.

**conic / paraboloid** — `z = x² + y²` is a paraboloid (a bowl). Cutting it with a plane gives a **conic**; that cut is our curve. Not a cone (`z² = x² + y²`, a funnel) and not a sphere (bounded).

**crystallographic restriction** — a 2-D lattice can only have 2-, 3-, 4-, or 6-fold rotational symmetry. **5-fold is impossible**, which is why the centered *pentagon* curve has no lattice behind it, and why Penrose tilings were a surprise.

**falsification test** — checking a theorem where it predicts something *should* exist, not only where it forbids. A rule that only ever predicts absences can look correct by accident. Used on `Z[√−2]` and the `11 | k` curves.

**Landau's problems / Conjecture F** — the open question of whether infinitely many primes have a given quadratic shape. Our curve's infinitude sits here. Unproven.

### OEIS sequences

| id | sequence |
|---|---|
| **A027862** | primes of the form `n² + (n+1)²` — **our curve** |
| **A002407** | cuban primes, `3n²+3n+1 = (n+1)³ − n³` |
| A005891 | centered pentagonal numbers |
| A001844 | centered square numbers (our curve, primes or not) |

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
