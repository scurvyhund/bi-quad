/* palbrute.c
 *
 * Exhaustive n-enumeration brute force for palindromic curve-values
 * p = 2n^2 + 2n + 1.  Independent cross-check for palsplit.c.
 *
 * Deliberately dumb: for every n it forms p and tests whether the
 * decimal digits read the same both ways.  It shares no residue/band
 * construction with palsplit -- a cross-check that reuses the
 * reasoning it is checking is worth nothing.
 *
 * Unlike palhunt_gmp.c this prints EVERY palindromic curve-value,
 * composite ones included.  That matters: palhunt_gmp reports only
 * GMP-certified primes, so at a digit-length with none its output is
 * indistinguishable from a brute that silently did nothing.  Printing
 * the composites gives the run a positive control.
 *
 * ---------------------------------------------------------------
 * THE LEADING-DIGIT ZONES  (2026-09-03)
 *
 * A palindrome's first digit equals its last.  Curve values end ONLY
 * in 1, 3 or 5 (p = 2m(m+1)+1 and m(m+1) mod 10 is 0, 2 or 6).  So a
 * palindromic curve value must also START with 1, 3 or 5, and the
 * n-ranges where p starts with 2, 4, 6, 7, 8 or 9 -- 59% of every
 * sweep -- are PROVABLY EMPTY and are not visited at all.
 *
 * This is exact, not lossy.  Contrast hunt.c's VALID_NMOD skip
 * (docs/skip_optimization.md), which destroys counts by dropping
 * div-5 palindromes.  This drops only n that cannot yield a
 * palindrome under any circumstances; div-5 palindromes, which start
 * and end in 5, live in the lead-5 zone and are still found.
 *
 * It also removes a real bug.  The previous version derived the
 * leading digit as (int)((long double)p / topld), commented "far more
 * than enough" -- it is not.  A 64-bit mantissa cannot hold p past
 * ~d=19, and at d=37 the quotient for
 *   n = 1732050807568877293,
 *   p = 5999999999999999999809846168119770285
 * rounds from 5.99999... up to exactly 6.0, so the value was silently
 * skipped although its true first and last digits are both 5.  The
 * zone bounds below are computed with exact u128 arithmetic and no
 * leading digit is derived at run time -- inside a zone it is known.
 * Same failure mode as the palsplit band edge; see
 * docs/palindrome_split_search.md sec. 4.
 *
 * The zones are worth only about 1.15x in wall-clock, NOT the 2.4x a
 * naive "we visit 41.4% of n" argument gives.  That argument assumes
 * uniform cost per n; it is not.  The dominant cost is is_pal_rev(),
 * the full digit reversal, which runs whenever first==last -- and that
 * count is UNCHANGED: 0.1409 of the range before, 0.1412 after.  The
 * zones skip 59% of the cheap work (one multiply, one mod) and none of
 * the expensive work.  Measured at d=19: 4.54s old, 3.95s new.
 *
 * So the correctness fix above is the point of this change; the speed
 * is a bonus.  What the zones DO buy is the ability to run one zone in
 * isolation -- e.g. lead-1 only at d=29, ~4.7 days, which contains
 * both known d=29 palindromes.
 *
 * --all disables the zones and sweeps every n, same output.  Kept so
 * the optimisation can be shown output-neutral.
 * ---------------------------------------------------------------
 *
 * CHECKPOINTING.  A d=29 lead-1 sweep is ~4.7 days, so progress is
 * saved after every block to palbrute_d<NN>.ckpt -- written to .tmp
 * then renamed, so a kill mid-write cannot corrupt it -- and resumed
 * automatically on restart.  Deleted on clean completion.  Same
 * pattern as hunt.c.
 *
 * Build:
 *   gcc -O3 -march=znver2 -mtune=znver2 -std=c99 -Wall -Wextra \
 *       -fopenmp -o palbrute palbrute.c -lgmp -lm
 *
 * Usage:
 *   ./palbrute <d> [n_start] [n_end] [--all]
 *
 *   With no range, sweeps the whole d-digit window.  n_start/n_end are
 *   inclusive and intersect the zones.  Progress and ETA go to stderr,
 *   results to stdout; redirect them separately.
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

#include "curve.h"      /* u128, curve, n_at, bq_pow10, u128_str */

#define NUM_THREADS   8
#define MAX_D         BQ_MAX_D
#define BLOCK_SIZE    5000000000LL   /* checkpoint unit */
#define NZONE         3

/* the only leading digits a curve palindrome can have */
static const int LEAD[NZONE] = {1, 3, 5};

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

/* least n with curve(n) >= target.  Seeded by n_at, then walked to the
 * true edge, so the result does not depend on the seed being right. */
static int64_t first_n_at_least(u128 target) {
   int64_t n = n_at(target);
   if (n < 0) n = 0;
   while (curve(n) < target) n++;
   while (n > 0 && curve(n - 1) >= target) n--;
   return n;
}

/* atomic checkpoint: write to .tmp, then rename over the target */
static void write_ckpt(const char *path, int zi, int64_t nnext,
                       long found, long long visited) {
   char tmp[80];
   snprintf(tmp, sizeof(tmp), "%s.tmp", path);
   FILE *f = fopen(tmp, "w");
   if (!f) {
      fprintf(stderr, "  WARNING: cannot write ckpt %s\n", tmp);
      return;
   }
   fprintf(f, "%d %lld %ld %lld\n", zi, (long long)nnext, found,
           visited);
   fclose(f);
   rename(tmp, path);
}

