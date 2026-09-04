# The density heuristic, tested across 13 curves

[`OPEN_PROBLEM.md`](OPEN_PROBLEM.md) states a density heuristic for prime palindromes on the curve and derives `E[at digit-length d] ≈ C′/d`, whose divergent sum is what makes the palindrome conjecture "false in spirit". It never names `C′`, and it is calibrated against a single curve — the one the conjecture is about.

[`curve_palindromes.txt`](curve_palindromes.txt) changed that. It holds every palindrome to d = 31 on **13 curves**, and the heuristic makes a *different* prediction for each of them. That converts an unfalsifiable back-of-envelope into something that can fail twelve times over.

This document reports what happened when it was run. Reproduce with:

    python3 docs/density_cross_curve.py

**Everything here is heuristic.** Nothing is a theorem. §2 and §3 are large-sample results that stand on their own; §4 and §5 concern the prime side, where the data is thin and the conclusions are correspondingly weak — §4 is explicit about which statistical test is valid and which is not.

---

## 1. What the model actually says

For `p = A·m² + B·m + C` over a decade of `m`, two curve-specific quantities set the palindrome density.

**`N_d`, the number of `m` giving a d-digit value.** Inverting the quadratic,

    N_d = (1/√A) · (1 − 10^(−1/2)) · 10^(d/2)  =  (1/√A) · 0.6838 · 10^(d/2)

**`P(first digit = last digit)`.** The last digit is *not* uniform — it is fixed by `m mod 10`, and each curve has its own signature. On our curve `p = 2m² + 2m + 1 = 2m(m+1) + 1`, and `m(m+1) mod 10 ∈ {0, 2, 6}`, so

    p mod 10 ∈ {1, 5, 3}   with probability   0.4, 0.4, 0.2

The first digit is not uniform either. Since `p ≈ A·m²` with `m` uniform, `p` has density `∝ 1/√p`, giving `P(first = j) = (√(j+1) − √j)/(√10 − 1)` — the same law on **every** curve here, because they are all quadratics with positive leading coefficient. Hence

    P(first = last) = Σ_v P(last = v) · P(first = v) = 0.1409   on k=1

against 0.10 for the naive model. A d-digit odd-length palindrome needs `(d−1)/2` digit-pair matches, one of which is that first/last pair, so

    E[raw palindromes at odd d] = N_d · P(f=l) · 10^(−(d−3)/2)
                                = (1/√A) · 21.62 · P(f=l)

The `10^(d/2)` cancels exactly: **the expected count is constant in `d`**, which is the whole reason the prime count goes like `1/d` and its sum diverges.

### Two corrections to OPEN_PROBLEM.md that nearly cancel

- `N_d ≈ 10^(d/2)/√2` there is `n_max`, not the *count* of d-digit `m` — it omits `n_min`. Overstates by **1.46×**.
- `P(palindrome) ≈ 10^(−d/2)` ignores the last-digit signature. Understates by **1.41×**.

Net effect on the raw constant: 2.24 (as written) versus 2.15 (corrected) — accidentally almost right, for two wrong reasons. They are recorded because they do **not** cancel for even `d`, nor on the sibling curves, where `A` and the signature differ.

---

## 2. Test 1 — raw palindrome density: the model passes

Odd `d` in [5, 31], 14 digit-lengths, per curve:

    curve       A   p mod 10        P(f=l)    pred     obs    o/p
    k=1         2   {1,3,5       }  0.1409    2.15    2.57   1.19
    k=3         2   {5,7,9       }  0.0864    1.32    1.71   1.30
    k=5         2   {3,5,7       }  0.1031    1.58    1.50   0.95
    k=7         2   {5,7,9       }  0.0864    1.32    1.07   0.81
    k=9         2   {1,3,5       }  0.1409    2.15    1.79   0.83
    k=11        2   {1,3,5       }  0.1409    2.15    2.86   1.33
    k=13        2   {5,7,9       }  0.0864    1.32    0.71   0.54
    k=15        2   {3,5,7       }  0.1031    1.58    1.57   1.00
    k=17        2   {5,7,9       }  0.0864    1.32    0.86   0.65
    k=19        2   {1,3,5       }  0.1409    2.15    3.21   1.49
    k=21        2   {1,3,5       }  0.1409    2.15    2.93   1.36
    cuban       3   {1,7,9       }  0.1254    1.57    0.93   0.59
    Z[sqrt-2]   3   {1,2,4,6,7,9}  0.1212    3.03    3.00   0.99

    TOTAL predicted 333.2   observed 346   ratio 1.04

