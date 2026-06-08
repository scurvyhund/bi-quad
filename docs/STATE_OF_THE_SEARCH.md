# State of the Search — the BigFermat / bi-quad capstone

> The long-running hunt on the curve **p(n) = 2n² + 2n + 1 = n² + (n+1)²** for two
> rare kinds of prime. This document is the honest summary of where the search
> stands, what is *proven*, what is *believed*, and why it stops where it stops.
>
> **Status (2026-06-07):** emirps brute-confirmed **through d = 26**; prime
> palindromes brute-confirmed **through d = 27**. Search complete at the agreed
> frontiers.

---

## 1. The curve and the two objects

Every value of `p(n) = 2n² + 2n + 1` is a sum of **two consecutive squares**,
`n² + (n+1)²`, and is therefore ≡ 1 (mod 4) — the tightest special case of the
primes in Fermat's two-square theorem. Hence "BigFermat." Two prizes live on it:

- **Bi-quadratic emirp** — a prime `p` on the curve whose digit-reversal
  `q = rev(p)` is **also** prime **and also** on the curve, with `q ≠ p`.
- **Prime palindrome on the curve** — a prime `p` that reads the same both ways.

A palindrome is just the `m = n` (i.e. `q = p`) case of the emirp relation, so
**both are survivors of the same sieve** and any obstruction kills both.

The consecutive-squares structure pins every value to one of six two-digit
endings `{01, 13, 21, 41, 61, 81}` — a digit filter proven by construction (see
[`ending_constraint_proof.md`](ending_constraint_proof.md)) that discards ~89% of
candidates before any primality test.

---

## 2. Results — proven by exhaustion

### Bi-quadratic emirps: exactly one, through d = 26

**`12641 ⟷ 14621` (d = 5; `79²+80² ⟷ 85²+86²`, both prime) is the only
bi-quadratic emirp through 26 digits.** Established by exhaustive `hunt.c` (every
n, GMP-exact); `EMIRPS = 0` for every d = 6…26, cross-checked against the modular
sieve. Representative tail of the brute log:

```
d=23  range=152,896,119,631  raw=3  prime-eligible=2  EMIRPS=0
d=24  range=483,499,983,437  raw=2  prime-eligible=0  EMIRPS=0
d=25  range=1,528,961,196,313 raw=7 prime-eligible=4  EMIRPS=0   [13.3 h]
d=26  range=4,834,999,834,365  raw=6 prime-eligible=2  EMIRPS=0   [53.9 h]
```

Every curve-reversal survivor through d = 26 has `p` and/or `q` composite, except
the d = 5 emirp itself. True obstructions (no survivor of any kind): d = 6, 10,
18, 20, 22. Full landscape: [`PROJECT_OVERVIEW.md`](PROJECT_OVERVIEW.md).

> Note: the headline was once *"no emirp ≤ 22 digits."* That was a sieve-log
> misread — the d = 5 emirp was always reported by the tools; the prose was wrong,
> not the engines. Corrected 2026-06-05. The tools were sound throughout.

### Prime palindromes: four, through d = 27

The **only** prime palindromes on the curve through **27 digits** are
**5, 181, 313, 3187813** — all ≤ 7 digits. Confirmed by `palhunt_gmp` (every n,
GMP-certified): `found = 0` for d = 8…27 (even d are trivially 0 — divisible by 11
— so the genuine tests are the odd d = 9, 11, …, 27, all empty). d = 27 cleared a
range of 15,289,611,963,133 n-values in ~22.3 h. The standing conjecture (Jim,
since ~1997):

> **3187813 is the largest prime palindrome on the curve.**

Now confirmed 20 digits past where it was first found.

---

## 3. What is proven vs. what is believed (the honesty line)

- **Brute-verified (two independent tools):** emirps to d = 26, palindromes to
  d = 27 — `hunt.c` (`EMIRPS = 0`) and `palhunt_gmp` (`found = 0`). This is the
  hard floor of the result.
- **Rests on `cvpipe`'s own logs (not yet brute-verified):** anything beyond the
  brute frontier. The fast pipeline ran further (a verified d = 24 run exists and
  cross-checks the brute tools), but d = 26 is the brute-force ceiling for emirps.
  The agreed stopping points sit exactly at the brute-verified floor; nothing is
  promoted beyond that.

---

## 4. Structural limits — what *can't* crack it

