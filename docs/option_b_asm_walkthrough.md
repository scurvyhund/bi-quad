# Option B — GCC Assembly Walkthrough

**Date:** 2026-04-10
**Subject:** Why the Option B filter compiles to something much better than its source code suggests
**Compiler:** `gcc -O3 -march=znver2 -mtune=znver2 -std=c99 -Wall -Wextra -fopenmp`

---

## TL;DR

Three discoveries from inspecting the generated assembly for `mod_obstruct.c` after
Option B was added:

1. **The binary-search `/2` is already a shift** — GCC emits a single `sarq %rax`
   (arithmetic shift right by 1), not an `idivq`. No hand optimization needed.

2. **The Phase 1 / Phase 3 `r % 10` computation uses no hardware division** —
   GCC folds `/10` into a magic-constant `imulq` + shift sequence, and recovers
   the remainder with a 3-instruction pattern. Zero real divides for the
   residue-class test.

3. **The 4-way `||` filter collapses to a single `btq` + `jc`** — the source code
   `if (r10 == 1 || r10 == 3 || r10 == 6 || r10 == 8)` becomes a bit-test against
   a precomputed mask `0x14A` (bits 1, 3, 6, 8 set). Four compares + four branches
   become *two micro-ops* in the compiled hot path.

The cumulative effect of (2) and (3) is why the Option B 40% residue skip produces
a measured 5-6× wall-clock speedup on k=10 through d=20 — the skip isn't "just"
eliminating iterations, it's eliminating *near-free* iterations in exchange for
*almost-free* work on the 60% that remain.

---

## How to reproduce

```bash
cd /home/jim/programming/c/BigFermat/bi-quad
gcc -O3 -march=znver2 -mtune=znver2 -std=c99 -Wall -Wextra -fopenmp \
    -S -o /tmp/mod_obstruct.s mod_obstruct.c

# Count actual hardware divisions
grep -cE "idivq|divq" /tmp/mod_obstruct.s     # ~20, all in ending_for_residue path

# Count shift-by-1 operations (what a /2 would compile to)
grep -cE "sarq$|sarq %|shrq$|shrq %" /tmp/mod_obstruct.s

# Find the btq instruction used for the Option B residue filter
grep -n "btq" /tmp/mod_obstruct.s
```

Note: `sorted_has_value_in_range()` gets fully inlined under `-O3`, so it won't
appear as a function symbol — you'll find its code inside `main:`.

---

## Finding 1 — The binary search `/2` is already a shift

### Source (`mod_obstruct.c`)

```c
static bool sorted_has_value_in_range(const long *arr, long len,
                                      long lo, long hi) {
    ...
    long left = 0, right = len;
    while (left < right) {
        long mid = left + (right - left) / 2;   // ← the /2 Jim noticed
        if (arr[mid] < lo)
            left = mid + 1;
        else
            right = mid;
    }
    ...
}
```

### Assembly at line 605-617 (inlined into `main`)

```
.L45:
    movq   %rcx, %rax           ; rax = right
    subq   %rdx, %rax           ; rax = right - left
    sarq   %rax                 ; rax = (right - left) >> 1   ← THE SHIFT
    addq   %rdx, %rax           ; rax = left + (right - left)/2 = mid
    cmpq   %r12, (%r9,%rax,8)   ; compare valid_firsts[mid] < fk_min
    jl     .L286                ; if yes, left = mid + 1 branch
    movq   %rax, %rcx           ; else right = mid
.L44:
    cmpq   %rdx, %rcx
    jg     .L45                 ; while (left < right)
```

### Why it's safe as `sarq` (one instruction)

`sarq` is arithmetic shift right — it preserves the sign bit. For a *non-negative*
operand, `sarq $1, x` is bit-for-bit identical to `shrq $1, x` and gives the same
result as integer `/2`. GCC uses `sarq` here because it's proved the operand
`(right - left)` is non-negative *by control-flow analysis*: the loop body is
only entered when `left < right`, so `right - left > 0`, so no sign-handling
adjustment is needed.

If GCC couldn't prove non-negativity (e.g. if the subtraction happened outside
the loop body), it would have emitted the slower signed-divide-by-2 pattern:

```
sarq $63, %rdx          ; extract sign bit
andq $1, %rdx           ; isolate
addq %rdx, %rax         ; round toward zero
sarq $1, %rax           ; actual shift
```

That's 4 instructions instead of 1. We get the 1-instruction version because of
the loop invariant. **Instinct was right, manual rewrite was unnecessary.**

