# Distributed run — who runs what (2026-06-05)

Two machines advancing the two frontiers in parallel, no core contention.

| Box | CPU / OS | Job | Tool | Range | Notes |
|-----|----------|-----|------|-------|-------|
| **nitroIII** | AMD Zen2, Fedora 42, 8c | **bi-quadratic emirp** brute force | `hunt.c` | d=25 → d=26 (auto) | d=24 done (EMIRPS=0); d=25 running, then auto-launches d=26 (~52h). |
| **nitroII** | Intel, BunsenLabs/Debian | **prime palindrome** hunt | `palhunt_gmp` | d=8 → d=27 | Build `-march=native`; runtime ~3×/digit (d≤25 hours, d=26–27 days). Extends/confirms the conjecture. |

## Repo topology (after 2026-06-05 tidy)
- **GitLab** `origin` = `git@gitlab.com:scurvyhund/bi-quad.git` — `master_dev` = active line.
- **Codeberg** = `git@codeberg.org:scurvyhund/bi-quad.git` — `master_dev` = active line (same).
  Old pre-divergence line preserved as tag `archive/codeberg-master_dev-2026-03`.
- Clone from either remote → active line by default. Push: `git push origin master_dev`
  and `git push codeberg master_dev`.

## Current results
- **Emirps:** `12641 ⟷ 14621` (d=5) is the ONLY bi-quadratic emirp through 24 digits
  (brute-verified, every n). d=25 pending → will extend to 25.
- **Palindromes:** conjecture (Jim, ~1997) — `3187813` is the largest prime palindrome
  on the curve. **nitroII (`palhunt_gmp`) has now GMP-certified d=8…25 with `found=0`**
  — independently re-confirming d≤21 (previously sieve-only) and *extending* the
  confirmation to **25 digits**; d=26 running (then d=27). So `3187813` remains the
  largest through 25 digits. A `*** PRIME PALINDROME ***` at d>7 would refute it.

## Pending
- cvpipe optimization (consec_sqr before Miller–Rabin) — to do together; in
  `gmp-cvo/cvpipe.c`, not bi-quad. See [`session_2026-06-05_emirp_d5_correction.md`](session_2026-06-05_emirp_d5_correction.md)
  for the emirp correction record.
