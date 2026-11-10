# cvpipe logs cross-checked against the hunt resweep (2026-11-10)

**Result: every usable `cvpipe` log agrees exactly with `hunt`, d=2..27.**

Reproduce with `python3 docs/cvpipe_xcheck.py` (exit 0 = agree).
A control log with one count altered made it report DISAGREE and exit 1.

## Why this was needed

The resweep (`docs/resweep_d13-27_provenance.md`) puts the emirp bound on one binary.
That removes the patchwork of builds, but a logic bug in `hunt` would then be present at every d.
Its independent check, `palsplit`, only finds palindromes, so it cannot reveal a missed non-palindrome pair.
And at even d, `hunt` skips the n with p mod 11 = 1 or 2, relying on the theorem in `docs/mod11_converse_constraint.md`.

`cvpipe` (in `gmp-cvo/`) is a separate program with its own reversal, curve test and filters.
It has no mod-11 wheel.

## Method

An emirp's p must start with 10, 12, 14, 16, 18 or 31 and end in 1 or 3.
The script confirms this from the curve's last-two-digit residues:
the prime-viable endings 01, 13, 21, 41, 61, 81, reversed, are exactly those six leading pairs.
`cvpipe` searches only those n, so skipping the rest cannot lose an emirp.

That gives an exact prediction for each run, computed in Python from the n values:

- `Consec-sq pre-filter` = non-palindrome survivors passing that filter
- `Palindromes found` = palindromic survivors passing it

Survivor values come from every hit line in `logs/`, plus a local brute force of all n < 7,000,000.
The harvested values reproduce the resweep's raw and palindrome counts at every d = 13..27.

## Results

| Log | d | Consec-sq, actual / predicted | Palindromes, actual / predicted | Zones complete |
|---|---|---|---|---|
| `10e25_run_with_bugFix.txt` | 2..25 | 24 / 24 | 19 / 19 | yes |
| `run_10e26.log` | 2..26 | 26 / 26 | 19 / 19 | yes |
| `run_10e23.log` | 23 | 2 / 2 | 0 / 0 | yes |
| `run_10e24.log` | 24 | 0 / 0 | 0 / 0 | yes |
| `run_10e25.log` | 25 | 0 / 0 | 4 / 4 | yes |
| `run_10e27.log` | 27 | 0 / 0 | 3 / 3 | yes |

"Zones complete" means the run's zones bracket the true n-range of every digit length and leading pair, and candidates generated equals zone n-values.

## Per-digit pass on the fixed binary (2026-11-10)

The two runs above match d=13..22 only as part of their totals, so each digit length was re-run on its own, using the binary built from `gmp-cvo` `5b9974f` (md5 `5d750a6d`).
Predictions were written down before the run.
Raw logs and `SHA256SUMS` are in `logs/preserved/cvpipe_d13-22_2026-11-10/`.

| d | 13 | 14 | 15 | 16 | 17 | 18 | 19 | 20 | 21 | 22 |
|---|---|---|---|---|---|---|---|---|---|---|
| Consec-sq, actual / predicted | 2 / 2 | 6 / 6 | 2 / 2 | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 | 2 / 2 | 0 / 0 |
| Palindromes, actual / predicted | 2 / 2 | 0 / 0 | 3 / 3 | 0 / 0 | 1 / 1 | 0 / 0 | 0 / 0 | 0 / 0 | 4 / 4 | 0 / 0 |
| Emirps | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| Prime palindromes (Otto primes) | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |

All ten runs exited 0 and walked every zone n-value.
The nonzero rows are the positive control: this run found what it was expected to find.

## What this establishes

- **A second enumerator agrees**, including on non-palindrome pairs, which `palsplit` could not check.
- **Even d does not rest on the theorem alone through d=26.**
  `cvpipe` examined the p mod 11 = 1, 2 classes that `hunt` skips, wherever an emirp could lie, and found nothing beyond the prediction.
  At d=27 the question does not arise: the mod-11 constraint applies only at even d.

The bound can be stated as: no bi-quadratic emirp for d=13..27, found by exhaustive `hunt` and matched by `cvpipe`.

## Limits

- The February-March 2026 `cvpipe` binaries were not preserved, so the d=23..27 logs cannot be tied to a checked build. d=13..22 can, through the per-digit pass.
- Both engines share GMP, the author and the machine.
- `run_10e28.log` covers d=28 and reports 0 consec-sq and 0 palindromes.
  **This is not a claim of no emirp at d=28.** No `hunt` run exists there, so the agreement the script prints for that row has nothing behind it, and the verification rule in `CLAUDE.md` requires brute-force confirmation.

## Logs that must not be cited

`run_10e22.log` and all six runs in `10e25_to_10e30.txt` hit a silent truncation.
`cvpipe_nitroIII_10e25-10e26_zones1-6.log` holds only a failed launch, so it records no search at all.
With `MAX_ZONES` at 64, a run from n=0 stopped partway through d=12: 64 zones and 63,680 n-values, whatever `Max prime` said.
Those logs still print the requested limit and find 12641, so they look healthy.

Fixed in `gmp-cvo` commit `5b9974f`: `MAX_ZONES` is back to 256, enough for every d that `uint64_t` n can reach, and running out of zones now exits 1 instead of dropping them.

| Check on the fix | Old source | New source |
|---|---|---|
| 1e14: zones, consec-sq, palindromes | 64, 10, 5 | 79, 18, 7 (as predicted) |
| 1e15: zones, consec-sq, palindromes | 64, 10, 5 | 85, 20, 10 (as predicted) |
| 1e8 and 1e10, below the cap | | output identical |
| Build with cap forced to 10 | | error message, exit 1 |

Smallest limits (1, 2, 99, 100, 101) and range mode also ran cleanly.
Valgrind shows no definite leaks, and the same OpenMP thread-stack noise before and after.

## A stray line in a record file

`logs/raw/hunt_d13_2026-09-11.raw` also holds `n=999 p=12641` and `range=100 ... EMIRPS=1`.
Since 999 does not generate 12641, this is not real `hunt` output; it may be left over from testing a wrapper script (not confirmed).
The genuine d=13 lines in the file are correct.
The script reports and excludes that n, and the file is left untouched as a record.