Count of `sarq $1` in the compiled output: 2 — matching the two separate binary
searches in Phase 3 (`sorted_has_value_in_range` at the outer check, and the
inline search for `fi` in the inner loop).

---

## Finding 2 — No `divq` for the `r % 10` test

### Source

```c
long r10 = r % 10;
if (r10 == 1 || r10 == 3 || r10 == 6 || r10 == 8)
    continue;
```

### Assembly

```
; (earlier, outside hot path) imulq with magic constant computes r / 10 into %rax
shrq $3, %rax                 ; final shift of the magic-multiply divide
leaq (%rax,%rax,4), %rax      ; rax = 5 * (r/10)          (via LEA, 1 µop)
addq %rax, %rax               ; rax = 10 * (r/10)
subq %rax, %rdx               ; rdx = r - 10*(r/10) = r % 10
btq  %rdx, %rdi               ; bit test: is bit (r%10) set in mask %rdi?
jc   .L21                     ; jump if carry (= bit was set) → continue
```

### What happened to the division?

GCC uses **Granlund-Montgomery magic-constant multiplication** to replace `x / 10`
(unsigned or non-negative signed) with:

1. Multiply by a precomputed magic constant
2. Shift the high half of the product

For divisor 10, the magic constant is `0xCCCCCCCCCCCCCCCD`, and the shift is
right-by-3. No hardware `divq`. Cost: one `imulq` (~3 cycles on Zen2) + shift
(~1 cycle). Compare to a real `divq` (~18-20 cycles). Roughly **6× speedup per
divide** even before the remainder trick.

### Recovering `x % 10`

After the magic-multiply gives us `q = x / 10`, GCC computes the remainder as
`r - 10*q`, but cleverly uses `leaq` to build `10*q` without a separate multiply:

- `leaq (%rax, %rax, 4), %rax` computes `rax + 4*rax = 5*rax` in one µop using
  the x86 scaled-index addressing hardware (which is otherwise used for array
  indexing like `arr[i*4]`). It's a 1-cycle micro-op.
- `addq %rax, %rax` then doubles it: `5*q * 2 = 10*q`.
- `subq %rax, %rdx` gives `r = x - 10*q = x mod 10`.

No division. Just `imulq` + three cheap arithmetic ops.

---

## Finding 3 — The 4-way `||` becomes one `btq`

This is the one that made me stop and stare.

### What you'd expect (naive compilation)

```
cmpq $1, %rdx      ; 1 µop
je   .Lskip        ; branch 1
cmpq $3, %rdx      ; 1 µop
je   .Lskip        ; branch 2
cmpq $6, %rdx      ; 1 µop
je   .Lskip        ; branch 3
cmpq $8, %rdx      ; 1 µop
je   .Lskip        ; branch 4
```

- 8 µops (4 compares + 4 jumps)
- 4 branch prediction slots consumed
- 4 opportunities to mispredict
- 2-4 cycles on the taken path
- Cold-cache cost: additional I-cache pressure from the compare+branch scheduling

### What GCC actually did

```
btq  %rdx, %rdi    ; 1 µop  — bit test against mask in %rdi
jc   .L21          ; 1 µop  — jump if bit was set
```

- 2 µops total
- 1 branch prediction slot
- 1 opportunity to mispredict
- **~2 cycles on the taken path**

### How it works

GCC noticed that the set `{1, 3, 6, 8}` can be encoded as a bitmask:

```
bit index:  9 8 7 6 5 4 3 2 1 0
value:      0 1 0 1 0 0 1 0 1 0   = 0x14A
```

- Bit 1 set (the `r10 == 1` case)
- Bit 3 set (the `r10 == 3` case)
- Bit 6 set (the `r10 == 6` case)
- Bit 8 set (the `r10 == 8` case)

Then `r10 ∈ {1,3,6,8}` is equivalent to "bit `r10` is set in `0x14A`", which is
**exactly what `btq` does**: `btq %rdx, %rdi` sets the carry flag to the value of
bit `rdx` in register `rdi`. Then `jc` branches on that single flag.

The mask `0x14A` is loaded into `%rdi` **once, before the loop**, so the inner
loop body doesn't even re-fetch the constant. It's sitting in a register for the
entire Phase 3 residue iteration. Zero per-iteration constant cost.

### Why this is particularly beautiful on Zen2

