# Siege plan — ranked list of what to try next

Working list, newest thinking at the top of each tier. Cost is wall-clock
on the 8-core box. "Payoff" is what the project can *claim* afterwards
that it cannot claim now.

Status key: **[ ]** not started · **[~]** in progress · **[x]** done

---

## Tier 0 — must happen before anything is published

**[x] 1. d = 25 brute — DONE 2026-09-03.** `found=5
checked=1528961196313 time=7184.1s` (2.0 h). All five values and indices
match a prediction registered before the run reached them; full range
swept; clean exit. `palbrute` is now a **validated instrument** at
d = 25 against ground truth established three other ways, which was the
whole point — it is the precondition for trusting it at d = 29.
Fourth independent line at d = 25 (`hunt_noopt` + qs factorisations,
`palsplit`, now `palbrute`). Its real job is a **positive control for
`palbrute` itself** before that tool is trusted with a multi-day d = 29
run. Watch for all five values with matching `n`, including the div-5
one at 57.7%.

**[x] 2. Fix `density_cross_curve.md` §4–§5.** DONE 2026-09-03.
The per-curve χ² is invalid — every cell has expected count < 5, and at
d ≥ 15 every cell is < 1, so the "good fit" there was the test losing
power, not the model gaining accuracy. Replace with the **pooled**
test (valid: all bins E ≥ 3.3, χ² = 3.95 on 5 df), quote `C′(k=1)` as
a **range 1.9–3.3**, and **delete the claim that the conjecture is not
decidable by search** — at C′ = 3.3 the d = 39…51 range carries a ~40%
chance of a hit, which is a real search, not a formality. Keep §2 and
§3 unchanged; they are large-sample and stand.
*Cost: an hour. Blocking for any push.*

**[x] 3. GitLab token — CHECKED, never exposed. 2026-09-03.**
`gitlab-token.txt` was untracked *and* unignored, so a `git add -A`
would have committed it. It never happened: the literal token string
appears in none of the 122 commits, the file was never tracked, and no
token-shaped filename was ever added. It is now gitignored, and no
untracked-and-unignored file remains in the tree. **Rotation is
optional hygiene, not incident response** — skipped deliberately.
Optional follow-up: a pre-commit hook refusing `glpat-`/`ghp_`/`gho_`,
which would also catch the file under a different name.

**[x] 4. Push — DONE 2026-09-03.** 11 commits, `8b6adac -> 1a7727a`,
verified landed on origin / github / codeberg.

**[x] 5. `STATE_OF_THE_SEARCH.md` — DONE 2026-09-03 (`1a7727a`).** Was — still reads "Status
(2026-07-05) … Search complete", palindromes "through d = 27". Now
contradicted by the d = 37 frontier. It is the capstone doc, so it
wants Jim's hand, not a unilateral edit.

---

## Tier 1 — cheap, and they sharpen the weakest numbers

**[x] 5a+5b. Leading-digit zones + the long-double fix. DONE
2026-09-03.** One edit, and the correctness half is the point.

A palindrome needs first digit == last; curve values end ONLY in 1, 3,
5; so a palindrome can only occur where `p` *starts* with 1, 3 or 5.
`palbrute` now computes those three n-ranges with exact u128 bounds and
never visits the other 59%. Inside a zone the leading digit is known,
so the buggy `(int)((long double)p / topld)` filter is gone entirely —
that is (5b), and it is the real payoff: it silently dropped candidates
at d = 37.

**The speed gain is ~1.15x, not the 2.41x first claimed.** That figure
came from "we visit 41.4% of n, so 2.41x", which assumes uniform cost
per n. It is not: the dominant cost is `is_pal_rev()`, the full digit
reversal, which runs whenever first==last — and that count is
unchanged (0.1409 of the range before, 0.1412 after). The zones skip
59% of the *cheap* work and none of the expensive work. Measured at
d = 19: 4.54s old, 3.95s new.

What the zones do buy, besides correctness, is the ability to run **one
zone in isolation**.

