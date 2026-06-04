# Session Archive — 2026-06-03: The range≥mod Cliff & Correctness Fix

> **Status: canonical record of this session.** Supersedes the earlier
> `session_2026-06-03_completion.md` and `session_2026-06-03_full_summary.md`
> (both written mid-session, before the root cause was found — they carry the
> now-retracted d=2k+2 saturation framing).

---

## TL;DR

What began as "resume the slow d=21 k=10 run" ended with discovering that the
obstruction algorithm had a **fundamental correctness bug**, not just a
performance problem. The per-residue feasibility test was only valid when
`range = n_max − n_min < mod = 10^k`. Past the cliff where `range ≥ mod`
(roughly **d ≥ 2k+1**), it both (a) over-counted survivors massively and
(b) ran for weeks per d. The over-count hit a *fake* saturation level that
**silently stopped the search** — so the project's headline conclusion
("obstructions exist only for d up to ~2k", "saturation at d=2k+2") was an
**artifact**, never a real result.

Fixed (commit `22a7121`), validated against an independent brute force, and all
affected notes corrected (commit `3ed786e` + memory + session-doc pointers).

---

## How we got here (the investigation, in order)

This is worth preserving because each step was a real, separately-diagnosed
problem — the bug hid behind two layers of instrumentation blindness.

1. **The "resumed" run wasn't what our notes said.** The PID in the prior
   handoff (67657) was stale; the live run was PID 82561, started 13:28. Its log
   went **silent at 13:54 yet burned ~34.8 CPU-hours** pinned at ~500% CPU until
   we killed it at 20:26. 3 of 8 threads had finished and gone idle; 5 were
   buried.

2. **First fix — load imbalance + blind checkpoints.** The rebuild used
   `schedule(static)` (fixed 1/8 blocks) and a checkpoint keyed on per-thread
   loop-local `r`. Per-residue cost is wildly skewed, so static scheduling
   buried whichever thread owned the costly region while the checkpoint went
   dark. → switched to `schedule(dynamic, 100000)` + aggregate/heavy heartbeats.

3. **Second fix — heartbeat thresholds too coarse.** Re-ran; still no output for
   20+ min on all 8 cores. The count-based heartbeats (flush every 2M iters,
   print every 100M) never fired because a single residue was costing >4.5 ms —
   a thread needed *hours* to reach a flush. → replaced with a **wall-clock
   (20 s) heartbeat** reporting heavy-rate and position.

4. **Third time — the heartbeat STILL didn't fire**, which was the real clue.
   It forced the question: *why is a single residue costing milliseconds?* That
   led to the root cause.

## Root cause — the range≥mod cliff

The Phase 3 check approximated the achievable first-k prefixes of `p=2n²+2n+1`
for a residue `r` as the **interval** `[prefix(first_n), prefix(last_n)]`. That
is exact **only when a residue has ≤1 n-value in range**, i.e. `range < mod`.

`range/mod` by digit count (it crosses 1 at the cliff; constant factor ≈2.16):

| (example) | range/mod | regime |
|---|---|---|
| d ≤ 2k−1 | < 1 | ≤1 n-value/residue → interval is a point → exact & fast |
| **d = 2k+1** | ≈ 2.16 | **cliff** — 2 n-values, far apart in prefix |
| d = 2k+2 | ≈ 6.8 | well past — multiple n-values |

At k=10 the cliff is **d=21**; at k=6 it's **d=13** (same range/mod=1.529 — k=6
d=13 is the exact small-scale analog of k=10 d=21, which is how we validated).

Past the cliff, a residue's 2+ n-values are `mod` apart, so their p-prefixes are
far apart, and the interval `[fk_min, fk_max]` spans a huge band:

- **Correctness:** the band includes prefixes **no real n produces**, counted as
  achievable → survivors over-counted → count hits the fake `sat_level=3·mod/5`
  → triggers the saturation early-break → **search stops**. Bogus.
- **Speed:** the inner loop sweeps millions of `valid_firsts` per residue, each
  calling `solve_residues` (Hensel). ~weeks per d.

## The fix (commit 22a7121)

Enumerate the **actual** n-values per residue (`t_first, +mod, +2·mod, … ≤
n_max`) and test each **exact** prefix via point membership
(`sorted_has_value_in_range(arr, len, f, f)`). The q-side likewise enumerates
actual m-values and tests `mk == q_first` instead of an interval. For
`range < mod` this collapses to the identical single-point check, so **all
prior d ≤ 2k results are unchanged.** Removed dead `t_last`, `t_mlast`,
`last_n_with_residue()`. Also kept the wall-clock heartbeat + dynamic scheduling.

## Validation (independent ground truth)

A standalone Python brute force (enumerates real n, builds the realized
`(first_k, last_k)` pair set, counts survivor residues — completely independent
of the interval logic) matched the fixed code **exactly**:

| d (k=6) | old (buggy) | fixed code | brute-force truth |
|---|---|---|---|
| 12 (range<mod) | 0 | 0 (obstruction) | 0 ✓ |
| 13 (range>mod) | 322,052 | **8** | **8** ✓ |
| 14 (range>mod) | 600,000 "sat" | **30** | **30** ✓ |

Also: clean build (zero warnings); k=6 d=11/d=12 still OBSTRUCTION; `|VE|`=54176
matches the program's valid-endings count.

## Corrected landscape (k=6, fixed code)

```
d=11:   0  (OBSTRUCTION)     d=15:   269
d=12:   0  (OBSTRUCTION)     d=16:  2494
d=13:   8                    d=17: 25292
d=14:  30                    (~10× growth per digit; NO 2k+2 saturation)
```

The real picture: survivors past the cliff are tiny and grow smoothly — nothing
like the instant fake-saturation the old code reported.

---

## What this overturns (open questions for review)

1. **"Obstructions only exist for d up to ~2k" — RETRACTED.** This was the proof
   strategy's backbone. With correct counting, survivor counts past the cliff
   are small but nonzero and we have no evidence they can't return to zero
   (an obstruction) at higher d. The reachable-d ceiling is **open again**.
2. **"Saturation at d=2k+2" / "pre-saturation ratio ~59.16%" — artifacts.** The
   59.16% was the over-count signature, not a feature.
3. **The k=10 numbers we DO have (d=11–20) are still valid** — all below the
   d=21 cliff. Only the never-completed d=21+ was ever affected by this bug.
   (Separately, the Phase-3 "composite-p" caveat from the Option-B work is still
   open for the small d=11–17 counts — unrelated to this bug.)
4. **Cost caveat going forward:** exact enumeration costs ~`range/mod` n-values
   per residue, so very high d (range/mod ≫ 10⁴) gets slow again. The transition
   region around obstructions is tractable; pushing far past it needs more work.

## Deferred next steps (agreed, not yet done)

- Map the corrected landscape cheaply at k=6/7/8 to see the true trajectory and
  where (if ever) real saturation occurs.
- Re-run k=10 d=21+ with the fixed binary (now ~1 hr-tractable for d=21).
- Re-validate the old k=10 d≤20 results (expected unchanged) and pinpoint where
  the old run hit fake saturation and stopped.

## Commits (branch master_dev)

- `22a7121` — Fix interval over-approximation that broke correctness at range≥mod
- `3ed786e` — docs: retract d=2k+2 saturation results as interval-bug artifacts

## Artifacts

- Code: `mod_obstruct.c` (fixed)
- Brute-force validator: was at `/tmp/brute_validate.py` (regenerate from the
  recipe in the `d21_range_mod_cliff` memory if needed)
- Project memory: `d21_range_mod_cliff`, `d21_stall_postmortem`, updated
  `project_biquad`
