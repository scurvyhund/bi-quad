/* test_curve.c
 *
 * Unit tests for the shared primitives in curve.h.
 *
 * isqrt_u128 is the one that matters: it sets the band edge in
 * palsplit and palcurve, and an off-by-one there is a SILENT miss --
 * the run just reports a smaller found= and looks like a clean
 * negative result.  The earlier long-double band edge had exactly
 * that failure mode and no regression at d<=27 could see it, so this
 * test exercises the places an isqrt actually breaks: exact squares,
 * the values either side of them, and the top of the u128 range.
 *
 * Build:
 *   gcc -O2 -std=c99 -Wall -Wextra -o test_curve test_curve.c -lm
 *
 * Usage:
 *   ./test_curve          (prints PASS/FAIL per group, exit 0 = all ok)
 */

#include <stdio.h>
#include <stdint.h>
#include "curve.h"

static int fails = 0;

static void bad(const char *what, u128 v) {
   char b[BQ_STRLEN];
   u128_str(v, b);
   printf("  FAIL %s at %s\n", what, b);
   fails++;
}

/* the defining property: r*r <= v < (r+1)*(r+1), checked by division
 * so nothing overflows */
static void check_isqrt(u128 v) {
   u128 r = isqrt_u128(v);
   if (r > 0 && r > v / r) bad("isqrt too big", v);
   if (v / (r + 1) >= r + 1) bad("isqrt too small", v);
}

