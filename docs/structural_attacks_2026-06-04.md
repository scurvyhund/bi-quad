# Structural Attacks — 2026-06-04: Why Neither Faster Search Nor a Congruence Proof Cracks It

> ⚠️ **Note 2026-06-05:** the structural conclusions here (no MITM √-speedup, no
> congruence obstruction) still stand. But the "no bi-quadratic emirp ≤ 22 digits"
> premise they reference is corrected: `12641 ⟷ 14621` IS an emirp (d=5) — it's the
> **only** one through 24 digits. See [`session_2026-06-05_emirp_d5_correction.md`](session_2026-06-05_emirp_d5_correction.md).

> After proving **no bi-quadratic emirp ≤ 22 digits**, we asked the two natural
> follow-ups: (a) can we *search* faster than brute force (10^(d/2)), and (b) can
> we *prove* non-existence for **all** large d? We attacked both — rigorously,
> by building probes and measuring against ground truth — and got three clean
> **negative** results. This documents what we tried, what we found, and why.
>
> Negatives are results too: they tell us (and future-us) exactly which doors are
> closed, and *why*. The probe code is archived in git (commit `7f33475`).

---

## TL;DR

| Attack | Goal | Probe | Verdict |
|---|---|---|---|
| **MITM** (split n) | faster search | `mitm_probe.c` | clean digit-separation only ~d/4 → **no √-speedup** |
| **Two-ended DP** | faster search | `mitm_probeB.c` | midpoint ~10^(d/2) → **brute in disguise** |
| **Congruence** | prove non-existence | `congru_probe.c` | **no obstruction, any modulus, any d** |

Common root cause: **digit-reversal of a quadratic is "anti-structural."**
Reversal doesn't commute with arithmetic, so it defeats *both* algebraic (MITM)
and modular (congruence) machinery — which is exactly why digit-reversal
Diophantine problems are famously hard / open.

---

## 1. MITM — the meet-in-the-middle search (faster search?)

**Idea.** Split `n = a·10ᵗ + b`. The binomial expansion
`p = 2n²+2n+1 = 2a²·10²ᵗ + 2a·10ᵗ + 4ab·10ᵗ + (2b²+2b+1)` has an exact keystone:

> **p's low t digits = (2b²+2b+1) mod 10ᵗ — a pure function of b.**

So (via reversal) q's *top* t digits depend only on b, and a controls the other
end. If the two ends were independent you'd collide two tables of ~10^(d/4)
instead of sweeping 10^(d/2) — a √-speedup (d=40: 10²⁰ → 10¹⁰).

**Probe (`mitm_probe.c`).** Measured the actual clean-zone widths:

```
 d    pureB(low)  pureA(high)  coupled
20       5            3           12
24       6            4           14
30       7            6           17
```

`pureB = t` exactly (the keystone holds), but `pureA` is *thin* — the low end is
**carry-immune** (clean `mod 10ᵗ`), the high end is **carry-exposed** (carries
from the bilinear middle smear *upward*). Coupled middle ≈ **d/2 + 1.5**.

**Verdict.** The easy MITM (two independent halves) is dead — only ~d/4 of the
digits separate cleanly; the `4ab` cross term bilinearly couples the middle half.

## 2. Two-ended digit-DP (faster search, take two)

**Idea.** Build p from both ends, pruning partial strings that can't satisfy both
"p on curve" and "rev(p) on curve." Hope: the two square-constraints prune so
hard that survivors stay small.

**Probe (`mitm_probeB.c`).** Counted, at the half-way point, the partial strings
simultaneously consistent with a p *and* a q (= the meet-in-the-middle cost):

```
 d    range(brute)   alive(B cost)   alive/range
12      483,500          11,236         2.3%
16   48,349,998       1,089,936         2.25%
```

`alive ≈ 0.023 × range ≈ 10^(d/2)`. The two constraints prune a flat ~44×
constant factor — **not** the exponential. Brute force with a coupon.

**Verdict.** Same wall as MITM, same reason: the entanglement is irreducible.

---

## 3. The CONGRUENCE venture (prove non-existence for all large d)

This is the one to dwell on, because it came from a genuinely good instinct and
the negative is informative.

### The instinct → the redirect

