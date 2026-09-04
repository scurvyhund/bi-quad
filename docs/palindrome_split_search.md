# The head/tail split search — palindromes in O(10^(d/4))

**Status: PROVEN, validated against every existing ground-truth set.**
Palindrome frontier moved **d = 27 → d = 37**.
Emirp frontier **unchanged at d = 27** — the method does not apply there, for a reason given below.

Tool: [`palsplit.c`](../palsplit.c). Prototype: `docs/palsplit.py`.

---

## 1. The result

`palhunt_gmp` cleared d = 27 in **22.3 hours**.
`palsplit` clears it in **0.12 seconds** — and reaches d = 37, ten digit-lengths further, in about 30 seconds of total compute.

The gain is not a constant factor. It is a change of exponent:

| method | work at digit-length d |
|---|---|
| n-enumeration (`palhunt_gmp`, `hunt.c`) | O(10^(d/2)) |
| head/tail split (`palsplit`) | **O(10^(d/4))** |

---

## 2. The idea

Every value on the curve satisfies `2p − 1 = (2n+1)²`, so `p` determines `n` and vice versa.

Now use the one thing a palindrome gives you for free:

> **A d-digit palindrome's high t digits are the reverse of its low t digits.**
> `p_i = p_(d−1−i)` for every i.

So enumerate `r = n mod 10^t`. That single choice fixes **both ends** of `p` at once:

- the **low t digits** directly, as `L = f(r) mod 10^t`;
- the **high t digits** as `H = rev(L)` — free, no extra enumeration.

And the high digits pin `n` by magnitude: `p ∈ [H·10^(d−t), (H+1)·10^(d−t))` gives `n ≈ sqrt(p/2)` inside a band of width `~10^(d/2 − t)`.

Intersect the two facts — `n` in a band of width `10^(d/2−t)`, and `n ≡ r (mod 10^t)` — and only `~10^(d/2 − 2t)` candidates survive per residue, over `10^t` residues.

Total work `≈ 10^t + 10^(d/2 − t)`, minimised at **t ≈ d/4**, giving `O(10^(d/4))`.

Measured, and it tracks:

| d | t | outer (10^t) | inner ops | 10^(d/4) |
|---|---|---|---|---|
| 21 | 5 | 100,000 | 219,727 | 177,828 |
| 25 | 6 | 1,000,000 | 2,198,195 | 1,778,279 |
| 27 | 7 | 10,000,000 | 2,196,119 | 5,623,413 |

---

## 3. Why this does NOT rescue the emirp search

This is the important negative, and it explains the earlier ruled-out result rather than contradicting it.

The speedup comes from the head being **determined** by the tail. For an emirp that is false: `p`'s head and `p`'s tail are **independent choices**. You must enumerate `10^(2t)` head/tail pairs, each leaving `10^(d/2 − 2t)` candidates —

    10^(2t) · 10^(d/2 − 2t)  =  10^(d/2)

— exactly the brute-force count, for every t. No exponent changes.

Stated honestly: this is a sound argument about *this family of splits*, not a proof over all possible decompositions. It is a strong reason to expect no analogous speedup, not a theorem forbidding one. That is the same wall `mitm_probe.c` hit in [`structural_attacks_2026-06-04.md`](structural_attacks_2026-06-04.md), now with a clean reason: *reversal is only free when the number is its own reverse.*

**Consequence for the project's framing.** [`STATE_OF_THE_SEARCH.md`](STATE_OF_THE_SEARCH.md) says the emirp and palindrome hunts are "survivors of the same sieve", since a palindrome is the degenerate `m = n` case. That is true of the *sieve*, but it is precisely the degeneracy that makes the palindrome problem strictly easier. Treating them as one problem is what held the palindrome frontier at d = 27. **They should be run as two separate searches from here on.**

---

## 4. The algorithm

For each residue `r` in `[0, 10^t)`:

