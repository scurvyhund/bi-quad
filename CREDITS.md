# Credits — third-party tools

## C-Quadratic-Sieve (SIQS) — Michel Léonard

- **Project:** C-Quadratic-Sieve — a Self-Initializing Quadratic Sieve
  (SIQS) integer factorizer written in C.
- **Author:** Michel Léonard.
- **Repository:** https://github.com/michel-leonard/C-Quadratic-Sieve
- **License:** released into the public domain by the author ("as is,"
  no warranty). Stated in the project README; there is no separate
  LICENSE file, so this is an informal public-domain dedication. No
  attribution is legally required — this credit is given by choice.

### How we use it

We use `qs` to **factor** large candidate values `p = 2n²+2n+1` from the
hunt. Specifically, at d = 27 the emirp brute (`hunt.c`) flagged three
palindromic curve-values that passed the composite sieve; `qs` factored
all three, proving each **composite** (see `pals.txt`). This is the
deciding primality step the fast engines cannot supply on their own.

### Why we trust the result

A quadratic sieve's **composite** verdict is self-certifying: it returns
the actual factors, and `factor₁ × factor₂ = p` with both factors > 1 is
a checkable **certificate of compositeness** — independently verifiable
with any big-integer multiply, no trust in the sieve's internals needed.
(This is categorically stronger than a probabilistic "probably prime.")

The tool itself is also high quality: self-contained C99 with no external
dependencies, built from the author's own separately-tested primitives
(a `cint` big-integer library and an AVL-tree module), CI-tested across
size tiers, Valgrind-clean, and capable of RSA-100-class inputs — far
beyond the 27-digit numbers we hand it. Longstanding, actively maintained.

### Attribution notes

- Michel Léonard's `qs` has been used and updated over several years in
  this project's workflow; this file records the debt explicitly.
- If a formal citation is ever wanted, cite: Michel Léonard,
  *C-Quadratic-Sieve* (SIQS), public domain,
  https://github.com/michel-leonard/C-Quadratic-Sieve.
