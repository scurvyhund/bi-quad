/* palcurve.c
 *
 * Palindromes on ANY conic curve of the family, via the same head/tail
 * split used by palsplit.c.  Generalised from p = 2n^2+2n+1 to
 * p = A*m^2 + B*m + C.
 *
 * The split works here for the same reason it works on our curve: the
 * speedup comes from the PALINDROME, not from the curve.  A d-digit
 * palindrome's high t digits are the reverse of its low t digits, so
 * enumerating r = m mod 10^t fixes both ends of p at once, and the
 * high end pins m by magnitude through the quadratic formula.  Cost is
 * O(10^(d/4)) on every curve in the table.
 *
 * Curves are recentred on m >= 0 so one enumeration covers every value
 * (the k-family's vertex sits at n = -k/2, so enumerating n >= 0 would
 * silently miss values; substituting u = 2n+k = 2m+1 fixes that):
 *
 *   k-family  p = n^2+(n+k)^2, odd k  ->  2m^2 + 2m + (1+k^2)/2
 *   cuban     Z[w]                    ->  3m^2 + 3m + 1
 *   Z[sqrt-2]                         ->  3m^2+2m+1  U  3m^2+4m+2
 *
 * Z[sqrt-2] needs two branches because its membership rule 3p-2 = a^2
 * admits a == 1 and a == 2 (mod 3); each is its own quadratic in m.
 *
 * Build:
 *   gcc -O3 -march=znver2 -mtune=znver2 -std=c99 -Wall -Wextra \
 *       -fopenmp -o palcurve palcurve.c -lgmp -lm
 *
 * Usage:
 *   ./palcurve <A> <B> <C> <d> [t] [--keep5]
 *
 * Example (our curve, d=7 -- must print 3187813 as PRIME):
 *   ./palcurve 2 2 1 7 2 --keep5
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <omp.h>
#include <gmp.h>

#include "curve.h"      /* u128, curve_abc, curve_abc_mod, m_at,
                         * digit_at, is_pal_d, rev_digits, u128_str */

#define NUM_THREADS   8
/* Tool-specific cap, NOT the pow10 table extent (BQ_MAX_D = 37).
 * The hard constraint is that 4*A*p inside m_at stay in a u128; for
 * A <= 3 that binds only at p < 2.83e37, i.e. d <= 37.  33 is a
 * CONSERVATIVE cap with ~4 digits of unused headroom, kept because no
 * palcurve result past d = 31 has ever been generated or checked.
 * m_at was measured exact at the top of every d from 30 to 37 for
 * A = 1,2,3 (test_curve.c covers the d = 33 edge).  Raising this is
 * safe arithmetically but puts out numbers nothing corroborates --
 * treat it as a deliberate decision, not a free win. */
#define MAX_D         33
#define MAX_T         13
#define MAX_HITS      512
#define BAND_GUARD    1000

/* the curve under search, p = g_a*m^2 + g_b*m + g_c */
static int64_t g_a, g_b, g_c;

int main(int argc, char *argv[]) {
   if (argc < 5) {
      fprintf(stderr, "usage: %s <A> <B> <C> <d> [t] [--keep5]\n",
              argv[0]);
      return 1;
   }
   g_a = strtoll(argv[1], NULL, 10);
   g_b = strtoll(argv[2], NULL, 10);
   g_c = strtoll(argv[3], NULL, 10);
   int d = atoi(argv[4]);
   int t = 0, keep5 = 0;
   for (int i = 5; i < argc; i++) {
      if (strcmp(argv[i], "--keep5") == 0) keep5 = 1;
      else t = atoi(argv[i]);
   }
   if (t == 0) t = d / 4;
   if (t < 1) t = 1;

   if (g_a < 1 || g_a > 3) {
      fprintf(stderr, "A must be in [1,3] (u128 headroom)\n");
      return 1;
   }
   if (d < 1 || d > MAX_D) {
      fprintf(stderr, "d must be in [1, %d]\n", MAX_D);
      return 1;
   }
   if (t > MAX_T || t >= d) {
      fprintf(stderr, "t must be in [1, min(%d, d-1)]\n", MAX_T);
      return 1;
   }

   init_pow10();
   omp_set_num_threads(NUM_THREADS);

   int64_t mod = (int64_t)bq_pow10[t];
   u128 step = bq_pow10[d - t];

   u128 hit_p[MAX_HITS];
   int64_t hit_m[MAX_HITS];
   int nhits = 0;

#pragma omp parallel for schedule(dynamic, 4096)
   for (int64_t r = 0; r < mod; r++) {
      int64_t low = curve_abc_mod(r, mod, g_a, g_b, g_c);
      if (!keep5 && low % 5 == 0) continue;
      if (low % 10 == 0) continue;
      int64_t high = rev_digits(low, t);

      u128 plo = (u128)high * step;
      u128 phi = plo + step;
      int64_t mlo = m_at(plo, g_a, g_b, g_c) - BAND_GUARD;
      int64_t mhi = m_at(phi, g_a, g_b, g_c) + BAND_GUARD;
      if (mlo < 0) mlo = 0;

      int64_t delta = ((r - mlo) % mod + mod) % mod;
      for (int64_t m = mlo + delta; m <= mhi; m += mod) {
         u128 p = curve_abc(m, g_a, g_b, g_c);
         if (p < bq_pow10[d - 1] || p >= bq_pow10[d]) continue;
         if (2 * t < d && digit_at(p, t) != digit_at(p, d - 1 - t))
            continue;
         if (!is_pal_d(p, d)) continue;
#pragma omp critical
         {
            int dup = 0;
            for (int i = 0; i < nhits; i++)
               if (hit_m[i] == m) dup = 1;
            if (!dup && nhits >= MAX_HITS) {
               fprintf(stderr, "FATAL: MAX_HITS exceeded\n");
               exit(2);
            }
            if (!dup) {
               hit_p[nhits] = p;
               hit_m[nhits] = m;
               nhits++;
            }
         }
      }
   }

   for (int i = 1; i < nhits; i++) {
      u128 kp = hit_p[i];
      int64_t km = hit_m[i];
      int j = i - 1;
      while (j >= 0 && hit_p[j] > kp) {
         hit_p[j + 1] = hit_p[j];
         hit_m[j + 1] = hit_m[j];
         j--;
      }
      hit_p[j + 1] = kp;
      hit_m[j + 1] = km;
   }

   mpz_t z;
   mpz_init(z);
   for (int i = 0; i < nhits; i++) {
      char buf[BQ_STRLEN];
      u128_str(hit_p[i], buf);
      mpz_set_str(z, buf, 10);
      printf("%s  m=%lld  %s\n", buf, (long long)hit_m[i],
             mpz_probab_prime_p(z, 40) ? "PRIME" : "COMPOSITE");
   }
   mpz_clear(z);
   fflush(stdout);
   return 0;
}