1. `L = f(r) mod 10^t` — the low t digits of any `p` with `n ≡ r`.
2. Skip if `L ≡ 0 (mod 5)` — `p` would end in 5 (production only; see §6).
3. Skip if `L ≡ 0 (mod 10)` — `p`'s leading digit would be 0.
4. `H = rev(L)` as a t-digit string — the high t digits.
5. Band: `nlo, nhi` from `sqrt(2·H·10^(d−t) − 1)`, guard ±2.
6. Step `n` through the band in strides of `10^t` starting at the first `n ≡ r`.
7. For each `n`: form `p`, reject unless d digits, test `p_t == p_(d−1−t)` as a cheap discriminator, then verify the full palindrome exactly.

The outer loop is a flat counter, so checkpointing is a single integer and the residue range splits trivially across threads. No shared state but the (tiny) hit list.

### The band edge — exact, and the mistake that got it there

The band edge needs `nlo = (sqrt(2·H·10^(d-t) - 1) - 1)/2` once per residue — up to 10^9 times per run. Making that a *bignum* `isqrt` would dominate everything, so the first version estimated it in `long double` and widened the band by a guard, leaning on the exact palindrome test as the backstop.

**That analysis was wrong, and the error is worth recording.** The claim was a 1000x safety margin on a ±2 guard. The real budget at the d = 37 ceiling, t = 9:

- `scale = 10^28 = 2^28 · 5^28`, and `5^28 ~ 2^65` — **not exactly representable** in a 64-bit mantissa; `powl` adds its own error.
- `high * scale` rounds again; `sqrtl` halves the accumulated relative error.
- The `(int64_t)` cast truncates, costing up to 1 more.

With `n ~ 2.24x10^18 ~ 2^61`, each rounding contributes `~2^61 · 2^-64 = 0.125` and the truncation adds ~1 — a deviation of roughly **1.5 against a guard of 2**. A margin of ~1.3x, not 1000x.

Worse, **the regression could not have caught it**. At d <= 27, `n ~ 2^43` and the error is `~2^-21` — utterly negligible. The precision risk existed *only* past the old frontier, and a silent miss there looks exactly like `found = 0`, which is what d = 37 reports.

**The fix was not a bigger guard. It was to stop estimating.**

`isqrt_u128()` computes the edge exactly: seed from `sqrtl`, a few Newton steps, then a final exact adjustment using division comparisons (`x > v/x`, never `x*x`, which would overflow near the top of the range). The seed's quality is irrelevant to the result — the adjustment loop guarantees exactness — so `long double` mantissa width stopped being load-bearing, and the build-time assert that briefly guarded it was removed rather than left to block non-x86 builds.

The whole computation now stays in u128 and no overflow is possible: `high < 10^t` and `step = 10^(d-t)`, so `plo = high·step < 10^d <= 10^37`, and `2·plo` stays under 3.4x10^38.

Cost: **21.6 s -> 29.5 s at d = 37**, about 36%. Bought outright: the floating-point failure mode is gone, not bounded.

`BAND_GUARD` is retained at ±1000 as pure belt-and-braces. With an exact edge it should be unnecessary; it costs roughly 2000 extra candidates across an entire 10^9-residue loop, which is unmeasurable, and the argument for a tight guard was wrong once already.

#### Both claims were then tested, not argued

The full suite — 27 configurations spanning d = 7 to d = 37, both `--keep5` and production modes, two t each — was run three ways:

    guard +/-2,    long double band   md5 5f2812ebe72bcea514dac6c485524c5e
    guard +/-1000, long double band   md5 5f2812ebe72bcea514dac6c485524c5e
    guard +/-1000, EXACT u128 band    identical to both

**All three agree byte for byte.** A 500-fold widening of the guard changes nothing, so the float estimate was not already dropping candidates; and replacing the estimate with an exact edge changes nothing either, so the float version was in fact correct throughout the published range. The exact version is what ships regardless — the point is not to have been lucky, it is to no longer depend on luck.

## 5. Correctness

Two properties matter, and both were tested rather than assumed.

### t-independence — and it is not merely an efficiency knob

`p_i = p_(d−1−i)` holds for **every** `t ≤ d`, so `H = rev(L)` and the band stay valid regardless of t. Every t returns the same set; t buys only speed.

Verified exhaustively — d = 13 (t = 3..6), d = 15 (t = 3..7), d = 21 (t = 5..7), d = 25 (t = 6,7): **all agree**, including at `t = (d−1)/2` where the windows touch, and at `t = 7 > (d−1)/2 = 6` for d = 13, where they overlap and the search merely wastes work.

