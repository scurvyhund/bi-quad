# The mod-11 constraint on even-digit converse pairs

**Status: PROVEN.**
Derived exhaustively over all residues mod 11,
and verified empirically against every converse pair known to the project (d = 5 … 27, zero violations).

Companion artifact (rendered, with worked digit diagrams):
<https://claude.ai/code/artifact/3d7fede1-9674-4a07-9f0b-1cbfe0901bd6>

Found 2026-08-29, while chasing a *coincidence* that turned out to be nothing —
see "Provenance" at the end.

## The result

> If `d` is even, no converse pair can exist unless `p ≡ 3, 5, 6, or 8 (mod 11)`.

Seven of the eleven residue classes are excluded outright.

This constrains the **coupling** between `p` and `rev(p)`.
It is therefore *not* covered by the congruence sweep ruled out in
[structural_attacks_2026-06-04.md](structural_attacks_2026-06-04.md),
which tested obstructions on `p` alone and found none.

## Definitions

*Local summary only — the canonical glossary is [`GLOSSARY.md`](GLOSSARY.md),
which also carries the generalized Theorems A and B from `curve_families.md`.*


A **converse pair** is `p` together with `q = rev(p)` where *both* lie on the curve.

The curve membership test, which is the move the whole proof depends on:

```
p = 2n² + 2n + 1 = n² + (n+1)²
  ⟺  2p − 1 = (2n+1)²
  ⟺  2p − 1 is an odd perfect square
```

Write `a = 2n + 1` and `b = 2m + 1`, so `a² = 2p − 1` and `b² = 2q − 1`.
An emirp is a converse pair that is prime on both sides;
a prime palindrome is the degenerate case `m = n`.

## Why 11 is the only modulus that sees reversal

Digit reversal respects no arithmetic structure.
Exactly two moduli say anything about it, both because a power of ten is trivially predictable there:

- **mod 9** — `10 ≡ 1`, so every power of ten is 1 and a number is congruent to its digit **sum**.
  Reversal preserves the digit sum, so `q ≡ p (mod 9)` for every `d`.
  True but weak: it treats all digit positions alike, so it cannot distinguish reversal from any other permutation of digits.
- **mod 11** — `10 ≡ −1`, so powers of ten **alternate** `1, −1, 1, −1, …` and a number is congruent to its **alternating** digit sum.
  This is the one that sees reversal, because reversing flips which positions carry the minus sign — and whether that flip matters depends entirely on the parity of `d`.

That last clause is the whole result in embryo.

## The reversal law

With digits `c₀` (units) through `c_{d−1}` (leading):

```
p = Σ cᵢ·10^i        ≡ Σ cᵢ·(−1)^i
q = Σ cᵢ·10^(d−1−i)  ≡ Σ cᵢ·(−1)^(d−1−i)
                      = (−1)^(d−1) · Σ cᵢ·(−1)^i
                      ≡ (−1)^(d−1) · p        (mod 11)
```

```
d odd   →  q ≡  p  (mod 11)
d even  →  q ≡ −p  (mod 11)
```

The law assumes `rev(p)` still has `d` digits, which fails only if `p` ends in 0.
On this curve `p = 2n²+2n+1` is always odd, so the hypothesis is free.

## The collision (even d)

```
a² = 2p − 1
b² = 2q − 1 ≡ 2(−p) − 1 = −2p − 1        (d even)

a² + b² ≡ (2p − 1) + (−2p − 1) = −2
a² + b² ≡ 9   (mod 11)
```

`p` cancels completely: whatever `p` is, the two squares attached to the two ends of a converse pair must sum to 9 mod 11.

The squares mod 11 are `{0, 1, 3, 4, 5, 9}`.
Ordered pairs from that set summing to 9:

| a² | b² | sum | both squares? |
|----|----|-----|---------------|
| 0  | 9  | 9   | yes |
| 9  | 0  | 9   | yes |
| 4  | 5  | 9   | yes |
| 5  | 4  | 9   | yes |
| 1  | 8  | 9   | no — 8 is not a square |
| 3  | 6  | 9   | no — 6 is not a square |

So `a² ∈ {0, 4, 5, 9}`.
Since `2⁻¹ ≡ 6 (mod 11)`, `p ≡ 6(a² + 1)`:

