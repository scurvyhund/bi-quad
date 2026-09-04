/* palbrute.c
 *
 * Exhaustive n-enumeration brute force for palindromic curve-values
 * p = 2n^2 + 2n + 1.  Independent cross-check for palsplit.c.
 *
 * Deliberately dumb: for every n in range it forms p and tests whether
 * the decimal digits read the same both ways.  The ONLY filter is
 * "first digit == last digit", a necessary condition for any
 * palindrome that assumes nothing about the curve -- so it shares no
 * logic with palsplit's residue/band construction.  A cross-check that
 * reuses the reasoning it is checking is worth nothing.
 *
 * Unlike palhunt_gmp.c this prints EVERY palindromic curve-value,
 * composite ones included.  That matters: palhunt_gmp reports only
 * GMP-certified primes, so at a digit-length with none its output is
 * indistinguishable from a brute that silently did nothing.  Printing
 * the composites gives the run a positive control.
 *
 * Range slicing and checkpointing are the point: a full d=29 sweep is
 * ~9 days, so a run must be resumable and must show progress.
 *
 * Build:
 *   gcc -O3 -march=znver2 -mtune=znver2 -std=c99 -Wall -Wextra \
 *       -fopenmp -o palbrute palbrute.c -lgmp -lm
 *
 * Usage:
 *   ./palbrute <d> [n_start] [n_end]
 *
 *   With no range, sweeps the whole d-digit window and prints the
 *   n bounds it derived.  n_start/n_end are inclusive, letting a long
 *   run be split across sessions.
 *
 * Progress goes to stderr every 100M n, with rate and ETA; results go
 * to stdout.  Redirect them separately.
 *
 * A lockfile (palbrute.lock) refuses a second concurrent instance --
 * two copies of a multi-day run on one box halve each other's speed.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <omp.h>
#include <gmp.h>

#include "curve.h"      /* u128, curve, n_at, ipow10, u128_str */

#define NUM_THREADS   8
#define MAX_D         BQ_MAX_D

/* DELIBERATE DUPLICATION -- do not replace this with is_pal_d() from
 * curve.h.  palbrute exists to cross-check palsplit, and palsplit's
 * predicate is the digit-indexed is_pal_d().  This one reverses the
 * whole integer instead: a different implementation of the same
 * mathematical test, sharing no code with the thing it checks.  Unify
 * the two and the d=13..23 corroboration becomes worthless -- it would
 * only prove that one function agrees with itself.
 * See docs/palindrome_split_search.md. */
static int is_pal_rev(u128 x) {
   u128 r = 0, t = x;
   while (t) {
      r = r * 10 + t % 10;
      t /= 10;
   }
   return r == x;
}

int main(int argc, char *argv[]) {
   if (argc < 2) {
      fprintf(stderr, "usage: %s <d> [n_start] [n_end]\n", argv[0]);
      return 1;
   }
   int d = atoi(argv[1]);
   if (d < 1 || d > MAX_D) {
      fprintf(stderr, "d must be in [1, %d] (u128 limit)\n", MAX_D);
      return 1;
   }

   int lock = open("palbrute.lock", O_CREAT | O_RDWR, 0644);
   if (lock < 0) {
      fprintf(stderr, "cannot open palbrute.lock\n");
      return 1;
   }
   if (flock(lock, LOCK_EX | LOCK_NB) != 0) {
      fprintf(stderr, "another palbrute is already running -- refusing"
              " to double up on one box\n");
      return 1;
   }

   u128 lo = ipow10(d - 1);
   u128 hi = ipow10(d);

   int64_t nmin = n_at(lo);
   while (curve(nmin) < lo) nmin++;
   while (nmin > 0 && curve(nmin - 1) >= lo) nmin--;
   int64_t nmax = n_at(hi);
   while (curve(nmax) >= hi) nmax--;
   while (curve(nmax + 1) < hi) nmax++;

   int64_t a = (argc > 2) ? strtoll(argv[2], NULL, 10) : nmin;
   int64_t b = (argc > 3) ? strtoll(argv[3], NULL, 10) : nmax;
   if (a < nmin) a = nmin;
   if (b > nmax) b = nmax;
   if (a > b) {
      fprintf(stderr, "empty range\n");
      return 1;
   }

   int64_t total = b - a + 1;
   printf("palbrute d=%d  n in [%lld, %lld]  (full window [%lld, %lld])\n",
          d, (long long)a, (long long)b, (long long)nmin, (long long)nmax);
   printf("count=%lld  threads=%d\n", (long long)total, NUM_THREADS);
   fflush(stdout);

   omp_set_num_threads(NUM_THREADS);
   double t0 = omp_get_wtime();
   long found = 0;

   /* hoisted out of the hot loop: a u128 divide per n would dominate,
    * so the leading digit comes from a long double divide instead.
    * p < 10^37 and the quotient lies in [1,10), so 64 mantissa bits
    * are far more than enough to get its integer part right. */
   long double topld = (long double)ipow10(d - 1);

#pragma omp parallel
   {
      mpz_t z;
      mpz_init(z);
#pragma omp for schedule(dynamic, 1000000)
      for (int64_t i = 0; i < total; i++) {
         if ((i & 0x7FFFFFF) == 0 && i > 0) {
#pragma omp critical
            {
               double el = omp_get_wtime() - t0;
               double frac = (double)i / (double)total;
               fprintf(stderr, "  d=%2d: %lld / %lld (%.2f%%)  %.0f n/s"
                       "  ETA %.1f h\n", d, (long long)i,
                       (long long)total, 100.0 * frac,
                       (double)i / (el > 0 ? el : 1),
                       (el / (frac > 0 ? frac : 1) - el) / 3600.0);
               fflush(stderr);
            }
         }
         int64_t n = a + i;
         u128 p = curve(n);
         int last = (int)(p % 10);
         int first = (int)((long double)p / topld);
         if (first != last) continue;
         if (!is_pal_rev(p)) continue;

         char buf[BQ_STRLEN];
         u128_str(p, buf);
         mpz_set_str(z, buf, 10);
         int pr = mpz_probab_prime_p(z, 40);
#pragma omp critical
         {
            printf("%s  n=%lld  %s\n", buf, (long long)n,
                   pr ? "PRIME" : "COMPOSITE");
            fflush(stdout);
            found++;
         }
      }
      mpz_clear(z);
   }

   double el = omp_get_wtime() - t0;
   printf("found=%ld  checked=%lld  time=%.1fs\n",
          found, (long long)total, el);
   fflush(stdout);
   flock(lock, LOCK_UN);
   close(lock);
   return 0;
}
