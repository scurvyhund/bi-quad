# Modular Obstruction Experiment — Design & Results

> ## ⚠️ CORRECTION NOTICE (2026-06-03)
>
> A **correctness bug** was found and fixed (commit `22a7121`): the Phase 3
> per-residue check approximated the achievable first-k prefixes as a contiguous
> interval, which is valid only when `range = n_max - n_min < mod = 10^k`. Past
> the "cliff" where `range >= mod` (roughly **d ≥ 2k+1**), residues have 2+
> n-values and the interval admits prefixes no real n produces — **massively
> over-counting survivors** and then hitting a **fake saturation** that silently
> stopped the search.
>
> **What this means for the results below:**
> - **Cells with d ≤ 2k are VALID** (range<mod → interval was exact).
> - **In every k-column, the last two filled cells — the large "~59%" count
>   and the "sat" cell — are ARTIFACTS.** They are over-counts and a fake
>   saturation, not real. *Proven:* the fixed code + an independent brute
>   force give k=6 d=13 = **8** (this doc says 591606) and d=14 = **30**
>   (this doc says "sat").
> - **The headline conclusion "obstructions only exist for d up to ~2k" is
>   WRONG.**
>   With correct counting, survivors past the cliff are tiny and grow ~10×/digit
>   (k=6: d13=8, d14=30, d15=269, d16=2494, d17=25292…). There is no 2k+2
>   saturation; the true landscape past the cliff is unknown and must be
>   recomputed. The "pre-saturation ~59.16%" figure is the over-count signature,
>   not a feature.
>
> The k=3..9 obstructions and survivor counts at **d ≤ 2k remain valid** (incl.
> the d=10 and d=18/k=9 obstructions). See
> [`session_2026-06-03_cliff_discovery.md`](session_2026-06-03_cliff_discovery.md)
> for the full analysis and validation.

## Goal

For each digit count d, determine whether it is structurally possible for a
d-digit converse pair (p, rev(p)) to exist where both p and rev(p) are of the
form 2n² + 2n + 1. If we can show that zero valid combinations survive for
all d >= 6, combined with the exhaustive search through 10^28 (covering
d <= 28), we have a **complete proof of uniqueness**.

> **NOTE 2026-11-19:** "exhaustive search through 10^28" overstates. The
> brute-force-verified bound is **d ≤ 27** (`hunt`; see README). The d = 28
> run (`gmp-cvo/logs/run_10e28.log`) is `cvpipe` alone and is **not a claim**
> under the project's verification rule — see `cvpipe_resweep_xcheck.md`.
> Two other cvpipe logs (`run_10e22.log`, `10e25_to_10e30.txt`) were silently
> truncated by `MAX_ZONES` and must not be cited at all.

---

## The Key Insight

The formula p = 2n² + 2n + 1 constrains both ends of the number simultaneously:

- **Last k digits of p** are determined by `n mod 10^k` (modular arithmetic)
- **First k digits of p** are determined by the magnitude of n (how big n is)

For a converse pair, the reversal ties them together:

- Last k digits of q = reverse of first k digits of p
- First k digits of q = reverse of last k digits of p

Both q's endings AND q's beginnings must be compatible with the form
2m² + 2m + 1. This creates a **two-sided squeeze** that tightens with
increasing k.

---

## Algorithm (for a given k)

![Control flow of mod_obstruct: argument parsing and checkpoint
resume, then Phase 1 (is_valid_ending bitset), Phase 2 (valid_firsts[],
two-pass and sorted), and Phase 3 (the OpenMP digit-count sweep with
the two-sided feasibility check and Hensel lift), with checkpoint
writes per d and per k](mod_obstruct.png)

The diagram is generated from [`mod_obstruct.dot`](mod_obstruct.dot):

    dot -Tpng docs/mod_obstruct.dot -o docs/mod_obstruct.png