| a² mod 11 | p mod 11 | b² mod 11 | q mod 11 |
|-----------|----------|-----------|----------|
| 0 | **6** | 9 | **5** |
| 9 | **5** | 0 | **6** |
| 4 | **8** | 5 | **3** |
| 5 | **3** | 4 | **8** |

Allowed `p mod 11`: `{3, 5, 6, 8}`.
Excluded: `{0, 1, 2, 4, 7, 9, 10}`.

The four flavours pair off into two swaps, `(6,5) ↔ (5,6)` and `(8,3) ↔ (3,8)`.
That symmetry is forced: if `p` is one end of a converse pair then `q` is the other end of the same pair, so the allowed set must be closed under swapping.

## Why odd d gives nothing

```
d odd  →  q ≡ p  →  b² = 2q − 1 ≡ 2p − 1 = a²
       →  b ≡ ±a (mod 11)
```

This constrains `a` against `b` but places **no restriction on `p` itself** — any residue works, since `b ≡ a` is always available.
The two ends stop fighting each other.

Both known odd-d objects of interest — the d=5 emirp 12641↔14621 and the d=7 near-miss 1426361↔1636241 — are invisible to this constraint, which is consistent with them existing.

## Corollary: even-digit palindromes (sanity check)

A prime palindrome on the curve is the degenerate converse pair `m = n`, hence `q = p`.
With `d` even:

```
q = p  and  q ≡ −p   →   p ≡ −p   →   2p ≡ 0   →   11 | p   →   p composite
```

That is the classical fact — *every even-digit palindrome is divisible by 11* — recovered as a special case.

**On this curve it is strictly stronger than that.**
A palindrome has `m = n`, so `b = a`, and the general even-`d` identity `a² + b² ≡ 9` becomes:

```
2a² ≡ 9   →   a² ≡ 9 · 2⁻¹ ≡ 9 · 6 = 54 ≡ 10   (mod 11)
```

but `10` is not a square mod 11 (the squares are `{0,1,3,4,5,9}`).
So there is no such `a` at all:

> **No even-digit palindrome lies on the curve `p = 2n²+2n+1`** — prime or composite.

Not "they exist and are composite": they do not exist.
Confirmed empirically — zero even-digit palindromes on the curve below 10^15.

Two consequences:

- The general even-`d` result is the same mechanism, applied to the case where the two ends may differ.
  The palindrome case is *tighter* precisely because `q = p` is the strongest possible coupling — it over-constrains to the point of impossibility.
- It explains a pattern already on disk: every palindrome dump (`pals_d13.txt`, `pals_d15.txt`, `pals_d21.txt`, `pals_d25.txt`) is at **odd** `d`.
  The even lengths were never worth searching, and
  the 3187813 conjecture (see [palindrome_insights.md](palindrome_insights.md)) is really a statement about odd `d` only.

The useful way to hold all of this:
digit reversal buys you a sign, `(−1)^(d−1)`.
At odd `d` the sign is `+1` and costs nothing.
At even `d` it is `−1`, and that minus sign *is* the constraint — fatal for palindromes, merely expensive for converse pairs.

## Verification

Every even-`d` converse pair known to the project, from an independent Python scan to 10^14 and from the `survivor` lines in `logs/` out to d = 27:

| d  | n             | p                          | p%11 | q%11 | flavour |
|----|---------------|----------------------------|------|------|---------|
| 8  | 2732          | 14933113                   | 8    | 3    | (8,3) ok |
| 12 | 304068        | 184915305385               | 8    | 3    | (8,3) ok |
| 14 | 2462740       | 12130181540681             | 6    | 5    | (6,5) ok |
| 14 | 2479439       | 12295240468321             | 8    | 3    | (8,3) ok |
| 14 | 2522215       | 12723142056881             | 3    | 8    | (3,8) ok |
| 26 | 3000805374559 | 18009665791970362179638081 | 3    | 8    | (3,8) ok |

Zero violations.
Also clean across all **21 distinct** known converse pairs at both parities —
15 from the scan to 10^14, 16 from the log harvest, overlapping in 10:
the reversal law `q ≡ (−1)^(d−1)·p` holds 21/21,
and the mod-9 corollary `3|a ⟺ 3|b` holds 21/21.
The `(5,6)` flavour has no known instance yet — permitted, just unobserved.

### Converse-pair census

| Source | Range | Pairs | Emirps |
|--------|-------|-------|--------|
| Independent Python scan | p < 10^14 | 15 | 1 (d=5) |
| Survivor lines in `logs/` | d = 7 … 27 | 16 | 0 |
| **Distinct union** | **d = 5 … 27** | **21** | **1** |