Also added: **atomic checkpointing** (`palbrute_d<NN>.ckpt`, tmp+rename,
auto-resume), which `palbrute` never had — its header claimed it, but
only stderr progress was ever persisted. Verified by `kill -9` mid-sweep
at d = 19: resumed from the checkpoint, identical hits and identical
`visited` count.

Verified: d = 13/15/17/19 identical across the pre-change binary, the
new zones, and `--all`; all 44 known k=1 palindromes (d = 2..37) fall
inside the zones; `p mod 10 ∈ {1,3,5}` confirmed exhaustively; Valgrind
clean with 9 checkpoints written during the run.

**[x] 5c. Early-exit palindrome predicate — 7.65x. DONE 2026-09-03.**
Jim spotted this the moment the function was on screen. `is_pal_rev`
reversed all d digits with **no early exit**, on every call — at d=29
that is 29 u128 divmod pairs, ~11.7 trillion times in a lead-1 sweep.
It dominated the run completely, which is also why the zones (5a) only
bought 1.15%.

Inside a zone the outermost digit pair is already known to match, so
the next pair fails 9 times in 10. `is_pal_fast` splits once into two
64-bit halves and compares pairs from the outside in, returning on the
first mismatch: ~1.11 comparisons instead of 29, in 64-bit rather than
u128 arithmetic.

    full d=25 sweep   7184.1s -> 938.8s     7.65x, same five palindromes
    d=29 in-zone rate 72 M n/s -> 526 M n/s 7.3x

Soundness — `is_pal_fast` has a NARROWER CONTRACT than the predicate it
replaces: it requires `d` to be the true digit count of `x`, where
`is_pal_rev` works on anything. Inside palbrute that is guaranteed by
the zone bounds. `is_pal_rev` is therefore KEPT as the reference, and
`--verify` runs both on every candidate and `exit(3)`s on disagreement.

Verified: `test_palpred.c` — 2.6M values at every d from 1 to 37,
driving genuine palindromes, one-digit corruptions and uniform random
(random curve values are almost never palindromes, so a data-only test
would never exercise the YES path); `--verify` clean at d = 13, 15, 17,
19, 21, 23 with counts 2/4/1/3/5/1; d = 25 reproduces the five from the
2026-09-03 run exactly, against a list registered before the run
reached them. A `#error` fires if `BQ_MAX_D` ever exceeds 37, where the
uint64 casts would silently truncate.

**Revised d=29 cost:** lead-1 zone ~16-27 h, **all three zones
~33-59 h** — so the COMPLETE d=29 is now cheaper than the partial was
this morning. Do the complete one; the claim becomes "d=29 exhaustively
brute-verified" rather than "the lead-1 part of it".

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

## Tier 2b — the 256-bit extension (the road to d = 51, and past it)

The current ceiling is **integer width, not algorithm**: `p < 10^37`
keeps `2p` inside a u128, and d = 37 is the last odd d whose `n` fits an
`int64_t`. Going wider is what unlocks §5's 25-40% chance of an actual
hit — the strongest argument on this list for spending real compute.

**The tooling already mostly exists**, in `~/programming/c/lock256/dev/
dev256.c` (see also `bigint-mul/`, `conversion/`). Written for embedded
work, deliberately without a bigint library — which turns out to be
exactly right here. GMP solves *arbitrary* precision with heap-allocated
limbs and a call per operation; we need *exactly* 256 bits, known at
compile time, staying in registers through ~10^13 inner-loop iterations.
For the hot loop a fixed-width type is not a workaround for GMP, it is
strictly better than GMP.

    struct u256 { u64 lo; u64 mid; u128 hi; };
    mul256b()        -- 4 partial products, carries threaded manually
    u256_to_string() -- limb-wise long division by 10

`mul256b` covers `curve(n) = 2n^2+2n+1`; `u256_to_string`'s ÷10 is the
digit extraction `is_pal_fast` needs past d = 37.