So `t ≤ (d−1)/2` is an **efficiency** bound, not a correctness bound.

**This makes a second t a free, fully independent cross-check** — the two runs partition the search completely differently while provably sharing an answer. It is mandatory for any result past d = 27, and every new result in §7 carries one.

### There is no `range < mod` cliff

`mod_obstruct` had a regime where it silently **over-counted** (see CLAUDE.md). This method has no analogue:

- band/mod large → more inner iterations (slower, still correct);
- band/mod small → fewer (still correct).

There is no over-count mechanism, because every hit is confirmed by an exact digit-by-digit palindrome test on the actual value. The failure mode that cost 22 CPU-days cannot occur here.

---

## 6. The div-5 flag — production vs regression

Curve values end in 1, 3, or 5 (units digit 5 for 4 of the 10 residues `n mod 10`). A palindrome ending in 5 also *begins* with 5 and is divisible by 5, so it cannot be prime.

Skipping them removes 40% of the outer loop but only 28% of the inner work — those residues force a leading digit of 5, hence larger `n` and a *narrower* band, so the discarded work was cheaper than average. Measured net: **1.56×** (d = 27: 19.7 s → 12.6 s in the Python prototype).

- `--keep5` **off** (default) for real runs.
- `--keep5` **on** for the regression, which must reproduce the full curve-value sets. Reporting only the pretty subset is exactly the lossiness [`skip_optimization.md`](skip_optimization.md) exists to warn about.

A corollary worth recording: **every prime palindrome on this curve begins and ends with 1 or 3.**

---

## 7. Validation and new results

### Regression — reproduces every existing ground-truth set

Run with `--keep5`, at two independent t each. Values *and* n match exactly.

| d | t | found | vs ground truth |
|---|---|---|---|
| 7 | 2 | 5, incl. 3187813 PRIME | positive control |
| 13 | 3, 5 | 2 | `pals_d13.txt` ok |
| 15 | 4, 6 | 4 | `pals_d15.txt` (3) + 1 div-5 |
| 21 | 5, 7 | 5 | `pals_d21.txt` (4) + 1 div-5 |
| 25 | 6, 8 | 5 | `pals_d25.txt` ok (lists its div-5) |
| 27 | 6, 8 | 3 | `pals.txt` ok |

Independently cross-checked against direct n-enumeration at d = 5, 7, 9, 11, 13, 15 — exact agreement, no misses, no spurious hits.

### New — d = 17, 19, 23 (gaps in the record) and d = 29…37

Prime-eligible palindromic curve-values (div-5 excluded, matching the `pals_d*.txt` convention):

    d=17   1   12951570707515921
                 n=80472264

    d=19   0   --
    d=23   0   --

    d=29   2   10856305724223132242750365801
                 n=73675999227099
               14966412087720702778021466941
                 n=86505526088570

    d=31   3   1014614035436689866345304164101
                 n=712254882551425
               1682059335368470748635339502861
                 n=917076696729469
               3167016920841776771480296107613
                 n=1258375325735882

    d=33   1   316370934175751979157571439073613
                 n=12577180410882082

    d=35   4   10489844562990321812309926544898401
                 n=72421835667809200
               16079176741142975657924114767197061
                 n=89663751709213505
               16716911115990684748609951111961761
                 n=91424589460359855
               31033505548338259895283384550533013
                 n=124566258570164697

    d=37   0   --

All values above are COMPOSITE.

**"COMPOSITE" here is proven, not probabilistic.** `mpz_probab_prime_p` returns 0 only when it has found a Miller–Rabin witness; a witness is a certificate of compositeness. (Only a *PRIME* verdict would be probabilistic.) These are not factorisations, so they are weaker than the `qs` certificates in `pals_d*.txt` — but the compositeness itself is settled.

### Independent corroboration of the new results

Every new value was re-verified from first principles by a separate Python implementation sharing no code with `palsplit`: correct digit count, palindromic, on the curve by **both** `p = 2n^2+2n+1` and `2p-1 = (2n+1)^2`, and composite under an independent Miller-Rabin. All pass; 8 of the 11 also yield a factor below 10^7.

