# The n mod 10 Skip Optimization in hunt.c

> ⚠️ **This optimization is LOSSY for counts.**
> Emirp results are unaffected — a div-5 `p` is composite, so no emirp is ever skipped, and the non-existence result through d=27 stands.
> But `survivors(raw)` and `palindromes` are **undercounted**: the skip silently drops the div-5 ones.
> Read [Validation — CORRECTED 2026-07-05](#validation--corrected-2026-07-05-the-optimization-is-lossy) before trusting any count from an optimized run.
> For the full survivor/palindrome corpus, use the pre-opt binary (built from commit `a974123`).

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
no approximation, no edge case *in the skip itself*.

The edge case is in the **bookkeeping**, not the arithmetic: the skipped
`n` are still genuine survivors and palindromes when `rev(p)` lands on
the curve, so removing them from the loop also removes them from the
counts. See the corrected Validation section below.

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

## Validation — CORRECTED 2026-07-05: the optimization is LOSSY

⚠️ The original claim here ("every count matched the known landscape
exactly") was **wrong**, and wrong in a revealing way: it compared the
optimized binary against *its own output*, not against the real
(pre-opt) landscape. A direct A/B — building the pre-opt source
(`a974123`) as `hunt_noopt` and running both with identical flags —
shows the counts do **not** match:

| d  | opt raw / pals | pre-opt raw / pals | landscape |
|----|----------------|--------------------|-----------|
| 5  | 6 / 0          | 6 / 0              | 6 / 0     |
| 7  | **5 / 3**      | 7 / 5              | 7 / 5     |
| 9  | **2 / 0**      | 6 / 0              | 6 / 0     |
| 11 | **2 / 0**      | 5 / 1              | 5 / 1     |
| 12 | **1 / 0**      | 2 / 0              | 2 / 0     |
| 13 | 4 / 2          | 4 / 2              | 4 / 2     |

The pre-opt binary reproduces the landscape; the optimized one does not.
The two palindromes the opt drops at d=7 are `5258525` and `5824285` —
**both end in 5** (÷5, composite). The skip is doing exactly what it
says, and that changes the bookkeeping counts.

**What this means:**

- **EMIRPS: exact and unaffected.** A div-5 `p` is composite, so it can
  never be prime, so it can never be an emirp. The skip cannot hide a
  real emirp. Every `EMIRPS = 0` result (the entire emirp non-existence
  conclusion through d=27) is valid — `EMIRPS` matches in both binaries.
- **`survivors(raw)` and `palindromes`: undercounted.** The opt drops
  the div-5 (trivially composite) ones. So this is a **lossy-but-
  emirp-safe** optimization, NOT the count-preserving speedup this doc
  originally described.

**Process lesson:** the mandated "confirm counts match the landscape"
check was recorded as passing but never actually run against the true
landscape. A real regression test (pre-opt values hard-coded) would
have caught this at d=7.

The **speedup is real** (the skip removes 40% of full-cost GMP
iterations). Clean singleton A/B on d=23, `hunt` vs `hunt_noopt`,
identical flags, same machine, 2026-07-05:

| binary | d=23 wall | raw | pals | prime-eligible |
|--------|-----------|-----|------|----------------|
| hunt_noopt (pre-opt) | 6031 s (1.68 h) | 3 | 1 | 2 |
| hunt (optimized)     | 3632 s (1.01 h) | 2 | 0 | 2 |

**Speedup = 1.66× (39.8% faster)** — the original "~35%" estimate was
conservative. And the count-delta is exactly the lossy signature: the opt
drops 1 survivor and 1 palindrome (both div-5), while **`prime-eligible`
is identical (2 = 2)** — that metric excludes div-5 by definition, so it
is opt-invariant. The skip only ever touches div-5 bookkeeping; the
prime-eligible and EMIRPS results are untouched.

Committed 2026-06-29 as `7dd367e`; lossiness identified 2026-07-05.

---

## Effect on the d=27 run — the mixed-binary footnote

The d=27 emirp hunt was launched 2026-06-28 **without** the
optimization (old binary). At approximately **19.8% completion**
(i = 3,030,000,000,000 of 15,289,611,963,133), the run was stopped,
the optimized binary was installed, and the run was resumed from the
checkpoint.

Consequence for the final d=27 summary:

- **`EMIRPS`**: exact and unaffected. Skipped `n` have composite `p`
  by definition and cannot be emirps.
- **`palindromes`**: **count IS affected** (corrected 2026-07-05). The
  opt drops div-5 palindromes (those ending in 5, e.g. `5258525` at
  d=7). At d=27 the recorded count is **3 prime-eligible palindromes**;
  any div-5 (ending-in-5) palindromes there were skipped and are not
  listed in `pals.txt`. The **conjecture** is unaffected — div-5
  palindromes are trivially composite, so "no *prime* palindrome at
  d=27" still holds — but the palindrome *count/corpus* is partial.
- **`survivors(raw)`**: undercounted, and by a **mixed convention** at
  d=27 specifically: the first 19.8% ran the old (count-preserving)
  binary and the remaining 80.2% ran the opt, so div-5 hits are counted
  for part of the range and skipped for the rest. The recorded d=27
  `raw=5` is therefore a floor, not the true all-n total. This is a
  bookkeeping total feeding the density graph — not a scientific claim.
  The emirp result is exact and unaffected.

The new binary PID 569981 resumed at `[34.28 h]` from checkpoint
`3035000000000 2 1 0 1`.