Verified against `mod_obstruct.c` at `4c7cee3` (diagram source at
`b8fa6e3`); still holds while `git log 4c7cee3.. -- mod_obstruct.c` is
empty. Re-checked 2026-11-19 after the audit's checkpoint fix `7320ce5`,
which the 2026-09-12 version predated: every
function and identifier it names is present in `mod_obstruct.c`, and the
argument order it shows (`max_d max_k min_k min_d`) matches `main()`.
Two corrections came out of that re-check:

- **Phase 3 was drawn as the pre-`22a7121` interval method**
  (`fk_min`/`fk_max`, `last_n_with_residue`, a loop over `valid_firsts`
  in an interval). The source has enumerated each actual n per residue
  since June. The 2026-09-12 "verified" note missed it: it checked that
  names existed, and `first/last_n_with_residue` half-matched. Phase 3
  now shows the exact per-n and per-m check.
- **The checkpoint box** now shows the per-invocation file (`ckpt_init`);
  the old box described the shared `mod_obstruct.ckpt` that let a sanity
  run's checkpoint hijack a full run.

Checking names is not checking the flow: this re-check read the loop.


### Phase 1: Build the Valid Endings Bitset

Compute all possible last-k-digit patterns of numbers of the form 2n² + 2n + 1.
Stored as a **bitset** (1 bit per entry) to minimize memory at high k.

```
is_valid_ending = bitset of size 10^k
for n = 0 to 10^k - 1:
    e = (2n² + 2n + 1) mod 10^k       // computed inline via __int128
    set bit e in is_valid_ending
```

The quadratic cycles mod 10^k, so we only need 10^k iterations.
The ending `(2r² + 2r + 1) mod m` is computed inline using 128-bit arithmetic
(`__int128`) for k ≥ 10, or native 64-bit for k ≤ 9, eliminating the need for
an `endings[]` array (which would be 80GB at k=10).

### Phase 2: Build Valid Firsts (sorted array, right-sized)

For q to be of the form 2m² + 2m + 1, its last k digits must be achievable.
But last k digits of q = reverse of first k digits of p. Therefore:

```
// Pass 1: count unique valid firsts (using temporary bitset for dedup)
num_firsts = 0
for each e where is_valid_ending[e]:
    f = reverse_k_digits(e)
    if f >= 10^(k-1) and f not yet seen:
        num_firsts++

// Pass 2: allocate right-sized array and fill (already in sorted order)
valid_firsts = malloc(num_firsts * sizeof(long))   // NOT mod-sized!
fill from bitset scan in ascending order
```

The two-pass approach allocates only for the actual count (~10.4% of mod),
avoiding the massive over-allocation of the original mod-sized array.

### Phase 3: Two-Sided Feasibility Check (OpenMP parallel)

For each digit count d and each residue r (mod 10^k):

```
p_ending = (2r² + 2r + 1) mod 10^k        // last k digits of p

// P-side check: do first-k digits of p land on a valid first?
fk_range = [first_k(smallest n ≡ r), first_k(largest n ≡ r)]
if no valid_first in fk_range: SKIP (binary search on sorted array)

// Q-side check: for each valid first f in fk_range:
q_ending = reverse_k(f)                    // last k digits of q
q_first  = reverse_k(p_ending)             // first k digits of q

// Find m residues where (2m²+2m+1) mod 10^k == q_ending
//   → solved on the fly via HENSEL LIFTING (no precomputed table)
m_residues = solve_residues(q_ending, k)

// Does any such m produce first-k prefix == q_first?
for each m_r in m_residues:
    mk_range = [first_k(smallest m ≡ m_r), first_k(largest m ≡ m_r)]
    if q_first in mk_range: SURVIVOR
```

If survivors == 0 for a given d, no converse pair with d digits can exist.

---

## Hensel Lifting (replacing Phase 2b lookup tables)

