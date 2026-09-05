# Density Heuristics — bi-quadratic emirps & prime palindromes on 2n²+2n+1

Two useful derivations: the n–d frontier formula (graph axis scaling) and the
emirp-vs-palindrome rarity comparison.

---

## 1. Frontier formula: log₁₀(n) from digit-length d

The curve is p = 2n² + 2n + 1 ≈ 2n² for large n.  A p with d digits satisfies
p < 10^d, so:

    2n² < 10^d
    n  < sqrt(10^d / 2)  =  10^(d/2) / sqrt(2)

Taking log₁₀:

    log₁₀(n_max)  =  d/2  −  log₁₀(√2)
                  =  d/2  −  ½·log₁₀(2)
                  =  (d − log₁₀(2)) / 2
                  =  (d − 0.30103) / 2

**Worked examples (used for graph frontier lines):**

| frontier | d  | log₁₀(n_max)          | used in graph |
|----------|----|------------------------|---------------|
| emirp    | 26 | (26 − 0.301)/2 = 12.85 | LXF = 12.85   |
| palindrome | 27 | (27 − 0.301)/2 = 13.35 | LXP = 13.35   |

Inverse (d from a known n boundary):

    d  =  2·log₁₀(n)  +  log₁₀(2)  +  1   (round up for digit count)

---

## 2. Expected-count heuristic: emirps vs prime palindromes

For a d-digit range, n runs from ~ 10^((d−1)/2)/√2 to ~ 10^(d/2)/√2,
so there are roughly

    N_d  ≈  10^(d/2) / √2

candidate n values.

### Prime palindrome at digit-length d

Requirements:
1. p = 2n²+2n+1 is a palindrome   [prob ~ 10^(−(d−1)/2) ≈ 10^(−d/2)]
2. p is prime                      [prob ~ 1/ln(p) ≈ 1/(d·ln10)]

Note: rev(p) = p trivially, so the "on-curve" check is free.

    E[palindromes at d]  ≈  N_d × 10^(−d/2) × 1/(d·ln10)
                         ≈  (1/√2) × 1/(d·ln10)
                         =  C′/d

Summing over all d: **Σ C′/d diverges** (harmonic series).
→ Infinitely many prime palindromes are *expected* by this heuristic.
→ The conjecture that 3187813 is the last is the *interesting* claim;
  the heuristic does not guarantee it.

### Bi-quadratic emirp at digit-length d

Requirements:
1. q = rev(p) is on the curve
   [prob ~ 10^(−d/2); d-digit nums on curve ≈ N_d/10^d]
2. p is prime                      [prob ~ 1/(d·ln10)]
3. q is prime                      [prob ~ 1/(d·ln10), independent of p]
4. q ≠ p (not a palindrome)        [probability ≈ 1 for large d]

    E[emirps at d]  ≈  N_d × 10^(−d/2) × [1/(d·ln10)]²
                    ≈  (1/√2) × 1/(d·ln10)²
                    =  C/d²

Summing over all d: **Σ C/d² converges** (p-series, p=2).
→ A *finite* total number of emirps is expected across all integers.
→ Expected total ≈ 1 (consistent with exactly one known: 12641 ⟷ 14621).

> **The structural factor is now MEASURED, not assumed (2026-09-04).**
> The `10^(-d/2)` term predicts a survivor count constant in d; observed
> counts are flat across 10^11 in range, corr = -0.028. See
> [`survivor_density_calibration.md`](survivor_density_calibration.md),
> which also corrects where the remaining probability sits: ~16% lies
> above d = 27, not ~2%.

### Comparison

| object        | expected count at d | sum over all d   |
|---------------|---------------------|------------------|
| prime palindrome | C′/d             | diverges (∞ expected) |
| bi-quadratic emirp | C/d²           | converges (≈ 1 expected) |

The extra factor of 1/(d·ln10) for emirps comes entirely from the **second
independent primality test** (q must also be prime).  The "rev on curve"
factor (10^(−d/2)) cancels with N_d the same way for both objects.

Note: both p and q are ≡ 1 (mod 4) automatically (all curve values are, by
construction), so the Fermat two-square structure is not an additional
constraint for either search — it is equally free for both.

### Calibration

Observed vs expected:
- 4 prime palindromes found (d = 1, 3, 3, 7); none found d = 8…27.
  C′/d predicts slow growth — 20 extra digits of search, roughly log(27/7)
  additional expected, consistent with 0 new ones found.
- 1 emirp found (d = 5); none found d = 6…27.
  C/d² predicts expected total ≈ 1 across all d — exactly what was observed.
