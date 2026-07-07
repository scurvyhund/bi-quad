# Bi-Quadratic Emirp Search — Project Overview & Glossary

> Canonical "start here" document. Plain-language but precise. If a term feels
> fuzzy, it's defined here. Last reframed 2026-06-04 (convergence-diagonal
> insight).
>
> ⚠️ **CORRECTION 2026-06-05:** a bi-quadratic emirp **does exist** —
> `12641 ⟷ 14621` at **d=5** (verified by `hunt.c`: `EMIRPS=2`). The old "no
> emirp ≤ 22 digits" headline was wrong (a prose misread of the small-d sieve
> output). The corrected result: **`12641 ⟷ 14621` is the ONLY bi-quadratic
> emirp through 27 digits** (brute-verified, every n). See
> [`session_2026-06-05_emirp_d5_correction.md`](session_2026-06-05_emirp_d5_correction.md).
> Note on metrics: a curve-reversal "survivor" splits into **emirp candidates**
> (non-palindrome) + **palindromes** (`p=rev(p)`). The README figure and the
> table below show the raw total, split by colour. `check_survivors.c`
> correctly counts the emirp-candidate (non-palindrome) part; `hunt.c`'s raw
> count is the total — both are right (check_survivors overflows 64-bit at
> d≥19 → use hunt.c there). Of ALL these survivors through d=27, only two are
> prime: the d=5 emirp and 3187813.

---

## 1. The Goal (plain language)

We are hunting for — or proving the (non)existence of — **bi-quadratic emirps**:
a number `p` such that **all** of these hold at once:

1. `p = 2n² + 2n + 1` for some integer `n`  (so `p` is in our sequence),
2. `p` is **prime**,
3. `q = reverse-the-digits(p)` is **prime**, and `q ≠ p`  (that makes `p` an
   *emirp*),
4. `q` is **also** of the form `2m² + 2m + 1` for some integer `m`.

Condition 4 is what makes it "bi-quadratic" and rare: not only must `p` and its
digit-reversal both be prime, but **both must lie on the same quadratic curve**
`2x²+2x+1`. (Note `2n²+2n+1 = n² + (n+1)²`, the sum of two consecutive squares.)

**Why a sieve instead of brute force?** Testing huge `p` for primality is
expensive, and most sizes may contain *no* candidate at all. So before testing
any primes, we ask a cheaper question: *for numbers of a given digit-length
`d`, is it even **structurally possible** — at the level of the first and last
digits — for such a `(p, q)` pair to exist?* If we can prove "no" for a whole
digit-length, we've eliminated infinitely... well, an entire size class, with
zero primality tests. That proof is called an **obstruction**.

**The seed constraint (proof by construction).** The first/last-digit filter
has real teeth because the curve's endings are tightly constrained: every
prime-eligible `p = 2n²+2n+1` ends in one of just **six** two-digit values —
`{01, 13, 21, 41, 61, 81}` — and (reversing, for the q-side) starts with one of
`{10, 12, 14, 16, 18, 31}`. This is a **proof by construction**: `p` is a sum
of two *consecutive* squares, so `even²+odd²` forces the last digit to {1,3},
and enumerating one full period (`n mod 50`) exhibits the complete two-digit
set. Full proof:
[`ending_constraint_proof.md`](ending_constraint_proof.md). The same
construction at depth `k` (digits via `n mod 10^k`) is the sieve's engine.

**So the concrete deliverable is the obstruction landscape:** for each digit
count `d`, either "**OBSTRUCTION** — no `d`-digit bi-quadratic emirp can exist"
or "**N survivors** — N digit-feasible candidates remain, worth
primality-testing."

---

## 2. Glossary (every term we use)

