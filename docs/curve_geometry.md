# What Kind of Curve Is This? — geometry, Fermat, and where the hardness lives

Written 2026-08-30, prompted by a good question: *"I picture elliptic curves as orbits of planets on a flat plane. Our curve sitting on its absolutely private curve — is that a good picture?"*

Short answer: the instinct is right that there's a private geometric object here, but the borrowed image comes from the wrong family of curves.
The correct picture is sharper, and it explains why three years of structural attacks failed.

---

## 1. It is not an elliptic curve

This distinction is not pedantry — it changes what tools apply.

| | equation | degree | genus | group law? |
|---|---|---|---|---|
| elliptic curve | y² = x³ + ax + b | 3 | 1 | **yes** |
| our curve | p = 2n² + 2n + 1 | 2 | 0 | no |

An elliptic curve's power comes from its **group law**: take two points on the curve, draw the line through them, and the third intersection point (reflected) is defined to be their "sum."
Points form an abelian group.
That structure is what gives elliptic curves their depth — Mordell–Weil rank, torsion subgroups, the whole apparatus behind ECC cryptography and the proof of Fermat's Last Theorem.

Our curve has none of that.
`p = 2n² + 2n + 1` is a **conic** — a parabola in the (n, p) plane.
Conics are, mathematically speaking, *solved*: genus 0, rationally parametrized, no rank, no hidden arithmetic structure.
Given one rational point you can sweep out all of them with a line.

So: no orbits, no group law, no mystery **in the curve itself.**

---

## 2. The picture that is actually true

Start from the definition rather than the expansion:

    p = n² + (n+1)²

That says the point `(n, n+1)` lies on the circle `x² + y² = p`.
And `(n, n+1)` always lies on the line `y = x + 1`.

So every prime on our curve is a **lattice point where the line y = x+1 punches exactly through a circle of radius √p**:

```
        y
        │        ·   ·   ·
        │     ·             ·        circle: x² + y² = p
  y=x+1 │   ·                 ·      radius √p
        │  ·        ○          ·
        │ ·      ↗    ╲         ·    (n, n+1) is a LATTICE point
        │·  (n,n+1)    ╲        ·    on both the circle and the line
        │ ·             ╲      ·
        │   ·            ╲   ·
        └──────────────────────────── x
```

Not planets in orbit — **targets**.
Concentric circles of radius √p, and the question is which ones the diagonal line `y = x + 1` strikes dead-on at integer coordinates, with p prime.

That is the private curve, and it really is private.
The membership test is the same one `curve_index()` implements:

    2p − 1 = 4n² + 4n + 1 = (2n+1)²

p is on the curve **iff 2p − 1 is an odd perfect square.**

---

## 3. Why the curve is "private" — the Fermat connection

Every curve prime satisfies `p ≡ 1 (mod 4)`:

    p = 2n² + 2n + 1 = 2n(n+1) + 1

`n(n+1)` is a product of consecutive integers, hence even, so `2n(n+1) ≡ 0 (mod 4)` and `p ≡ 1 (mod 4)`.

Now bring in **Fermat's two-square theorem**: every prime `p ≡ 1 (mod 4)` is a sum of two squares in *exactly one* way (up to order and sign).

That gives the real characterization:

> Our curve is not a special class of primes.
> It is the primes whose **unique** two-square representation happens to land on **consecutive** integers.

Compare:

All six values below are `≡ 1 (mod 4)`, so Fermat applies to every prime among them:

| v | prime? | two squares | consec? | on curve | curve prime |
|---|---|---|---|---|---|
| 5 | yes | 1² + 2² | yes | ✔ n=1 | ✔ |
| 13 | yes | 2² + 3² | yes | ✔ n=2 | ✔ |
| 17 | yes | 1² + 4² | no | ✘ | ✘ |
| 25 | **no** | 3²+4² *and* 0²+5² | yes | ✔ n=3 | ✘ composite |
| 29 | yes | 2² + 5² | no | ✘ | ✘ |
| 41 | yes | 4² + 5² | yes | ✔ n=4 | ✔ |