**[ ] 20. `isqrt256` — the crux.** The u128 recipe (long double seed →
Newton → exact correction) needs `v/x`, a full **u256 ÷ u256** division,
which is the one primitive `dev256.c` lacks — its ÷10 is a small-divisor
case and much easier. The alternative avoids division entirely: the
classic bit-by-bit integer sqrt, two bits of input per iteration,

    rem = (rem << 2) | next two bits
    if (rem >= 2*root+1) { rem -= 2*root+1; root += 1; }
    root <<= 1

shifts, compares, add/sub — nothing else. **Exact by construction**, so
there is no seed to be wrong about and no correction loop, which matters
because the band edge is exactly where the u128 version went wrong
(`palindrome_split_search.md` §4).

Cost concern, to be **measured not predicted**: 128 iterations per call,
and `palsplit` calls the band edge twice per outer residue — ~2×10^13
calls at d = 51 with t ≈ 13. Whether that dominates is an empirical
question. Build the correct version first, with a test that checks
`r² ≤ v < (r+1)²` by comparison rather than by squaring (the way
`test_curve.c` does for u128). Then measure. Then optimise if needed.

**[ ] 21. Comba (product-scanning) multiply, and `mul512`.**
`mul256b` uses **operand scanning** — walk the rows of the schoolbook
grid, propagate carries as you go. At 2×2 limbs the carries are
trackable by hand, which is what its WARNING block documents. At 8×8
limbs there are 64 partial products and overlapping carry chains; that
difficulty is what stopped the extension to 512 bits, and stopping was
the right call.

**Product scanning fixes the shape.** Walk the *columns* instead: for
output limb k, accumulate every `x[i]*y[k-i]`, emit one word, carry the
rest.

    u128 acc = 0;  u64 ovf = 0;
    for (k = 0; k < 2*N; k++) {
       for (i = max(0,k-(N-1)); i <= min(k,N-1); i++) {
          u128 p = (u128)x[i] * y[k-i];
          acc += p;
          if (acc < p) ovf++;        /* the ONLY carry rule */
       }
       r[k] = (u64)acc;
       acc  = (acc >> 64) | ((u128)ovf << 64);
       ovf  = 0;
    }

The carry bookkeeping is two lines and **identical for every column**,
so going 256 → 512 → 1024 changes the loop bounds and nothing else. The
hard part stops growing with the width.

Two cautions. `acc < p` detects the wrap because unsigned overflow is
*defined* to wrap — one of the few places the project's signed-by-default
rule must be deliberately inverted, and it wants a comment saying so at
the declaration. And this is written from the standard algorithm: before
trusting it, check it against `mul256b` on millions of random inputs, the
way `test_palpred.c` checked the fast predicate.

A Comba `mul512` would put **d ≈ 70** in reach — past where §5 puts the
expectation of a hit at 1.

---

## Tier 3 — expensive, decide deliberately

**[ ] 13. d = 29, COMPLETE (all three zones) — ~33-59 h.**
`./palbrute 29` now splits into three zones; the lead-1 zone is 19.2%
of the full range and **contains both known d = 29 palindromes** (at
1.94% and 10.33%), giving positive controls plus a large must-be-empty
region. The claim it earns is clean: *every d = 29 palindrome beginning
with 1 has been independently confirmed.* **This is the real
corroboration gap** — d = 31…37 rests on `palsplit` alone. **d = 29 is
now closed**: `palbrute 29` finished 2026-09-05 in 26.7 h with found=2,
matching `palsplit` exactly. See STATE_OF_THE_SEARCH §3.

Costs, corrected against the measured d = 25 run (1.529e12 n in 7184 s
= 213 M n/s average):

    lead-1 zone only   ~4.7 days   (72 M n/s measured IN that zone)
    all three zones    ~9 days
    full sweep        ~11 days

*An earlier note said 25 days for the full sweep and 3.8 for a partial.
Both were wrong: the 25 applied the slow lead-1 rate to the whole
range, and the 3.8 applied the global 41.4% zone fraction to a slice
that is mostly lead-1.*

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
