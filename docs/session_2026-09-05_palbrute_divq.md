# palbrute: the second cut at one hot loop

**Date:** 2026-09-05
**Outcome:** 1.76x on the palbrute inner loop, plus a silent
correctness bug found during validation of the speedup.
**Trigger:** "are you sure we've examined palbrute to the utmost and
there is no further opt gain to be had?"

---

## 1. Why the question was worth asking twice

We had already optimised this exact loop once. Commit `8bdc609`
("palbrute: early-exit palindrome predicate, 7.65x") replaced
`is_pal_rev` — which reversed all `d` digits of a u128 with no early
exit, ~29 divmod pairs per call — with `is_pal_fast`, which exits on
the first mismatched digit pair and does its inner work in 64-bit.

That was a real 7.65x and it was correctly reasoned. The trap is what
it did to the *shape* of the remaining cost.

`is_pal_fast` splits `p` into a low half and a high half, then walks
digit pairs inward. Inside a zone the outermost pair is already known
to match, so the next pair matches with probability 1/10 and the walk
returns after **~1.11 comparisons on average**.

So after the first optimisation the digit walk was essentially free —
and the *entry split* was the entire function. The fix that worked
hid the cost that remained behind its own success. That is the general
lesson: **optimising the dominant term promotes whatever was second,
and the profile you reasoned from is now stale.**

## 2. The first hypothesis, and why it was wrong

The last digit of a curve value is a pure function of `n mod 5`:

| `n mod 5` | 0 | 1 | 2 | 3 | 4 |
|---|---|---|---|---|---|
| last digit of `p = 2n(n+1)+1` | 1 | 5 | 3 | 5 | 1 |

Proof: `2n(n+1) mod 10` is determined by `n(n+1) mod 5`, because
doubling a residue mod 5 and reducing mod 10 is injective on the
relevant range. Verified exhaustively.

This looked like a big win. The zone filter tests `p % 10 == LEAD[k]`
for every `n` and discards 60-80% of them; the table says which `n`
can possibly pass, so we could stride over only those and skip the
test entirely. In the lead-1 zone that is 2 `n` in 5, in lead-3 only
1 in 5.

Measured gain: **6%.**

The reason is the whole point of section 1. The stride removes the
*cheap* part (`curve` + `p % 10`) and leaves the *expensive* part
(`is_pal_fast`) untouched — and `is_pal_fast` is called on exactly the
same set of `n` either way. It was the right idea aimed at the wrong
term, and only measurement said so.

Kept here because the reasoning is sound and the identity is true; it
is just not where the time goes.

## 3. Finding the real cost

Disassembling the OpenMP outlined loop:

    $ objdump -d palbrute | awk '/main\._omp_fn\.0>:/,/^$/' \
        | grep -o 'call.*<__u[a-z0-9]*' | sort | uniq -c
          3 call   402630 <__udivti3
          2 call   402740 <__umodti3

`__udivti3` and `__umodti3` are libgcc's **software** 128-bit divide
routines. GCC emitted calls to them in the hottest loop in the
project.

Component timings (single thread, Zen 2, d=29):

| component | cost |
|---|---|
| `curve(n)` alone | 0.94 ns |
| `p % 10` (zone filter) | 3.89 ns |
| `is_pal_fast` | **23.5 ns** |
| full inner loop, per `n` of window | 10.35 ns |

Sanity check against production: the d=29 run did 63,327,388,727,965
`n` in 96,091.6 s on 8 threads = 82.4M n/s/thread = **12.1 ns per n**,
against 10.35 ns in the benchmark. The model is faithful.

### Benchmark discipline

The first version of this benchmark reported `0.00 ns` for three of
four loops — GCC had deleted them as dead code. Accumulating into a
variable is *not* enough; the compiler proved the accumulator unused
and removed the work.

The fix is an optimisation barrier:

```c
#define SINK(x) __asm__ volatile("" : "+r"(x))
```

This tells GCC the value is read and modified by opaque code, so it
must be materialised in a register — with no actual instruction
emitted. An earlier version used a `"memory"` clobber as well, which
also forces the `bq_pow10[]` lookups to reload every iteration and
inflates every number; use the register-only form.

