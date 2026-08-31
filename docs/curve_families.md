# Curve Families — one mod-11 theorem for every conic curve

Written 2026-08-30, from the question: *"our numbers are there because of the consecutive-squares constraint — can we build numbers with a **different** constraint that end up on a different curve?"*

The answer is yes, and chasing it produced the strongest theoretical result in the project so far: **the mod-11 argument was never about our curve.** It works for an entire class of curves, and the palindrome question for any of them collapses to a single yes/no test.

This document is deliberately slow. Every algebraic step is shown, every symbol is defined where it first appears, and each stage has worked numbers. Nothing is left as "it follows that".

---

## 1. A curve here is two constraints stacked

Our curve is usually written `p = 2n² + 2n + 1`, but that hides its structure. Written the other way:

    p = n² + (n+1)²

there are clearly **two separate ingredients**:

| ingredient | what it is | ours |
|---|---|---|
| the **quadratic form** | the shape being summed | `x² + y²` |
| the **linear condition** | how x and y are tied | `y = x + 1` |

That gives two independent dials.
`nearby_curves.md` turned only the second one (`y = x + k`).
This document turns the **first**, which is where the real generality was hiding.

### Where the quadratic forms come from

`x² + y²` is not arbitrary. It is the **norm form** of the Gaussian integers `Z[i]` — the size of the complex number `x + yi`.
Other number rings have their own norm forms:

| ring | norm form | name |
|---|---|---|
| `Z[i]` | `x² + y²` | Gaussian integers |
| `Z[ω]` | `x² + xy + y²` | Eisenstein integers (ω = cube root of 1) |
| `Z[√−2]` | `x² + 2y²` | — |

Pick a form, impose "consecutive" (`y = x + 1`), and you get a curve.

| ring | form | with y = x+1 | becomes | OEIS |
|---|---|---|---|---|
| `Z[i]` | x²+y² | n²+(n+1)² | 2n²+2n+1 | A027862 **(ours)** |
| `Z[ω]` | x²+xy+y² | n²+n(n+1)+(n+1)² | 3n²+3n+1 | A002407 |
| `Z[√−2]` | x²+2y² | n²+2(n+1)² | 3n²+4n+2 | — |

**The Eisenstein one is a small gift.** `3n² + 3n + 1` is exactly `(n+1)³ − n³`, a **difference of consecutive cubes**. Primes of that shape are the classical *cuban primes*. So the natural sibling of "sum of consecutive squares" turns out to be "difference of consecutive cubes" — same idea, one ring over.

---

## 2. Every one of these has the same membership shape

To test whether some number `p` is on a curve, we need a rule. For ours it is the familiar one:

> `p` is on the curve **iff `2p − 1` is an odd perfect square.**

Here is where that comes from, in full:

    p        = n² + (n+1)²
             = n² + n² + 2n + 1
             = 2n² + 2n + 1
    2p       = 4n² + 4n + 2
             = (4n² + 4n + 1) + 1
             = (2n + 1)² + 1
    2p − 1   = (2n + 1)²                       ← write a = 2n+1

*Check with real numbers:* n = 79 → p = 79² + 80² = 6241 + 6400 = **12641**.
Then `2p − 1 = 25281`, and `√25281 = 159 = 2(79) + 1`. ✓

Now do the same for the other three curves. **Nothing clever happens** — it is completing the square each time:

**k-family**, `p = n² + (n+k)²`:

    2p       = 4n² + 4kn + 2k²
             = (2n + k)² + k²
    2p − k²  = (2n + k)²                       ← a = 2n+k

**Cuban**, `p = 3n² + 3n + 1`:

    4p       = 12n² + 12n + 4
             = 3(4n² + 4n + 1) + 1
             = 3(2n + 1)² + 1
    4p − 1   = 3(2n + 1)²                      ← a = 2n+1

**Z[√−2]**, `p = n² + 2(n+1)² = 3n² + 4n + 2`:

    3p       = 9n² + 12n + 6
             = (9n² + 12n + 4) + 2
             = (3n + 2)² + 2
    3p − 2   = (3n + 2)²                       ← a = 3n+2