The `25` row earns its place twice.
It shows the curve carries **composites** as well as primes — `25 = 3² + 4²` is a perfectly good curve point at n=3, it just isn't prime — and it shows why Fermat's uniqueness clause says *prime*: 25 has two representations, so uniqueness fails the moment you leave the primes.

So the curve primes are a thin slice cut out of the `p ≡ 1 (mod 4)` primes by a *geometric* condition (the two squares are adjacent), which is the same as saying the lattice point sits on `y = x + 1`.

**Verified**, not merely asserted: across every prime below 200,000, `on-curve ⟺ its two-square form is consecutive` holds with **zero** violations (87 curve primes, all `≡ 1 mod 4`, beginning 5, 13, 41, 61, 113, 181, 313, 421).

This is OEIS **A027862**, and its infinitude is Landau's fourth problem / Hardy–Littlewood Conjecture F territory — open.

---

## 4. Where the hardness actually lives

This is the payoff, and it retro-explains the failed attacks.

**The curve is the easy part.**
Genus 0. No resistance. Membership is a square test.

All of bi-quad's difficulty comes from two conditions that are **foreign to the algebra**:

1. **primality** — not an algebraic condition on the curve at all
2. **decimal digit reversal** — base-10, utterly alien to `2n² + 2n + 1`

The emirp question asks for a point whose *decimal representation*, reversed, is another point on the same curve, with both prime.
Reversal is not a polynomial map, not a linear map, not any map the curve's geometry can see.

That is precisely why the three ruled-out attacks failed (`structural_attacks_2026-06-04.md`):

| attack | why it died |
|---|---|
| MITM, `n = a·10^t + b` | cross-term couples the middle digits |
| two-ended digit-DP | survivors ≈ 0.023·range ≈ 10^(d/2) |
| congruence, 50 moduli | **zero** obstructions, d ∈ [8,30] |

All three attacked *structure*. The structure isn't where the hardness is.
The project's own conclusion — *"real obstructions are non-congruential/sporadic"* — is this fact, arrived at empirically before it was understood geometrically.

---

## 5. The one bridge that exists

Against that background, the mod-11 result (`mod11_converse_constraint.md`) is the exception that proves the rule.

It works for exactly one reason:

    10 ≡ −1 (mod 11)     ⟹     rev(p) ≡ (−1)^(d−1) · p  (mod 11)

Digit reversal, which is otherwise algebraically invisible, becomes a *sign flip* mod 11.
That is the single place where the base-10 world and the algebraic world touch.

And it paid: at even d, a converse pair forces `p mod 11 ∈ {3,5,6,8}` — 7 of 11 residue classes eliminated — with the corollary that **no even-digit palindrome lies on the curve at all**, which is strictly stronger than the classical "even-digit palindromes are divisible by 11."

It also explains why that result does not generalize.
There is no analogous bridge for mod 7, mod 13, or anything else: `10 ≡ −1` is special to 11, and mod 9 (`10 ≡ 1`) is too weak — it only sees the digit *sum*, so it cannot distinguish reversal from any other digit permutation.

---

## 6. Summary for the next person

- The curve is a **conic (genus 0)**, not an elliptic curve. No group law. Don't reach for elliptic machinery.
- The right mental image: **lattice points where `y = x+1` crosses the circle `x² + y² = p`.**
- Membership test: **`2p − 1` is an odd perfect square.**
- The curve primes are the `p ≡ 1 (mod 4)` primes whose unique Fermat two-square form is **consecutive**.
- **The hardness is not in the curve.** It is in primality + base-10 reversal, both alien to the algebra. Attacks on curve structure will keep failing.
- **mod 11 is the only known bridge**, because `10 ≡ −1 (mod 11)`. Look for new results where base-10 and algebra touch — not in the geometry.

---

## See also

- `mod11_converse_constraint.md` — the even-d constraint and palindrome corollary
- `structural_attacks_2026-06-04.md` — the three eliminated approaches, in detail
- `density_heuristics.md` — frontier formula and emirp-vs-palindrome rarity
- `OPEN_PROBLEM.md` — current statement of the search
