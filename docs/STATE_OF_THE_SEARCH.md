# State of the Search — the BigFermat / bi-quad capstone

> The long-running hunt on the curve **p(n) = 2n² + 2n + 1 = n² + (n+1)²**
> for two rare kinds of prime. This document is the honest summary of where
> the search stands, what is *proven*, what is *believed*, and why it stops
> where it stops.
>
> ⚠ **Status (2026-09-04): the palindrome conjecture is REFUTED.** A
> fifth prime palindrome exists at **d = 59** (De Geest, 2026-04-24).
> §2's headline is struck; §3's bounded statements stand. The
> palindrome frontier belongs to a GPU search and is not ours to
> take; **the emirp is now the live question.** See
> [`session_2026-09-04_palindrome_conjecture_refuted.md`](session_2026-09-04_palindrome_conjecture_refuted.md).
>
> **Status (2026-09-03):** emirps brute-confirmed **through d = 27**
> (unchanged since 2026-07-05). Prime palindromes now searched **through
> d = 37**, with the multi-tool verified floor at **d = 25**. The two
> objects no longer share a frontier — or a search cost. See §4: the
> ~10^(d/2) wall still stands for emirps, and has been broken for
> palindromes.

---

## 1. The curve and the two objects

Every value of `p(n) = 2n² + 2n + 1` is a sum of **two consecutive squares**,
`n² + (n+1)²`, and is therefore ≡ 1 (mod 4) — the tightest special case of
the primes in Fermat's two-square theorem. Hence "BigFermat." Two prizes live
on it:

- **Bi-quadratic emirp** — a prime `p` on the curve whose digit-reversal
  `q = rev(p)` is **also** prime **and also** on the curve, with `q ≠ p`.
  The name "bi-quadratic" means both primes satisfy the same quadratic
  form 2n²+2n+1 — two numbers, one curve.
- **Prime palindrome on the curve** — a prime `p` that reads the same both ways.

A palindrome is just the `m = n` (i.e. `q = p`) case of the emirp relation, so
**both are survivors of the same sieve** and any obstruction kills both.

The consecutive-squares structure pins every value to one of six two-digit
endings `{01, 13, 21, 41, 61, 81}` — a digit filter proven by construction (see
[`ending_constraint_proof.md`](ending_constraint_proof.md)) that discards ~89%
of candidates before any primality test.

---

## 2. Results — proven by exhaustion

### Bi-quadratic emirps: exactly one, through d = 27

**`12641 ⟷ 14621` (d = 5; `79²+80² ⟷ 85²+86²`, both prime) is the only
bi-quadratic emirp through 27 digits.** Established by exhaustive `hunt.c`
(every n, GMP-exact); `EMIRPS = 0` for every d = 6…27, cross-checked against
the modular sieve. Representative tail of the brute log:

```
d=23  range=152,896,119,631  raw=3  prime-eligible=2  EMIRPS=0
d=24  range=483,499,983,437  raw=2  prime-eligible=0  EMIRPS=0
d=25  range=1,528,961,196,313 raw=7 prime-eligible=4  EMIRPS=0   [13.3 h]
d=26  range=4,834,999,834,365  raw=6 prime-eligible=2  EMIRPS=0   [53.9 h]
d=27  range=15,289,611,963,133 raw=5 prime-eligible=3 EMIRPS=0   [120.3 h]
```

The d = 27 run (completed 2026-07-05, ~5.0 days) is the gap-closing run that
brought the emirp frontier up to the palindrome one. Its 5 raw survivors are
3 palindromic curve-values (all composite — see below) plus one reciprocal
non-palindrome pair (`n=9654578976541 ↔ n=16495551105259`, p/q swapped), not an
emirp. (The d=27 counts are div-5-excluded — the run used the skip-optimized
binary; div-5 palindromes are trivially composite and unlisted. See
[`skip_optimization.md`](skip_optimization.md).)

Every curve-reversal survivor through d = 27 has `p` and/or `q` composite,
except the d = 5 emirp itself. True obstructions (no survivor of any kind): d =
6, 10, 18, 20, 22. Full landscape:
[`PROJECT_OVERVIEW.md`](PROJECT_OVERVIEW.md).

> Note: the headline was once *"no emirp ≤ 22 digits."* That was a
> sieve-log misread — the d = 5 emirp was always reported by the tools; the
> prose was wrong, not the engines. Corrected 2026-06-05. The tools were
> sound throughout.

### Prime palindromes: four, through d = 37

The **only** prime palindromes on the curve through **37 digits** are
**5, 181, 313, 3187813** — all ≤ 7 digits. Even d are trivially 0 (divisible by
11), so the genuine tests are the odd d = 9, 11, …, 37, all empty.

Through d = 27 this was established by `palhunt_gmp` (every n, GMP-certified),
with d = 27 clearing 15,289,611,963,133 n-values in ~22.3 h, and independently
corroborated by the emirp brute `hunt.c`, which flagged exactly 3 palindromic
curve-values there — all three factored composite by quadratic sieve.