**Any micro-benchmark reporting a suspiciously round or suspiciously
small number is measuring nothing.** Check the disassembly.

## 4. The fix: one instruction GCC refused to emit

`is_pal_fast` needs `x / 10^h` and `x % 10^h`. x86-64 has exactly this
instruction:

    DIV r64    ;  RDX:RAX / r64  ->  quotient in RAX, remainder in RDX

One instruction, ~20-40 cycles on Zen 2, versus ~90 for the software
path. So why doesn't GCC use it?

Because **`DIV r64` faults (#DE) if the quotient does not fit in 64
bits.** That is a hardware trap, not a wrong answer. Given u128
operands GCC cannot prove the quotient fits, so it must fall back to
the generic routine. It is being correct, not stupid.

We can prove what GCC cannot. In a `d`-digit sweep, `x < 10^d`, and
the divisor is `10^h` with `h = floor(d/2)`. So

    quotient  <  10^d / 10^h  =  10^ceil(d/2)

and the quotient fits iff `10^ceil(d/2) <= 2^64 = 1.8446744e19`, i.e.
`ceil(d/2) <= 19`, i.e. **`d <= 38`**.

The file already contains:

```c
#if BQ_MAX_D > 37
#error "is_pal_fast: 10^ceil(d/2) exceeds uint64 for BQ_MAX_D > 37"
#endif
```

The guard that was already there for the u64 casts is the same bound
this needs, with one digit of margin. That the constraint landed
exactly on an existing invariant is a good sign the change fits the
code rather than fighting it — but the two are now **coupled**, and
the comment in the source says so: relaxing that guard no longer
produces a wrong answer, it produces a SIGFPE.

A second gift falls out. We need `hi = x / 10^(d-h)`, and
`d - 2h` is 0 for even `d` and 1 for odd `d`. So the second division
is a 64-bit divide by a literal 1 or 10 — which GCC compiles to a
multiply-high. **One `divq` yields both halves.**

```c
static inline void divq_u128(u128 x, uint64_t den, uint64_t *q,
                             uint64_t *r) {
   __asm__ ("divq %[den]"
            : "=a" (*q), "=d" (*r)
            : [den] "r" (den),
              "a" ((uint64_t)x), "d" ((uint64_t)(x >> 64)));
}
```

Guarded by `#if defined(__x86_64__) && !defined(PALBRUTE_NO_ASM)`,
with the original u128 arithmetic as the portable fallback. Both paths
are built and validated.

## 5. The connection to dev256

The constraints `"a"` and `"d"` are RAX and RDX by name, and the
operands handed to them are

    (uint64_t)x           /* low  64 bits */
    (uint64_t)(x >> 64)   /* high 64 bits */

which is character-for-character the idiom in
`bigint-mul/u128-native/dev256.c:241-242`:

```c
x->lo  = (u64)x_int;     // low 64 bits of x_int
x->mid = x_int >> 64;    // hi 64 bits of x_int
```

This is not a coincidence, and the symmetry is exact:

| | instruction | direction |
|---|---|---|
| `dev256` schoolbook multiply | `MUL r64` | two u64 in -> product **out** in RDX:RAX |
| `palbrute` palindrome split | `DIV r64` | RDX:RAX **in** -> quotient RAX, remainder RDX |

They are inverse operations on the same register pair. `dev256` spends
its life *assembling* RDX:RAX out of 64-bit limbs; palbrute's hot loop
spends its life *taking one apart*. The shift-and-cast is how the u128
is handed across in both directions.

### The distinction that matters

A shift splits in **base 2^64**. The palindrome test needs a split in
**base 10^h**.

In `dev256` the halves are already there — bits 127..64 and 63..0 are
two registers, and the split is free because it is only relabelling.
That is why schoolbook multiply is cheap.

Decimal has no such luck: `10^h` is not a power of two, so no shift
produces it and a real division is unavoidable. What `divq` buys is
not avoiding the division — it is doing it in one instruction instead
of ninety cycles of software.

## 6. Results

Per `n` of window, lead-1 zone, d=29, single thread:

| variant | ns per n | speedup |
|---|---|---|
| current (`__udivti3`) | 10.51 | 1.00x |
| **single `divq`** | **5.96** | **1.76x** |
| divq + mod-5 stride | 5.90 | 1.78x |

The mod-5 stride is worth **1%** once `divq` is in: the loop becomes
latency-bound on that one instruction and the `p % 10` hides
completely in its shadow. Dropped — it would have added stride
arithmetic and checkpoint-accounting complexity for nothing.

### The benchmark overstated it -- production is 1.19x

**The 1.76x above is a single-threaded, boost-clock number and it does
not survive contact with the real run.**

This box is a Ryzen 7 4700U: a 15 W laptop part, 8 cores / 8 threads,
base 1.4-2.0 GHz. A single-threaded benchmark runs one core at full
boost. The production sweep runs all 8 threads and settles near base
clock, thermally limited. Both binaries are affected, but not by the
same factor -- the loop's bottleneck shifts when the clock halves.

Same-zone (z0), same-block comparison, d=29 old log vs d=31 new run:

| block | d=29 (old) | d=31 (new) | ratio |
|---|---|---|---|
| 5e9 | 675 M n/s | 894 M n/s | 1.32x |
| 10e9 | 653 M | 772 M | 1.18x |
| 20e9 | 638 M | 751 M | 1.18x |
| 30e9 | 620 M | 741 M | 1.19x |

Both runs decay ~6% further across the zone as the chip heats (d=29
z0 ended at 582 M n/s).

**Measured production speedup: ~1.19x.**

Projected sweep times, using the measured figure and d=29's zone mix
(zones 1-2 are faster than zone 0: the lead-3 zone sends only 1 n in 5
to is_pal_fast, against 2 in 5 for lead-1 and lead-5):

| d | before | benchmark said | actual |
|---|---|---|---|
| 31 | 11.1 d | 6.3 d | **~9.6 d** |
| 33 | 111 d | 63 d | ~93 d |
| 35 | 3.0 yr | 1.7 yr | ~2.5 yr |

### The methodology error, recorded

Two mistakes compounded:

1. **Benchmarked single-threaded on a thermally-limited part.** On a
   15 W chip the difference between one core boosting and eight cores
   sustained is roughly 2x in clock. Any micro-benchmark here must be
   run at the thread count the real job uses.
2. **Compared a benchmark ratio against a production average.** The
   659 M n/s headline for d=29 is the mean over all three zones, but
   the benchmark modelled the lead-1 zone only -- the slowest one.
   Same-zone, same-block is the only fair comparison, and it is
   available for free in the checkpoint logs.

The correct procedure, for next time: build both binaries, run each on
the same restricted n-range with all 8 threads, and compare. That
costs minutes and would have caught this before any claim was made.

**The change is still worth having** -- 1.19x is real, and the zone
bug it uncovered (section 7) mattered far more than the speed.

## 7. The bug the validation found

Validating the speedup against the old binary, every full-window sweep
from d=1 to d=21 matched **except d=1**, where the new build reported
one extra hit: `5  n=1  PRIME`.

`5` is the first term of A050239. The old binary was missing it. The
speedup did not cause this — both binaries behaved identically — but
running d=1 at all is what exposed it.

The zone report gave it away:

    zones (a curve palindrome must start with 1, 3 or 5):
      lead 1  n in [0, 0]  1 values
      lead 3  n in [1, 1]  1 values      <-- n=1 gives p=5, not 3xxx

Zones are built by filtering `LEAD[] = {1, 3, 5}` and storing the
survivors **compacted**:

```c
for (int k = 0; k < NZONE; k++) {
   ...
   if (z0 > z1) continue;              /* empty zone skipped */
   zlo[nz] = z0; zhi[nz] = z1; nz++;   /* stored at nz, not k */
}
```

but the search then recovered the target digit from the **slot**
index:

```c
int want = all ? -1 : LEAD[k];         /* k is the COMPACT index */
```

Once any zone is empty, every later zone shifts down a slot and is
searched for the **wrong leading digit**. It matches nothing, and the
run reports a clean sweep.

At d=1 the lead-3 zone is empty, so the real lead-5 zone landed in
slot 1 and was searched for digit 3. `p=5` was visited, tested against
the wrong digit, and dropped.

### Why this was dangerous well beyond d=1

For a full-window sweep at d >= 3 all three zones are non-empty, so
every published result — d=29 included — is unaffected. That was
confirmed by output-diffing old against new across d=1..21.

But the zone bounds are clamped to the requested range:

```c
if (z0 < a) z0 = a;
if (z1 > b) z1 = b;
if (z0 > z1) continue;
```

so **any run given an explicit `n_start`/`n_end` can empty a zone** —
which is exactly what you do to split a large `d` across machines or
re-run a slice. Demonstrated on d=13, restricted to `n >= 1000000`,
which clips the lead-1 zone:

    OLD:  found=0  visited=340381        <-- clean sweep, WRONG
    NEW:  3166046406613  n=1258182       <-- known d=13 palindrome
          found=1  visited=340381

The old binary visited all 340,381 candidates, tested every one
against the wrong digit, found nothing, and reported success.

**Fix:** carry the leading digit with the zone instead of re-deriving
it from position.

```c
int zlead[NZONE];
...
zlead[nz] = LEAD[k];
...
int want = zlead[k];
```

The distributed d=27 run in `distributed_run_2026-06-05.md` used
ranged invocations and should be re-checked against this.

### The lesson

This is a **parallel-array desync**: two arrays indexed by different
things, where one index silently stands in for the other. It survived
a line-by-line correctness audit (2026-09-04) that was specifically
hunting off-by-one errors, because nothing here is off by one — the
arithmetic is all correct, the *indices mean different things*.

It was found by running the smallest, most degenerate input, which the
audit had not done because d=1 is trivially uninteresting. Degenerate
inputs are where compaction bugs live: they are the only cases small
enough for a bucket to come up empty.

**Corollary:** the "found=0" outcome is not self-validating. A search
that reports nothing looks the same whether it searched correctly or
searched the wrong thing. Every zero result needs a positive control —
which is precisely why palbrute prints composite palindromes too.

## 8. Left on the table

`p(n+1) - p(n) = 4n + 4`, so `lo` and `hi` could be carried
incrementally across consecutive `n` — one 128-bit division per
OpenMP chunk instead of one per `n`, with 64-bit carry propagation in
between (divide-by-constant, ~5 cycles). Estimated a further ~2x.

Not done. It requires restructuring `schedule(dynamic, 1000000)` into
a loop over chunk indices carrying state, which is real risk in the
one tool whose entire job is to be trustworthy.

**The independence line.** palbrute exists to cross-check palsplit, so
speedups must not import palsplit's reasoning. The rule applied here:

- **Changing how the digit split is computed is fine.** `divq` is a
  different instruction for the same arithmetic; palbrute remains a
  dense sweep testing every `n` with a whole-number predicate, and
  `is_pal_rev` remains untouched as the `--verify` reference.
- **Changing how candidates are enumerated is not.** Adopting
  palsplit's residue-jump structure would make the two tools agree
  because they share a method, which is worth nothing. Incremental
  digit maintenance stays on the right side of this line; a residue
  enumeration does not.

## 9. Validation performed

- Output-diffed old vs new binaries, full sweeps, all odd `d` in
  1..21: identical results (only the `time=` line differs).
- `divq` build vs `-DPALBRUTE_NO_ASM` portable build: identical.
- `--verify` (runs `is_pal_fast` and `is_pal_rev` on every candidate,
  aborts on disagreement) and `--slow` (`is_pal_rev` only): identical.
- Standalone exhaustive agreement, `n < 6e6`, all `d <= 17`: zero
  mismatches, 13 palindromes found by both predicates.
- Ranged-run regression for the zone bug, d=13 from n=1000000.
- Known values recovered: `5`, `181`, `313`, `3187813` (all PRIME),
  plus the composite palindromes at each `d`.