The AMD Zen2 branch predictor (TAGE-style) is extremely good at pattern recognition.
The Option B filter has a *perfectly periodic* skip pattern — the `r` values cycle
through `0..9` modulo 10, so the predictor sees the same repeating sequence
(`NN-Y-N-Y-NN-Y-N-Y` where Y = skip taken) every 10 iterations. After a few
microseconds of warmup, the predictor locks onto this pattern and runs essentially
zero mispredictions for the rest of the loop. Combined with the single `jc`
(one branch slot instead of four), the branch prediction hardware has the easiest
possible job.

This is why the Option B speedup **exceeds** the naive 40% iteration cut. The
40% of iterations we skip cost *almost nothing* (a `btq` + `jc` = 2 µops = ~2
cycles), while the 60% we keep benefit from improved cache residency, fewer
branches in flight, and less BTB pressure.

---

## What IS costing cycles — the real `divq` instructions

```bash
grep -cE "idivq|divq" /tmp/mod_obstruct.s
# 20+
```

These are **real hardware divisions**, all in the `ending_for_residue` code path:

```c
static inline long ending_for_residue(long r, long m) {
    if (__builtin_expect(m <= 1000000000L, 1)) {
        unsigned long long rr = (unsigned long long)r;
        return (long)((2*rr*rr + 2*rr + 1) % (unsigned long long)m);  // divq here
    }
    __int128 rr = (__int128)r;
    return (long)((2*rr*rr + 2*rr + 1) % (__int128)m);                 // __int128 divq
}
```

Since `m = 10^k` is not a power of 2, GCC **cannot** eliminate this with a shift.
It emits a real `divq` (or `__int128` divide via `__udivmodti4`). Cost on Zen2:
- 64-bit `divq`: ~18-20 cycles latency
- `__int128` divide: significantly more (calls out to libgcc)

### Is this optimizable?

**Yes, in principle** — via **Barrett reduction**. The idea: precompute a "magic"
constant for dividing by `10^k` *once* per k, then replace each `divq` with:

```
mulq  magic_hi_bits     ; high multiply, ~4 cycles
subq  ...               ; adjust
cmp   ...
cmov  ...               ; final correction
```

Roughly 2-3 multiplies + a few shifts/adds, total ~10-12 cycles vs. 18-20 for
`divq`. Potential ~2× speedup on each call.

**Expected benefit at k=10**:
- Phase 1: ~6×10^9 `divq` calls × ~10 cycles saved = ~60 seconds
- Phase 3: much fewer calls (pre-filter removes most residues at small d),
  maybe another ~10-30 seconds total

**Potential total savings**: ~1-2 minutes off Phase 1+2 and small-d Phase 3 work.

**Downside**: Barrett reduction is fiddly to get right — the constant depends
on the word size, the operand range, and whether you need round-up or round-down.
It's easy to have off-by-one errors that only show up at specific input values.

**Recommendation**: not worth it right now. The 1-2 minutes we'd save is negligible
compared to the hours-to-days still pending for k=10 d=21 and beyond. Come back to
this as a polish pass once the current k=10 run finishes.

---

## Why read compiler output at all?

Two reasons:

1. **Calibration**. When Jim looked at `left + (right - left) / 2` and thought
   "that should be a shift," he was correctly reading the source the way the
   hot path actually runs. Checking the assembly confirmed his instinct was
   right *and* showed that GCC beat him to the punch. That's not a failure —
   it's a sign that his mental model of the code matches what's actually
   happening at the metal.

2. **Opportunity detection**. The things GCC *can't* optimize (real `divq`,
   GMP function calls, `__int128` modulo) become visible as soon as you look.
   Those are the real targets for future optimization work, not the things
   that look like divisions in the source but compile to shifts and multiplies.

The Option B transformation is a particularly good case study because the
source-level intent ("skip these residues") looks expensive (4 compares) but
compiles to something almost trivial (1 bit-test). Without looking at the asm,
it would have been easy to assume the Option B win was "just" 40% iteration
savings — but the compiler turns it into something even better.

---

## References

- `mod_obstruct.c` — current source
- `/tmp/mod_obstruct.s` — generated assembly (regenerate with the gcc command above)
- Commit `001d73c` — Option B correctness fix
- Zen2 uops / latency tables: Agner Fog's instruction tables,
  https://www.agner.org/optimize/instruction_tables.pdf
- Hacker's Delight (Warren) — source for many of these bit-level identities
- Granlund & Montgomery, "Division by Invariant Integers using Multiplication"
  (1994) — the foundational paper behind GCC's magic-multiply divide