int main(int argc, char *argv[]) {
   if (argc < 2) {
      fprintf(stderr, "usage: %s <d> [n_start] [n_end] [--all]\n",
              argv[0]);
      return 1;
   }
   int d = atoi(argv[1]);
   int all = 0;
   int64_t argn[2];
   int nargn = 0;
   for (int i = 2; i < argc; i++) {
      if (strcmp(argv[i], "--all") == 0) all = 1;
      else if (nargn < 2) argn[nargn++] = strtoll(argv[i], NULL, 10);
   }
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

   init_pow10();

   u128 lo = bq_pow10[d - 1];
   u128 hi = bq_pow10[d];
   int64_t nmin = first_n_at_least(lo);
   int64_t nmax = first_n_at_least(hi) - 1;

   int64_t a = (nargn > 0) ? argn[0] : nmin;
   int64_t b = (nargn > 1) ? argn[1] : nmax;
   if (a < nmin) a = nmin;
   if (b > nmax) b = nmax;
   if (a > b) {
      fprintf(stderr, "empty range\n");
      return 1;
   }

   /* the searchable zones: p must start with LEAD[k] */
   int64_t zlo[NZONE], zhi[NZONE];
   int nz = 0;
   if (all) {
      zlo[0] = a;
      zhi[0] = b;
      nz = 1;
   } else {
      for (int k = 0; k < NZONE; k++) {
         int64_t z0 = first_n_at_least((u128)LEAD[k] * lo);
         int64_t z1 = first_n_at_least((u128)(LEAD[k] + 1) * lo) - 1;
         if (z0 < a) z0 = a;
         if (z1 > b) z1 = b;
         if (z0 > z1) continue;
         zlo[nz] = z0;
         zhi[nz] = z1;
         nz++;
      }
   }

   int64_t window = b - a + 1;
   int64_t visit = 0;
   for (int k = 0; k < nz; k++) visit += zhi[k] - zlo[k] + 1;

   char ckpt[64];
   snprintf(ckpt, sizeof(ckpt), "palbrute_d%02d.ckpt", d);

   int start_z = 0;
   int64_t start_n = nz ? zlo[0] : 0;
   long found = 0;
   long long visited = 0;
   {
      FILE *cf = fopen(ckpt, "r");
      if (cf) {
         int czi;
         long long cn, cv;
         long cfound;
         if (fscanf(cf, "%d %lld %ld %lld", &czi, &cn, &cfound, &cv)
                == 4
               && czi >= 0 && czi < nz
               && cn >= zlo[czi] && cn <= zhi[czi] + 1) {
            start_z = czi;
            start_n = (int64_t)cn;
            found = cfound;
            visited = cv;
            printf("resuming: zone %d n=%lld  found=%ld so far\n",
                   czi, (long long)cn, found);
         }
         fclose(cf);
      }
   }

   printf("palbrute d=%d  n in [%lld, %lld]\n",
          d, (long long)a, (long long)b);
   if (all) {
      printf("mode=all  window=%lld  threads=%d\n",
             (long long)window, NUM_THREADS);
   } else {
      printf("zones (a curve palindrome must start with 1, 3 or 5):\n");
      for (int k = 0; k < nz; k++) {
         printf("  lead %d  n in [%lld, %lld]  %lld values\n",
                LEAD[k], (long long)zlo[k], (long long)zhi[k],
                (long long)(zhi[k] - zlo[k] + 1));
      }
      printf("window=%lld  visiting=%lld (%.1f%%)  threads=%d\n",
             (long long)window, (long long)visit,
             100.0 * (double)visit / (double)window, NUM_THREADS);
   }
   fflush(stdout);

   omp_set_num_threads(NUM_THREADS);
   double t0 = omp_get_wtime();

   for (int k = start_z; k < nz; k++) {
      int want = all ? -1 : LEAD[k];
      int64_t n0 = (k == start_z) ? start_n : zlo[k];

      for (int64_t blk = n0; blk <= zhi[k]; blk += BLOCK_SIZE) {
         int64_t blk_end = blk + BLOCK_SIZE - 1;
         if (blk_end > zhi[k]) blk_end = zhi[k];

#pragma omp parallel
         {
            mpz_t z;
            mpz_init(z);
#pragma omp for schedule(dynamic, 1000000)
            for (int64_t n = blk; n <= blk_end; n++) {
               u128 p = curve(n);
               int last = (int)(p % 10);
               /* inside a zone the leading digit is KNOWN, so this is
                * the whole first==last test -- and it is exact */
               if (want >= 0) {
                  if (last != want) continue;
               } else {
                  if (last != (int)(p / bq_pow10[d - 1])) continue;
               }
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

         visited += blk_end - blk + 1;
         write_ckpt(ckpt, k, blk_end + 1, found, visited);

         double el = omp_get_wtime() - t0;
         double frac = (double)visited / (double)visit;
         fprintf(stderr, "  d=%2d z%d: %lld / %lld (%.2f%%)  %.0f n/s"
                 "  ETA %.1f h\n", d, k, (long long)visited,
                 (long long)visit, 100.0 * frac,
                 (double)visited / (el > 0 ? el : 1),
                 (el / (frac > 0 ? frac : 1) - el) / 3600.0);
         fflush(stderr);
      }
   }

   double el = omp_get_wtime() - t0;
   printf("found=%ld  visited=%lld  window=%lld  time=%.1fs\n",
          found, (long long)visited, (long long)window, el);
   if (!all) {
      printf("skipped=%lld  (provably palindrome-free: p would start"
             " with 2,4,6,7,8 or 9)\n",
             (long long)(window - visit));
   }
   fflush(stdout);

   remove(ckpt);
   flock(lock, LOCK_UN);
   close(lock);
   return 0;
}