From d = 29 to d = 37 the frontier was moved by `palsplit` (2026-09-03), which
searches palindromes in **O(10^(d/4))** rather than O(10^(d/2)) — see §4. It
reproduces every earlier ground-truth set exactly, fills in d = 17, 19 and 23
(which had never been checked by anything), and reports the new digit-lengths
as: d=29: 2 palindromes, d=31: 3, d=33: 1, d=35: 4, d=37: 0 — **all
composite, every one with a Miller–Rabin witness, so compositeness is proven
rather than probabilistic.** The whole sweep to d = 37 costs about 30 seconds;
d = 27 alone takes 0.12 s against `palhunt_gmp`'s 22.3 h.

The conjecture (Jim, since ~1997) was:

> ~~**3187813 is the largest prime palindrome on the curve.**~~

> ⚠ **REFUTED 2026-09-04.** A fifth prime palindrome exists at **59
> digits**: `n = 91732095351342012927350087594`. Found by Patrick De
> Geest 2026-04-24 with Robert Xiao's `cudapalin`; verified here
> independently. The bounded form — *largest through d = 37* — still
> stands; the unbounded claim does not. See
> [`session_2026-09-04_palindrome_conjecture_refuted.md`](session_2026-09-04_palindrome_conjecture_refuted.md).

---

## 3. What is proven vs. what is believed (the honesty line)

The frontier moved ten digits in 2026-09; the *verified floor* moved two. Those
are different numbers and the distinction matters more than the headline.

- **Emirps — brute-verified to d = 27.** `hunt.c` (`EMIRPS = 0`), every n,
  GMP-exact. Unchanged.
- **Palindromes — multi-tool verified to d = 25.** Four independent lines agree
  on the complete palindrome set at d = 25: `hunt_noopt` (2026-07-06, with
  quadratic-sieve factorisations as self-checking certificates), `palsplit`,
  and an exhaustive `palbrute` sweep (2026-09-03). `palbrute` shares no logic
  with `palsplit` — different palindrome predicate, different search strategy —
  which is the point of it. d = 13…23 likewise agree between `palsplit` and
  `palbrute`.
- **Palindromes — d = 29…37 rests on `palsplit` alone.** Two independent split
  widths `t` agree (correctness is t-independent, so this is a real
  cross-check, not a repeat), and the compositeness of every candidate is
  proven by MR witness. But no second *tool* has swept that range. A d = 29
  brute is ~4 days and is the outstanding item.
- **Rests on `cvpipe`'s own logs:** anything beyond the brute frontier on the
  emirp side. A verified d = 24 run exists and cross-checks the brute tools.

---

## 4. Structural limits — what *can't* crack it

Rigorously ruled out by measurement (see
[`structural_attacks_2026-06-04.md`](structural_attacks_2026-06-04.md)):

- **No faster EMIRP search.** Meet-in-the-middle gives no √-speedup — the
  digit-reversal of a quadratic entangles the middle digits irreducibly. A
  two-ended DP is ~brute. The emirp search is ~10^(d/2)-bound. Re-confirmed
  2026-09-03: the head and tail of `p` are *independent* choices, so splitting
  at width `t` costs `10^(2t) · 10^(d/2−2t) = 10^(d/2)` for **every** `t` —
  the `t` cancels and there is nothing to optimise.
- **This does NOT hold for palindromes, and no longer stands unqualified.**
  A palindrome's head is *determined* by its tail, which is exactly the
  independence the emirp case lacks. Enumerating `r = n mod 10^t` therefore
  fixes both ends of `p` at once, giving **O(10^(d/4))** — balanced at
  `t ≈ d/4`. This is `palsplit` (2026-09-03), and it moved the palindrome
  frontier from d = 27 to d = 37 in about 30 seconds of compute. The claim
  above was written when both objects shared one bound; they no longer do.
  See [`palindrome_split_search.md`](palindrome_split_search.md).
- **No congruence proof.** A sweep of 50 base-aligned / 2-power / 5-power moduli
  found **zero** obstructions: the real obstructions are **non-congruential**
  (sporadic). A non-existence theorem, if one exists, needs new mathematics.

There is, in short, **no known elementary proof of non-existence**, and no
shortcut that beats brute force. The search is honest exhaustion against a wall.

---

## 5. The heuristic — why this shape is expected

A coin-flip density argument:

- **Bi-quadratic emirps are expected to be *finite*** — expected count ≈ `C/d²`
  per length; the sum converges, with an **expected total ≈ 1 across all
  integers.** That a single emirp (`12641 ⟷ 14621`) exists and no other has
  appeared through 27 digits is exactly what "expected total ≈ 1" predicts.
- **Prime palindromes may be *infinite*** — expected count ≈ `C′/d`; the sum
  diverges. So the conjecture "3187813 is the last" is the *interesting* claim:
  the heuristic does **not** guarantee it, which is precisely why it's worth
  hunting.

