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
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <omp.h>
#include <gmp.h>

#define NUM_THREADS   8
#define MAX_D         37
#define CKPT_STRIDE   100000000LL

/* unsigned only as a fixed-width decimal container -- see palsplit.c */
typedef unsigned __int128 u128;

static u128 ipow10(int e) {
   u128 r = 1;
   while (e-- > 0) r *= 10;
   return r;
}

static u128 isqrt_u128(u128 v) {
   if (v == 0) return 0;
   u128 x = (u128)sqrtl((long double)v);
   if (x == 0) x = 1;
   for (int i = 0; i < 6; i++) {
      u128 y = (x + v / x) / 2;
      if (y == x) break;
      x = y;
   }
   while (x > 1 && x > v / x) x--;
   while (x + 1 <= v / (x + 1)) x++;
   return x;
}

static inline u128 curve(int64_t n) {
   u128 v = (u128)n;
   return 2 * v * v + 2 * v + 1;
}

static int is_pal(u128 x) {
   u128 r = 0, t = x;
   while (t) {
      r = r * 10 + t % 10;
      t /= 10;
   }
   return r == x;
}

static void u128_str(u128 v, char *buf) {
   char tmp[44];
   int i = 0;
   if (v == 0) {
      strcpy(buf, "0");
      return;
   }
   while (v > 0) {
      tmp[i++] = (char)('0' + (int)(v % 10));
      v /= 10;
   }
   for (int j = 0; j < i; j++) buf[j] = tmp[i - 1 - j];
   buf[i] = '\0';
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

   int64_t nmin = (int64_t)((isqrt_u128(2 * lo - 1) - 1) / 2);
   while (curve(nmin) < lo) nmin++;
   while (nmin > 0 && curve(nmin - 1) >= lo) nmin--;
   int64_t nmax = (int64_t)((isqrt_u128(2 * hi - 1) - 1) / 2);
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
         if (!is_pal(p)) continue;

         char buf[44];
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
