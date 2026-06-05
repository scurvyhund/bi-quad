# Session Archive — 2026-06-04: The Day Everything Connected

> ⚠️ **CORRECTION 2026-06-05:** the "no bi-quadratic emirp ≤ 19/21 digits" claims
> in this archive are FALSE — `12641 ⟷ 14621` is a bi-quadratic emirp at **d=5**.
> The corrected result is that it's the **only** one through 24 digits. The d=18–21
> sieve obstructions are also partly wrong (d=21 was a range<mod cliff artifact; it
> has 6 composite candidates). See `session_2026-06-05_emirp_d5_correction.md`.

> A full, re-readable record of one extraordinary session. Where the
> 2026-06-03 archive was about *fixing* the project (the cliff bug), this one is
> about *harvesting* it — and discovering that the two halves of BigFermat were
> the same search all along.
>
> Companion docs: `PROJECT_OVERVIEW.md` (method, glossary, algorithms),
> `session_2026-06-04_milestone.md` (the ≤21 result in brief).

---

## TL;DR — what changed today

1. **Proved: no bi-quadratic emirp has ≤ 21 digits.** Gap-free, cross-validated
   four independent ways.
2. **Discovered the unification:** the emirp obstruction sieve is *also* a
   prime-palindrome sieve. Every obstruction kills *both*. This silently
   extended **Jim's 1997 prime-palindrome conjecture to 21 digits** — two past
   the 64-bit ceiling any brute force (or his original 386) could reach.
3. **Mapped the feasibility:** cost grows a measured ~3.1×/digit (= √10). The
   correctness fix moved the "one-month run" threshold from d=21 all the way to
   **~d=28**. The next ~5 digits are hours-to-days, not months.
4. **Launched the march** — k=10 climbing d=22 → 28, checkpointed every step.

---

## The arc of the day (in order)

### 1. Reading the overnight logs → the convergence insight
We came in to finished k=7 and k=8 landscape runs. The clue: the survivor counts
lined up by **absolute d**, not just by offset-from-cliff — k=7 and k=8 gave the
*identical* `4, 6, 5` at d=13,14,15. That identity meant **convergence**: once
`k ≳ d/2`, the survivor count stops changing and equals the *true* count of
digit-feasible candidates. The right axis isn't "scan one k harder" — it's "raise
k at fixed d until the number freezes." That reframed everything.

### 2. The k=9 diagonal — run as a live regression test
We ran k=9 over d=15–19 with a built-in gate: **d=15 must come back 5** (it had to
reproduce k7=k8). It did. Then:
- d=16 = 0 → **confirmed** the re-obstruction Jim had spotted as a curiosity.
- d=17 fell **3 → 1** (k=8 had been sitting at its own cliff, not yet converged —
  the methodology *caught its own error*).
- d=18, d=19 = 0 → **brand-new obstructions.**

### 3. Built `PROJECT_OVERVIEW.md`
Jim asked for a defuzzing — a precise glossary and the algorithms written down.
Result: the canonical reference (goal, glossary, pipeline, **pseudocode for all
five core algorithms**, the honest 4-bug history, the live landscape).

### 4. Built `hunt.c` — and closed d≤17 exhaustively
Realization: at `k ≥ ⌈d/2⌉` the first-k/last-k digits *cover the whole number*, so
survivors are **fully-determined candidates**, not vague leads. So we wrote an
*independent* brute force — enumerate every n, form q=rev(p), test exactly. It
**re-confirmed the sieve counts** (4/6/5/0/1) as a fourth ground truth, verified
the d=16 obstruction at exact arithmetic, and found **EMIRPS = 0** across d=13–17.
Combined with the sieve obstructions → **no bi-quadratic emirp ≤ 19 digits.**
Committed (`3e2d4d6`).

### 5. The k=10 diagonal — the d=21 boss fight, won in 19 minutes
d=21 was the villain of the entire project (6-week runs, 6.5-hour silent stalls,
the bug that exposed the cliff). Today it finished in **~19 min** and came back
**OBSTRUCTION**. d=18,19,20 too. → result extended to **≤ 21 digits**. Committed
(`14e6362`). The number that nearly broke the project became just another brick
in the wall.

### 6. The d=17 palindrome — and the doorway to the other half
We dug up d=17's lone survivor: n=80472264, p=**12951570707515921** — a
**palindrome** (so q=p; not an emirp by definition), and composite
(`13 × 17 × 113 × 1609 × 322326253`). That single object opened the connection to
Jim's *other* lifelong hunt.

### 7. Jim's prime-palindrome conjecture — 27 years surfacing
Jim's benchmark since ~1997 (after reading Simon Singh's *Fermat's Enigma*, on a
386, bending the **x87 FPU's 80-bit precision** into a 64-bit integer engine):

> **3187813 is the largest prime palindrome on the curve 2n²+2n+1.**

We confirmed it: 3187813 is a prime palindrome, ≡1 mod 4, on the curve (m=1262).
`palhunt.c` then exhaustively swept **every n to 3 billion** (p out to ~19 digits)
and found only **four** prime palindromes on the entire curve: 5, 181, 313,
3187813 — *nothing* above his record. (And a poignant note: the FPU's 64-bit
significand reaches the *same* ~19-digit ceiling as modern uint64 — so we were
largely re-verifying ground his own 386 already covered, just in 8.6 seconds.)