Jim asked: *can we work in a different number base to eliminate the carries?* The
honest literal answer is no (carries live in every positional base; residue
systems kill them but destroy the digits/reversal we need; and anyway carries
weren't the wall — the bilinear coupling was). **But the instinct was right, aimed
one notch over:** you don't change the base of the *numbers* — you choose the
**modulus** to align with base 10, and the reversal becomes clean, carry-free
algebra:

- **mod 9:** a number ≡ its digit-sum; reversal doesn't change the digits →
  **rev(p) ≡ p (mod 9).** The reversal becomes *invisible*.
- **mod 11:** a number ≡ its *alternating* digit-sum; reversing flips the sign by
  digit-count parity → **rev(p) ≡ ±p (mod 11).**

The mod-11 fact is the same one that makes even-length palindromes divisible by 11
(rev=p, even d ⇒ p ≡ −p ⇒ 11|p) — the rule we used to kill even-d prime
palindromes. So the base-aligned moduli **9, 11, 99, 101, 1001, …** (divisors of
10ᵏ ± 1) are exactly where digit-reversal stops fighting us.

### The probe (`congru_probe.c`)

For a modulus M and digit-length d, an **exact reachability DP** over the joint
state `(p mod M, rev(p) mod M)`: each digit position i contributes `10ⁱ` to p and
`10^(d−1−i)` to rev, so the reversal is captured *exactly*, no carries, no
approximation. Leading/trailing digits constrained odd & nonzero. If **no**
reachable state has both coordinates in `C_M = {2x²+2x+1 mod M}`, that (M,d) is a
**congruence obstruction** — a pencil-and-paper proof that no d-digit emirp exists.

### The sweep & the result

**50 moduli** — base-aligned (9, 11, 33, 99, 101, 1001, 1111, 7, 13, 91, 143, 41,
37, 111, 999, 121, 1221, 239), all powers of 2 and 5 (4…1024, 5…625, the curve's
*strongest* low-digit structure), and composites — across **d = 8…30**.

> **ZERO obstructions. Anywhere.** Every (M, d) is feasible.

### Why — and what it means

Modular feasibility is **too weak**. With d ≥ 8 free middle digits you can always
satisfy a fixed-M curve constraint on both ends. Structurally: for a base-aligned
M the reachable `(P,R)` lives in a linear subspace (R ≡ ±P, etc.), and an
obstruction would need that subspace to miss `C_M × C_M` — but `C_M ∩ (±C_M) ≠ ∅`
for every modulus we tested. The **only** modulus that *sees* an obstruction is one
big enough to pin every digit, i.e. ~10^d = the whole number — which is exactly
the mod-10ᵏ sieve, and *that's* why the sieve needed k ~ d/2. **There is no
fixed-M shortcut.**

> **The real obstructions (d=16, 18, 19, 20, 21, 22) are NON-congruential —
> "sporadic." No finite modulus explains them.**

So a non-existence *theorem* via congruence does not exist. If one exists at all,
it needs genuinely new mathematics, or the question is open.

---

## 4. The consolation prize: a heuristic for *why* (the coin-flip argument)

The same brainstorm produced a positive insight — the standard density heuristic
(à la Mersenne/Fermat-prime heuristics):

- d-digit curve numbers: ~10^(d/2). Chance rev(p) is also on the curve: ~10^(−d/2).
  → curve-reversal *pairs* per d ≈ **O(1)** (matches d=13→4, d=14→6 ✓).
- **Emirps need TWO primes** (p and rev(p), different numbers): ×1/(d·ln10)² →
  expected ≈ **C/d²**. **Σ 1/d² converges → finitely many emirps, likely none.**
  *The heuristic predicts our "no emirp" result.*
- **Prime palindromes need ONE prime** (p = rev(p)): ×1/(d·ln10) → expected ≈
  **C′/d**. **Σ 1/d diverges → possibly infinitely many.** This mildly *challenges*
  Jim's conjecture that 3187813 is the last — a new (very sparse) champion could
  lurk at large odd d. (Caveats: tiny constant; structural rules like ÷11 could
  override; divergence is agonizingly slow.)

The single extra primality condition is what tips emirps (1/d², finite) versus
palindromes (1/d, maybe infinite) — a clean, satisfying distinction.

---

## 5. Implications & artifacts

- **Search is ~10^(d/2)-bound.** No structural speedup; brute force (`hunt.c`,
  `palhunt_gmp.c`) caps near d≈25.
- **No elementary congruence non-existence proof.** The contribution is the
  **computational** result (no emirp ≤ 22 digits, 4-way validated) + the
  obstruction landscape + the palindrome unification + this heuristic.
- **Don't re-walk these three roads** without a genuinely new idea.

**Probe code** (archived in git, recover with `git show 7f33475:<file>`):
`mitm_probe.c`, `mitm_probeB.c`, `congru_probe.c`. Memory:
`structural_attacks_ruled_out`. Companion: [`PROJECT_OVERVIEW.md`](PROJECT_OVERVIEW.md),
[`session_2026-06-04_archive.md`](session_2026-06-04_archive.md).