| Term | Precise meaning |
|---|---|
| **`n`, `m`** | The index integers. `p` comes from `n`; `q` comes from `m`. |
| **`p`** | `p = 2n² + 2n + 1` ( = `n² + (n+1)²` ). The number we test. |
| **`q`** | `q = rev(p)`, the digit-reversal of `p`. For a hit, `q` must *also* equal `2m²+2m+1`. |
| **emirp** | A prime that becomes a **different** prime when its digits are reversed (not a palindrome). |
| **bi-quadratic emirp** | Our target: `p` satisfying all four conditions in §1. Also called a *converse prime pair* `(p, rev(p))`. |
| **`d`** | Digit-length of `p` (the "size class" we test one at a time). |
| **`k`** (depth) | How many **leading and trailing** digits we constrain. The modulus is `mod = 10^k`. Bigger `k` = stronger filter = more work. |
| **residue `r`** | A class `mod 10^k`. Concretely `r = n mod 10^k` — the **last `k` digits of `n`**. The search loops `r` over `0 … 10^k − 1`. |
| **ending** | The last `k` digits of `p`, i.e. `(2r²+2r+1) mod 10^k` — fixed by `r`. |
| **valid ending** | An ending that is (a) **achievable** by some `n`, and (b) **prime-eligible** — last digit ∉ {0,5} (else divisible by 5 → composite → never prime). ≈ 5.42% of all `10^k` residues. |
| **valid_firsts** | The set of achievable first-`k`-digit patterns = `reverse_k(valid endings)`. Used to check `p`'s leading digits. |
| **composite-5 filter** | Dropping endings whose last digit is 0 or 5. Half of all achievable endings end in 5; without this filter they masquerade as survivors and hide real obstructions. |
| **Hensel lifting** (`solve_residues`) | Given a target ending mod `10^k`, find which residues solve `2m²+2m+1 ≡ target` by lifting solutions mod 10 → mod 100 → … → mod `10^k`. This is how the `q`-side is checked. |
| **survivor** | A residue `r` for which a **real** `(n, m)` pair passes *all* the first/last-digit constraints. A digit-level-feasible candidate. |
| **obstruction** | `survivors = 0` for a `(d, k)`. A genuine **proof**: no `d`-digit bi-quadratic emirp can exist. |
| **range** | `n_max − n_min`: how many `n` produce a `d`-digit `p`. |
| **mod** | `10^k`. |
| **`range/mod`** | Average number of `n`-values per residue class. The single most important quantity — it decides which regime we're in. `range/mod ≈ 2.16 × 10^((d−1)/2 − k)`. |
| **cliff** | The `d` (for a given `k`) where `range/mod` crosses ~1.5, i.e. **`d ≈ 2k+1`**. *Below* the cliff: ≤1 `n` per residue → check is exact & cheap. *Above*: many `n` per residue → over-dense regime. |
| **converged count** | For a fixed `d`, the survivor count **stabilizes** once `k` is large enough that `d` sits at/below `k`'s cliff (`k ≳ d/2`). That stable value is the **true** candidate count for length `d`. The real obstruction landscape lives on this *converged diagonal*. |
| **saturation** ⚠️ | *Retracted concept.* The theoretical max survivor count is `3·mod/5`. An old bug made counts falsely hit this ceiling and stop the search early. **It is NOT a real phenomenon** — see §4. |

---

## 3. How the search works (the pipeline)

For each `(k, d)`:

- **Phase 1 — build the filters.** Compute the set of **valid endings** mod
  `10^k` (achievable `2n²+2n+1` residues, last digit ∉ {0,5}). Stored as a
  bitset. `valid_firsts` = reversed valid endings, sorted.
- **Phase 2 — bounds.** Compute `n_min, n_max` so that `p = 2n²+2n+1` has
  exactly `d` digits. This sets `range = n_max − n_min`.