The k-family splits into exactly three groups by last-digit signature, and the observed densities reproduce the predicted ordering:

| `p mod 10` | predicted | observed | curves |
|---|---|---|---|
| {1,3,5} | 2.15 | 2.67 | k=1, 9, 11, 19, 21 |
| {3,5,7} | 1.58 | 1.54 | k=5, 15 |
| {5,7,9} | 1.32 | 1.09 | k=3, 7, 13, 17 |

This is the substantive result of §2. The grouping is not an input — it falls out of `m mod 10` arithmetic — and the three predicted densities are separated by less than a factor of two, so getting the order right on 443 palindromes is not free. Aggregate agreement is 4%.

`Z[√−2]` is worth noting: it is the union of two branches (`3m²+2m+1` and `3m²+4m+2`), so it has six reachable last digits instead of three and roughly twice the `N_d`. The model predicts 3.03 — the highest of any curve — and observes 3.00.

---

## 3. Test 2 — even `d`: a null test for Theorem B

The digit model knows nothing about quadratic residues mod 11. It therefore predicts even-`d` palindromes on **every** curve, at `10^(−1/2)` the odd-`d` rate.

[Theorem B](curve_families.md) says otherwise: an even-`d` palindrome on `αp + β = γa²` requires `β/γ` to be a quadratic residue mod 11. Of the 13 curves, that permits exactly two.

    curve           pred    obs
    k=1              8.9      0
    k=3              5.4      0
    k=5              6.5      0
    k=7              5.4      0
    k=9              8.9      0
    k=11             8.9      8  <- Theorem B PERMITS  (β/γ ≡ 0)
    k=13             5.4      0
    k=15             6.5      0
    k=17             5.4      0
    k=19             8.9      0
    k=21             8.9      0
    cuban            6.4      0
    Z[sqrt-2]       12.4     19  <- Theorem B PERMITS  (β/γ ≡ 9, a QR)

    on the curves Theorem B FORBIDS: predicted 77, observed 0

**Seventy-seven predicted palindromes, none of which exist, on precisely the eleven curves the theorem forbids — and on the two it permits, the model is accurate (8.9 vs 8) or the right order (12.4 vs 19).**

This is the strongest confirmation of Theorem B in the project, and it is stronger than "we looked and found none". An absence is only evidence against a background expectation, and until now there was no quantitative background. The density model supplies one: it says those 77 palindromes *should* be there, on digit-statistical grounds, and an arithmetic obstruction is the only thing standing where they would be.

---

## 4. Test 3 — prime palindromes: what can and cannot be concluded

Moving from raw palindromes to prime ones needs two more factors, both curve-specific:

- **the div-5 dead fraction** — palindromes ending in 5 begin with 5 and are composite;
- **the Hardy–Littlewood local density** `∏_q (1 − ω(q)/q)/(1 − 1/q)`, where `ω(q)` is the number of roots of `A m² + B m + C` mod `q`, computed by Legendre symbol over `q ≤ 5×10⁴`, excluding `q ∈ {2,5}` (already handled by the digit analysis).

### The per-curve test is invalid — and this is worth stating explicitly

Fitting one global scale and testing per-curve fit gives:

    d range       primes  chi2/12df   cells with E<5 / E<1
    d >= 5            30      44.3       13/13 and  0/13
    d >= 11           14      19.1       13/13 and  6/13
    d >= 15            7      11.5       13/13 and 13/13

It is tempting to read that as "the model fails at small `d` and fits at large `d`", and to conclude that `C′/d` is sound asymptotically. **That reading is wrong.** With 13 curves and at most 30 primes, *every* expected cell count is below 5 at every threshold, and past d = 15 every one is below 1. The χ² approximation does not hold there. The apparent improvement is the test **losing the power to detect anything** as the data thins — not the model gaining accuracy.

This is recorded rather than quietly fixed because it is an easy and seductive mistake: a goodness-of-fit statistic that improves as you discard data is almost always measuring your sample size, not your model.

One thing the d ≥ 5 row *does* support, because the counts there are large enough to matter: the model is decisively rejected per-curve (k=19 shows 12 primes against 3.1 predicted). Replacing a crude coprimality boost with the full Hardy–Littlewood product moved χ² only from 46.0 to 44.3, so **the per-curve spread is not explained by local densities.** What explains it is unresolved; 69% of all prime palindromes across all 13 curves lie at d ≤ 9, where modelling "p is prime" as an independent event of probability `1/ln p` is worthless, and that is the leading suspect.

### The valid test: pool the curves, bin the digit-lengths

