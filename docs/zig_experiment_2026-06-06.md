# Zig vs C — a hands-on evaluation (bi-quad, 2026-06-06)

> Question Jim asked: is **Zig** worth learning *as "a better tool"* (not a
> replacement) for bi-quad-type work — tight numeric loops, GMP, exhaustive
> search, native-arch tuning — given that **easy C/asm interop is paramount**?
>
> Method: port `palhunt_gmp.c` to Zig and run it head-to-head. This doc records
> what the experiment actually showed. The companion written review (in-session)
> covers the broader language survey; this is the empirical part.
>
> ⚠️ **All timings here are PRELIMINARY (n=1, taken while the d=26 emirp hunt
> saturated all 8 cores → ~3:1 oversubscription on the 2 pinned cores).** They
> establish *direction and root-cause*, not publication-grade magnitudes. A clean
> re-measure should wait until the box is idle.

---

## Verdict (the answer to the actual question)

**Yes — Zig earns a place as "a better tool," with one known cost.** The port:

- **Built first try** (Zig 0.14.1, `zig build-exe ... -lc -lgmp`). No fight.
- **C interop works as advertised** — GMP called via `@cImport("gmp.h")` with
  **zero hand-written bindings**; same `libgmp.so`, so no perf change. (0.14
  still has `@cImport`; 0.16 moved C translation to the build system. For a small
  tool, 0.14's builtin is much simpler.)
- **Native `u128`** — the C needed a hand-rolled hi/lo splitter just to *print*
  `unsigned __int128`; in Zig `std.fmt` prints it directly, so that code vanished.
- **Performance is equal to C** (see below) once the parallelism is matched.
- **The one real cost is OpenMP** — Zig has none; the `#pragma omp parallel for`
  becomes a hand-built thread pool. That cost is **real but bounded** (~10 lines).

Recommended adoption path (unchanged, low-risk): use `zig cc` as a drop-in C
compiler first (zero Zig learned), then port one small tool, then judge. Keep the
heavily-OpenMP tools (`mod_obstruct.c`) in C unless a port genuinely delights.

---

## The performance story (and two corrected hypotheses)

The interesting part was *debugging an apparent gap*, and it produced two
walk-backs worth recording as method:

1. **Naive port looked ~25% slower** (multi-core, d=18–19, contended).
2. **Hypothesis A — "it's the `u128` divide":** the loop calls `__udivti3` for
   `p % 10` every iteration; objdump showed C uses libgcc's `__udivti3`, Zig uses
   compiler-rt's. Plausible… **but wrong.** A **single-core** run (which removes
   multi-thread scheduling) came out **tied** (Zig 3.0s vs C 3.2s @ d=17). So
   codegen and the divide routine are equal. *Lesson: isolate one variable.*
3. **Hypothesis B — "it's the scheduling":** the gap appeared **only** in the
   multi-threaded contended runs. The C used OpenMP `schedule(dynamic, 1M)`; the
   Zig port used **static contiguous chunks**. Under oversubscription an unfair
   scheduler starves some static chunks, and completion waits on the unluckiest
   thread; OpenMP's dynamic work-stealing doesn't have that tail. **Confirmed:**
   replacing Zig's static split with a shared atomic chunk-dispenser (mimicking
   `schedule(dynamic, 1M)`) **closed the gap entirely** — Zig then matched C
   (16.3s vs 17.0s @ d=19).

**So the 25% was an *oversubscription* artifact of static partitioning, NOT a
property of Zig.** What it *does* validate empirically: OpenMP's dynamic
scheduler is genuine value, and without OpenMP you re-implement it by hand. That
was the headline caveat of the written review, now demonstrated and *recovered*.

| run (d=19, taskset -c 6,7, contended) | wall |
|---|---|
| C (OpenMP dynamic) | ~17.0 s |
| Zig, static chunks (first port) | ~22 s |
| Zig, dynamic dispenser (fixed) | ~16.3 s |
| C vs Zig @ d=17, **single core** | 3.2 s vs 3.0 s (tied) |

---

## Spinoff: a small, real optimization (the `n mod 5` skip)

Chasing the divide led to a genuine — if modest — optimization of the **C tool
itself**. The last digit of `p = 2n²+2n+1` depends only on `n mod 5`:

```
n mod 5 :  0  1  2  3  4
last(p) :  1  5  3  5  1
```

So track `n mod 5` with cheap 64-bit math (one `u128 % 5` per d, outside the
loop), skip `n ≡ 1,3` (last digit 5 → composite) **without computing p**, and
never do a 128-bit `p % 10` in the hot path. This is the digit-ending "proof by
construction" applied to the brute inner loop.

**Measured by CPU-time (user+sys — contention-independent, the right metric here),
d=19:**

| build | CPU-work | vs baseline |
|---|---|---|
| baseline C | 33.91 s | — |
| **opt C** | **29.92 s** | **−11.8%** |
| baseline Zig | 32.81 s | — |
| opt Zig | 30.83 s | −6.0% (the C/Zig spread is n=1 noise; ignore) |

**Honest reading (corrected from an in-flight "~2×" guess that the data killed):**
~10% is the true value — free, no downside, but modest. mod-5 skips 40% of n yet
buys only ~10%, which proves **skip-fraction does not map to speedup**: the
skipped iterations were cheap; the cost lives in the *survivor* path
(`curve()` u128-multiply + leading-digit float-divide on the 60% that pass), which
mod-5 doesn't touch.

### About going further (mod-50) — deliberately NOT pursued
The full 2-digit-ending filter `{01,13,21,41,61,81}` lives mod 50 and would shrink
survivors from ~60% to ~12%. Because it attacks the *survivor* path, it would help
**more** than mod-5 — but **the magnitude is unverified, and it is NOT the cvpipe
"9×"** (that figure counts *candidates fed to primality testing*, a different
metric than this loop's runtime; conflating them is the same error as the "2×" and
"9×" guesses above). Also: `cvpipe` already applies this filter, so building it
into `palhunt` may duplicate an existing tool. Left as a noted possibility, no
number attached.

---

## Methodology notes (reusable)

- **Under CPU oversubscription, measure CPU-time (`/usr/bin/time` user+sys), not
  wall-clock.** Wall-clock is gated by core availability and hands freed cycles to
  other processes; user-time counts the work the process actually did. (Even so,
  user-time under contention is n=1-noisy from cache/TLB eviction — re-measure
  idle for real numbers.)
- **Isolate one variable** (single-core vs multi-core) before blaming codegen.
- **Resist skip-fraction→speedup intuition** — verify against the survivor path.

## Artifacts (experiment files; canonical `palhunt_gmp.c` is UNTOUCHED)
- `palhunt_gmp.zig` — faithful Zig port (native u128, GMP `@cImport`, dynamic
  thread pool).
- `palhunt_opt.c` / `palhunt_opt.zig` — the `n mod 5` optimization in each.
- Build:
  - `zig build-exe palhunt_gmp.zig -O ReleaseFast -lc -lgmp -mcpu=znver2 -femit-bin=palhunt_zig`
  - `gcc palhunt_opt.c -o palhunt_opt_c -O3 -march=znver2 -std=c99 -Wall -fopenmp -lgmp`

The mod-5 win (~10%) is available to fold into the canonical tool later if wanted;
not done now (we're finalizing the search, and this is a tool optimization, not a
change to any mathematical result).
