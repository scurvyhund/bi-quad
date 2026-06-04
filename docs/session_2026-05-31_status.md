# Session Status — 2026-05-31

> ⚠️ **Superseded by 2026-06-03 correction:** any "saturation at d=2k+2" or
> obstruction-ceiling claim here is an ARTIFACT of an interval bug (fixed in
> commit 22a7121). See `docs/modular_obstruction_design.md` correction notice
> and project memory `d21_range_mod_cliff`. Results at d ≤ 2k remain valid.

## Summary
Synced fixed code from nitroII to nitroIII, rebuilt, and restarted d=21 run with integer overflow fix.

## Key Fix Applied
**Commit 47917ff** — "Fix survivor-count overflow, add composite-5 filter (both sides), correct saturation level"

Three bugs fixed and committed:
1. **Integer overflow:** `int survivors` → `long survivors` (overflowed at k≥10)
   - Previous d=21 showed `-1,074,376,056` (wrapped value)
   - Recovered clean count: 3,220,591,240
2. **Missing composite-5 filter:** Added p-side check to eliminate numbers ending in 5 (divisible by 5 → composite)
3. **Saturation level:** Corrected to `3·mod/5` (was `mod`)

## Current Status: d=21 Rerun (Clean)
- **Command:** `./mod_obstruct_optB 50 10 10 21`
- **Log:** `logs/run_k10_d21_fixed_v2.log`
- **Started:** 2026-05-31 18:27 AKDT
- **Monitor:** Task b4mk3vixg — reports progress every 5 min + final elapsed time
- **Expected:** Correct survivor count (likely OBSTRUCTION or moderate survivors)

## Baseline Results (k=10, d=11..20)
From `logs/run_clean_optB_fixed.log` (clean run, all filters applied):
```
d=11: OBSTRUCTION
d=12: OBSTRUCTION
d=13: 4 survivors
d=14: 6 survivors
d=15: 5 survivors
d=16: OBSTRUCTION
d=17: 1 survivor
d=18: OBSTRUCTION
d=19: OBSTRUCTION
d=20: OBSTRUCTION
```

Saturation expected at d=21+ (survivors would be ~6B, at the 60% cap).

## Commits & Status
- **Branch:** master_dev @ 47917ff (3 fixes, all validated)
- **Uncommitted:** Minor changes (Makefile debug targets, helper programs) — not blocking
- **Build:** Clean, zero warnings, `-march=znver2` production target

## Documentation
- `docs/nitroIII_rescue_handoff.md` — nitroIII boot/rescue context
- `docs/session_2026-05-26_handoff.md` — detailed bug analysis & validation
- `docs/modular_obstruction_design.md` — architecture & design notes
- Memory system: `/home/jim/.claude/projects/.../memory/` — full context preserved

## Next Steps
1. **Monitor d=21 completion** (expected 12-48 hours)
2. **If successful:** Continue to d=22, d=23, etc.
3. **If d=22+ slow:** Consider native `__int128` optimization in inner loops
4. **Pending:** consec_sqr optimization for cvpipe (separate project, not urgent)