- **Phase 3 — the residue scan.** Loop `r` over all `10^k` residues. For each
  `r`:
  1. Find the first real `n ≡ r (mod 10^k)` in `[n_min, n_max]`; skip `r` if
     none.
  2. `p_ending = (2r²+2r+1) mod 10^k`. It must be a **valid ending**
     (prime-eligible). `p`-side last digits ✓.
  3. `q_first = reverse_k(p_ending)` — `q`'s leading digits. Must be achievable
     (`≥ prefix_min`).
  4. **Enumerate the actual `n`-values** for this residue
     (`n, n+mod, n+2·mod, … ≤ n_max`). For each, compute `p`'s exact first-`k`
     prefix `f`:
     - `f` must be a **valid_first** (so `q`'s last-`k` is achievable) —
       `p`-side leading digits ✓.
     - `q_ending = reverse_k(f)` must be a valid ending — `q`-side last
       digits ✓.
     - **Hensel-solve** for `m`-residues giving `q_ending`, then enumerate real
       `m`-values and check whether any yields `q`'s first-`k` == `q_first` —
       `q`-side leading digits ✓.
  5. If a real `(n, m)` clears all four, the residue is a **survivor**.
- **Result:** `survivors = 0` → **OBSTRUCTION**; else the survivor count.

The four constraints, in one line: *`p`'s last-`k` ↔ `q`'s first-`k`, and `p`'s
first-`k` ↔ `q`'s last-`k`, with both sides required to be real, prime-eligible
points on the `2x²+2x+1` curve.*

**What the digit filter buys — before any primality test.** Phase 1's six
valid endings `{01,13,21,41,61,81}` (and their reverses `{10,12,14,16,18,31}`)
follow from a [proof by construction](ending_constraint_proof.md): `p` is a sum
of two *consecutive* squares, so `even²+odd²` forces the last digit to {1,3}
and one period (`n mod 50`) pins the rest. Keeping only those ≈6-of-90 leading
patterns discards **≈ 89% of candidates (~9× speedup)** up front:

| exhaustive search through | candidates `n` ≈ `√(10ᵈ/2)` | filter culls (~89%) | reach primality test |
|---|---|---|---|
| d ≤ 24            | ≈ 707 billion   | **≈ 629 billion**   | ≈ 78 billion   |
| d ≤ 25            | ≈ 2.24 trillion | **≈ 1.99 trillion** | ≈ 246 billion  |
| d ≤ 26            | ≈ 7.07 trillion | **≈ 6.29 trillion** | ≈ 778 billion  |
| d ≤ 27 (frontier) | ≈ 22.4 trillion | **≈ 19.9 trillion** | ≈ 2.46 trillion |

Deepened to `k` digits, the *same* construction collapses whole digit-lengths
to zero survivors — an obstruction proven with **no primality tests at all**.

---

## 3b. Algorithms (pseudocode)

These mirror the real functions in `mod_obstruct.c`. Five pieces do all the
work.

### A. Digit helpers (cheap, exact)

```
reverse_k(v, k):              # flip the low k digits of v
    out = 0
    repeat k times: out = out*10 + (v mod 10); v = v // 10
    return out

ending_for_residue(r, mod):   # last k digits of p, from r = n mod 10^k
    return (2*r*r + 2*r + 1) mod mod          # mod = 10^k

compute_first_k(n, d, k):     # leading k digits of p = 2n²+2n+1
    p = 2*n*n + 2*n + 1
    return p // 10^(d-k)                       # GMP big-int divide
```

### B. Digit-length bounds — `compute_n_bounds(d)`

`p` has exactly `d` digits  ⇔  `10^(d-1) ≤ 2n²+2n+1 < 10^d`. Invert the
quadratic:

```
n_min = smallest n with 2n²+2n+1 ≥ 10^(d-1)     # via integer sqrt, then verify
n_max = largest  n with 2n²+2n+1 < 10^d
range = n_max - n_min                            # how many n give a d-digit p
```

(The `+1` / verify steps guard against integer-sqrt rounding — a real off-by-one
would corrupt every count, so the code re-checks the digit count explicitly.)

### C. Hensel lifting — `solve_residues(target, k)`

"Which residues `x` solve `2x²+2x+1 ≡ target (mod 10^k)`?" Solve mod 10 by brute
force, then **lift** each solution one digit at a time: a solution mod `10^j`
extends to mod `10^(j+1)` only by prepending one of the 10 digits, so we test
just `10 × (#solutions)` candidates per level instead of all `10^(j+1)`.

```
solve_residues(target, k):
    sols = [ r in 0..9 : (2r²+2r+1) mod 10 == target mod 10 ]   # base case
    pow = 10
    for j in 1 .. k-1:
        next_mod = pow * 10
        sols' = []
        for r in sols:                       # each surviving low-j-digit solution
            for c in 0..9:                   # try each next digit
                r_new = r + c*pow
                if (2*r_new² + 2*r_new + 1) mod next_mod == target mod next_mod:
                    sols'.append(r_new)
        sols = sols'; pow = next_mod
    return sols
```

This is exact and fast because the solution set stays small (a handful) at every
level. It's used to answer "can some real `m` produce this `q`-ending?"

### D. Phase 1 — build the filters

```
for r in 0 .. 10^k - 1:
    e = ending_for_residue(r, 10^k)
    if (e mod 10) in {0,5}: continue          # composite-5 filter (not prime-eligible)
    mark e in is_valid_ending (bitset)
valid_firsts = sorted( reverse_k(e,k) for each valid ending e )
```

### E. Phase 3 — the residue scan (the core, fixed version)

```
survivors = 0
for r in 0 .. 10^k - 1:                        # parallel over threads (dynamic)
    n0 = first n ≥ n_min with n ≡ r (mod 10^k)
    if n0 > n_max: continue                    # no real n in this class → skip (kills ~99.99%)

    p_ending = ending_for_residue(r, 10^k)      # p's last k digits (fixed by r)
    if p_ending not in is_valid_ending: continue        # p-side last digits ✗
    q_first = reverse_k(p_ending, k)            # q's leading digits (fixed by r)
    if q_first < prefix_min: continue           # not an achievable magnitude

    surv = false
    for n in {n0, n0+mod, n0+2·mod, … ≤ n_max}:        # EXACT: every real n in this class
        f = compute_first_k(n, d, k)            # p's leading k digits for THIS n
        if f not in valid_firsts: continue              # p-side leading digits ✗
        q_ending = reverse_k(f, k)
        if q_ending not in is_valid_ending: continue    # q-side last digits ✗

        for m_r in solve_residues(q_ending, k):         # q-side: which m give that ending?
            for m in {first m ≡ m_r, +mod, … ≤ n_max}:  # every real such m
                if compute_first_k(m, d, k) == q_first:  # q's leading digits match ✓
                    surv = true; break
        if surv: break
    if surv: survivors += 1

report: survivors == 0  →  OBSTRUCTION ,  else  survivors
```

**The one line that mattered most:** iterating `n in {n0, n0+mod, …}` and
testing each *exact* prefix `f`. The old code replaced that inner loop with a
single *interval* `[prefix(first n), prefix(last n)]` — exact only when the
class holds one `n` (below the cliff), catastrophically wrong above it (§4,
bug 4).

---

## 4. The methods — and how we developed them (the honest history)

The current method is the survivor of a chain of bugs we found and fixed.
Knowing the history is what keeps us from re-trusting retracted results.

1. **Survivor-counter overflow.** `int` counter wrapped negative at `k≥10`
   (counts exceed 2³¹). → changed to `long`. (Also broke saturation detection,
   causing a runaway "d=222 march".)
2. **Missing composite-5 filter.** Endings divisible by 5 are composite but were
   counted as survivors — on *both* the `p`-side and `q`-side. → filter both.
   This *unmasked real obstructions* that the dirt had been hiding.
3. **Wrong saturation level.** Max possible is `3·mod/5`, not `mod` (only 3 of 5
   last-digit classes are prime-eligible). → corrected.
4. **The big one — interval over-approximation (the `range ≥ mod` cliff).** The
   per-residue check used to approximate `p`'s achievable prefixes as an
   *interval* `[prefix(first n), prefix(last n)]`. That is exact **only when a
   residue has ≤1 `n`-value** (i.e. `range < mod`, below the cliff). Past the
   cliff each residue has 2+ `n`-values whose prefixes are far apart, so the
   interval admitted prefixes **no real `n` produces** → it **massively
   over-counted**, hit the fake `3·mod/5` saturation, and **silently stopped the
   search**. The headline "obstructions only up to ~2k / saturation at d=2k+2"
   was an **artifact of this bug**, never a real result.
   → **Fix:** *enumerate the actual `n`-values* and test exact prefixes as point
   checks. Collapses to the identical cheap check below the cliff (so all old
   `d ≤ 2k` results stand), and is correct *and* tractable above it.
   **Validated** against a fully independent Python brute force — exact match
   (k=6: d13=8, d14=30 vs the old bug's 322052 / "600000 saturated").

5. **The reframing (2026-06-04) — the convergence diagonal.** The post-cliff
   numbers are partly an artifact of *counting residues* in the over-dense
   regime. The **real** science is read along the *converged diagonal*: for
   each `d`, increase `k` until the count stops changing (`k ≳ d/2`). That
   stable value is the true count of digit-feasible candidates of length `d`.
   The count is **monotone non-increasing in `k`** (more constrained digits →
   fewer or equal survivors), so it converges *downward* to the truth.
   Confirmed: `d=13:4, d=14:6, d=15:5` are stable across `k=7,8(,9)` →
   converged. `d=16:0` reproduces at `k=8` and `k=9` → a confirmed
   **re-obstruction**. `d=17` fell `3→1` from `k=8→k=9` (k=8 was at its own
   cliff, not yet converged) — true count ∈ {0,1}.

**Two regimes, one rule:**
- An **obstruction (count 0) at *any single* `k` is already a complete proof** —
  no convergence needed. Zero residues passing the `k`-digit feasibility test
  means no such emirp can exist, and monotonicity keeps it 0 for all higher `k`.
- A **nonzero count** is trusted only when it (a) sits at/below its `k`'s cliff
  and (b) reproduces at the next `k` up. Anything read from the over-dense
  (`range ≥ mod`) region is suspect until confirmed on the diagonal.

Independent brute-force cross-checks at small `k` are the ground truth.

---

## 5. Current understanding (the converged landscape so far)

![The curve p=2n²+2n+1: prime palindromes (gold) cluster at d≤7 while the curve runs on barren; and the obstruction landscape by digit-length](biquad_curve_landscape.png)

*Figure — **top:** the curve on log–log axes; the four prime palindromes
(5, 181, 313, 3187813, all d ≤ 7) and the lone bi-quadratic emirp
`12641 ⟷ 14621` (d=5). **bottom:** the survivor landscape (raw curve-reversal
pairs, stacked) — purple = the d=5 emirp, teal = emirp candidates (composite),
gold = palindromes (only 3187813 is prime), red ✗ = obstruction (no survivor
at all). Regenerate with `generate_graph.py`.*

```
d:     5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27
cand:  6* 0  2  2  6  0  4  2  2  6  2  2  0  0  0  0  2  0  2  2  2  6  2   emirp candidates (non-pal)
pal:   0  0  5  0  0  0  1  0  2  0  4  0  1  0  3  0  5  0  1  0  5  0  3   palindromes (gold)
raw:   6  0  7  2  6  0  5  2  4  6  6  2  1  0  3  0  7  0  3  2  7  6  5   total (hunt.c)
                                                       (* d=5 cand holds the EMIRP)

  d = 5                : EMIRP — 12641 ⟷ 14621 (the ONLY bi-quadratic emirp)
  d = 6,10,18,20,22    : OBSTRUCTION — no curve-reversal survivor at all
  d = 17, 19           : only palindromic survivors (composite) → no emirp candidate
  all other d          : emirp candidates exist, ALL composite (hunt.c)
  NB: d=7's 5 palindromes include 3187813 (PRIME). The sieve's old "d=21
      obstruction" was a range<mod cliff artifact (§4); hunt shows d=21 has 2
      composite emirp candidates + 5 palindromes.
  NB2: d=5..26 counts INCLUDE div-5 (trivially composite) survivors &
      palindromes. d=27's (cand=2, pal=3, raw=5) is div-5-EXCLUDED --
      it came from the skip-optimized binary, which drops div-5 values
      (see skip_optimization.md). So the true all-n d=27 totals are a
      touch higher; the excluded values are all ÷5 composites, so the
      emirp and prime-palindrome conclusions are unchanged. Mixed
      convention footnoted rather than recomputed (that needs the slow
      pre-opt binary, ~days, for composite bookkeeping only).
```

The landscape is **not** a simple low-`d` band that ends. After the lone emirp
at d=5, emirp candidates recur in scattered windows — **all composite** — and
true obstructions (no survivor of any kind) sit at **{6, 10, 18, 20, 22}**. The
trend toward **no *further* bi-quadratic emirp** is strong (density heuristic:
expected total ≈ 1), consistent with `12641 ⟷ 14621` being the only one that
exists.

### The hunt (exhaustive brute force, d=5–27) — `hunt.c`

`hunt.c` directly enumerates **every** `n`, forms `q = rev(p)`, and tests
exactly: is `q` on the curve (`2q−1` a perfect square)? are `p`, `q` both
prime? `q ≠ p`? This is the **authoritative** tool (GMP-exact). NB:
`check_survivors.c` is correct but counts only the **non-palindrome** emirp
candidates (the teal part); `hunt.c`'s "raw" count is the total (candidates +
palindromes). Both agree once palindromes are accounted for; check_survivors
only overflows 64-bit at d≥19 → use hunt.c past there.

**Result: `EMIRPS = 2` at d=5** (the pair 12641↔14621, counted both directions);
**`EMIRPS = 0` for every d = 6 … 27.** Every other emirp candidate has `q`
exactly on the curve yet `p` and/or `q` composite (Miller–Rabin, 40 rounds).

**→ `12641 ⟷ 14621` (d=5) is the ONLY bi-quadratic emirp through 27 digits.**
Exhaustively verified by brute force for d=5–27 (d=27 completed 2026-07-05, ~5.0
days; the gap-closing run that lifted the emirp frontier to the palindrome one).
The next emirp, if any, has **d ≥ 28**. The density heuristic (Σ C/d² →
expected total ≈ 1) is consistent with it being the only one that exists.

**Cost note:** exact enumeration costs ~`range/mod` work per residue, so
pushing far past a `k`'s cliff gets expensive (~10× per digit). The tractable,
meaningful move is to extend the diagonal (raise `k` at fixed `d`), not to
chase the post-cliff tail.

---

## 6. Open questions

1. ~~Does the `d=16` re-obstruction survive at `k=9`?~~ **Resolved: yes** —
   confirmed by sieve (k8,k9) and exact brute force.
2. ~~Do any small-`d` survivors yield a genuine emirp?~~ **Resolved: no** — d≤19
   is exhaustively clear (see §5 hunt). The search moves to **d ≥ 20**.
3. **The central question:** do obstructions become *total* for all large `d`
   (→ non-existence of bi-quadratic emirps), or do prime-eligible candidates
   persist? **Status:** search is `~10^(d/2)`-bound (brute force `hunt.c`
   reaches ~d=27), and a faster search (MITM) *and* a congruence non-existence
   proof have both been rigorously **ruled out** — see
   [`structural_attacks_2026-06-04.md`](structural_attacks_2026-06-04.md).
   A density heuristic predicts emirps are *finite* (≈`C/d²`, sum converges);
   a theorem, if any, needs new mathematics.
4. The Phase-3 "composite-`p`" caveat from the Option-B work — does the sieve's
   survivor count ever miss a candidate the brute force would catch? So far they
   agree exactly (d=13–17); worth keeping as a guard as `d` grows.
```