The original design precomputed a reverse lookup from each valid ending back
to all residues that produce it. At k=10 (mod=10^10), these tables would
require ~20GB+. Instead, we solve `2m² + 2m + 1 ≡ target (mod 10^k)` on
the fly using iterative Hensel lifting (background: Keith Conrad,
[*Hensel's Lemma*](https://kconrad.math.uconn.edu/blurbs/gradnumthy/hensel.pdf)):

1. **Base case:** brute-force solutions mod 10 (at most 4: endings 1,3,5 only)
2. **Lift:** from mod 10^j to mod 10^(j+1) — try 10 digit extensions per
   solution
3. **Growth:** bounded by the CRT split below — not "≤ doubles per step"

**Proven bound** (`hensel_cap()` in `mod_obstruct.c`; the root count
mod 2^k is 4, mod 5^k peaks at max(2·5^⌊(k−1)/2⌋, 5^⌊k/2⌋)):

    hensel_cap(k) = 4 · max(2·5^⌊(k−1)/2⌋, 5^⌊k/2⌋)
                  = 500 (k=6), 5000 (k=9), 12500 (k=10), 125000 (k=13)

The buffers are heap-allocated at that size, the lift is exact `u128`,
and exceeding the cap is a fatal error, never a truncation.

> **Corrected 2026-11-19.** This section used to say "max ≈ 4 × 2^(k−1),
> ~2048 at k=10, fits in a 32KB stack buffer". That estimate was wrong
> (k=9 has targets with 5000 solutions), and the code built on it — a
> fixed `MAX_HENSEL_SOLS = 4096` that silently dropped solutions, plus
> an int64 overflow at k ≥ 10 — produced the false k=10 d=21
> OBSTRUCTION. Fixed `417ca34`; see `docs/audit_2026-11.md`.

---

## Memory-Efficient Design

The original implementation used bool arrays and full-sized lookup tables,
consuming ~180GB at k=10. The current design:

| Structure | Original (k=10) | Current (k=10) |Savings |
|-----------|-----------------|-----------------|---------|
| `is_valid_ending` | bool array: 10GB | bitset: **1.25GB** | 8× |
| `endings[]` | long array: 80GB | **eliminated** (inline `__int128`) | ∞ |
| `valid_firsts[]` | mod-sized: 80GB | right-sized: **~8GB** | 10× |
| `is_valid_first[]` | bool array: 10GB | temp bitset: **1.25GB** (freed) | 8× |
| Phase 2b lookup | ~20GB | **eliminated** (Hensel lifting) | ∞ |
| **Total** | **~180GB** | **~10GB** | **~18×** |

### Build

```
gcc -O3 -march=znver2 -mtune=znver2 -std=c99 -Wall -Wextra -fopenmp \
    -o mod_obstruct mod_obstruct.c -lgmp
```

---

## Results (k=3 through k=9)

> ⚠️ **The "saturation at d = 2k+2" below is an ARTIFACT** (see correction
> notice at top). In each column, the large count and the "sat" cell (both at
> d ≥ 2k+1) are bogus. Cells at d ≤ 2k are valid.

~~Saturation consistently occurs at d = 2k + 2.~~ The fraction of valid endings
stabilizes at ~10.42% for k ≥ 5.

### Obstruction Map

| d \ k | 3 | 4 | 5 | 6 | 7 | 8 | 9 |
|-------|---|---|---|---|---|---|---|
| 4 | **OBS** | | | | | | |
| 5 | 6 | 6 | | | | | |
| 6 | **OBS** | **OBS** | **OBS** | | | | |
| 7 | 599 | 7 | 7 | 7 | | | |
| 8 | sat | 2 | 2 | 2 | 2 | | |
| 9 | | 5930 | 6 | 6 | 6 | 6 | |
| 10 | | sat | **OBS** | **OBS** | **OBS** | **OBS** | **OBS** |
| 11 | | | 59227 | 5 | 5 | 5 | 5 |
| 12 | | | sat | 2 | 2 | 2 | 2 |
| 13 | | | | 591606 | 4 | 4 | 4 |
| 14 | | | | sat | 6 | 6 | 6 |
| 15 | | | | | 5916383 | 6 | 6 |
| 16 | | | | | sat | 2 | 2 |
| 17 | | | | | | 59161418 | 1 |
| 18 | | | | | | sat | **OBS** |
| 19 | | | | | | | 591606462 |
| 20 | | | | | | | sat |

**OBS** = obstruction (0 survivors), **sat** = saturated (all residues survive).

> ⚠️ **Artifact rows:** every "sat" cell, and the large count immediately above
> it in each column (i.e. all cells at d ≥ 2k+1), are products of the interval
> bug — over-counts + fake saturation, NOT real. E.g. d=13/k=6 shows 591606 but
> the true value is **8**; d=14/k=6 "sat" is truly **30**. Valid cells: d ≤ 2k.

### Key Observations

1. **Persistent obstructions at d=10:** Every k from 5 through 9 finds an
   obstruction at d=10. This is a robust structural feature.

2. **New obstruction at d=18, k=9:** The first obstruction beyond d=10,
   appearing only at k=9. The single lone survivor at d=17 (k=9) is notable.

3. **Survivor counts stabilize across k:** For a given d, the survivor count
   converges as k increases (e.g., d=11 stabilizes at 5 for k ≥ 6).

4. **Pre-saturation ratio ~59.16%:** The d value just before saturation
   consistently shows ~59.16% survival rate (591606/10^6, 5916383/10^7, etc.).

5. **Valid endings fraction ~10.42%:** Stabilizes by k=5, reflecting the
   image size of the quadratic map mod 10^k.

---

## Scaling Considerations

| k | mod = 10^k | Memory (current) | Phase 1 time | Status |
|---|-----------|------------------|-------------|--------|
| 3 | 1,000 | trivial | instant | complete |
| 4 | 10,000 | trivial | instant | complete |
| 5 | 100,000 | trivial | instant | complete |
| 6 | 1,000,000 | ~1 MB | instant | complete |
| 7 | 10,000,000 | ~10 MB | <1s | complete |
| 8 | 100,000,000 | ~100 MB | ~5s | complete |
| 9 | 1,000,000,000 | ~1 GB | ~30s | complete |
| 10 | 10,000,000,000 | ~10 GB | ~minutes | **running** |
| 11 | 100,000,000,000 | ~100 GB | ~hours | future |

For k ≥ 11, CRT decomposition (working mod 2^k and 5^k separately, then
combining) could extend the practical reach.

---

## Limitations

- ~~**Saturation at d = 2k + 2:** For d values beyond ~2k, the first-k-digit
  range becomes wide enough that valid firsts are always achievable. This means
  each k value can only produce obstructions for d up to roughly 2k.~~
  **❌ RETRACTED (2026-06-03):** this was an ARTIFACT of the interval bug, not a
  real phenomenon. The "saturation" was a fake count produced by admitting
  prefixes no real n generates. With correct enumeration, survivors past d=2k
  are small and grow slowly — there is NO 2k+2 saturation, and obstructions
  beyond ~2k are NOT ruled out. The reachable-d ceiling for the proof strategy
  is therefore an OPEN question again. (This also means the proof-coverage claim
  below — "obstruction for d=6..50" — is not supported by current data.)

- **Pre-saturation performance:** The d value just before saturation has ~59%
  survivors, each requiring Hensel lifting calls. This is the computational
  bottleneck — approximately 2.6× slower than the original array-lookup approach
  for k ≤ 9, but essential for k ≥ 10 where arrays don't fit in memory.

- **Proof coverage:** Even if obstruction doesn't cover all d to infinity,
  proving it for d=6..50 (combined with exhaustive search through d=27 --
  d=28 is cvpipe-only, see the note in the Goal section) would
  be a significant result — far beyond what brute-force search alone achieves.

---

## What to Look For in k=10 Results

1. **d=10 obstruction should persist** (strong expectation from k=5..9 trend)
2. **d=18 obstruction should persist** (appeared at k=9)
3. **Possible new obstructions** at d values that previously had small survivor
   counts (d=17 had just 1 survivor at k=9 — could drop to 0 at k=10)
4. ~~**Saturation expected at d=22** (following the d = 2k + 2 pattern)~~
   **❌ The d=2k+2 pattern was an artifact — do NOT expect saturation at d=22.**
   With the fixed code, survivors past the d=21 cliff (range≥mod) will be small
   and nonzero, growing slowly. The true k=10 landscape for d≥21 is unknown and
   must be measured with the corrected binary (commit 22a7121).
