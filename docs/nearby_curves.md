# Nearby Curves — the mod-11 result is a family theorem

Written 2026-08-30, from the question: *"might there be insight gained from looking at numbers on other curves 'nearby' — is that even a thing?"*

It is a thing, it has a precise definition, and it produced two genuine generalizations plus one useful negative result.

---

## 1. What "nearby" means

Our curve is the line `y = x + 1` crossing circles `x² + y² = p` (see `curve_geometry.md`).
The natural neighbours slide the line:

    p = n² + (n+k)²

- **k = 0** → `2n²`, never prime past 2.
- **k even** → `2(n² + kn + k²/2)`, always even. Dead.
- **k odd** → live.

So the family is **k = 1, 3, 5, 7, 9, …** and we live at **k = 1**.

### Membership generalizes cleanly

Set `a = 2n + k`. Then:

    2p = a² + k²

Verified with zero counterexamples for k ∈ [0,11], n ∈ [0,300).
At k=1 this is our familiar `2p − 1 = a²`, i.e. *2p−1 is an odd perfect square*.

---

## 2. PROVEN — the mod-11 constraint holds for every odd k

The reversal law is unchanged: `10 ≡ −1 (mod 11)`, so at even d, `q = rev(p) ≡ −p (mod 11)`.

Substituting `2p = a² + k²` and `2q = b² + k²`:

    b² + k² ≡ −(a² + k²)
    a² + b² ≡ −2k²   (mod 11)

Squares mod 11 are `{0, 1, 3, 4, 5, 9}`.
Solving for each k:

| k | k² mod 11 | target −2k² | allowed p mod 11 | classes killed |
|---|---|---|---|---|
| **1** | 1 | 9 | **{3, 5, 6, 8}** | 7 of 11 |
| 3 | 9 | 4 | {1, 5, 6, 10} | 7 of 11 |
| 5 | 3 | 5 | {2, 4, 7, 9} | 7 of 11 |
| 7 | 5 | 1 | {3, 4, 7, 8} | 7 of 11 |
| 9 | 4 | 3 | {1, 2, 9, 10} | 7 of 11 |
| 11 | 0 | 0 | {0} | 10 of 11 |
| 13 | 4 | 3 | {1, 2, 9, 10} | 7 of 11 |

The k=1 row is exactly the result proven in `mod11_converse_constraint.md`.
**It was never a fact about our curve** — it is one row of a family theorem.

Empirically confirmed: across all even-d converse pairs found below 10⁸ on k = 1,3,5,7,9,11, **zero** violations.

---

## 3. PROVEN — no even-digit palindrome, unless 11 | k

A palindrome is the degenerate converse pair `m = n`, hence `b = a`:

    2a² ≡ −2k²      ⟹      a² ≡ −k²   (mod 11)

Since `11 ≡ 3 (mod 4)`, **−1 is a quadratic non-residue mod 11**.
So `a² ≡ −k²` forces `(a·k⁻¹)² ≡ −1`, which is impossible — *unless* `11 | k`, in which case both sides vanish and `a ≡ 0 (mod 11)` is permitted.

> **Theorem.** For every odd k with `11 ∤ k`, no even-digit palindrome lies on the curve `p = n² + (n+k)²` at all.

This is strictly stronger than the classical statement ("even-digit palindromes are divisible by 11") in the same way the k=1 version was.

### The exception is real, which is what makes it a mechanism

The theory does not merely forbid; it says **exactly where** the exception lives. Searching to 2×10⁸:

| k | 11 \| k? | even-digit palindromes on curve |
|---|---|---|
| 1, 3, 5, 7, 9, 13 | no | **0** |
| 11 | yes | 523325 |
| 22 | yes | 4114, 296692 |
| 33 | yes | 5445 |

`4114 = 33² + 55²` — a genuine curve point at n=33, k=22.

**But none of them is prime**, and cannot be: every even-digit palindrome is divisible by 11 (classical). So the two regimes reach the same destination by different roads:

- `11 ∤ k` — even-digit palindromes are **not on the curve** (our theorem)
- `11 | k` — they are on the curve but **automatically composite** (classical)

Either way there is no even-digit *prime* palindrome anywhere in the family.

---

## 4. TREND — our curve is unremarkable, and that is the point

Exhaustive below 10⁹:

| k | curve primes | converse pairs | emirp pairs |
|---|---|---|---|
| 1 | 3349 | 16 | **1** — 12641 ↔ 14621 |
| 3 | 2170 | 8 | 0 |
| 5 | 4444 | 8 | **1** — 37 ↔ 73 |
| 7 | 2810 | 2 | 0 |
| 9 | 2228 | 4 | 0 |
| 11 | 2997 | 8 | 0 |

**k=5 has its own bi-quadratic emirp: 37 ↔ 73.**

    37 = 1² + 6²   (n=1, k=5)   prime
    73 = 3² + 8²   (n=3, k=5)   prime
    rev(37) = 73,  rev(73) = 37

It is d=2 — *even* — and it obeys the constraint above: `37 mod 11 = 4`, `73 mod 11 = 7`, both in k=5's allowed set `{2,4,7,9}`.

So **12641 ↔ 14621 is not a freak of our curve**; it is what this family does. Converse pairs are the bottleneck on every curve (2–16 below a billion), and primality on top behaves like a coin toss.

This is TREND, not proof — six curves is a small sample, and the emirp counts are far too low for the differences between them to mean anything.

But it corroborates the central claim of `curve_geometry.md` empirically: **the difficulty is family-wide, therefore it does not live in our curve.** It lives in primality + base-10 reversal, which every member of the family shares.

---

## 5. What this does and does not open

### Does — a control group

Any future structural attack should be run against k = 3, 5, 7 as well.
An attack that "works" only at k=1 is almost certainly exploiting an artifact, because nothing in the family distinguishes k=1.
This is cheap insurance against the three dead ends in `structural_attacks_2026-06-04.md`.

### Does — a smaller test case

`37 ↔ 73` is a **d=2** emirp. The smallest case previously available was 12641 (d=5).
Useful if the engines ever gain a k parameter.

### Does NOT — a palindrome positive control

This was the initial hope and it fails, for the reason in §3: the `11|k` palindromes are all divisible by 11, so a *prime*-palindrome search correctly reports nothing on those curves too.

Recorded here so nobody re-derives the idea and builds the tooling.

Note also that `palhunt` already has positive controls on our own curve — it finds **5, 181, 313, 3187813**. A search returning none of those is broken. That regression marker already existed.

---

## 6. Verification

Every claim above is checked by:

    python3 docs/nearby_curves.py

which independently re-derives the membership identity, the mod-11 table, the palindrome theorem and its `11|k` exception, and the family emirp census.
It is deliberately standalone — it shares no code with `hunt.c` or `mod_obstruct.c`, so agreement is evidence rather than a common bug.

---

## See also

- `curve_geometry.md` — why the curve is a conic, and where the hardness lives
- `mod11_converse_constraint.md` — the k=1 case, proven in full
- `structural_attacks_2026-06-04.md` — the three eliminated approaches
- `GLOSSARY.md` — terms used across the project
- `curve_families.md` — the general theorem: turning the *quadratic form* dial
