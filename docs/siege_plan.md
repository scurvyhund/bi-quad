# Siege plan — ranked list of what to try next

Working list, newest thinking at the top of each tier. Cost is wall-clock
on the 8-core box. "Payoff" is what the project can *claim* afterwards
that it cannot claim now.

Status key: **[ ]** not started · **[~]** in progress · **[x]** done

---

## Tier 0 — must happen before anything is published

**[~] 1. Finish the d = 25 brute** — running, ~2 h left.
Fourth independent line at d = 25 (`hunt_noopt` + qs factorisations,
`palsplit`, now `palbrute`). Its real job is a **positive control for
`palbrute` itself** before that tool is trusted with a multi-day d = 29
run. Watch for all five values with matching `n`, including the div-5
one at 57.7%.

**[ ] 2. Fix `density_cross_curve.md` §4–§5.**
The per-curve χ² is invalid — every cell has expected count < 5, and at
d ≥ 15 every cell is < 1, so the "good fit" there was the test losing
power, not the model gaining accuracy. Replace with the **pooled**
test (valid: all bins E ≥ 3.3, χ² = 3.95 on 5 df), quote `C′(k=1)` as
a **range 1.9–3.3**, and **delete the claim that the conjecture is not
decidable by search** — at C′ = 3.3 the d = 39…51 range carries a ~40%
chance of a hit, which is a real search, not a formality. Keep §2 and
§3 unchanged; they are large-sample and stand.
*Cost: an hour. Blocking for any push.*

**[ ] 3. Rotate the GitLab token.** `gitlab-token.txt` was untracked
*and* unignored — one `git add -A` from three public mirrors. Now
covered by `.gitignore`, but whether it ever entered history before
2026-09-03 cannot be determined from here.

**[ ] 4. Push.** Six commits local: `c76b726`, `20300fb`, `5952ed8`,
`2d1948a`, `06a584d`, `5c08375`. Do (2) first.

**[ ] 5. `STATE_OF_THE_SEARCH.md` is stale** — still reads "Status
(2026-07-05) … Search complete", palindromes "through d = 27". Now
contradicted by the d = 37 frontier. It is the capstone doc, so it
wants Jim's hand, not a unilateral edit.

---

## Tier 1 — cheap, and they sharpen the weakest numbers

**[ ] 5a. Skip the provably-empty 59% of every brute range. NEW, and
the biggest single win on this list.**
A palindrome needs first digit == last digit; curve values end ONLY in
1, 3, 5; therefore a palindrome can only occur where `p` *starts* with
1, 3 or 5. The other 59% of the n-range cannot contain one and can be
skipped outright — three range checks, ~10 lines in `palbrute.c`.

    d = 25 / 29 / 37   searchable 41.4%   speedup 2.41x
    full d=29    24.6 days  ->  10.2 days
    the partial   9.2 days  ->   3.8 days

**Exact, not lossy** — unlike `hunt`'s `VALID_NMOD` skip
(`skip_optimization.md`), which destroys counts by dropping div-5
palindromes. This drops only `n` that cannot yield a palindrome at all;
every palindrome survives, div-5 included.

Found by asking why `palbrute`'s throughput jumped 6x mid-run: it is
the leading digit of `p` crossing from 1 to 2, where `first == last`
becomes impossible. The mod-10 structure of the curve is legible in the
wall-clock of a program that was never told about it.

*Do NOT rebuild the binary under the running d=25 job. Apply after it
finishes, with a before/after showing all five hits still found.*
*Cost: an hour. Payoff: makes item 13 a weekend instead of a fortnight.*

**[ ] 6. Raise `palcurve`'s `MAX_D` from 33 to 37.**
The cap is conservative: the real u128 limit on `4·A·p` binds at d ≤ 37
for A ≤ 3, verified exact at the top of every d from 30 to 37. Six more
digits on 13 curves is *minutes* of compute and directly attacks the
two weakest results — `C′` rests on 7 primes, and the d = 21–31 bin
runs low (3 observed vs 7.0 expected). **Best value-for-compute on the
whole list.**
*Cost: minutes. Payoff: tightens C′, tests the high-d deficit.*

**[ ] 7. Is the d = 21–31 deficit real?**
3 observed against 7.0 expected is the bin nearest every extrapolation
we make. P(≤3) ≈ 0.08 under Poisson — suggestive, not significant. If
real, `C′` falls at high d and the search odds drop. (6) is how to find
out.

**[ ] 8. Explain the 1.4× residual in raw density.**
Predicted 2.15 raw palindromes per odd d on k=1, observed 2.57. Prime
suspects: conditioning on `p ≢ 0 mod 3` and `mod 11`, which the digit
model ignores, and digit-position correlations inside a palindrome.