int main(void) {
   init_pow10();

   printf("isqrt_u128: exact squares and their neighbours\n");
   for (int e = 0; e < 63; e++) {
      u128 k = (u128)1 << e;
      for (int off = -2; off <= 2; off++) {
         u128 base = k + (u128)off;
         if (e == 0 && off < 0) continue;
         u128 sq = base * base;
         if (sq > 0) check_isqrt(sq - 1);
         check_isqrt(sq);
         check_isqrt(sq + 1);
      }
   }
   for (u128 v = 0; v < 2000; v++) check_isqrt(v);

   printf("isqrt_u128: powers of ten (the band-edge inputs)\n");
   for (int d = 1; d <= BQ_MAX_D; d++) {
      u128 p = bq_pow10[d];
      check_isqrt(2 * p - 1);          /* exactly what n_at() feeds it */
      check_isqrt(p);
   }

   printf("isqrt_u128: top of the u128 range\n");
   u128 top = ~(u128)0;
   for (int i = 0; i < 40; i++) check_isqrt(top - (u128)i);

   printf("n_at: band edge brackets the curve\n");
   for (int64_t n = 0; n < 200000; n++) {
      u128 p = curve(n);
      if (n_at(p) != n) { printf("  FAIL n_at(curve(%lld))\n",
                                 (long long)n); fails++; break; }
      if (n > 0 && n_at(p - 1) != n - 1) {
         printf("  FAIL n_at(curve(%lld)-1)\n", (long long)n);
         fails++; break;
      }
   }

   /* Top of the range matters more than the bottom: m_at forms
    * 4*A*p, and palcurve caps d at 33 precisely so that product still
    * fits a u128 for A <= 3.  Small m cannot exercise that.  Walk down
    * from the largest m whose curve value is still under 10^33. */
   printf("m_at: TOP of the range (the 4*A*p limit palcurve caps on)\n");
   int64_t abc[][3] = {{2,2,1},{3,3,1},{3,2,1},{3,4,2},{2,2,181},{1,1,1}};
   for (int c = 0; c < 6; c++) {
      int64_t A = abc[c][0], B = abc[c][1], C = abc[c][2];
      u128 cap = bq_pow10[33];
      int64_t hi = (int64_t)isqrt_u128(cap / (u128)A);
      while (hi > 0 && curve_abc(hi, A, B, C) >= cap) hi--;
      while (curve_abc(hi + 1, A, B, C) < cap) hi++;
      for (int64_t m = hi; m > hi - 2000 && m >= 0; m--) {
         u128 p = curve_abc(m, A, B, C);
         if (m_at(p, A, B, C) != m) {
            printf("  FAIL m_at top (%lld,%lld,%lld) m=%lld\n",
                   (long long)A, (long long)B, (long long)C,
                   (long long)m);
            fails++;
            break;
         }
         /* one below the curve value must land on m-1 */
         if (m > 0 && m_at(p - 1, A, B, C) != m - 1) {
            printf("  FAIL m_at top-1 (%lld,%lld,%lld) m=%lld\n",
                   (long long)A, (long long)B, (long long)C,
                   (long long)m);
            fails++;
            break;
         }
      }
   }

   printf("n_at: TOP of the range (d = %d, the palsplit frontier)\n",
          BQ_MAX_D);
   {
      u128 cap = bq_pow10[BQ_MAX_D];
      int64_t hi = (int64_t)isqrt_u128(cap / 2);
      while (hi > 0 && curve(hi) >= cap) hi--;
      while (curve(hi + 1) < cap) hi++;
      for (int64_t n = hi; n > hi - 2000; n--) {
         u128 p = curve(n);
         if (n_at(p) != n || n_at(p - 1) != n - 1) {
            printf("  FAIL n_at top n=%lld\n", (long long)n);
            fails++;
            break;
         }
      }
   }

   printf("m_at: small m\n");
   for (int c = 0; c < 6; c++) {
      int64_t A = abc[c][0], B = abc[c][1], C = abc[c][2];
      for (int64_t m = 0; m < 50000; m++) {
         u128 p = curve_abc(m, A, B, C);
         int64_t got = m_at(p, A, B, C);
         if (got != m) {
            printf("  FAIL m_at(%lld,%lld,%lld @ m=%lld) = %lld\n",
                   (long long)A, (long long)B, (long long)C,
                   (long long)m, (long long)got);
            fails++;
            break;
         }
      }
   }

   printf("digit_at / is_pal_d / rev_digits\n");
   if (digit_at(123456789, 0) != 9) { printf("  FAIL digit_at 0\n"); fails++; }
   if (digit_at(123456789, 8) != 1) { printf("  FAIL digit_at 8\n"); fails++; }
   if (!is_pal_d(3187813, 7)) { printf("  FAIL is_pal_d yes\n"); fails++; }
   if (is_pal_d(3187814, 7))  { printf("  FAIL is_pal_d no\n"); fails++; }
   if (rev_digits(1234, 4) != 4321) { printf("  FAIL rev\n"); fails++; }
   if (rev_digits(1200, 4) != 21) { printf("  FAIL rev zeros\n"); fails++; }

   /* the split's core identity: the high t digits of a palindrome are
    * the reverse of its low t digits.  This is the claim the whole
    * O(10^(d/4)) result rests on -- check it directly. */
   printf("split identity: high t digits == rev(low t digits)\n");
   for (int64_t n = 0; n < 300000; n++) {
      u128 p = curve(n);
      int d = 0;
      while (d <= BQ_MAX_D && p >= bq_pow10[d]) d++;
      if (!is_pal_d(p, d)) continue;
      for (int t = 1; t * 2 <= d; t++) {
         int64_t low  = (int64_t)(p % bq_pow10[t]);
         int64_t high = (int64_t)(p / bq_pow10[d - t]);
         if (rev_digits(low, t) != high) {
            printf("  FAIL split identity n=%lld t=%d\n",
                   (long long)n, t);
            fails++;
         }
      }
   }

   printf("u128_str round-trip\n");
   char b[BQ_STRLEN];
   u128_str(0, b);
   if (b[0] != '0' || b[1]) { printf("  FAIL str 0\n"); fails++; }
   u128_str(~(u128)0, b);
   if (strcmp(b, "340282366920938463463374607431768211455")) {
      printf("  FAIL str max: %s\n", b); fails++;
   }

   printf(fails ? "\n%d FAILURE(S)\n" : "\nALL PASS\n", fails);
   return fails ? 1 : 0;
}
