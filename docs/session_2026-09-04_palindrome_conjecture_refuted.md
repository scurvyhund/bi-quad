# The 1997 palindrome conjecture is REFUTED

**2026-09-04.** A fifth prime palindrome on the curve exists. It has 59
digits. The conjecture that `3187813` is the largest is **false**.

    n = 91732095351342012927350087594                              [29]
    p = 2n² + 2n + 1
      = 16829554635095405876254045429592454045267850459053645592861 [59]

Found by **Patrick De Geest, 2026-04-24**, published on Carlos Rivera's
[Puzzle 14](https://www.primepuzzles.net/puzzles/puzz_014.htm) and
tracked at [worldofnumbers.com/sumsquare.htm](https://www.worldofnumbers.com/sumsquare.htm).
Method: **`cudapalin`**, Robert Xiao's CUDA program for palindromic
quadratic solutions, run on a GeForce 4090 (16,384 cores).

## Verified here, independently

From `n` alone, not trusting the published digit string (the copy we
first read was garbled):

| check | result |
|---|---|
| `p = 2n² + 2n + 1` | exact |
| palindrome | yes |
| `p mod 10` | 1 — consistent with the ending constraint |
| `mpz_probab_prime_p(p, 50)` | 1 (probable prime, 50 MR rounds) |

Both lengths are prime (29 and 59), which De Geest noted.

## What is false, and what still stands

Be precise — most of our statements were bounded, and bounded
statements survive.

**FALSE — unbounded claims:**
- "3187813 is the largest prime palindrome on the curve."
- "Only four prime palindromes: 5, 181, 313, 3187813." There are at
  least five.

**STILL TRUE — bounded claims:**
- "3187813 remains the largest prime palindrome on the curve **through
  d = 37**." Our search was correct; it simply did not reach far
  enough. The fifth term is at d = 59.
- Every proven structural result: the ending constraint, the mod-11
  even-`d` obstruction, the zone argument, `is_pal_fast`'s contract.

**No contradiction with Alekseyev.** His A050239 comment bounds terms
below `10^47`; this one is ≈`1.68 × 10^58`.

## The density heuristic was right

This is the part worth keeping. `density_heuristics.md` and
`STATE_OF_THE_SEARCH` §5 both record that the palindrome density
**diverges**, i.e. the model predicts infinitely many prime palindromes
on the curve, and that the conjecture "3187813 is the last" therefore
*bets against our own heuristic*. We wrote that the conjecture was the
interesting claim precisely because the heuristic did not support it.

The heuristic won. A model that survives a live test against a 59-digit
discovery is worth more than the conjecture it just killed.

Note the contrast that now matters: the **emirp** density
**converges**, expected total ≈ 1. Same machinery, opposite prediction,
and that prediction is *not* refuted.

## Consequences

1. **Prime Curios submission on 3187813 withdrawn** — it asserted the
   refuted conjecture. See `PrimeCurios.txt`.
2. **The d ≥ 39 palindrome frontier is not ours to take.**
   `sumsquare.htm` carries a *sequentially indexed* enumeration of
   palindromes on this curve reaching **index 69** (the 59-digit term),
   with entries by Xiao (index 53–59, 2022-12-06) and De Geest
   (index 47–52, to 2021-06-16). Consecutive indices imply complete
   enumeration. There is no unsearched window at d = 39…58.
3. **Tier 2b is re-scoped.** Its stated payoff was "§5's 25-40% chance
   of an actual hit" for palindromes. That is gone — the hits exist and
   have been found, by a GPU operation purpose-built for this shape.
   256-bit remains worth building, but as verification and as
   infrastructure for the emirp question, not as a palindrome race.
4. **The mod-11 even-`d` result is not novel.** `sumsquare.htm` states
   "No even-length palindromes exist!" outright. Our derivation via
   quadratic residues on the curve is our own; the fact is not.
5. **Check A027572** — referenced by `sumsquare.htm` and absent from
   A050239's Cf. list.

## Why the emirp question is different in kind

`cudapalin` is fast because **a palindrome is determined by half its
digits** — that is what makes the problem massively parallel. An emirp
has no such structure: `rev(p)` is not determined by half of `p`, so
every `n` must be generated and tested, which is what `hunt.c` does at
O(10^(d/2)).

That asymmetry is the likely reason palindromes reached 59 digits while
**no bound on the bi-quadratic emirp has ever been published**. Michel
Marcus's A050239 comment (2025-11-30) says only "there are 2 **known**
emirps of the form x² + (x+1)²" — a count, not a bound.

Our exhaustion — `EMIRPS = 0` for every d = 6…27, every `n`, GMP-exact
— appears to be the strongest statement anyone has on it.