Only 15 converse pairs below 10^14, exactly one of which is an emirp.
**Primality is not the bottleneck — the pairing is.**

## A hypothesis that died here

An apparent regularity — `b ≡ −a (mod 9)` whenever `3 ∤ a` — held 8/8 in the scan to 10^14, roughly 1-in-256 by chance.
Pulling the d=15…27 survivors out of `logs/` broke it:
the d=23 pair (n=70936538324) has `a ≡ b ≡ 2 (mod 9)`.

Small-sample artifact, discarded.
Recorded so it is not rediscovered and re-chased.

## What this is worth

- **Not a speedup.**
  Enumerating converse pairs is still O(10^(d/2)).
  Excluding 7 of 11 classes prunes a constant fraction; it does not touch the exponent.
  The search ceiling around d = 25–27 is unchanged.
- **Not an obstruction.**
  Four classes survive, so it proves nothing about non-existence.
- **A free correctness assertion**, which is where the real value sits given this codebase's history (interval over-approximation, the d=21 cliff, the lossy skip-optimisation).

### Where the assertion is valid — and where it is NOT

The word "survivor" means different things in the two engines, and the constraint applies to only one of them unconditionally.
Getting this backwards would fire the check on legitimate candidates and abort a 100-hour run, so it is worth stating precisely.

| Engine | What a survivor is | Assertion valid? |
|--------|--------------------|------------------|
| `hunt.c` | exact converse pair: `q` is the literal digit reversal and `on_curve(q)` passed | **always** |
| `mod_obstruct.c` | residue class matching on `p`'s first `k` and last `k` digits | **only when `d ≤ 2k`** |

`mod_obstruct` pins the first `k` and last `k` digits of `p`.
Those cover all `d` digits only when `d ≤ 2k`.
Past that the middle digits are free, `q` need not equal `rev(p)`, and `p mod 11` is unconstrained — a survivor violating the four classes there is **not** a bug.
Within `d ≤ 2k` the survivor is a genuine converse pair and the check is exact.
(At the standard `k = 10` that means `d ≤ 20`, which is also where the `range < mod` validity limit sits.)

Both assertions are implemented as **loud diagnostics, never `abort()`** — a long run should not die on a check, and aborting a worker mid-pass discards the survivor count it was about to report.

```c
/* hunt.c -- survivors are exact converse pairs, so no guard.
   At even d: a palindrome would need a^2 = 10 (mod 11), impossible,
   so pal must be unreachable; otherwise p mod 11 in {3,5,6,8}. */
if (d % 2 == 0) {
   unsigned long r11 = mpz_fdiv_ui(p, 11);
   if (pal || (r11 != 3 && r11 != 5 && r11 != 6 && r11 != 8)) {
      /* report loudly, do not abort */
   }
}

/* mod_obstruct.c -- MUST be guarded: only d <= 2k pins every digit. */
if (d % 2 == 0 && d <= 2 * k) {
   unsigned long nr  = mpz_fdiv_ui(t_n, 11);
   unsigned long r11 = (2 * nr * nr + 2 * nr + 1) % 11;
   if (r11 != 3 && r11 != 5 && r11 != 6 && r11 != 8) {
      /* report loudly, do not abort */
   }
}
```

Both are now in the tree.
(Note: the `11` in the k=6 sanity command `./mod_obstruct 50 6 6 11` is `min_d`, not a modulus.
Also, the build line in `CLAUDE.md` omits `-lgmp`, which `mod_obstruct.c` needs.)

## Provenance

This came out of chasing a coincidence that was not real.

The d=7 pair (n=844, m=904) and the d=5 emirp (n=79, m=85) share a fingerprint:
both have `a ≡ 6`, `b ≡ 0 (mod 9)` — the rare "3 divides both" class — and both have `p ≡ q ≡ 2 (mod 11)`.
It looked like a link between the near-miss and the only known emirp.

It is not.
A third pair at d=21 (n=8529868069) sits in the identical class,
and `a ≡ 6 (mod 9)`, `p ≡ 5 (mod 9)` and `n ≡ 7 (mod 9)` are all the *same fact* rather than three independent matches.
Two coordinates hitting 3 of 21 pairs is a coincidence.

The fingerprint was a dead end.
The machinery built to test it was not.
