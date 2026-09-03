/* palsplit.c
 *
 * Head/tail split search for palindromic curve-values
 * p = 2n^2 + 2n + 1 = n^2 + (n+1)^2.
 *
 * A d-digit palindrome's high t digits are the reverse of its low t
 * digits.  Enumerating r = n mod 10^t therefore fixes BOTH ends of p
 * at once: the low t digits directly (p = f(r) mod 10^t), and the
 * high t digits as their reverse.  The high digits in turn pin n to a
 * narrow band via n ~ sqrt(p/2).  Intersecting "n in band" with
 * "n = r (mod 10^t)" leaves ~10^(d/2 - 2t) candidates per residue over
 * 10^t residues; balanced at t ~ d/4 this is O(10^(d/4)), against
 * O(10^(d/2)) for straight n-enumeration (hunt.c, palhunt_gmp).
 *
 * This works because a palindrome's head is DETERMINED by its tail.
 * It does NOT carry over to the emirp search, where the head and tail
 * of p are independent choices -- see docs/palindrome_split_search.md.
 *
 * Correctness does not depend on t: p_i = p_(d-1-i) holds for every
 * t <= d, so every t returns the same set.  t only sets the speed.
 * Running two different t is therefore a free cross-check, and is
 * MANDATORY before believing any result past the d=27 frontier.
 *
 * Build:
 *   gcc -O3 -march=znver2 -mtune=znver2 -std=c99 -Wall -Wextra \
 *       -fopenmp -o palsplit palsplit.c -lgmp -lm
 *
 * Usage:
 *   ./palsplit <d> [t] [--keep5]
 *
 *     d        decimal digit length.  Odd d only in practice: curve
 *              values are never 0 mod 11, and every even-digit
 *              palindrome is divisible by 11, so no even d has a
 *              curve palindrome of any kind.
 *     t        split width (default d/4, the balance point).
 *     --keep5  keep palindromes divisible by 5.  These cannot be
 *              prime, but the pals_d*.txt regression sets list some
 *              of them, so the regression needs this flag.
 *
 * Example:
 *   $ ./palsplit 27 7 --keep5
 *   d=27 t=7 keep5=1 outer=10^7
 *   108491007414868414700194801  n=7365154696775   COMPOSITE
 *   318216440234333432044612813  n=12613810689762  COMPOSITE
 *   318288756988131889657882813  n=12615243893562  COMPOSITE
 *   found=3
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <float.h>
#include <omp.h>
#include <gmp.h>

#define NUM_THREADS   8
#define MAX_D         37        /* p < 10^37 keeps 2p inside u128 */
#define MAX_T         13
#define MAX_HITS      256
#define CKPT_MASK     0x3FFFFFF /* checkpoint every ~67.1M residues */
#define BAND_GUARD    1000      /* see docs/palindrome_split_search.md */

/* The band edge is computed EXACTLY (isqrt_u128); long double is only
 * a Newton seed, so mantissa width is not load-bearing.  An earlier
 * version estimated the edge in long double and needed an 80-bit
 * mantissa to be safe -- see docs/palindrome_split_search.md sec. 4. */

/* unsigned only for the fixed-width decimal container: no arithmetic
 * here can go negative, and we need the full 128-bit range. */
typedef unsigned __int128 u128;

static u128 pow10u[MAX_D + 2];

static void init_pow10(void) {
   pow10u[0] = 1;
   for (int i = 1; i <= MAX_D + 1; i++) {
      pow10u[i] = pow10u[i - 1] * 10;
   }
}

static inline u128 curve(int64_t n) {
   u128 v = (u128)n;
   return 2 * v * v + 2 * v + 1;
}

static inline int64_t curve_mod(int64_t r, int64_t m) {
   u128 v = (u128)r;
   return (int64_t)((2 * v * v + 2 * v + 1) % (u128)m);
}

static inline int digit_at(u128 p, int i) {
   return (int)((p / pow10u[i]) % 10);
}

static int is_pal(u128 p, int d) {
   for (int i = 0; i < d / 2; i++) {
      if (digit_at(p, i) != digit_at(p, d - 1 - i)) return 0;
   }
   return 1;
}

/* Exact integer sqrt of a u128.  Seeded from long double, then Newton,
 * then a final exact adjustment -- so the result is exact regardless of
 * how good the seed was.  Comparisons use division, never x*x, which
 * would overflow near the top of the range.  This removes floating
 * point from the band edge entirely: the band is exact, not estimated. */