### The common shape

Every line above has the form

> **`α·p + β = γ·a²`**

for whole numbers α, β, γ, where `a` is a linear function of `n`:

| curve | α | β | γ | a | membership rule |
|---|---|---|---|---|---|
| **ours** | 2 | −1 | 1 | 2n+1 | `2p − 1 = a²` |
| k-family | 2 | −k² | 1 | 2n+k | `2p − k² = a²` |
| cuban | 4 | −1 | 3 | 2n+1 | `4p − 1 = 3a²` |
| Z[√−2] | 3 | −2 | 1 | 3n+2 | `3p − 2 = a²` |

That is the whole setup. Everything below uses only `αp + β = γa²`.

---

## 3. Why 11, and only 11

We need one fact about decimal reversal. Write `rev(p)` for `p` with its digits reversed — `rev(12641) = 14621`.

The key observation is that **`10 ≡ −1 (mod 11)`**. That is just `10 = 11 − 1`.

Because of it, powers of ten alternate in sign mod 11:

    10⁰ ≡  1
    10¹ ≡ −1
    10² ≡  1        (since (−1)² = 1)
    10³ ≡ −1
    ...  10^j ≡ (−1)^j

A d-digit number is `p = Σ cⱼ·10^j`. Reversing the digits sends the digit at position `j` to position `d−1−j`. So mod 11 every digit's sign flips by the same factor `(−1)^(d−1)`, giving:

> **`rev(p) ≡ (−1)^(d−1) · p  (mod 11)`**

Two cases, and the difference between them is the entire story:

| d | `q = rev(p)` satisfies | consequence |
|---|---|---|
| **odd** | `q ≡ +p` | `p − q ≡ 0` |
| **even** | `q ≡ −p` | **`p + q ≡ 0`** |

*Check:* `p = 12641` (d=5, odd). `12641 mod 11 = 3`. `14621 mod 11 = 3`. Equal ✓
*Check:* `p = 37` (d=2, even). `37 mod 11 = 4`, `73 mod 11 = 7`, and `4 + 7 = 11 ≡ 0` ✓

**Why no other modulus works.** We need reversal to do something algebraic. Mod 9, `10 ≡ 1`, so every power of ten is `1` and `q ≡ p` *always* — that only sees the digit **sum**, so it cannot tell reversal apart from any other shuffling of the digits. Useless. For any other modulus, `10^j` cycles through several values with no symmetry, and reversal has no clean expression at all.

**11 is the only modulus in which digit reversal is an algebraic operation.** Everything in this document is spending that one fact.

---

## 4. The master theorem

Take any curve with membership `αp + β = γa²`. Suppose `p` and `q = rev(p)` are **both** on it — a *converse pair* — and suppose `d` is **even**.

Write `a` for p's parameter and `b` for q's:

    α·p + β = γ·a²        (1)   p is on the curve
    α·q + β = γ·b²        (2)   q is on the curve

Add (1) and (2):

    α(p + q) + 2β = γ(a² + b²)

Now reduce mod 11. Because d is even, §3 gives `p + q ≡ 0 (mod 11)`, so the entire first term vanishes:

    2β ≡ γ(a² + b²)        (mod 11)

Divide by γ. (Legitimate as long as 11 ∤ γ; a *modular inverse* `γ⁻¹` is the number with `γ·γ⁻¹ ≡ 1`. For example `3⁻¹ ≡ 4 (mod 11)`, because `3 × 4 = 12 ≡ 1`.)

> ### Theorem A — the even-d constraint
> For any curve `αp + β = γa²` and any converse pair at even d:
> $$a^2 + b^2 \equiv \frac{2\beta}{\gamma} \pmod{11}$$

This is a real restriction because **not every number mod 11 is a square.** The squares mod 11 — the *quadratic residues* — are found by squaring 0…10:

    0²=0  1²=1  2²=4  3²=9  4²=5  5²=3
    6²=3  7²=5  8²=9  9²=4  10²=1

    squares mod 11 = {0, 1, 3, 4, 5, 9}      (6 of 11)
    non-squares    = {2, 6, 7, 8, 10}        (5 of 11)

