# Palindromes on the curve `p = 2n² + 2n + 1 = n² + (n+1)²`

A summation of what we know about **palindromic values on the curve** —
what is proven by construction, what is conjectured, and the structure in
between. Companion to [`STATE_OF_THE_SEARCH.md`](STATE_OF_THE_SEARCH.md)
and [`PROJECT_OVERVIEW.md`](PROJECT_OVERVIEW.md).

Corpus: 33 palindromic curve-values on record — the small primes, the
complete odd d = 7…25 set (from `hunt.c` / `hunt_noopt`, div-5 inclusive),
and d = 27's three prime-eligible ones. (d = 25's five were value-dumped
2026-07-06 by the count-preserving `hunt_noopt`, div-5 inclusive — see
[`pals_d25.txt`](../pals_d25.txt); d = 27's div-5 palindromes were skipped
by the optimized binary — see §9.)

---

## Established facts — quick reference

| fact | status |
|---|---|
| `p ≡ 1 (mod 4)` — every curve value is a `4n+1` number | **PROVEN** (by construction) |
| `p` is a sum of two **consecutive** squares `n²+(n+1)²` | **PROVEN** (definitional) |
| `p` is **never** divisible by 11 → palindromes only at **odd** d | **PROVEN** |
| `p` ends in one of `{01,13,21,41,61,81}`, else `…5` (div-5) | **PROVEN** |
| a palindrome is the `m = n` (degenerate) case of an emirp | **PROVEN** |
| only four prime palindromes: `5, 181, 313, 3187813` | **CONJECTURE** (brute d≤27) |
| `3187813` is the largest prime palindrome on the curve | **CONJECTURE** (since 1997) |

---

## 1. The Fermat confluence

Being a `4n+1` number and a sum of two squares is **not** special to the
palindromes — *every* value on the curve has it by construction:
`n² + (n+1)²` is even² + odd² ≡ 1 (mod 4). What is striking is the
**confluence**: each prime palindrome here is *simultaneously*

- a **palindrome**,
- a sum of two **consecutive** squares, and
- a **Fermat `4n+1` prime** — the `p ≡ 1 (mod 4) ⇔ p = a²+b²` theorem, in
  its tightest case, where `a` and `b` are consecutive.

```
      5 = 1² + 2²          181 = 9²  + 10²
    313 = 12² + 13²    3187813 = 1262² + 1263²   (a palindromic prime!)
```

That last line is the whole project in one number — Fermat's two-square
theorem (the book that started it) surfacing inside the palindrome hunt.

---

## 2. Only four are prime — yet the curve keeps making palindromes

The only prime palindromes are `5, 181, 313, 3187813`, all at **d ≤ 7**.
But palindromic *values* keep appearing at nearly every odd d out to 27.
The desert past `3187813` is a **primality** desert, not a palindrome
desert — the curve never stops minting palindromes; past d = 7 they are
simply all composite.

---

## 3. Odd d only — the ÷11 dodge *(PROVEN)*

Every even-length palindrome is divisible by 11. And **no curve value is
ever divisible by 11**: `p = n² + (n+1)²` is a sum of two *coprime*
squares, and a prime ≡ 3 (mod 4) — like 11 — divides a sum of two squares
only if it divides both. Consecutive integers are coprime, so 11 cannot.
Hence **zero palindromes at even d**, prime or composite. Genuine tests
are the odd d.

---

## 4. …but not always — a Poisson coincidence, not an obstruction

Some odd d are still empty: **d = 5 and d = 9 have zero palindromes.**
This is not a congruential obstruction — it is sparse counting.

A d-digit number is on the curve ⇔ `2p−1` is a perfect (odd) square. So
count the d-digit palindromes `P` with `2P−1` a perfect square. There are
`~9·10^((d−1)/2)` palindromes of length d, and the chance `2P−1` is a
square is `~10^(−d/2)`. Multiply:

> **expected on-curve palindromes per odd d ≈ 9/√10 ≈ 2.85 — the `d`
> cancels.**

A near-constant-mean Poisson process. So `P(empty) ≈ e^(−2.85) ≈ 5.8%`
per odd d — a couple of empty odd d (5, 9) are *expected by chance*, and
the mean not decaying is why palindromic values persist at every odd d.
This matches the project's recurring theme: obstructions here are
**sporadic, non-congruential**.

**Independent cross-check** — a *palindrome-first* enumerator
([`palfirst.py`](../palfirst.py)) that generates palindromes and tests
`2P−1` for squareness reproduces the n-first `hunt.c` exactly:

| d | on-curve palindromes | prime |
|---|---|---|
| 5 | *(none)* | — |
| 7 | 1690961, 3162613, 3187813, 5258525, 5824285 | 3187813 |
| 9 | *(none)* | — |
| 11 | 58281418285 | — |
| 13 | 1635446445361, 3166046406613 | — |

Two unrelated methods agreeing is the ground truth.

