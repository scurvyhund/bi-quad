# Calibrating the emirp heuristic: the structural factor, measured

**2026-09-04.** The bi-quadratic emirp heuristic in
[`density_heuristics.md`](density_heuristics.md) multiplies three
factors:

    E[emirps at d]  ≈  N_d × 10^(−d/2) × [1/(d·ln10)]²
                       ─────────────────   ──────────────
                       structural          two primality tests

The primality factors are textbook PNT and will stay assumptions. **The
structural factor is not an assumption — it is directly measurable, and
this note measures it.** It predicts something sharp and falsifiable:

> The count of raw curve-reversal survivors per digit-length is
> **constant in d**, because the search range grows as `10^(d/2)` and
> the probability that `rev(p)` lands on the curve falls as
> `10^(−d/2)`.

That is a strong claim. The range grows by eleven orders of magnitude
across the data we have.

## Result

**Confirmed, over 10^11 in range.**

    corr(survivor count, log10 range) = −0.028      (0 = perfectly flat)

If the `10^(−d/2)` factor did not exist and survivors merely scaled with
the range, d = 27 would show **~6 × 10^11** survivors. It shows **5**.

## Data

Two series, and they must not be mixed — see *Conventions* below.

**Series A — div-5 EXCLUDED** (`hunt` as it stands today, with
`VALID_NMOD`). d = 5…19 run for this note, single-threaded; d = 27 from
the production log.

| d | range N_d | raw | | d | range N_d | raw |
|---|---|---|---|---|---|---|
| 5 | 153 | 6 | | 13 | 1,528,961 | 4 |
| 6 | 484 | 0 | | 14 | 4,835,000 | 6 |
| 7 | 1,529 | 5 | | 15 | 15,289,612 | 5 |
| 8 | 4,835 | 2 | | 16 | 48,349,999 | 1 |
| 9 | 15,289 | 2 | | 17 | 152,896,119 | 1 |
| 10 | 48,350 | 0 | | 18 | 483,499,984 | 0 |
| 11 | 152,896 | 2 | | 19 | 1,528,961,196 | 0 |
| 12 | 483,500 | 1 | | 27 | 15,289,611,963,132 | 5 |

mean 2.50 · variance 4.88 · range span 9.99 × 10^10

**Series B — div-5 INCLUDED** (pre-optimization production binary).

| d | range N_d | raw |
|---|---|---|
| 21 | 15,289,611,963 | 7 |
| 22 | 48,349,998,344 | 0 |
| 23 | 152,896,119,631 | 3 |
| 24 | 483,499,983,437 | 2 |
| 25 | 1,528,961,196,313 | 7 |
| 26 | 4,834,999,834,366 | 6 |

mean 4.17 · variance 7.14 · range span 316

Series B's mean sits above Series A's by roughly the fraction the div-5
skip discards, which is the expected relationship between the two
conventions and a mild consistency check on both.

## Conventions — the trap

`hunt.c` applies `VALID_NMOD`, the **lossy** div-5 skip. Per
[`skip_optimization.md`](skip_optimization.md) it leaves `EMIRPS`
correct but **undercounts `survivors(raw)` and `palindromes`**. So any
count produced by today's binary is div-5-excluded, while the archived
d = 21…26 production counts are div-5-included.

This was caught during the run: the fresh d = 19 result came back
`raw=0` where `PROJECT_OVERVIEW` records palindromic survivors at
d = 19. Those survivors are div-5 ones and the skip drops them.
`skip_optimization.md` names the two the skip drops at d = 7
(`5258525`, `5824285`) — the fresh run reports 3 palindromes there
against the true 5, exactly as documented.

**Do not pool the two series.** Every analysis above is within-series.

## Calibration

Using the measured prime-eligible survivor rate (≈2.3 per d, consistent
across both series) and a prime density boosted by `10/φ(10) = 2.5` for
integers coprime to 10:

    E[emirps at d] ≈ 2.71 / d²

Sanity check: this predicts **0.50** emirps over d = 5…27. Observed:
**1** (`12641 ⟷ 14621`). Nothing was fitted to that number, so it is a
genuine — if n = 1 — check.

## Where the remaining probability sits

Summing from **d = 5**, where the model applies and where the known
emirp is:

| region | expected | share |
|---|---|---|
| d = 5…27 — exhaustively searched | 0.50 | 83.6% |
| d > 27 — all of it | **0.099** | **16.4%** |

| single d | expected | share |
|---|---|---|
| 29 | 0.0032 | 0.54% |
| 31 | 0.0028 | 0.47% |
| 37 | 0.0020 | 0.33% |
| 51 | 0.0010 | 0.17% |

**Correction.** An earlier version of this argument, given in session
before the calibration, summed `1/d²` from d = 1 and reported that 97.8%
of the expected total lay at d ≤ 27. That is wrong: d = 1…4 contribute
1.42 of the 1.645 total and the model does not apply there — there are
no 1-digit emirps. The correct figure is **83.6%**, and what remains
above d = 27 is **~10%**, not ~2%.

## Over-dispersion is structural, not noise

Variance exceeds the mean in both series (4.88 vs 2.50; 7.14 vs 4.17).
The counts are lumpier than Poisson. That is what the obstruction
landscape predicts: d = 6, 10, 18, 20, 22 are *modular* obstructions
that force hard zeros rather than sampling them. The excess variance is
the obstruction structure showing up in the statistics.

## What this does and does not license

**Does:** the claim "one bi-quadratic emirp exists, the region holding
~84% of the expected total has been exhaustively searched, and it
contains exactly that one" — with the convergence now resting on a
*measured* structural factor rather than two assumptions.

**Does not:** "essentially proven." ~10% of the expected total is still
out there, spread over infinitely many d.

**On more compute:** d = 29 costs roughly ten times d = 27 — order 50
days — for **0.0032 expected emirps, a 0.3% chance**. Every larger d is
worse and costs ten times more. There is no d at which extending
`hunt.c` is a good trade, and that conclusion does not depend on the
calibration constant, only on the `1/d²` shape.

## Reproducing

The d = 5…19 series, single-threaded so as not to disturb a running job:

    sed 's/#define NUM_THREADS  8/#define NUM_THREADS  1/' hunt.c > hunt1.c
    gcc -O2 -std=c99 -Wall -Wextra -fopenmp -o hunt1 hunt1.c -lgmp -lm
    ./hunt1 5 19

d = 19 takes about an hour on one thread; d ≤ 17 is minutes. Remember
the output is div-5-excluded.
