# Session archive — 2026-08-30

A long session in two halves: a filing-system cleanup that turned up two real bugs, and a run of mathematics that generalized the mod-11 result twice.
Kept because much of the reasoning is worth re-reading, and because several of the wrong turns are as instructive as the results.

**Raw transcript** (5.1 MB, 3032 lines), under
`~/.claude/projects/`:

    -home-jim-programming-c-BigFermat-bi-quad/
      bf65a096-f79c-4111-879b-c64f0744d2d0.jsonl

A gzipped copy is kept at
`~/programming/c/conversion/_archive/transcripts/`.
That path is outside any repo and is not mirrored — this document is the durable record.

---

## Part 1 — the d2* cleanup (outside this repo)

Work in `~/programming/c/conversion/`, recorded here only because it produced findings worth keeping.

### Two real bugs, in a place nobody looks

`mult_128.h` is a 47 MB generated lookup table used by `d2base`. Because it is *data*, no compiler warning, linter, or code review has ever examined it. Both bugs had been live for years:

1. **`bin_str[0]` was `"000"`** — three characters where every other entry has four. Any zero nibble in the most significant bytes printed one digit short, so `Bin:` output was **wrong at and above `65536 · 2^128`** (~2.23×10⁴³). `Hex:` unaffected.
2. **Entries 2999/3000 and 29999/30000 were transposed**, breaking the monotonicity that `calc_msbyte()`'s linear scan depends on. It stopped one entry early, so **both `Hex:` and `Bin:` were wrong** inside two narrow bands.

Sweep across both defect regions: **67 wrong / 284 before, 0 / 284 after.**

Fixed by *generating* the table (`gen_mult_128.py`) rather than editing it — monotonicity is now structural.

**The lesson worth carrying:** a generated data file is invisible to every tool that checks code. Verify its invariants explicitly — formula, ordering, field widths. Banked as §14 of `~/claude_cmdln_gold/cmdline_gold.txt`.

Jim had already found bug #1: the corrected table was sitting in a scratch file, `mul128_h.txt`, that never made it back into the header.

### Outcome

- `312M → 47M`; everything removed is regenerable, the one irreplaceable artifact archived.
- `bigdec2hex` moved out of `d2base_dev/final-d2h/` to its own sibling directory (it was a published repo nested inside another project).
- `d2base_dev` given its own repo, three mirrors, branch protection.
- `~/bin/src` now holds **symlinks, not copies** — the drift cannot silently reopen.
- Four `d2*` lineages documented in `~/bin/src/README-d2x.txt`.
- The asm-library reconcile was **parked deliberately** — see `~/programming/asm/TODO-reconcile.txt`.

---

## Part 2 — the mathematics

Three documents, each answering one question, each generalizing the last.

### `curve_geometry.md` — the planetary-orbit question

**No, and the distinction matters.** Our curve is a **conic**: degree 2, genus 0, no group law. Elliptic curves are degree 3, genus 1, *with* a group law — that is where their depth comes from. Reaching for elliptic machinery here is a category error.

The accurate picture: every curve prime is a **lattice point where the line `y = x+1` crosses the circle `x² + y² = p`.** Not orbits — targets.

Via **Fermat's two-square theorem**, the curve primes are exactly the `p ≡ 1 (mod 4)` primes whose *unique* two-square representation happens to be **consecutive**. Verified with zero violations across every prime below 200,000.

**The payoff is retrospective:** the curve is the easy part. All difficulty is primality + base-10 reversal, both foreign to the algebra — which is why every attack on curve *structure* has failed.

### `nearby_curves.md` — "any insight from curves nearby?"

Slide the line: `p = n² + (n+k)²`, odd k only.

- Membership generalizes: **`2p = a² + k²`** with `a = 2n+k`.
- **The mod-11 even-d constraint holds for every odd k**, always killing 7 of 11 classes. Our `{3,5,6,8}` is one row of a table.
- **No even-digit palindrome unless `11 | k`** — because −1 is a non-residue mod 11. The exception appeared exactly where predicted (`523325`, `4114`, `296692`, `5445`).
- **k=5 has its own emirp: 37 ↔ 73.** So `12641 ↔ 14621` is not a freak of our curve.

### `curve_families.md` — "can a different constraint build a different curve?"

