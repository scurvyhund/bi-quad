# Session Handoff — 2026-05-26

Wrap-up of the nitroII debugging session. Pick up tonight on nitroIII.

## Machines
- **nitroII** — Intel i7-10510U dev box. Claude Code ran here; this repo copy lives here.
- **nitroIII** — AMD Zen2 compute box (Makefile targets `-march=znver2`). Runs production
  inside `screen`. Was running a ~6-week `d=21 / k=12` job logging to
  `run_clean_optB.log` (pid 765788). That run was killed during this session and the
  log was **not** captured.

## Three bugs found and fixed in `mod_obstruct.c`

### 1. Integer overflow in the survivor counter
`int survivors` overflows at k≥10 where counts exceed `INT_MAX` (2,147,483,647).
Observed value at d=21/k=12 was `-1074376056` (a wrap).

- **Knock-on:** the saturation test `survivors == mod` can never be true once
  `mod > INT_MAX`, so saturation is never detected → the skip never fires → the run
  grinds through every `d` up to `max_d` (this is the "d=222" march that was observed,
  and the slowdown).
- **Recovered d=21 count:** `-1074376056 + 2^32 = 3,220,591,240` (~3.22e9). Valid because
  the true count is < 2^32, so the wrap is unambiguous. **The 6 weeks of compute is not
  lost — it was a display artifact.** Caveat: this is the **dirty** count (pre-filter);
  a clean re-run will be lower but still far from an obstruction (3.22e9 ≫ 0).
- **Fix:** `int survivors` → `long survivors`; printf `%6d` → `%6ld`.

### 2. Missing prime-eligibility (composite-5) filter — the run was "dirty"
`p = 2n²+2n+1` is always odd, so it can only end in **{1, 3, 5}**. Endings in `5` are
divisible by 5 → composite → never prime → can never be part of an emirp pair.
The bitset admitted them anyway. **Exactly half of all achievable endings end in 5**
(oracle: 50/106, 500/1044, …), so the dirt inflated survivor counts and *masked real
obstructions*.

There were **two** holes, not one:
- **q-side** — `valid_firsts` is built from `reverse_k(valid endings)`, and Phase 3 checks
  `q_ending` against the bitset. Cleaning the Phase-1 bitset cleans this path.
- **p-side** — `p_ending` (p's own last-k digits) was computed at the top of the Phase-3
  loop but **never validated**. A residue where *p itself* ends in 5 was still counted.
  This was *not* covered by the original session-ref plan (which only patched Phase 1).

- **Fix:**
  - Phase 1: `if (e % 10 == 5 || e % 10 == 0) continue;` before admitting an ending.
    (Also removed a stray flood `printf("\te = %ld, n = %ld")` that would dump 10^k lines.)
  - Phase 3: `if (!BITSET_GET(is_valid_ending, p_ending)) continue;` right after computing
    `p_ending`, mirroring the existing `q_ending` check.

### 3. Saturation level wrong after filtering
With the filter, the maximum possible survivor count is **`3·mod/5`**, not `mod` — only
3 of every 5 residues have a clean `p_ending` (last digit of `2r²+2r+1` is 1 or 3 for
`r ≡ 0,2,4 (mod 5)`). The plateau was confirmed at exactly `3·mod/5` for k=3..7
(600, 6000, 60000, 600000, 6000000). `10^k` is divisible by 5 so `3*mod/5` is exact.
Without this fix the filtered run never detects saturation and never skips.

- **Fix:** `survivors == mod` → `survivors == sat_level` (where `sat_level = mod/5*3`),
  at both the `(saturated)` tag and the skip site.

## Validation done (nitroII, `-march=native`, k ≤ 7, fast)
- Clean build, **zero warnings** (also compile-clean with production `-march=znver2`).
- `valid_endings` = **56 / 544 / 5424 / 54176 / 541696** — exact match to an independent
  Python oracle (achievable endings minus those ending in 0/5).
- **No negative survivors**; no flood debug lines.
- Filter is monotone (clean ≤ dirty everywhere) and **unmasks real obstructions**:
  e.g. k=5 d=9 went `6 → 0`; obstruction counts rose (k=6: 1→4, k=7: 1→4). All previously
  reported obstructions are preserved.
- Saturation detected at `3·mod/5` and the skip fires correctly.

## TODO tonight (on nitroIII)
1. Sync this commit to nitroIII (`git pull` if pushed, or `scp mod_obstruct.c`).
2. `make` on nitroIII (uses znver2) and restart the run.
3. The filter **changes results** — prior dirty runs need re-running for clean numbers.
   Note the obstruction payoff is in the small-count deep-tail `d`s, not huge-count `d`
   like 21, so prioritise accordingly.
4. Decide: record the recovered dirty d=21 count (3,220,591,240) or recompute clean.
5. Optional rigor: build a full-pipeline oracle for small k to validate survivor counts
   beyond `valid_endings` (currently validated via the endings oracle + monotone diff).
6. Ignore the `mod_obstruct_test` binary in this dir — it's Intel-arch, nitroII-only.

## Notes
- nitroII→nitroIII passwordless SSH was *not* completed (key generated on nitroII, host
  key for nitroIII added to known_hosts, but the public key was never authorized on
  nitroIII). Not needed if Claude Code runs directly on nitroIII tonight.
- The earlier `mod_obstruct_session_ref.txt` (prior session) described bug #2's Phase-1
  fix; that plan is now implemented **and extended** (p-side hole + saturation level).