So `a²` and `b²` are each confined to a 6-element set, and their sum must hit one specific target. Usually only a few combinations work, which then pins down `p mod 11`.

### Theorem B — the palindrome corollary

A **palindrome** is the special case `q = p` (the number reads the same backwards), so `m = n` and therefore `b = a`. Substitute into Theorem A:

    a² + a² ≡ 2β/γ
       2a²  ≡ 2β/γ
        a²  ≡ β/γ            (mod 11)      — halving is fine, 2 is invertible

> ### Theorem B
> An **even-digit palindrome** can lie on the curve **only if `β/γ` is a square mod 11.**
> If `β/γ` is a non-square, **no even-digit palindrome lies on that curve at all.**

Note what this is *not*. The classical fact is that every even-digit palindrome is divisible by 11, hence composite. Theorem B is stronger where it applies: such numbers are not merely composite, they are **not on the curve in the first place**.

### Why odd d gives nothing

Repeat the derivation with odd d, where `q ≡ +p`. Now **subtract** (2) from (1):

    α(p − q) = γ(a² − b²)

and `p − q ≡ 0`, so `a² ≡ b² (mod 11)`, i.e. `a ≡ ±b`. That is satisfiable in many ways and eliminates almost nothing.

**This is why every interesting object we know lives at odd d** — `12641 ↔ 14621` at d=5, `3187813` at d=7. Odd d is where the constraint has no teeth.

---

## 5. Worked example — our own curve

α = 2, β = −1, γ = 1.

**Theorem A.** `a² + b² ≡ 2(−1)/1 = −2 ≡ 9 (mod 11)`.

Which pairs of squares sum to 9? From `{0,1,3,4,5,9}`:

    0 + 9 = 9  ✓        4 + 5 = 9  ✓
    9 + 0 = 9  ✓        5 + 4 = 9  ✓
    (1+3=4, 1+4=5, 3+4=7, 3+5=8, 9+9=18≡7, ... no others)

So `a² ∈ {0, 4, 5, 9}`.

Convert to `p`. From `2p − 1 = a²` we get `p = (a² + 1)/2 ≡ 6(a² + 1)`, using `2⁻¹ ≡ 6 (mod 11)`:

    a²=0 → p ≡ 6(1)  = 6
    a²=4 → p ≡ 6(5)  = 30 ≡ 8
    a²=5 → p ≡ 6(6)  = 36 ≡ 3
    a²=9 → p ≡ 6(10) = 60 ≡ 5

> **p mod 11 ∈ {3, 5, 6, 8}** — 7 of the 11 classes eliminated.

This is exactly the result proven independently in `mod11_converse_constraint.md`. It now arrives as one line of a general formula.

**Theorem B.** `β/γ = −1 ≡ 10 (mod 11)`. Is 10 a square mod 11? The squares are `{0,1,3,4,5,9}` — **10 is not there.**

> **No even-digit palindrome lies on our curve at all.**

*Why 10 is not a square, in one line:* `−1` is a square mod a prime `P` exactly when `P ≡ 1 (mod 4)`. Here `11 ≡ 3 (mod 4)`, so `−1` is not a square. That single fact is what kills even-digit palindromes on our curve.

---

## 6. All four curves at once

Applying the same two formulas mechanically:

| curve | β/γ | allowed p mod 11 | even-d palindrome? |
|---|---|---|---|
| **ours** (Z[i], y=x+1) | −1 ≡ 10 | {3, 5, 6, 8} | **impossible** |
| k=3 (Z[i], y=x+3) | −9 ≡ 2 | {1, 5, 6, 10} | **impossible** |
| cuban (Z[ω], y=x+1) | −1/3 ≡ 7 | {3, 4, 7, 8} | **impossible** |
| Z[√−2], y=x+1 | −2 ≡ 9 | {0, 2, 9} | **POSSIBLE** |
| k=11 (Z[i], y=x+11) | −121 ≡ 0 | {0} | **POSSIBLE** |

The last two are the interesting rows: `9` and `0` **are** squares mod 11, so Theorem B does not forbid anything there.

