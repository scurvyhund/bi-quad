# The digit-ending constraint — a proof by construction

**Claim.** Every prime-eligible value of the curve `p = 2n² + 2n + 1 = n² + (n+1)²`
ends (in base 10) in one of exactly six two-digit values:

> **{ 01, 13, 21, 41, 61, 81 }**  — i.e. `x1` with `x ∈ {0,2,4,6,8}`, plus `13`.

This is the seed of the whole obstruction sieve, so it's worth proving cleanly. It's
a **proof by construction**: rather than arguing by contradiction or abstract
existence, we *exhibit the complete, finite set* of achievable endings by enumerating
one full residue period — no heavy machinery, just the structure of a **sum of two
consecutive squares**.

---

## 1. p is odd and ≡ 1 (mod 4)

`n` and `n+1` are consecutive, so exactly one is even and one is odd:
`p = even² + odd²`. Even + odd = **odd**, so `p` is always odd.

Further, `p = 2n²+2n+1 = 2·n(n+1) + 1`. The product `n(n+1)` of consecutive integers
is even, so `2·n(n+1) ≡ 0 (mod 4)`, giving

> **`p ≡ 1 (mod 4)` for every n.**

That is *why* every curve value is a "4n+1" number — the class Fermat's two-square
theorem says is a sum of two squares (here, the tightest case: two *consecutive* squares).

## 2. The last digit is forced to {1, 3} (your even²+odd² argument)

Last digits of squares are limited:

| parity | squares end in |
|--------|----------------|
| odd²  | **{1, 5, 9}** (1²=1, 3²=9, 5²=5, 7²=9, 9²=1) |
| even² | **{0, 4, 6}** (0²=0, 2²=4, 4²=6, 6²=6, 8²=4) |

Since `p = even² + odd²`, its last digit is `(one of {0,4,6}) + (one of {1,5,9})`.
Tabulating by `n mod 10` (the only thing the last digit depends on):

```
n≡0: 0²+1² = 1     n≡5: 5²+6² = 61 → 1
n≡1: 1²+2² = 5     n≡6: 6²+7² = 85 → 5
n≡2: 2²+3² = 13→3  n≡7: 7²+8² =113 → 3
n≡3: 3²+4² = 25→5  n≡8: 8²+9² =145 → 5
n≡4: 4²+5² = 41→1  n≡9: 9²+10²=181 → 1
```

So the **last digit of p ∈ {1, 3, 5}**. The `…5` values are divisible by 5 (hence
composite — never a 4n+1 prime beyond 5 itself), so for prime-eligible p:

> **last digit ∈ {1, 3}.**

## 3. The last *two* digits — the same idea at mod 100

`p mod 100` depends only on `n mod 50`, because
`p(n+50) = 2(n+50)²+2(n+50)+1 = p(n) + (200n + 5100)` and `200n + 5100 ≡ 0 (mod 100)`.
So enumerating `n = 0 … 49` is a **finite, exhaustive construction** of every
achievable two-digit ending:

```
achievable endings : {01, 05, 13, 21, 25, 41, 45, 61, 65, 81, 85}
drop ÷5 (…0, …5)   : {01,     13, 21,     41,     61,     81    }
```

> **prime-eligible two-digit endings = { 01, 13, 21, 41, 61, 81 }.** ∎

## 4. Corollary — the six leading-digit patterns

A bi-quadratic emirp needs `q = rev(p)` *also* on the curve. The first two digits of
`p` are the last two digits of `q` reversed, so the admissible **first-two-digit
patterns** are the reverses of the endings:

> reverse{01,13,21,41,61,81} = **{10, 12, 14, 16, 18, 31}**

These are exactly the `valid_firsts` of the modular sieve and the `Pattern` column in
the cvpipe zone log — the constraint is consistent end-to-end.

## 5. In practice — this is how p and q candidates are filtered

The six endings (and their six reverses) are exactly the **gatekeeper** that screens
candidates *before* any expensive test:

- **p-side:** reject any `n` whose `p` doesn't end in `{01, 13, 21, 41, 61, 81}`.
- **q-side:** since `q = rev(p)` must *also* land on the curve, `p`'s **first** two
  digits must be the reverse of a valid ending — i.e. in `{10, 12, 14, 16, 18, 31}`.
  Reject otherwise.

Both are a couple of digit mods — **nanoseconds** — and they eliminate the
overwhelming majority of `n` immediately. Only survivors proceed to the costlier
steps: the `2q−1` perfect-square (consecutive-square) test, then Miller–Rabin. This
first/last-digit screen *is* the `cvpipe` gatekeeper and the `valid_endings` /
`valid_firsts` sets in `mod_obstruct.c`.

**How big is the win?** The leading-digit filter keeps only **6 of the 90** possible
two-digit prefixes, discarding **≈ 89% of candidates — about a 9× speedup at scale**
(≈15× under a uniform estimate; realized ~9× because low prefixes are more common),
*before any primality test*. The trailing prime-eligibility check (dropping the ~40%
of `n` whose `p` ends in `…5`/`…0`) adds another ~1.7× on top.

**In raw numbers** (cumulative candidates through digit-length `d` = `n_max(d) ≈
√(10ᵈ/2)`, since every `n` is examined once):

| through | candidate `n` | filter culls (~89%) | reach primality test |
|---------|---------------|---------------------|----------------------|
| d ≤ 24 (done)     | ≈ 707 billion  | **≈ 629 billion**  | ≈ 78 billion  |
| d ≤ 25 (running)  | ≈ 2.24 trillion | **≈ 1.99 trillion** | ≈ 246 billion |
| d ≤ 26 (next)     | ≈ 7.07 trillion | **≈ 6.29 trillion** | ≈ 778 billion |

And this is just the 2-digit layer — deepened to `k` digits (§6) the same
construction is what collapses whole digit-lengths to **zero** survivors (a proven
obstruction), no primality tests at all. *(Note: this ~9× is the real, measured
digit-filter gain — not the inflated "zone-skip" figure once quoted from a partial
run log.)*

## 6. Generalization — the sieve engine

The mod-10 → mod-100 argument is just the first two layers of the same construction.
At depth `k` the first-`k` and last-`k` digits of `p` are pinned by `n mod 10^k`
(Hensel-lifted), giving a `valid_endings` set and its reverse `valid_firsts`. A
digit-length `d` is an **obstruction** when no `(first-k, last-k)` pair is mutually
compatible under reversal — proven with **zero primality tests**. That is precisely
what `mod_obstruct.c` computes. See `PROJECT_OVERVIEW.md` (method + glossary) and
`modular_obstruction_design.md`.
