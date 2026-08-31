# The geometry — a paraboloid, and three lattices

Companion to the visual version:
**[Three Lattices, One Construction](https://claude.ai/code/artifact/580f6343-6575-4ac6-86cb-7e22e27a2b6c)**
(mirrored in this repo at `docs/artifacts/three_lattices.html`).

Written 2026-08-30, from the question: *"can we make an image showing how the square/cuban curves sit on a sphere/cone — and what does the constraint do to the geometry?"*

---

## The correction first: it is a paraboloid

There is a genuine 3-D surface here, but it is neither of the usual guesses:

| surface | equation | why not |
|---|---|---|
| sphere | x²+y²+z² = r² | bounded; p runs to infinity |
| cone | z² = x²+y² | z **squared** — straight sides, apex |
| **paraboloid** | **z = x²+y²** | **ours** — a bowl, not a funnel |

Set `z = p`. The surface `z = x² + y²` holds every pair whose squares sum to p.
Cut it with the vertical plane `y = x + 1` — the "consecutive" condition — and the cut is the parabola

    p = 2n² + 2n + 1

**"Conic section" is literal here**: a plane cutting a quadric surface. That is the definition, and it is why the curve has genus 0 and no group law (`curve_geometry.md`).

---

## The construction: circles, in three different lattices

Seen from above, the level sets of the bowl are circles. A curve point is a **lattice point** sitting exactly on one, and on the ray `y = x + 1`.

The three curves of `curve_families.md` differ only in **which lattice you count on**. Each quadratic form is the squared distance to the origin in its own ring's natural embedding:

| ring | lattice | basis | form | curve |
|---|---|---|---|---|
| Z[i] | square | (1,0) (0,1) | x²+y² | 2n²+2n+1 |
| Z[ω] | triangular | (1,0) (½,√3/2) | x²+xy+y² | 3n²+3n+1 |
| Z[√−2] | rectangular | (1,0) (0,√2) | x²+2y² | 3n²+4n+2 |

**Verified**, not asserted: for every lattice point within radius 6, `|embedded point|² == the form`, exactly, in all three cases.

So the "three different quadratic forms" are one idea. If you insist on plotting `x² + xy + y² = p` on ordinary square graph paper you get an ellipse of axis ratio `√3` — but that ellipse is an artefact of forcing a triangular lattice onto square paper. In its own lattice it is a circle.

The line `y = x+1` stays a straight ray under every embedding, because the embedding is linear. In the triangular lattice the curve points step by a constant `(1.5, √3/2)`.

---

## What each constraint does to the geometry

This is the part that answers "does the geometry constrain the math":

| constraint | effect on the picture | geometric? |
|---|---|---|
| quadratic form | picks **which lattice** | yes |
| linear condition | picks **which ray** through it | yes |
| primality | thins the hits, no visible pattern | **no** |
| digit reversal | nothing whatsoever | **no** |

The geometry hands you the curve for free. Every question about the curve *as a curve* is already answered — it is a plane cutting a bowl. Then two conditions arrive from entirely outside the geometry and supply all of the difficulty.

Which retro-explains the three dead ends in `structural_attacks_2026-06-04.md`: MITM, two-ended digit-DP and the 50-modulus congruence sweep all attacked structure, and the structure was never the hard part.

---

## The one place the two worlds touch

`10 ≡ −1 (mod 11)`, so powers of ten alternate `+1, −1`, and reversal becomes a sign flip:

    rev(p) ≡ (−1)^(d−1) · p   (mod 11)

At even d that is `p + q ≡ 0`, strong enough to kill 7 of 11 residue classes and to forbid even-digit palindromes on most of these curves (`curve_families.md`, Theorems A and B).
At odd d it collapses to `a² ≡ b²` and says almost nothing — and both objects we care about, the d=5 emirp and the d=7 palindrome, sit at odd d.

---

## See also

- `curve_geometry.md` — conic vs elliptic; the Fermat characterization
- `curve_families.md` — Theorems A and B for any conic curve
- `nearby_curves.md` — the k-family
