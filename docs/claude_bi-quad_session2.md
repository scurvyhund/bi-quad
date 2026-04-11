# Claude Code Session 2 — k=10 Status Check

**Date:** 2026-03-31
**Model:** Claude Opus 4.6 (1M context)

## Current Run Status

| Field      | Value                                          |
|------------|------------------------------------------------|
| PID        | 7028                                           |
| Command    | `./mod_obstruct 50 10 10 21`                   |
| Started    | 2026-03-19 22:50 (resumed at k=10, d=21)      |
| Wall time  | 11 days, 15 hours                              |
| CPU time   | 92 days (8 threads, ~100% utilization)         |
| Memory     | ~9 GB RSS                                      |
| Status     | Computing d=21 — no output yet                 |

## Confirmed k=10 Results (through d=20)

| d   | Survivors       | Notes                              |
|-----|-----------------|------------------------------------|
| 11  | 5               |                                    |
| 12  | 2               |                                    |
| 13  | 4               |                                    |
| 14  | 6               |                                    |
| 15  | 6               |                                    |
| 16  | 2               |                                    |
| 17  | 1               |                                    |
| 18  | **0 (OBSTRUCTION)** | Persists from k=9              |
| 19  | 3               | Was 591,606,462 at k=9             |
| 20  | **0 (OBSTRUCTION)** | Was SATURATED at k=9 — new!    |
| 21  | *computing...*  | ~2-5 days remaining                |

## Why d=21 Is the Bottleneck

At k=10, the inner loop iterates over **10 billion** residue classes
per d value. For d ≤ 20, most residues were eliminated cheaply:

- **Small d (e.g., d=11):** 99.999% of residues have no
  representatives in [10^10, 10^11−1] — instant skip.
- **Obstruction d values:** Most residues fail early filter checks
  (valid_firsts range, q_first prefix) — fast rejection.

For **d=21**, the range [10^20, 10^21−1] has ~9×10^10 values per
residue class. No residues skip. Each of the 10 billion must go
through full GMP arithmetic + Hensel lifting. This is the
saturation boundary — the single most expensive d value in the run.

## Completion Estimate

- Original estimate: ~12+ days for d=21
- Elapsed: 11.6 days — tracking the estimate closely
- **Remaining: ~2-5 days**

Once d=21 completes:
- If it **saturates** → d=22..50 skipped → k=10 done
- If it shows **few survivors** → remaining d values compute fast
  (minutes/hours each) → potential to push obstructions much further

## Significance

### Independent Verification of cvpipe
The obstructions at d=10, d=18, and d=20 are **mathematical proofs**
that no d-digit bi-quadratic emirp pairs exist. This independently
confirms cvpipe's negative results from a completely different
algorithmic angle.

### The Squeeze Is Tightening
The collapse from k=9 → k=10 is dramatic:

| d  | k=9 survivors  | k=10 survivors | Change            |
|----|----------------|----------------|-------------------|
| 17 | 1              | 1              | Stable            |
| 18 | **0**          | **0**          | Obstruction holds |
| 19 | 591,606,462    | 3              | 99.9999995% drop  |
| 20 | SATURATED      | **0**          | Complete collapse |

This pattern has not been seen at any prior k value. The saturation
boundary (previously at d=2k+2) may be breaking down entirely.

### Checkpointing Gap
No mid-d checkpointing exists. If the process dies during d=21,
the ~12-day computation restarts from scratch. Results through d=20
are safe on disk. Future runs should consider chunked checkpointing
for expensive d values.

## What Comes Next

See session notes for discussion of forward strategy depending on
the d=21 outcome.