Pooling all 13 curves and binning so that every expected count is ≥ 3.3:

    d          obs   model 1/d
    3           11        10.0
    5            7         6.0
    7            5         4.3
    9            4         3.3
    11-13        7         5.0
    15-19        4         5.3
    21-31        3         7.0

    chi2 = 3.95 on 5 df   (p ~ 0.56)

**The `1/d` shape is consistent with the data.** That is the defensible statement about the prime side: the *shape* survives a valid test on 41 primes. The per-curve *constants* do not — there is not enough data to test them.

## 5. `C′` for our curve, and what more compute would buy

Two estimates, from the same model and the same data:

    pooled over all 13 curves : 3.28
    fitted on d >= 15 alone   : 1.86     (7 primes -- weak)
    => quote as a RANGE       : 1.9 - 3.3

The pooled figure is the better-supported one; the `d ≥ 15` fit rests on 7 primes and is quoted only to bound the uncertainty. Anything downstream of `C′` inherits a factor-of-two spread.

    search range        E (C'=1.9)        E (C'=3.3)
    d = 29 .. 37       0.28 -> 25%       0.50 -> 39%
    d = 39 .. 51       0.29 -> 25%       0.51 -> 40%
    d = 29 .. 51       0.57 -> 44%       1.02 -> 64%

Two consequences.

**Our curve is not special.** The silence after 3187813 (d = 7) is unremarkable: 3 primes observed on k=1 at odd d ≥ 3 against 4.5 predicted. Twelve siblings say a curve going quiet early is ordinary — cuban stops at d = 3, k=7 and k=13 never produce one at all, while k=5 reaches d = 17 and k=19 reaches d = 31. Whatever makes 3187813 interesting, it is not that this curve behaves unusually.

**Extending the search is a real bet, not a formality.** The `palsplit` sweep to d = 37 ([`palindrome_split_search.md`](palindrome_split_search.md)) carried an expected yield of 0.28–0.50, so finding nothing was the likely outcome either way and is weak evidence at best. But pushing to the practical ceiling near d = 51 carries a **25–40% chance of an actual hit**, and the d = 29…51 range taken together is **44–64%**. That is worth doing on its own terms.

**The caveat that cuts against this.** The `d = 21–31` bin runs low — 3 observed against 7.0 expected. That is the bin nearest every extrapolation made here. Under Poisson, P(≤3) ≈ 0.08: suggestive, not significant. If the deficit is real rather than noise, `C′` falls at high `d` and the odds above are optimistic. Settling it needs more data at high `d`, which is cheap — see §6.

**What this does not say.** Nothing here bounds how far one would have to search to *settle* the conjecture, and an earlier draft of this document claimed it did. A divergent `Σ C′/d` grows like `log d`, so expectation accumulates slowly, but "slowly" is not "never", and the constant is known only to a factor of two. The honest summary is narrower: **the reachable frontier is worth searching, a null result there will remain weak evidence, and the conjecture stays open either way.**

## 6. What would sharpen this

- **The 1.4× residual on k=1** (predicted 2.15, observed 2.57 raw). Candidates: conditioning on `p ≢ 0 mod 3` and `mod 11`, which the digit model ignores; and correlations between digit positions in a palindrome.
- **The `d = 21–31` deficit (3 observed vs 7.0).** The single most consequential open number here: it sits right where every extrapolation starts. More data at high `d` settles it.
- **`C′` is known only to a factor of two** (1.9–3.3). Extending `curve_palindromes.txt` past d = 31 would tighten it. `palcurve`'s `MAX_D` is capped at 33, conservatively — the u128 arithmetic is exact to d = 37 (see its `MAX_D` comment), so ~6 more digits are available at the cost of a decision to publish uncorroborated sibling data.
- **Even-`d` on the two permitted curves.** `Z[√−2]` observed 19 against 12.4 predicted; k=11 is accurate. Worth checking whether the two-branch structure is being weighted correctly.
- **The emirp side.** The same machinery should give `C/d²` and a per-curve constant for converse pairs. That sum converges to ≈ 1, which is the other half of `OPEN_PROBLEM.md` and the half the heuristic says is *right*.

---

## See also

- [`OPEN_PROBLEM.md`](OPEN_PROBLEM.md) — the heuristic this tests, and both open problems
- [`curve_palindromes.txt`](curve_palindromes.txt) — the 443-palindrome dataset, 13 curves, d ≤ 31
- [`curve_families.md`](curve_families.md) — Theorems A and B
- [`palindrome_split_search.md`](palindrome_split_search.md) — the O(10^(d/4)) search and the d = 37 frontier
- [`density_cross_curve.py`](density_cross_curve.py) — reproduces every number above