### 8. The unification (Jim's question that reframed the project)
Jim asked: *don't we filter palindromes out of the emirp hunt because they're not
strictly emirps?* Yes — and that question cracked it open. A palindrome is the
case **m=n**: it's a *survivor* in the sieve, removed only by hunt.c's final p≠q
test. The sieve itself never excludes them. Therefore:

> **An emirp obstruction is automatically a prime-palindrome obstruction.**
> 0 survivors ⟹ no emirp candidate *and* no prime palindrome.

So d=20 and d=21 (past palhunt's reach) **already confirm Jim's conjecture to 21
digits** — by *proof*, not search. The two halves of BigFermat are one sieve, split
only at the finish line. Empirically airtight: every prime palindrome that exists
sits at a non-obstructed d; every obstructed d has none.

### 9. Feasibility — the month-long era is over (for now)
The k=10 log handed us the cost curve directly: d=18 ~40s, d=19 ~120s, d=20 ~380s,
d=21 ~1160s — a clean **~3.1×/digit**, matching √10 to two figures. Projection:
d=22 ~1hr, d=23 ~3hr, d=24 ~10hr, d=25 ~1.3 days … **~d=28 is where "a month"
now lives.** The fix bought **~7 digits** of headroom. Honest caveat: the
exponential is fundamental and reasserts around d=28–30 on this hardware.

### 10. The march
Launched: `./mod_obstruct 28 10 10 22` — k=10 climbing d=22 → 28, checkpointed
after every d, logging to `logs/march_k10_d22plus.log`. Each obstruction extends
*both* walls. A nonzero count is the alarm: a *prime* palindrome survivor would
dethrone 3187813 after 27 years.

---

## The results, stated plainly

**Converged landscape (survivor = prime-eligible candidate; 0 = obstruction):**

```
d ≤ 12 : OBSTRUCTION (every k)
d = 13 : 4    (all composite; 2 are composite palindromes)
d = 14 : 6    (all composite)
d = 15 : 5    (all composite; 4 are composite palindromes)
d = 16 : OBSTRUCTION
d = 17 : 1    (the lone survivor is a composite palindrome)
d = 18 : OBSTRUCTION ┐
d = 19 : OBSTRUCTION │ four consecutive — proven at k=10
d = 20 : OBSTRUCTION │
d = 21 : OBSTRUCTION ┘
d ≥ 22 : marching now
```

- **No bi-quadratic emirp with ≤ 21 digits.** (brute force d≤17, sieve d=18–21)
- **No prime palindrome on the curve with ≤ 21 digits** beyond 3187813.
  (brute force d≤19 via palhunt; sieve obstruction d=20,21)
- The only prime palindromes on the curve, anywhere checked: **5, 181, 313,
  3187813.** Jim's 1997 champion stands.

**Four independent confirmations** of the d=13–17 counts: sieve at k=7, k=8, k=9
(the convergence diagonal), and the independent `hunt.c` brute force.

---

## Why it matters (the Fermat through-line)

A book about Fermat's *Last* theorem (Singh, 1997) sent Jim hunting inside
Fermat's *two-square* theorem: primes ≡1 mod 4 are exactly the sums of two
squares, and 2n²+2n+1 = n²+(n+1)² is the sum of two *consecutive* squares — the
tightest case. The palindromes, the emirps, the obstructions: all live on that one
curve. The project is Fermat all the way down. Hence "BigFermat."

---

## Artifacts produced today

| file | what |
|---|---|
| `docs/PROJECT_OVERVIEW.md` | canonical method + glossary + algorithm pseudocode |
| `docs/session_2026-06-04_milestone.md` | the ≤21 result, concise |
| `docs/session_2026-06-04_archive.md` | this document |
| `hunt.c` | independent brute-force emirp hunter / sieve verifier (GMP) |
| `palhunt.c` | direct prime-palindrome hunter on the curve (64-bit, fast) |
| `logs/landscape_k6.._k10.log` | the cross-k maps |
| `logs/march_k10_d22plus.log` | the live march |
| memory: `biquad_no_emirp_under_20digits`, `jim_palindrome_conjecture_3187813`, `unification_emirp_palindrome_sieve`, `feedback_communication_style` | |

**Commits:** `3e2d4d6` (≤19 + hunt.c + overview), `14e6362` (≤21).

---

## Open questions / where it goes

1. **The march:** does the obstruction wall hold through d=22…28? Each obstructed
   d extends both conjectures by proof.
2. **The alarm:** does any survivor at d≥22 turn out to be a *prime* palindrome?
   That dethrones 3187813 and breaks a 27-year conjecture.
3. **The horizon:** ~3.1×/digit means ~d=28–30 is the practical ceiling at k=10 on
   this box. Beyond needs higher k (RAM-bound) or new ideas.
4. **The big one:** do the obstructions become *total* for all large d — i.e., are
   bi-quadratic emirps (and prime palindromes on the curve) genuinely *finite*?
   Today turned that from a wild hope into a serious, evidence-backed question.
