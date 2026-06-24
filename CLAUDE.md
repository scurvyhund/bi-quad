# bi-quad / BigFermat — Claude Standing Instructions

## Project Overview

Searches for bi-quadratic emirp pairs (p, rev(p)) and prime palindromes
on the curve p = 2n² + 2n + 1 = n² + (n+1)².

The ONLY known bi-quadratic emirp: 12641 ↔ 14621 (d=5, n=79/85).
Jim's palindrome conjecture: 3187813 (d=7) is the largest prime
palindrome on this curve (held since 1997; confirmed to d=21 by sieve).

The emirp sieve and palindrome search are the SAME sieve — a modular
obstruction kills both. See docs/ for the full technical picture.

## Repo / Push Convention

Three remotes — push ALL three on every commit:

    git push origin master_dev    # GitLab (canonical)
    git push github master_dev
    git push codeberg master_dev

## Build

    gcc -O3 -march=znver2 -mtune=znver2 -std=c99 -Wall -Wextra \
        -fopenmp -o mod_obstruct mod_obstruct.c

## Mandatory Sanity Check Before Any Long Run

After ANY rebuild or code sync, run the k=6 sanity test FIRST:

    ./mod_obstruct 50 6 6 11

Verify output includes:
- Header with valid_endings count
- At least one "d=XX survivors = ..." line
- Completion summary

Regression markers (fixed-code values, not the buggy old ones):
- d=11: OBSTRUCTION, d=12: OBSTRUCTION
- d=13: 8 survivors, d=14: 30 survivors

ONLY start long runs after this passes. Skipping cost 22 CPU-days
(May 2026 run — synced code printed a header but never checkpointed).

## Checkpointing — Mandatory for k≥9 and d>15

Any run at k≥9 or where individual d values take >30 minutes MUST
emit progress checkpoints (every 100M residues to stderr). No output
= no way to detect a stall until hours are wasted. Standard pattern:

    if (r > 0 && r % 100000000 == 0)
        fprintf(stderr, "  d=%2d: checked %ld / %ld\n", d, r, mod);

## Save Context Before Long Runs

Save any new findings or plans to memory BEFORE kicking off a long
computation. Session context is lost if the run outlasts the session.

## Verification Rule

Never declare "no emirp at d=X" from the fast engines alone.
Always cross-check with the brute-force pair:

- bi-quad/hunt.c — exhaustive GMP brute; enumerates every n
- bi-quad/docs/brute_validate.py — independent Python ground-truth

Fast engines (suspect until verified):
- mod_obstruct.c — has had real bugs (interval over-approx; d=21 cliff)
- gmp-cvo/cvpipe — internally consistent but high-d runs not always
  independently brute-verified

## Critical Algorithm Constraint: range < mod

mod_obstruct is valid and tractable ONLY when range < mod (at most
one n-value per residue). The cliff by k:

- k=10 (mod=10^10): valid d≤20; cliff at d=21 (range/mod=1.53)
- k=6  (mod=10^6):  valid d≤12; cliff at d=13 (range/mod=1.53)

Past the cliff the old interval method OVER-COUNTS (wrong results)
AND is slow (weeks). Fix committed 22a7121: enumerate actual n-values
per residue (1-2 at the cliff) and do exact point checks.

If regression fails (d=13 returns 322052 or "saturated") the binary
is the old buggy version — do not proceed.

## Ruled-Out Structural Attacks — Do Not Revisit

Three approaches rigorously tested and eliminated (2026-06-04).
Do NOT re-propose without a genuinely new idea:

1. MITM (split n = a·10^t + b) — no √-speedup; cross-term couples
   the middle ~d/2 digits. Probe archived at commit 7f33475.
2. Two-ended digit-DP — midpoint survivors ~0.023·range ≈ 10^(d/2);
   brute force in disguise, not a speedup.
3. Congruence obstruction — 50 moduli, d in [8,30]: ZERO obstructions.
   Real obstructions are non-congruential/sporadic.

Full writeup: docs/structural_attacks_2026-06-04.md.

## Runbook Convention

At session wrap-up, append genuinely reusable CLI commands to
docs/runbook.txt (curated, grouped by section, ≤80 cols). Commit
and push all three remotes. Skip one-off probes and typos — only
add commands worth re-running. Cross-project universals go to
~/notes/cmdline_gold.txt (or both).

## Communication Style

- Separate PROVEN results from TREND/hypothesis explicitly.
- Name live risks; don't oversell results.
- Frame hard-won findings with earned weight — the rigor matters.
- Call out immediately (don't silently comply) when Jim is going
  overboard, scope-creeping, or overcomplicating. Name the concrete
  downside and offer the simpler path. Bluntness is welcome.