**A true cross-check exists at d = 23, from an exhaustive `hunt.c` run.** `logs/hunt_d23-25.log` records 3 raw survivors, the third flagged `(p|q div 5)` at `n=165058650666`. That survivor **is a palindrome** — `p = 54488716319691361788445`. `palsplit 23 --keep5` returns exactly it and nothing else; `palsplit 23` (production) returns 0. Both agree with `hunt.c` exactly.

Note that the `check_d*.log` files cannot corroborate palindromes: they enumerate *emirp* candidates and exclude the degenerate `q = p` case by design, which is why `check_d17.log` reports 0 structural matches despite a palindrome existing at d = 17.

### Independent brute-force verification (`palbrute.c`)

`palbrute.c` enumerates every n and reverses the digits. Its only filter is `first digit == last digit` — a necessary condition for any palindrome that assumes nothing about the curve — so it shares no logic with the residue/band construction it is checking. Unlike `palhunt_gmp` it prints **every** palindromic curve-value including composites; `palhunt_gmp` reports only certified primes, so at a digit-length with none its output is indistinguishable from a brute that silently did nothing.

Exhaustive, completed 2026-09-03:

| d | 13 | 15 | 17 | 19 | 21 | 23 |
|---|---|---|---|---|---|---|
| `palbrute` | 2 | 4 | 1 | 3 | 5 | 1 |
| `palsplit --keep5` | 2 | 4 | 1 | 3 | 5 | 1 |

Values and n match in every case. **This is the first independent corroboration of d = 17, 19 and 23**, which had none: `check_d*.log` enumerates emirp candidates and excludes the degenerate `q = p` case by design.

Note d = 19: all three are div-5 (`5227371841481737225`, `5649436330336349465`, `5816694029204966185`), so production mode correctly reports 0. **The comparison is only meaningful with `--keep5` on.**

Still outstanding: d = 29..37 have no brute corroboration. A full d = 29 sweep is 152,896,119,631,324 n-values, ~9.3 days at the measured rate — the two known palindromes sit at 1.96% and 10.33% of that range, so a partial run from the start gives both a positive control and a large must-be-empty region.

> **Therefore: 3187813 remains the largest prime palindrome on the curve through d = 37.**
> The 1997 conjecture is now confirmed 30 digits past where it was first found, up from 20.

---

## 8. Limits, and what it would take to go further

**Current ceiling is d = 37, and it is an integer-width limit, not an algorithmic one.** `palsplit` runs the hot loop in `unsigned __int128` with `int64_t` for `n`:

- `p < 10^37` keeps `2p` inside u128;
- `n < 2.24×10^18` at d = 37 — the last odd d that fits `int64_t` (d = 39 needs `n` up to 2.24×10^19, past `INT64_MAX`).

To go further, the hot loop needs a wider `n` (`__int128`) and a wider `p` (GMP, or a fixed 256-bit type — GMP per candidate would likely cost more than the algorithm saves, so a fixed-width type is the better bet).

Projected reach, at t = d/4:

| d | ops | estimate |
|---|---|---|
| 41 | ~3×10^10 | under an hour, threaded |
| ~49–51 | ~10^13 | days — the practical ceiling |

That range matters: it is where the density heuristic in [`OPEN_PROBLEM.md`](OPEN_PROBLEM.md) expects the palindrome count to approach 1. Reaching d ≈ 50 would make the search a genuine **test** of the conjecture rather than another confirmation of it.

## 9. Mandatory regression before any run past d = 27

Same role as the `mod_obstruct` k=6 sanity check. After any rebuild:

    ./palsplit 7  2 --keep5     # 5 hits, incl. 3187813 PRIME
    ./palsplit 13 3 --keep5     # 2   ./palsplit 13 5 --keep5   # 2
    ./palsplit 15 4 --keep5     # 4   ./palsplit 15 6 --keep5   # 4
    ./palsplit 21 5 --keep5     # 5   ./palsplit 21 7 --keep5   # 5
    ./palsplit 25 6 --keep5     # 5   ./palsplit 25 8 --keep5   # 5
    ./palsplit 27 6 --keep5     # 3   ./palsplit 27 8 --keep5   # 3

