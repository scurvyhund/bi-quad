# The n mod 10 Skip Optimization in hunt.c

## What it is

For the curve `p = 2n² + 2n + 1 = n² + (n+1)²`, the last digit of `p`
is fully determined by `n mod 10`:

| n mod 10 | p mod 10 | Prime-eligible? |
|----------|----------|-----------------|
| 0        | 1        | yes             |
| 1        | 5        | **no** — div 5  |
| 2        | 3        | yes             |
| 3        | 5        | **no** — div 5  |
| 4        | 1        | yes             |
| 5        | 1        | yes             |
| 6        | 5        | **no** — div 5  |
| 7        | 3        | yes             |
| 8        | 5        | **no** — div 5  |
| 9        | 1        | yes             |

When `n mod 10 ∈ {1, 3, 6, 8}`, `p` is divisible by 5 and therefore
composite (for `p > 5`). Such an `n` cannot produce a prime `p`, an
emirp, or a prime palindrome — ever, by arithmetic. Checking those `n`
values is wasted work.

The optimization: skip all `n` where `n mod 10 ∈ {1, 3, 6, 8}` before
any GMP computation. This eliminates **40% of all loop iterations**.
Estimated wall-clock speedup: **~35%** (the remaining 60% still bears
the full GMP cost per iteration).

This is a **proof by arithmetic**, not a heuristic. No primality test,
no approximation, no edge case.

---

## History — found in 2010, lost in refactoring, recovered in 2026

The optimization was present in the **original 2010 C code** under two
forms:

### `converse-otto-orig.c` and `newrev.c` (2010)

Both files contained:

```c
int skip_val[6] = {2, 2, 1, 2, 2, 1};
```

used in a six-step inner loop that incremented `x` (the `n` variable)
by those amounts, cycling through valid residues:

```
x=5 (+2) x=7 (+2) x=9 (+1) x=10 (+2) x=12 (+2) x=14 (+1) x=15 ...
```

The gaps `{2, 2, 1, 2, 2, 1}` are exactly the spacings between
consecutive valid `n mod 10` values in `{0, 2, 4, 5, 7, 9}` — the
same set as `VALID_NMOD[]` today, just expressed as an additive
stride rather than a lookup table.

The comment in `converse-otto-orig.c` documented the reasoning:
numbers ending in 5 cannot be prime, so the step pattern skips them
before doing any prime testing.

### Lost during the GMP / OpenMP refactor

When `hunt.c` was rewritten to use GMP (arbitrary precision) and
OpenMP (parallel threads), the inner loop structure changed from a
hand-crafted stride loop to a linear index `i = 0 .. range-1` feeding
`n = n_min + i`. The skip table did not survive the port. The code
continued to catch composite-p survivors via the `elig_survivors`
filter (checking `p mod 5 != 0` after the fact), but it still computed
`p`, reversed its digits, and ran `on_curve(q)` for every `n` —
including the 40% that were provably useless.

The omission went unnoticed because:
- The `elig` filter correctly excluded them from prime testing
- Their contribution to `survivors(raw)` was small and non-critical
- No regression test checked for them explicitly

### Recovered 2026-06-29

During a code review of `hunt.c` while the d=27 run was in flight,
the observation that `p` can only end in `{1, 3, 5}` on this curve
led back to the same table. Cross-checking against `converse-otto-orig.c`
confirmed it was the same optimization, 16 years later.

---

## Implementation in hunt.c

```c
/* n%10 in {1,3,6,8} -> p ends in 5 -> composite; skip those. */
static const int VALID_NMOD[10] = {1,0,1,0,1,1,0,1,0,1};
```

In the per-d setup (after `compute_n_bounds`):

```c
int nmod_base = (int)mpz_fdiv_ui(n_min, 10);
```

In the inner loop, before any GMP work:

```c
if (!VALID_NMOD[(nmod_base + i % 10) % 10]) continue;
```

`nmod_base` is computed once per digit-length (one cheap GMP call).
The skip check itself uses only native integer arithmetic — no GMP
involved for the 40% of iterations that are discarded.

---

## Validation

After adding the optimization, `./hunt 5 19` was run and every count
matched the known landscape exactly:

- d=5: raw=6, pals=0, EMIRPS=2 (12641⟷14621 found) ✓
- d=6: raw=0 ✓
- d=7: raw=5, pals=3 ✓
- d=8..19: all survivor and palindrome counts identical to pre-opt ✓

Committed 2026-06-29 as `7dd367e`.

---

## Effect on the d=27 run — the mixed-binary footnote

The d=27 emirp hunt was launched 2026-06-28 **without** the
optimization (old binary). At approximately **19.8% completion**
(i = 3,030,000,000,000 of 15,289,611,963,133), the run was stopped,
the optimized binary was installed, and the run was resumed from the
checkpoint.

Consequence for the final d=27 summary:

- **`EMIRPS`**: unaffected. Skipped `n` values have composite `p`
  by definition and cannot be emirps.
- **`palindromes`**: unaffected. A palindrome with `p` divisible by 5
  is composite and irrelevant to the conjecture.
- **`survivors(raw)`**: slightly lower than a full all-n run would
  produce. The first 19.8% was scanned without the skip, so composite-p
  on-curve hits (n ending in 1, 3, 6, or 8 that happen to have
  `rev(p)` on the curve) were counted for that portion. The remaining
  80.2% skips them. In practice this means `survivors(raw)` at d=27
  may be 1–5 lower than it would be with a uniform all-n scan.
  This count is not a scientific claim — it is a bookkeeping total
  that feeds the density graph. The emirp and palindrome results are
  exact and unaffected.

The new binary PID 569981 resumed at `[34.28 h]` from checkpoint
`3035000000000 2 1 0 1`.