The strongest result of the day. A curve here is **a quadratic form plus a linear condition**; `nearby_curves` turned only the linear dial. Turning the *form* dial gives the norm forms of other rings — and the Eisenstein case, `3n²+3n+1`, is a **difference of consecutive cubes** (the cuban primes).

All of them share one membership shape, `αp + β = γa²`, and adding the equations for `p` and `q` at even d gives:

> **Theorem A** `a² + b² ≡ 2β/γ (mod 11)`
> **Theorem B** even-digit palindrome needs `a² ≡ β/γ`; impossible when that is a non-residue

One formula reproduces every result derived that day. **Confirmed in both directions** — it forbids where it should, and where it *permits* palindromes they exist (`Z[√−2]`: 22, 66, 647746, …).

**Scope, stated honestly:** Theorem A binds **even d only**. Odd d collapses to `a² ≡ b²`, which eliminates almost nothing — and *both* known objects, the d=5 emirp and the d=7 palindrome, live at odd d. A genuine generalization that does **not** advance the emirp search.

**The cuban curve has its own palindrome story:** below 10⁹ its only prime palindromes are **7** and **919 = 18³ − 17³** — a structural sibling of the 1997 conjecture on a different curve.

---

## Exact counts, k=1 (computed exhaustively)

| d | pts | pairs | emirp | pals | prime pals |
|---|---|---|---|---|---|
| 1 | 2 | 0 | 0 | 2 | 1 (5) |
| 3 | 15 | 0 | 0 | 3 | 2 (181, 313) |
| 5 | 153 | 6 | 2 | 0 | 0 |
| 7 | 1,529 | 2 | 0 | 5 | 1 (3187813) |
| 9 | 15,290 | 6 | 0 | 0 | 0 |
| 11 | 152,896 | 4 | 0 | 1 | 0 |
| 13 | 1,528,961 | 2 | 0 | 2 | 0 |
| 2,4,6,8,10,12,14 | — | 0–6 | 0 | **0** | **0** |

Two things fall out that are not heuristics: the palindrome column is **zero at every even d** (the theorem, confirmed empirically seven times), and the prime-palindrome column sums to exactly `palhunt`'s regression markers.

---

## Corrections made during the session

Recorded because each changed what got built.

**"the other three asm routines aren't on disk"** — searched the wrong tree; `~/programming/asm/` has all four.

**"k=22 gives a palindrome positive control"** — those palindromes are divisible by 11, hence composite. The control would have proved nothing.

**"`palhunt` has never had a positive control"** — it finds 5, 181, 313, 3187813.

**"the Codeberg protection rule didn't save"** — propagation lag; my 8-second re-check was too short.

**"25 is not on the curve"** — it is, at n=3. It is merely composite.

**"three years of structural attacks"** — the repo is 5½ months old and the attacks were one session.

**`.gitignore` with trailing comments** — no such syntax; the patterns matched nothing.

The `d2base_dev` "unresolved drift" claim from earlier in the session was also wrong: `d2base.c` and `latest-test.c` are **token-identical**, differing only in indentation. A byte-size gap was read as a content difference.

---

## Open threads

1. **Asm library reconcile** — parked, `~/programming/asm/TODO-reconcile.txt`.
2. **`gh` holds `delete_repo`** on the account token; narrowing requires revoke + re-auth.
3. **GitLab `d2base_dev`** force-push toggle may still be on from the history rewrite.
4. **Expected-count table for d=3…27** — asked for, then correctly walked back: a density heuristic gives *expected counts*, not predictions, and must not be allowed to masquerade as a theorem. Still worth doing, clearly labelled.
5. **A second bridge at odd d** — the real prize. §3 of `curve_families.md` argues 11 is the only modulus in which reversal is algebraic, so any new result must come from somewhere other than a modulus.

---

## Files added this session

| file | what |
|---|---|
| `docs/curve_geometry.md` | conic vs elliptic; Fermat; where the hardness lives |
| `docs/nearby_curves.md` | the k-family; mod-11 generalizes |
| `docs/nearby_curves.py` | 7 standalone checks |
| `docs/curve_families.md` | Theorems A and B for any conic curve |
| `docs/curve_families.py` | 7 standalone checks |
| `docs/GLOSSARY.md` | symbols, terms, and the `k` collision |
