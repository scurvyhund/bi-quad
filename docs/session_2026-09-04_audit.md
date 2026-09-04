# Correctness audit — 2026-09-04

A read-first audit for off-by-one, rounding and unintended-overflow
defects, run *while the d=29 palbrute sweep was live*. That constraint
set the method: reading is free, so almost all of this is reading.
Nothing was rebuilt in `bi-quad` — `make` here would relink `palbrute`
under the running process and leave it executing a deleted inode.

**No defect was found in any code path behind a published result.** The
two findings below are latent: both require inputs that have never been
used.

## Verified correct

These were checked closely enough to be worth recording, so the next
audit need not redo them.

- **`isqrt_u128` (`curve.h`) is exact regardless of its seed.** The two
  adjust loops compare with `x > v / x` and `x + 1 <= v / (x + 1)`,
  which decide `x² > v` without ever forming `x²`. The `sqrtl` seed and
  the six Newton steps are pure acceleration; deleting them would leave
  the result unchanged. This is the construction that removed floating
  point from the band edge, and it holds.
- **`compute_n_bounds` (`curve_gmp.h`) walks to both true edges.** Seed
  bias cannot survive it. This is what retired the bogus `+1` guard
  documented in that file's comment.
- **`is_pal_fast` (`palbrute.c`) is a correct palindrome test.** At
  iteration `i` it compares `lo % 10` (digit `i`) against `hi / t` with
  `t = 10^(h-1-i)` (digit `d-1-i`), for `i < d/2` — so odd `d` skips the
  middle digit, as it must. The u64 narrowing holds with room: at
  `d = 37`, `h = 18` and `10^19 = 1.0e19` against `UINT64_MAX = 1.8e19`.
  The `#if BQ_MAX_D > 37` guard makes the failure a build error rather
  than a silent truncation.
- **The zone bounds are exact integer arithmetic.** `first_n_at_least`
  seeds from `n_at` then walks both ways, so `LEAD[k] * 10^(d-1)` edges
  are exact. The premise checks out independently: `2n²+2n+1 mod 10`
  over `n mod 10` gives exactly `{1,3,5}`, so `LEAD = {1,3,5}` is
  complete, not a heuristic.
- **The checkpoint round-trip neither re-scans nor skips.**
  `write_ckpt(..., blk_end + 1, ...)` stores the first *unprocessed* n
  and resume starts exactly there. Confirmed against the live d=29
  checkpoint: `86520678118655 - 70710678118655 = 15,810,000,000,000`,
  which is precisely the recorded `visited`.
- **`keep_n[i % 110]` (`hunt.c`) is a valid fusion.** `110 ≡ 0` mod both
  10 and 11, so `i % 110` preserves both residues of `n = n_min + i`.
- **`is_pal_rev` cannot overflow.** A `d`-digit reversal is at most `d`
  digits, and `MAX_D = 37 < 39`.
- **The two d=29 COMPOSITE verdicts are sound.** `mpz_probab_prime_p`
  returning 0 is definitive; only a *positive* result would be
  probabilistic.

## Finding 1 — `palcurve` does not validate B or C (latent)

`palcurve.c` validates `A` to `[1,3]`, `d`, and `t`. `g_b` and `g_c`
come straight from `strtoll` with no bounds check. Two consequences in
`m_at` (`curve.h`):

    u128 disc = 4 * (u128)a * p + (u128)(b * b);

- `b * b` is computed in `int64_t` *before* the widening cast. For
  `|B| > ~3.04e9` that is signed overflow — undefined behaviour. The fix
  is `(u128)b * (u128)b`.
- `if (s < (u128)b) return 0;` and `curve_abc`'s `(u128)b`, `(u128)c`
  wrap for negative coefficients, so a negative B or C yields silently
  wrong values rather than an error. The header's own derivation is
  written as `m = (-B + sqrt(B² - 4AC + 4Ap)) / 2A`, i.e. it
  contemplates a negative B that the implementation cannot represent.

**Not reachable from any result on record.** All 13 curves in
`curve_palindromes.txt` are `A ∈ {2,3}`, `B ∈ {2,3,4}`,
`C ∈ {1..221}` — small and positive. That is not luck: the recentring
onto `m >= 0` exists precisely to keep them so. Suggested fix is to
reject negative or oversized B/C at entry, matching how A is handled.

## Finding 2 — the long-double leading-digit filter survives in `palhunt_opt.c`

`siege_plan` 5b removed this filter from `palbrute` because it silently
dropped candidates at d=37. The identical construct is still at
`palhunt_opt.c:69`:

    int first = (int)((long double)p / P1ld); /* fast exact leading digit */

The trailing comment is false — it is neither exact nor safe at the top
of the range. `palhunt_opt` and `palhunt_gmp` are both superseded (see
`siege_plan` item 16) and no current result depends on them, so this is
recorded rather than patched: the real fix is the consolidation item 16
already describes, not a local edit.

## Outside `bi-quad`

- `recursion/perms.c`, `recursion/perm-test.c` — **fixed.** A `uint8_t`
  length made `length - 1` wrap to 255 on empty input, and the
  permutation walked off the end of the buffer into the environment
  block (`./perm-test ""` printed `sSHELL=/bin/bash`). `perm-test.c`
  additionally truncated `strlen` mod 256 *before* its length check, so
  a 256-char argument read as length 0, and its loop counter was
  `int8_t` against a bound that could reach 255. Lengths are now
  validated before narrowing, and all indices are signed.
- `lock256/lock256-orig.c` — **reported, not fixed.** Two live bugs,
  both already fixed in its successor `dev/dev256.c`: `if (x->lo == 0)
  return 0;` exits silently with status 0 whenever x is a multiple of
  2^64, and the input check tests digit *count* (`> 39`) rather than
  magnitude, so 39 nines wrap past 2^128-1 without complaint. Left
  alone because the file is a deliberately preserved original.

## Effect on the live run

None. Nothing in `bi-quad` was rebuilt or modified except this file.
Measured cost of the whole audit: cumulative rate 580.2 → 579.8 M n/s
(0.07%), all of it from one Valgrind run that hit the `perm-test`
runaway before a timeout was in place. ETA unmoved at 20.6 h.