Rigorously ruled out by measurement (see
[`structural_attacks_2026-06-04.md`](structural_attacks_2026-06-04.md)):

- **No faster search.** Meet-in-the-middle gives no √-speedup — the digit-reversal
  of a quadratic entangles the middle digits irreducibly. A two-ended DP is ~brute.
  Search is ~10^(d/2)-bound.
- **No congruence proof.** A sweep of 50 base-aligned / 2-power / 5-power moduli
  found **zero** obstructions: the real obstructions are **non-congruential**
  (sporadic). A non-existence theorem, if one exists, needs new mathematics.

There is, in short, **no known elementary proof of non-existence**, and no shortcut
that beats brute force. The search is honest exhaustion against a wall.

---

## 5. The heuristic — why this shape is expected

A coin-flip density argument:

- **Bi-quadratic emirps are expected to be *finite*** — expected count ≈ `C/d²`
  per length; the sum converges, with an **expected total ≈ 1 across all
  integers.** That a single emirp (`12641 ⟷ 14621`) exists and no other has
  appeared through 26 digits is exactly what "expected total ≈ 1" predicts.
- **Prime palindromes may be *infinite*** — expected count ≈ `C′/d`; the sum
  diverges. So the conjecture "3187813 is the last" is the *interesting* claim:
  the heuristic does **not** guarantee it, which is precisely why it's worth
  hunting.

The single extra primality condition (both ends prime *and* on-curve, vs. the
palindrome's one end) is what tips emirps from divergent to convergent.

---

## 6. Why the search stops here

This is a **resource boundary, not a mathematical one.** Each additional digit is
~3.2× the compute of the last; the heuristic says the expected yield beyond the
known results is ≈ 0 (emirps) and the palindrome conjecture has held 20 digits
deep. Burning weeks of CPU per digit for a near-certain null is not where the
interest lives. So the agreed final frontiers are:

- **Emirps: d = 26** (done) — then stop.
- **Palindromes: d = 27** (done) — then stop.

The capstone rests on the brute-verified floor plus the convergent heuristic. No
overclaim of a proof that doesn't exist; no pretense that the wall is anything but
a wall.

---

## 7. Reviving the search — paused, not closed

Nothing here forecloses a hit at higher d. The obstructions are non-congruential,
the heuristic is a probability not a theorem, and the only thing stopping the
search is compute. **If the economics change, the hunt reopens:**

- **What reopens it:** a 128-bit-native CPU, a genuine quantum sieve, a materially
  faster engine — anything that pushes the ~10^(d/2) wall outward. (The 2026 Zig
  re-tooling experiment, [`zig_experiment_2026-06-06.md`](zig_experiment_2026-06-06.md),
  is one small example of keeping the engine sharp.)
- **What to rebuild:** `hunt.c` (emirps, the ground truth), `palhunt_gmp.c`
  (palindromes past the 64-bit wall), `mod_obstruct.c` (the sieve). Build + sanity
  steps are in [`runbook.txt`](runbook.txt). Sanity check first (d = 5 must report
  the emirp; d = 3/7 must report 181/313/3187813).
- **Where to resume:** emirp frontier **d = 26**, palindrome frontier **d = 27**.
  Pick up at the next digit.
- **What's still open:** does a second bi-quadratic emirp exist at any d ≥ 27? Is
  3187813 truly the last prime palindrome? Both are *unproven* — only unobserved.

---

## 8. The human thread

The hunt began after Simon Singh's *Fermat's Enigma* (1997) — a book about
Fermat's *Last* theorem that sent the search toward his *two-square* theorem. The
first champion, the palindrome **3187813**, was found on a **386** by bending the
x87 FPU's 80-bit registers into a 64-bit integer engine. Nearly three decades
later the same number stands as the conjectured largest, now verified to 27 digits
by GMP across two machines — and the curve has given up exactly one bi-quadratic
emirp, `12641 ⟷ 14621`, for all that searching. A long quiet correspondence with
one curve, `2n² + 2n + 1`, carried from a 386 to here.

---

*See also:* [`PROJECT_OVERVIEW.md`](PROJECT_OVERVIEW.md) (glossary, algorithms,
full landscape) · [`ending_constraint_proof.md`](ending_constraint_proof.md) (the
digit filter) · [`structural_attacks_2026-06-04.md`](structural_attacks_2026-06-04.md)
(ruled-out attacks) · [`zig_experiment_2026-06-06.md`](zig_experiment_2026-06-06.md)
(tooling coda).
