# Emirp resweep d=13..27 — single-binary provenance

**Status: COMPLETE 2026-09-11. No bi-quadratic emirp at any d in
13..27, all on one binary.**

## Why this run existed

The d=13..27 emirp results had accumulated over months across several
builds of `hunt.c`. Each individual result was fine; the *chain* was a
patchwork, and "no emirp below 28 digits" is a claim about the chain,
not about any one leg. This resweep re-ran every length on a single
byte-verified binary so the bound rests on one artifact.

## The binary

`./hunt`, built 2026-09-03, `md5 24d279f4535e…`, from `hunt.c` at
commit `2d1948a`.

The working tree's `hunt.c` differs from `2d1948a` — commit `45a8ae7`
replaced three em-dashes with `--` **in comments only**. That is a
whitespace/comment change, so per the standing rule it was not taken on
faith: both versions were compiled with the production flags and the
generated assembly compared.

    md5 561cd56aac99af7aa233bcc685377f6b   (2d1948a)
    md5 561cd56aac99af7aa233bcc685377f6b   (HEAD)

Byte-identical. `curve_gmp.h` is unchanged from `2d1948a`. The current
source therefore reproduces the swept binary exactly, and d=19..21 was
run on the existing binary rather than a rebuild.

## Results

| d | range | raw | pals | prime-eligible | EMIRPS | wall |
|---|---|---|---|---|---|---|
| 13 | 1528961 | 4 | 2 | 4 | 0 | 0.03 s |
| 14 | 4835000 | 6 | 0 | 6 | 0 | 0.05 s |
| 15 | 15289612 | 5 | 3 | 5 | 0 | 0.27 s |
| 16 | 48349998 | 1 | 0 | 0 | 0 | 0.62 s |
| 17 | 152896120 | 1 | 1 | 1 | 0 | 3.18 s |
| 18 | 483499983 | 0 | 0 | 0 | 0 | 6.56 s |
| 19 | 1528961196 | 0 | 0 | 0 | 0 | 29.47 s |
| 20 | 4834999835 | 0 | 0 | 0 | 0 | 67.63 s |
| 21 | 15289611963 | 6 | 4 | 6 | 0 | 345.48 s |
| 22 | 48349998344 | 0 | 0 | 0 | 0 | 748 s |
| 23 | 152896119631 | 2 | 0 | 2 | 0 | 3898 s |
| 24 | 483499983437 | 1 | 0 | 0 | 0 | 8103 s |
| 25 | 1528961196313 | 5 | 4 | 4 | 0 | 33278 s |
| 26 | 4834999834365 | 4 | 0 | 2 | 0 | 80449 s |
| 27 | 15289611963133 | 4 | 3 | 3 | 0 | 401478 s |

d=19..21 were the last hole: they had been resting on a June build.
Filled 2026-09-11 (~7 min total).

## The null result is earned, not assumed

`EMIRPS=0` everywhere is the headline, and a sweep that can only report
zero has no way to say it was working. Three things say it was:

1. **Positive control within the run.** The sweep is demonstrably
   finding things — 6 survivors and 4 palindromes at d=21, 4 and 3 at
   d=27. A silent sweep would show zeros everywhere.
2. **Known-answer control for the refilled legs.** d=19..21 had June
   results from a *different* binary. `prime-eligible` and `EMIRPS`
   match exactly at all three. The raw counts drop (3→0 at d=19, 7→6
   at d=21) because this build skips n where p is divisible by 5 —
   and that is confirmed, not hand-waved: `palsplit --keep5` finds
   exactly 3 div-5 palindromes at d=19 and exactly 1 at d=21.
3. **Independent method.** `palsplit` (head/tail split search, shares
   no enumeration strategy with `hunt`) reproduces the palindrome
   counts: 0 at d=19, 4 at d=21 — the latter identical to the stored
   `pals_d21.txt`. Likewise 3 at d=27, matching `pals_d27.txt`.

## Known limitation of the record

The d=13..27 legs were run through a wrapper whose `grep` was the only
consumer of `hunt`'s stdout, so their per-hit lines were discarded at
the pipe. The counts are intact and the emirp result was never at risk
(`*** EMIRP ***` matched the filter), but the *values* for those legs
are not in the resweep log. They are recoverable in seconds with
`palsplit`, and the d=27 set is in `pals_d27.txt`. Fixed for all future
legs by `scripts/resweep_leg.sh`, which tees the full stream to
`logs/raw/` and filters only the view. d=19..21 above were run under
the fixed script and their raw captures exist.

## Runtime model

Wall time per leg scales by **parity**, not a uniform √10 — see
runbook §19. Predictions for this run: d=19 32.5 s (actual 29.47),
d=20 68.7 s (actual 67.63), d=21 340.5 s (actual 345.48), d=27
92–116 h (actual 111.5 h).
