/*
 * palhunt.c -- Find PRIME PALINDROMES on the curve p = 2n^2+2n+1 (= n^2+(n+1)^2).
 *
 * Every such p is == 1 (mod 4) -- a "4n+1 prime". This enumerates n, forms p,
 * keeps the ones whose decimal digits are a palindrome, and tests primality
 * (deterministic Miller-Rabin, exact for all 64-bit p). Reports every hit.
 *
 * 64-bit: p < 2^64 requires n < ~3.03e9 (p up to ~19-20 digits).
 *
 * Build: gcc palhunt.c -o palhunt -O3 -march=znver2 -std=c99 -Wall -fopenmp
 * Usage: ./palhunt [max_n]      (default 3000000000)
 */
/*
 * STATUS 2026-09-05 -- FROZEN.  DO NOT CONVERGE ONTO curve.h.
 *
 * This file is in NO Makefile target (not TARGET, PALS, CHECKS, TESTS,
 * nor clean).  It predates curve.h: last touched 2026-06-23 by the
 * mass re-indent c248dd9, while curve.h was created 2026-09-03 in
 * 20300fb.  It therefore still carries its own ipow10 / isqrt128 /
 * curve / is_pal, which have DIVERGED from the shared versions --
 * curve.h seeds isqrt with sqrtl and refines by Newton, this file
 * binary-searches; curve.h takes int64_t n, this file takes u128.
 *
 * That divergence is not a defect to clean up.  palhunt.c is the 64-bit
 * ancestor of the palindrome line, kept as the record of what the
 * pre-u128 search actually ran.
 * Rewriting this onto the shared header would retroactively hollow out
 * that corroboration: an independent check stops being independent the
 * moment it shares an implementation with the thing it checks.  Same
 * argument as leaving De Geest's transcribed table unedited --
 * see docs/palindrome_split_search.md section 7.
 *
 * If you need these primitives in NEW code, include curve.h.  If you
 * need to re-run this tool, rebuild from the Build line above and
 * re-run its cross-check; do not "modernise" it first.
 *
 * AMENDED 2026-11-19 (bounds audit, docs/audit_2026-11.md) -- the ONLY
 * change since the freeze: max_n is parsed strictly and refused past
 * MAX_N_U64 = 3037000499, the largest n with 2n^2+2n+1 < 2^64.  Past
 * it p wrapped silently and the wrapped values were tested as if real.
 * Shares no code with curve.h; the search itself is untouched, and
 * output for every valid max_n is byte-identical.
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <omp.h>

#define NUM_THREADS 8
#define MAX_N_U64   3037000499ULL   /* largest n: 2n^2+2n+1 < 2^64 */

/* 64-bit modular multiply via 128-bit */
static inline uint64_t mulmod(uint64_t a, uint64_t b, uint64_t m) {
   return (uint64_t)(((unsigned __int128)a * b) % m);
}
static uint64_t powmod(uint64_t a, uint64_t e, uint64_t m) {
   uint64_t r = 1; a %= m;
   while (e) { if (e & 1) r = mulmod(r, a, m); a = mulmod(a, a, m); e >>= 1; }
   return r;
}
/* deterministic Miller-Rabin, exact for all n < 2^64 */
static bool is_prime(uint64_t n) {
   if (n < 2) return false;
   for (uint64_t p = 2; p < 40; p++) {
      if (p*p > n) return true;
      if (n % p == 0) return n == p;
   }
   uint64_t d = n - 1; int r = 0;
   while ((d & 1) == 0) { d >>= 1; r++; }
   static const uint64_t bases[] = {2,3,5,7,11,13,17,19,23,29,31,37};
   for (int i = 0; i < 12; i++) {
      uint64_t a = bases[i] % n;
      if (a == 0) continue;
      uint64_t x = powmod(a, d, n);
      if (x == 1 || x == n - 1) continue;
      bool comp = true;
      for (int j = 0; j < r - 1; j++) {
         x = mulmod(x, x, n);
         if (x == n - 1) { comp = false; break; }
      }
      if (comp) return false;
   }
   return true;
}

static inline bool is_palindrome(uint64_t x) {
   uint64_t r = 0, t = x;
   while (t) { r = r * 10 + t % 10; t /= 10; }
   return r == x;
}

int main(int argc, char *argv[]) {
   uint64_t max_n = 3000000000ULL;
   if (argc > 2) {
      fprintf(stderr, "usage: %s [max_n]   (max_n <= %llu)\n", argv[0],
              (unsigned long long)MAX_N_U64);
      return 1;
   }
   if (argc > 1) {
      char *end;
      errno = 0;
      unsigned long long v = strtoull(argv[1], &end, 10);
      if (errno || end == argv[1] || *end != '\0' || argv[1][0] == '-'
            || v < 1 || v > MAX_N_U64) {
         fprintf(stderr, "max_n must be an integer in [1, %llu]: 2n^2+2n+1"
                 " must fit 64 bits\n", (unsigned long long)MAX_N_U64);
         return 1;
      }
      max_n = v;
   }

   printf("\n  PRIME PALINDROMES on p = 2n^2+2n+1  (n <= %llu)\n",
         (unsigned long long)max_n);
   printf("  ===============================================\n\n");
   fflush(stdout);

   double t0 = omp_get_wtime();
   long count = 0;

   #pragma omp parallel for schedule(dynamic, 1000000) num_threads(NUM_THREADS)
   for (uint64_t n = 1; n <= max_n; n++) {
      uint64_t p = 2*n*n + 2*n + 1;       /* fits in u64 for n < ~3.03e9 */
      if (!is_palindrome(p)) continue;
      if (!is_prime(p)) continue;
      #pragma omp atomic
      count++;
      #pragma omp critical
      {
         int d = 0; uint64_t t = p; while (t) { d++; t /= 10; }
         printf("  n=%-11llu  p=%llu  (%d digits)\n",
               (unsigned long long)n, (unsigned long long)p, d);
         fflush(stdout);
      }
   }

   printf("\n  found %ld prime palindromes on the curve  [%.1fs]\n\n",
         count, omp_get_wtime() - t0);
   return 0;
}