---

## 7. The falsification test

A theorem that only ever says "no" is hard to trust — it can look right merely by predicting the absence of rare things. Theorem B is better than that: it says exactly **where the exception lives**, which is a claim that can fail.

So it was tested where it predicts palindromes should **exist**.

**Z[√−2] curve, `p = n² + 2(n+1)²`, searched to 2×10⁸:**

    22       =   2² + 2·3²        p mod 11 = 0
    66       =   4² + 2·5²        p mod 11 = 0
    647746   = 464² + 2·465²      p mod 11 = 0
    10644601 = 1883² + 2·1884²    p mod 11 = 0
    62344326                      p mod 11 = 0

Five even-digit palindromes, every one with `p mod 11` inside the permitted set `{0, 2, 9}`.

And on the curves where Theorem B forbids them — ours, k=3, k=5, k=7, k=9, cuban — an identical search found **zero**.

> The theorem is confirmed in **both** directions: it forbids where it should, and permits where it should.

(All of them are composite, since the classical "divisible by 11" fact still applies. So there is still no even-digit *prime* palindrome anywhere — but for two genuinely different reasons, which Theorem B is what lets us tell apart.)

---

## 8. Recipe — applying this to a curve you invent

1. **Write the curve** as a polynomial in n. Example: `p = 5n² + 2n + 3`.
2. **Complete the square** to reach `αp + β = γa²`, with `a` linear in n.
3. **Compute `2β/γ` mod 11** → Theorem A gives the allowed `a²`, hence allowed `p mod 11` at even d.
4. **Compute `β/γ` mod 11.** In `{0,1,3,4,5,9}`? Even-digit palindromes are possible. Otherwise **impossible**.
5. **Sanity requirement:** 11 must divide neither γ nor α, or the inverses do not exist and the argument does not apply.

Step 4 is a table lookup. A question that took a full session to settle for one curve is now a five-second check for any of them.

---

## 9. What this does and does not give us

**Does:** a real generalization. The mod-11 result is a property of *conics with a decimal-reversal condition*, not of our curve. Any future curve can be classified instantly.

**Does:** a sharper picture of the difficulty. Theorem A constrains **even d only**, and every object of interest — the emirp at d=5, the palindrome at d=7 — lives at **odd d**, where §3 shows the argument is toothless. The mod-11 bridge is real but narrow, and it does not touch the cases we actually care about.

**Does not:** any progress on the emirp search itself. Theorem A prunes even-d candidates and nothing else. `hunt.c`'s mod-110 wheel already banks that (~1.53× at even d) and there is no more to extract.

**Does not:** anything at odd d. That would need a second bridge between base 10 and the algebra, and §3 argues that 11 is the only modulus that provides one.

### Census across the families (below 10⁹)

| curve | curve primes | converse pairs | emirps |
|---|---|---|---|
| ours, k=1 | 3349 | 16 | **1** — 12641 ↔ 14621 |
| k=5 | 4444 | 8 | **1** — 37 ↔ 73 |
| cuban | 3325 | 24 | 0 |
| Z[√−2] | — | 0 | 0 |

The cuban curve has the **most** converse pairs of any we have measured, and no emirp. Ours has fewer and has one. With counts this small the difference is noise, not signal — but it does reinforce `curve_geometry.md`: nothing distinguishes our curve, so the hardness is not in it.

**The cuban curve has its own palindrome story:** below 10⁹ its only prime palindromes are **7** and **919**, where `919 = 18³ − 17³`. That is a structural sibling of the 1997 conjecture, sitting on a different curve.

---

## 10. Verification

    python3 docs/curve_families.py

Re-derives the membership identity for all four curves, checks Theorem A and Theorem B against exhaustive search, and confirms the `Z[√−2]` falsification case. Standalone — shares no code with the C engines.

---

## See also

- `curve_geometry.md` — why our curve is a conic, and where the hardness lives
- `nearby_curves.md` — the k-family (turning the *linear* dial)
- `mod11_converse_constraint.md` — the original single-curve proof
- `GLOSSARY.md` — symbols and terms