Values *and* n must match `pals_d13/15/21/25.txt` and `pals.txt` exactly. The whole suite takes under a second. Two rules that are not optional:

1. **Every result past d = 27 must be run at two different t.** Correctness is t-independent, so the two runs share an answer while partitioning the search completely differently. A disagreement means a bug, not a tie to break.
2. **After any edit to `curve.h`, run `make testcurve` first.** It checks `isqrt_u128` at exact squares and their neighbours, at every `2·10^d − 1` the band edge actually feeds it, and at the top of the u128 range; it checks `n_at`/`m_at` invert the curve exactly over the first 10^5 values; and it checks the split identity itself (high t digits == reverse of low t digits) against every palindrome under n = 3×10^5. A band-edge off-by-one is a *silent* miss, so this runs before the regression, not after.
3. **`MAX_HITS` overflow is fatal, by design** — the tool aborts with a nonzero exit rather than silently truncating. Exhaustiveness is the entire value of the tool; a truncated list that looks complete is the worst failure it could have.

---

## 10. Shared code — `curve.h`, and one deliberate duplication

The three tools share their primitives through `curve.h`: the `u128`
container, the pow10 table, `isqrt_u128`, `curve`/`curve_abc`, the band
inverses `n_at`/`m_at`, `digit_at`, `is_pal_d`, `rev_digits` and
`u128_str`. Before this, `isqrt_u128` existed in five copies across the
palindrome tools — five chances for the band-edge fix of §4 to be
applied to four of them.

**It is header-only, which departs from the project rule of factoring
shared logic into a `.h`/`.c` pair.** `curve`, `curve_mod`, `digit_at`
and `isqrt_u128` are called tens of millions of times in the innermost
loop; moving them to a separate translation unit would cost the
inlining the O(10^(d/4)) result depends on. The deviation is confined
to this header.

**`palbrute` keeps its own palindrome predicate, on purpose.**
`palsplit` tests palindromicity with the digit-indexed `is_pal_d`;
`palbrute` reverses the whole integer. Two independent implementations
of the same mathematical test is the entire content of the d = 13…23
corroboration above. Unifying them would leave a check that only proves
one function agrees with itself. `curve()` itself *is* shared, and must
be — if the two tools disagreed about the curve they would be
corroborating nothing.

`isqrt_u128` is shared despite being load-bearing for the band, because
`palbrute` uses it only to seed its n-window and then corrects the seed
against `curve()` directly (`while (curve(nmin) < lo) nmin++`). A wrong
isqrt cannot manufacture agreement between the two tools.

The extraction was verified output-neutral: the full regression matrix
(23 palsplit configs × 2 div-5 modes, 8 palcurve curves, palbrute at
d = 13 and 15) is byte-identical to the pre-extraction binaries apart
from timing jitter, and `docs/curve_palindromes.txt` regenerates
identically — all 443 palindromes on all 13 curves. The boundaries were
checked separately, since the matrix does not reach them: `palsplit` at
d = 35 and d = 37 (the published frontier) and `palcurve` at its
d = 33 cap. Valgrind is clean at the configs tested (`palbrute` d = 13,
`palsplit` d = 17, `palcurve` d = 17, `test_curve`), single-threaded —
**this does not discharge the pre-run Valgrind on the multi-day d = 29
sweep**, whose checkpoint path only fires past 134M iterations and was
never entered here.

### A limit that turned out not to be one

`palcurve`'s `MAX_D = 33` was documented as the point where `4·A·p`
stops fitting a u128. Measuring it says otherwise: for A ≤ 3 that
product binds only at `p < 2.83×10^37`, i.e. **d ≤ 37**, and `m_at` was
verified exact at the top of every d from 30 to 37 for A = 1, 2, 3.
The cap has about four digits of unused headroom.

It has been left at 33 and the comment corrected to say why: raising it
is arithmetically free but would emit palcurve results past d = 31,
where nothing corroborates them. That is a decision to take
deliberately, not a limit to quietly relax.

---

**What this does not do.** Nothing here touches the emirp problem (§3), and nothing here is a proof of non-existence at any d — it is still exhaustive search, only with a better exponent. The conjecture remains open.