**Compositeness certificates (every odd d, 13…27).**
For each prime-eligible on-curve palindrome, an in-repo, self-checkable
`f₁·f₂·… = p` factorization (via Michel Léonard's `qs`) is stored:
- [`pals_d13.txt`](../pals_d13.txt) — 2 values
- [`pals_d15.txt`](../pals_d15.txt) — 3 values (+1 div-5, omitted)
- [`pals_d21.txt`](../pals_d21.txt) — 4 values (+1 div-5, omitted)
- [`pals_d25.txt`](../pals_d25.txt) — 4 values
- [`pals.txt`](../pals.txt) / [`d27_qs_certificates.txt`](d27_qs_certificates.txt) — 3 values (d=27)

d=17 has an explicit factorization in the 2026-06-04 session archive;
d=11/19/23 palindromes are all div-5 (trivially composite). So every odd
d from 9 to 27 has a stored certificate or a trivial-divisibility record —
no step rests on a `found=0` summary alone.

---

## 5. The ending signature — `…13` is the densest

![Ending distribution: …13 is the densest prime-eligible ending and holds
313 and 3187813; after d = 25 the div-5 (…5) group ties …13 as the
largest, all composite](palindrome_endings.png)

Among prime-eligible palindromes (last digit 1 or 3), the two-digit
endings are far from uniform:

| ending (→ leading) | count |
|---|---|
| **`…13`** (`31…`) | **11** ← densest by far |
| `…21` (`12…`) | 4 |
| `…01` (`10…`) | 2 |
| `…61` (`16…`) | 2 |
| `…41` (`14…`) | 2 |
| `…81` (`18…`) | 1 |

**What d = 25 added** (count-preserving dump, 2026-07-06) — it *sharpened*
the signal rather than blurring it: three of its five palindromes landed
in the densest `…13` channel.

| ending | before (d ≤ 27) | + d = 25 | now |
|---|---|---|---|
| `…13` | 8 | +3 | **11** |
| `…41` | 1 | +1 | 2 |
| `…5` (÷5) | 10 | +1 | 11 |

`…13` is the single most common prime-eligible ending — now **~2.75× the
next**, and d = 25 alone added three more. It comes from the curve's
ending supply: `…13` occurs 10× per period of 50 in n, versus 4× for each
other ending (last-digit-3 funnels into the *one* ending `13`, while
last-digit-1 spreads across five). Grouped, the five "last-digit-1"
endings now only **tie** `13` (11 vs 11) — despite carrying 2× the
aggregate supply — because a palindrome's leading digits mirror its ending
and Benford favours leading `1`. Net: **`13` is over-represented per
ending, more so after d = 25;** the famous primes `313` and `3187813` are
both `31…13`.

---

## 6. Div-5 palindromes — a third of the population

Of the 33 palindromic curve-values, the last-digit split is a dead heat:

| last digit | of 28 (d ≤ 27) | of 33 (+ d = 25) | note |
|---|---|---|---|
| 1 (`1…1`) | 10 | **11** | prime-eligible |
| 3 (`31…13`) | 8 | **11** | prime-eligible |
| **5 (`5…5`)** | 10 | **11** | **÷5 — trivially composite** |

Folding in d = 25 tips the split to a clean **11 / 11 / 11**. That tidy
three-way tie is almost certainly a **coincidence** — at n = 33 in a
sparse, near-constant-mean Poisson process (§4) there is no structural
reason the ÷5 group should track the 1- and 3-ending groups, and the next
odd d will likely break it. The durable fact is the ÷5 share: **~1/3 end
in 5** (`5258525`, `5649436330336349465`, …) — composite on sight. Some
odd d (11, 19, 23) produce *only* div-5 palindromes. These are exactly
what the skip-optimized `hunt` drops (§9); they are irrelevant to the
conjecture but real members of the palindrome population.

---

## 7. Why the conjecture is the *interesting* claim *(heuristic — TREND)*

Two densities diverge:

| object | expected count per odd d | over all d |
|---|---|---|
| palindromic **value** | `~2.85` (constant) | persists indefinitely |
| palindromic **prime** | `~2.85 / (d·ln10) = C′/d` | sum **diverges** |
| bi-quadratic **emirp** | `C/d²` | sum **converges** (≈ 1) |

The prime-palindrome sum `Σ C′/d` diverges — so the heuristic does **not**
predict a last one. "3187813 is the largest" therefore **bets against its
own heuristic**, which is exactly what makes it a conjecture worth chasing
and not a theorem. (The emirp side is the opposite: `C/d²` converges, so
"only one emirp" is what the heuristic *predicts*.)

---

## 8. A palindrome is a degenerate emirp

Set `m = n`, so `q = rev(p) = p`: the emirp relation collapses to a
palindrome. One sieve, one obstruction, both objects — which is why the
emirp hunt and the palindrome hunt are the same search.

---

## 9. Verification & methodology

- **Three independent methods agree** on the palindrome counts: the
  n-first brute (`hunt.c`), the modular sieve (`mod_obstruct.c`), and the
  palindrome-first enumerator (`palfirst.py` / `palhunt_gmp`).
- **Prime-eligible ≠ prime.** Passing the composite sieve is not
  primality. Definitive *compositeness* is a factorization (a self-
  checkable certificate: `f₁·f₂ = p`); we use `qs` for that — see
  [`CREDITS.md`](../CREDITS.md). The d = 27 palindromes were certified
  composite this way.
- **The skip-opt is lossy for div-5.** The optimized `hunt` skips
  `n mod 10 ∈ {1,3,6,8}` (where `p` is ÷5), so it undercounts div-5
  survivors and palindromes — but never a prime-eligible one, and never
  an emirp. See [`skip_optimization.md`](skip_optimization.md). Counts
  here labelled "div-5 inclusive" come from the pre-opt binary.