The single extra primality condition (both ends prime *and* on-curve, vs. the
palindrome's one end) is what tips emirps from divergent to convergent.

`C′` has since been bounded and the model tested against 13 curves of the same
family — see [`density_cross_curve.md`](density_cross_curve.md). For this curve
`C′ ≈ 1.9–3.3`. The raw-palindrome density is confirmed across all 13 curves to
4%, each curve's own last-digit signature predicting its own constant. Two
things follow that bear on §6 and §7. The silence after 3187813 is
**statistically ordinary** — 3 prime palindromes observed on this curve against
4.5 predicted, while twelve siblings range from none at all (k=7, k=13) to
d = 31 (k=19); whatever makes 3187813 interesting, it is not that this curve
behaves unusually. And the reachable frontier is a **real bet**: d = 29…51
carries a 44–64% chance of an actual hit.

---

## 6. Where each search stops, and why

Both boundaries are **resource boundaries, not mathematical ones** — but they
are now different boundaries, for different reasons.

- **Emirps: d = 27.** Each additional digit is ~3.2× the compute of the last
  and the search is ~10^(d/2)-bound with no known way around it (§4). The
  heuristic expects a total of ≈ 1 emirp across all d, and we have it. Burning
  weeks of CPU per digit for a near-certain null is not where the interest
  lives. The d = 27 run was the deliberate gap-closing pass; the emirp side
  stands there.
- **Palindromes: d = 37, and this is no longer where the economics say stop.**
  `palsplit` cut the exponent in half, so d = 37 costs seconds rather than the
  weeks the old bound implied. The current ceiling is **integer width, not
  compute**: `p < 10^37` keeps `2p` inside a `u128`, and d = 37 is the last odd
  d whose `n` fits an `int64_t`. A 256-bit hot loop reaches d ≈ 51, where §5
  puts the odds of an actual hit at 25–40%.

So the honest statement is no longer "search complete". It is: the emirp side
is parked at a wall we cannot see past, and the palindrome side has a frontier
that moved and a verified floor that has not yet caught up to it. The capstone
rests on the multi-tool verified floor (§3) plus the heuristic. No overclaim of
a proof that doesn't exist; no pretense that the emirp wall is anything but a
wall.

---

## 7. Reviving the search — paused, not closed

Nothing here forecloses a hit at higher d. The obstructions are
non-congruential, the heuristic is a probability not a theorem, and the only
thing stopping the search is compute. **If the economics change, the hunt
reopens:**

- **What reopens it:** a 128-bit-native CPU, a genuine quantum sieve, a
  materially faster engine — anything that pushes the ~10^(d/2) wall outward.
  (The 2026 Zig re-tooling experiment,
  [`zig_experiment_2026-06-06.md`](zig_experiment_2026-06-06.md), is one small
  example of keeping the engine sharp.)
- **What to rebuild:** `hunt.c` (emirps, the ground truth), `palsplit.c`
  (palindromes, O(10^(d/4)) — supersedes `palhunt_gmp.c`), `palbrute.c` (the
  independent palindrome cross-check), `mod_obstruct.c` (the sieve). The shared
  primitives live in `curve.h` (u128 side) and `curve_gmp.h` (GMP side); run
  `make tests` after touching either. Build + sanity steps are in
  [`runbook.txt`](runbook.txt). Sanity check first (d = 5
  must report the emirp; d = 3/7 must report 181/313/3187813). Note: `hunt.c`
  contains a **~35% speedup** via the n mod 10 skip optimization (added
  2026-06-29) — see [`skip_optimization.md`](skip_optimization.md) for the math,
  full history (the same table existed in the 2010 original code and was lost in
  the GMP refactor), and the mixed-binary footnote for the d=27 run.
- **Where to resume:** the frontiers have separated. Emirps sit at **d = 27**
  and the next genuine test is **d = 28**. Palindromes are searched to
  **d = 37**; the next genuine digit is **d = 39**, which needs a 256-bit hot
  loop. The most valuable pending run is not at the frontier at all — it is a
  **d = 29 brute** (~4 days) to bring the verified floor up behind `palsplit`.
- **What's still open:** does a second bi-quadratic emirp exist at any d ≥ 28?
  Is 3187813 truly the last prime palindrome? Both are *unproven* — only
  unobserved.

---

## 8. The human thread

The hunt began after Simon Singh's *Fermat's Enigma* (1997) — a book about
Fermat's *Last* theorem that sent the search toward his *two-square* theorem.
The first champion, the palindrome **3187813**, was found on a **386** by
bending the x87 FPU's 80-bit registers into a 64-bit integer engine. Nearly
three decades later the same number stands as the conjectured largest, now
verified to 27 digits by GMP across two machines — and the curve has given up
exactly one bi-quadratic emirp, `12641 ⟷ 14621`, for all that searching. A long
quiet correspondence with one curve, `2n² + 2n + 1`, carried from a 386 to here.

---

*See also:* [`PROJECT_OVERVIEW.md`](PROJECT_OVERVIEW.md) (glossary, algorithms,
full landscape) · [`ending_constraint_proof.md`](ending_constraint_proof.md)
(the digit filter) · [`skip_optimization.md`](skip_optimization.md) (n mod 10
skip — history, math, d=27 mixed-run footnote) ·
[`structural_attacks_2026-06-04.md`](structural_attacks_2026-06-04.md)
(ruled-out attacks) ·
[`zig_experiment_2026-06-06.md`](zig_experiment_2026-06-06.md) (tooling coda).