**[ ] 9. `Z[√−2]` even-d: 19 observed vs 12.4 predicted.** The only
permitted curve the model gets wrong. Check whether the two-branch
union is being weighted correctly in `N_d` and the last-digit pooling.

**[ ] 10. Widen the comparison class** — `palcurve` on k = 23, 25, 27…
More curves is more power for every density test, at minutes each.

---

## Tier 2 — the other half of the open problem

**[ ] 11. Derive `C/d²` per curve for the EMIRP side and test it.**
`OPEN_PROBLEM.md` predicts emirps converge to ≈ 1 total, and this is
the half the heuristic calls *correct*. The same machinery applies:
`N_d`, the last-digit signature, Hardy–Littlewood. Test against
`hunt.c`'s survivor counts, which already exist for d ≤ 25. Would give
the emirp conjecture the quantitative footing the palindrome side now
has.
*Cost: a day. Payoff: the whole open problem, not half of it.*

**[ ] 12. Does the split have an emirp analogue after all?**
Established this session that head and tail are independent choices, so
`10^(2t) · 10^(d/2−2t) = 10^(d/2)` for every t — the `mitm_probe.c`
wall. Worth one more look at whether a *different* decomposition (by
residue class rather than digit position) breaks it.

---

## Tier 3 — expensive, decide deliberately

**[ ] 13. d = 29 partial brute — ~9.5 days, or ~3.8 days after (5a).**
`n = 70710678118655..128166678118655`, 37.6% of the full sweep, covering
both known palindromes (at 1.96% and 10.33%) plus a large
must-be-empty region. **This is the real corroboration gap**: d = 29…37
rests on `palsplit` alone. Valgrind the checkpoint path at the d = 29
config first (the d = 25 pre-flight covered the path, not that config).
*Full d = 29 is ~25 days — corrected from the "9.3 days" that assumed a
low-d rate.*

**[ ] 14. 256-bit `palsplit` to reach d = 41…51.**
Current ceiling is integer width, not algorithm: `p < 10^37` keeps `2p`
in u128, and `n < 2.24×10^18` is the last odd d fitting `int64_t`. Needs
a wider `n` (`__int128`) and a fixed 256-bit `p` — GMP per candidate
would likely cost more than the algorithm saves. At C′ ≈ 3.3 this
carries a **~40% chance of an actual hit**, which is the strongest
argument on this list for spending real compute.
*Cost: days of work + days of compute.*

**[ ] 15. `pals_d29..37.txt` with qs factorisations.** Project
convention pairs those files with proven compositeness. Currently we
have Miller–Rabin witnesses (compositeness *is* proven) but not
factorisations.

---

## Ruled out — do not re-litigate

- **Algebraic palindrome families.** Exhaustive search over every
  palindromic seed `n < 2×10⁶`, all `(A,B)` inferred from seed pairs for
  `A ∈ {10,100,1000,10000}`, plus `n = a·10^k + b` for `a,b < 200`,
  `k ≤ 18`. Exactly one family of length ≥ 4 exists — `n → 100n + 106`
  from n = 16, giving 545 → 5824285 → 58281418285 → 582818040818285 —
  and it **dies at term 5**. Nothing else reaches length 3. This is a
  useful negative: it is the independence the density model assumes.
- **Pattern-hunting in the 41 k=1 palindromes.** `n mod 10` is
  consistent with uniform (χ² = 11.4, 9 df, p ≈ 0.25); apparent `n`
  clustering is just `n ≈ √(p/2)`. The one real pattern found so far —
  density by last-digit signature — came from *aggregating* 443
  palindromes across 13 curves, not from staring at one list.
- **`palhunt_gmp`'s `isqrt128`.** Checked: pure integer binary search on
  `m ≤ x/m`, no floating point. The d = 27 result is unaffected by the
  band-precision bug that hit `palsplit`.

---

## Code hygiene (low priority, no scientific content)

**[ ] 16.** `palhunt_gmp.c` / `palhunt_opt.c` still share five
duplicated functions (`isqrt128`, `ipow10`, `curve`, `is_pal`,
`to_mpz`). Both are superseded — `hunt` gives palindrome counts free and
`palsplit` gives the complete set ~670,000× faster. Retire or fold into
`curve.h`.

**[ ] 17.** `mod_obstruct_bkup.c` keeps its own `compute_n_bounds` and
five residue helpers. Backup file, not a build target — left alone
deliberately.

---

## Environment gotcha, learned the hard way

`diff` is aliased to `diff -Ety` (side-by-side) in this environment and
the alias is **live in non-interactive shells**. Side-by-side output has
no `<`/`>` prefixes, so `diff | grep -c '^[<>]'` reports **zero changes
for files that differ**. Three "identical" verifications this session
were produced that way; two held under `cmp`, one did not. Use `cmp -s`,
`diff -q`, or `/usr/bin/diff`.