static inline u128 isqrt_u128(u128 v) {
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

static inline int64_t rev_digits(int64_t v, int t) {
   int64_t h = 0;
   for (int i = 0; i < t; i++) {
      h = h * 10 + v % 10;
      v /= 10;
   }
   return h;
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
      fprintf(stderr, "usage: %s <d> [t] [--keep5]\n", argv[0]);
      return 1;
   }

   int d = atoi(argv[1]);
   int t = 0;
   int keep5 = 0;

   for (int i = 2; i < argc; i++) {
      if (strcmp(argv[i], "--keep5") == 0) keep5 = 1;
      else t = atoi(argv[i]);
   }
   if (t == 0) t = d / 4;
   if (t < 1) t = 1;

   if (d < 1 || d > MAX_D) {
      fprintf(stderr, "d must be in [1, %d] (u128 limit)\n", MAX_D);
      return 1;
   }
   if (t > MAX_T || t >= d) {
      fprintf(stderr, "t must be in [1, min(%d, d-1)]\n", MAX_T);
      return 1;
   }

   init_pow10();
   omp_set_num_threads(NUM_THREADS);

   int64_t mod = (int64_t)pow10u[t];
   u128 step = pow10u[d - t];

   u128 hit_p[MAX_HITS];
   int64_t hit_n[MAX_HITS];
   int nhits = 0;

   printf("d=%d t=%d keep5=%d outer=10^%d threads=%d\n",
          d, t, keep5, t, NUM_THREADS);
   fflush(stdout);

   double t0 = omp_get_wtime();

#pragma omp parallel for schedule(dynamic, 4096)
   for (int64_t r = 0; r < mod; r++) {
      if ((r & CKPT_MASK) == 0 && r > 0) {
#pragma omp critical
         {
            fprintf(stderr, "  d=%2d: residue %lld / %lld\n",
                    d, (long long)r, (long long)mod);
            fflush(stderr);
         }
      }

      int64_t low = curve_mod(r, mod);        /* low t digits of p */
      if (!keep5 && low % 5 == 0) continue;   /* div-5: cannot be prime */
      if (low % 10 == 0) continue;            /* leading digit is 0 */

      int64_t high = rev_digits(low, t);      /* high t digits of p */

      /* exact: high < 10^t and step = 10^(d-t), so plo < 10^d */
      u128 plo = (u128)high * step;
      u128 phi = plo + step;
      int64_t nlo = (int64_t)((isqrt_u128(2 * plo - 1) - 1) / 2)
                    - BAND_GUARD;
      int64_t nhi = (int64_t)((isqrt_u128(2 * phi - 1) - 1) / 2)
                    + BAND_GUARD;
      if (nlo < 0) nlo = 0;

      int64_t delta = ((r - nlo) % mod + mod) % mod;

      for (int64_t n = nlo + delta; n <= nhi; n += mod) {
         u128 p = curve(n);
         if (p < pow10u[d - 1] || p >= pow10u[d]) continue;
         if (2 * t < d && digit_at(p, t) != digit_at(p, d - 1 - t))
            continue;
         if (!is_pal(p, d)) continue;
#pragma omp critical
         {
            int dup = 0;
            for (int i = 0; i < nhits; i++)
               if (hit_n[i] == n) dup = 1;
            if (!dup && nhits >= MAX_HITS) {
               fprintf(stderr, "FATAL: MAX_HITS (%d) exceeded at d=%d;"
                       " result is truncated\n", MAX_HITS, d);
               fflush(stderr);
               exit(2);
            }
            if (!dup) {
               hit_p[nhits] = p;
               hit_n[nhits] = n;
               nhits++;
            }
         }
      }
   }

   double el = omp_get_wtime() - t0;

   /* insertion sort -- nhits is tiny */
   for (int i = 1; i < nhits; i++) {
      u128 kp = hit_p[i];
      int64_t kn = hit_n[i];
      int j = i - 1;
      while (j >= 0 && hit_p[j] > kp) {
         hit_p[j + 1] = hit_p[j];
         hit_n[j + 1] = hit_n[j];
         j--;
      }
      hit_p[j + 1] = kp;
      hit_n[j + 1] = kn;
   }

   mpz_t z;
   mpz_init(z);
   for (int i = 0; i < nhits; i++) {
      char buf[44];
      u128_str(hit_p[i], buf);
      mpz_set_str(z, buf, 10);
      int pr = mpz_probab_prime_p(z, 40);
      printf("%s  n=%lld  %s\n", buf, (long long)hit_n[i],
             pr ? "PRIME" : "COMPOSITE");
   }
   mpz_clear(z);

   printf("found=%d  time=%.2fs\n", nhits, el);
   fflush(stdout);
   return 0;
}
