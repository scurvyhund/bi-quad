/* test_palpred.c
 *
 * Proves palbrute's fast palindrome predicate is equivalent to the
 * dumb one it replaced, before a multi-day run is trusted to it.
 *
 * is_pal_rev  -- reverses all d digits, no early exit.  The original.
 * is_pal_fast -- compares digit pairs from the outside in and returns
 *                on the first mismatch.  ~7x faster at d=29.
 *
 * Random curve values are almost never palindromes, so a test that
 * only fed it real data would prove nothing about the YES path.  This
 * drives all three cases at every d from 1 to 37: constructed
 * palindromes (must be YES), palindromes with one digit corrupted
 * (must be NO), and uniform random d-digit values (must agree).
 *
 * Build:
 *   gcc -O2 -std=c99 -Wall -Wextra -o test_palpred test_palpred.c -lm
 *
 * Usage:
 *   ./test_palpred        (exit 0 = equivalent everywhere tested)
 */

#include <stdio.h>
#include <stdint.h>

#include "curve.h"

/* ---- the two predicates, verbatim from palbrute.c ---------------- */

static int is_pal_rev(u128 x) {
   u128 r = 0, t = x;
   while (t) {
      r = r * 10 + t % 10;
      t /= 10;
   }
   return r == x;
}

static int is_pal_fast(u128 x, int d) {
   int h = d / 2;
   if (h == 0) return 1;
   uint64_t lo = (uint64_t)(x % bq_pow10[h]);
   uint64_t hi = (uint64_t)(x / bq_pow10[d - h]);
   uint64_t t = (uint64_t)bq_pow10[h - 1];
   for (int i = 0; i < h; i++) {
      if (lo % 10 != hi / t) return 0;
      lo /= 10;
      hi %= t;
      t /= 10;
   }
   return 1;
}

/* ---- driver ------------------------------------------------------ */

static uint64_t rs = 88172645463325252ULL;

static uint64_t rnd(void) {
   rs ^= rs << 13;
   rs ^= rs >> 7;
   rs ^= rs << 17;
   return rs;
}

static long fails = 0;
static long long checked = 0;

static void check(u128 x, int d, const char *what) {
   int f = is_pal_fast(x, d);
   int r = is_pal_rev(x);
   checked++;
   if (f != r) {
      char b[BQ_STRLEN];
      u128_str(x, b);
      printf("  FAIL d=%d %s: fast=%d rev=%d  x=%s\n", d, what, f, r, b);
      fails++;
   }
}

/* build a d-digit palindrome from random digits */
static u128 make_pal(int d) {
   int dig[40];
   for (int i = 0; i < (d + 1) / 2; i++) {
      dig[i] = (int)(rnd() % 10);
      if (i == 0 && dig[i] == 0) dig[i] = 1 + (int)(rnd() % 9);
   }
   for (int i = 0; i < d / 2; i++) dig[d - 1 - i] = dig[i];
   u128 v = 0;
   for (int i = 0; i < d; i++) v = v * 10 + (u128)dig[i];
   return v;
}

int main(void) {
   init_pow10();

   for (int d = 1; d <= BQ_MAX_D; d++) {
      /* 1. genuine palindromes -- exercises the YES path */
      for (int k = 0; k < 20000; k++) check(make_pal(d), d, "pal");

      /* 2. one digit corrupted -- exercises the NO path, and the
       * near-misses are where an off-by-one in the pairing shows */
      for (int k = 0; k < 20000; k++) {
         u128 p = make_pal(d);
         int pos = (int)(rnd() % (unsigned)d);
         int delta = 1 + (int)(rnd() % 9);
         int old = (int)((p / bq_pow10[pos]) % 10);
         int nw = (old + delta) % 10;
         if (pos == d - 1 && nw == 0) continue;   /* keep d digits */
         p = p + (u128)nw * bq_pow10[pos]
               - (u128)old * bq_pow10[pos];
         check(p, d, "corrupted");
      }

      /* 3. uniform random d-digit values */
      for (int k = 0; k < 20000; k++) {
         u128 v = 0;
         for (int i = 0; i < d; i++) {
            int dg = (int)(rnd() % 10);
            if (i == 0 && dg == 0) dg = 1;
            v = v * 10 + (u128)dg;
         }
         check(v, d, "random");
      }

      /* 4. the extremes of the decade */
      check(bq_pow10[d - 1], d, "10^(d-1)");
      check(bq_pow10[d] - 1, d, "10^d - 1");
   }

   /* 5. real curve values, including every known palindrome we have */
   for (int64_t n = 0; n < 400000; n++) {
      u128 p = curve(n);
      int d = 0;
      while (d <= BQ_MAX_D && p >= bq_pow10[d]) d++;
      check(p, d, "curve");
   }

   printf("checked %lld values, d = 1..%d\n", checked, BQ_MAX_D);
   printf(fails ? "\n%ld FAILURE(S)\n" : "\nEQUIVALENT -- ALL PASS\n",
          fails);
   return fails ? 1 : 0;
}
