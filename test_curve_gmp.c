/* test_curve_gmp.c
 *
 * Unit tests for the curve-membership test in curve_gmp.h.
 *
 * WHY THIS EXISTS.  Before the extraction, on_curve (hunt.c) and
 * is_consec_sq (check_d5/d7/d9, check_survivors) were separate
 * implementations, and their incidental agreement was the only
 * evidence the predicate was right.  That evidence was weak -- the
 * two tools count different things by design, so they were never a
 * real cross-check -- and unifying them removes it entirely.
 *
 * This replaces it with a real one: on_curve is checked against the
 * u128 path in curve.h, which decides membership by a DIFFERENT
 * ROUTE.  GMP asks mpz_perfect_square_p of 2q-1; curve.h takes
 * n_at(v) and re-evaluates curve(n) == v.  The two are not fully
 * independent -- n_at calls isqrt_u128, which test_curve validates
 * separately -- but the DECISION is, and the re-evaluation is what
 * makes it sound: a wrong n_at fails curve(n) != v and rejects, so it
 * cannot manufacture a false accept.
 *
 * Build:
 *   gcc -O2 -std=c99 -Wall -Wextra -o test_curve_gmp \
 *       test_curve_gmp.c -lgmp -lm
 *
 * Usage:
 *   ./test_curve_gmp        (exit 0 = all pass)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "curve.h"
#include "curve_gmp.h"

static int fails = 0;

/* exact u128 -> mpz, two 64-bit limbs, least significant first */
static void u128_to_mpz(mpz_t z, u128 v) {
   uint64_t w[2] = { (uint64_t)v, (uint64_t)(v >> 64) };
   mpz_import(z, 2, -1, sizeof(uint64_t), 0, 0, w);
}

/* independent oracle: is v on the curve, decided in u128 with no GMP */
static int on_curve_u128(u128 v, int64_t *m_out) {
   if (v == 0) return 0;
   int64_t n = n_at(v);
   if (curve(n) != v) return 0;
   *m_out = n;
   return 1;
}

int main(void) {
   init_pow10();

   mpz_t q, m, s;
   mpz_inits(q, m, s, NULL);
   char buf[BQ_STRLEN];

   /* 1. every value below a bound, both ways round.  This is the test
    * that would catch a parity or off-by-one slip: it visits the
    * curve values AND all the non-values between them. */
   printf("exhaustive agreement with the u128 oracle, v < 2000000\n");
   for (u128 v = 0; v < 2000000; v++) {
      int64_t mu = -1;
      int want = on_curve_u128(v, &mu);
      u128_str(v, buf);
      mpz_set_str(q, buf, 10);
      int got = on_curve(q, m, s) ? 1 : 0;
      if (got != want) {
         printf("  FAIL membership disagrees at v=%s"
                " (gmp=%d u128=%d)\n", buf, got, want);
         fails++;
         if (fails > 5) break;
      }
      if (want && mpz_get_si(m) != mu) {
         printf("  FAIL index disagrees at v=%s\n", buf);
         fails++;
         if (fails > 5) break;
      }
   }

   /* 2. big curve values and their immediate neighbours -- n+-1 off
    * the curve must be rejected, which is where a sloppy sqrt shows */
   printf("large n: on-curve accepted, v+-1 rejected\n");
   for (int e = 3; e <= 18; e++) {
      for (int64_t d = -2; d <= 2; d++) {
         int64_t n = (int64_t)ipow10(e) + d;
         if (n < 0) continue;
         u128 v = curve(n);
         int64_t mu = -1;
         u128_str(v, buf);
         mpz_set_str(q, buf, 10);
         if (!on_curve(q, m, s) || mpz_get_si(m) != n) {
            printf("  FAIL curve(%lld) rejected\n", (long long)n);
            fails++;
         }
         if (!on_curve_u128(v, &mu) || mu != n) {
            printf("  FAIL oracle rejects curve(%lld)\n", (long long)n);
            fails++;
         }
         for (int off = -1; off <= 1; off += 2) {
            u128_str((off < 0) ? v - 1 : v + 1, buf);
            mpz_set_str(q, buf, 10);
            if (on_curve(q, m, s)) {
               printf("  FAIL curve(%lld)%+d accepted\n",
                      (long long)n, off);
               fails++;
            }
         }
      }
   }

   /* 3. past u128 -- the oracle cannot follow here, so this checks
    * the GMP path against its own definition: 2q-1 = (2m+1)^2 */
   printf("beyond u128: 2q-1 = (2m+1)^2 for m near 10^25\n");
   {
      mpz_t big, chk;
      mpz_inits(big, chk, NULL);
      for (int64_t d = 0; d < 50; d++) {
         mpz_ui_pow_ui(big, 10, 25);
         mpz_add_ui(big, big, (unsigned long)d);
         /* q = 2*big^2 + 2*big + 1 */
         mpz_mul(q, big, big);
         mpz_mul_ui(q, q, 2);
         mpz_addmul_ui(q, big, 2);
         mpz_add_ui(q, q, 1);
         if (!on_curve(q, m, s) || mpz_cmp(m, big) != 0) {
            printf("  FAIL big curve value at 10^25+%lld\n",
                   (long long)d);
            fails++;
         }
         mpz_add_ui(chk, q, 1);
         if (on_curve(chk, m, s)) {
            printf("  FAIL big value+1 accepted\n");
            fails++;
         }
      }
      mpz_clears(big, chk, NULL);
   }

   /* 3b. curve.h's u128 helpers at the TOP of the range, against GMP.
    * test_curve.c stops isqrt at 2^124; nothing checked 2^128-1, nor
    * curve() at INT64_MAX.  Edges first, then 200k pseudo-random
    * values spread over every bit width 1..128. */
   printf("u128 helpers vs GMP at the top of the range\n");
   {
      mpz_t z, r, w;
      mpz_inits(z, r, w, NULL);
      const u128 top = ~(u128)0;
      const u128 s64 = (u128)UINT64_MAX * UINT64_MAX;  /* (2^64-1)^2 */
      const u128 h127 = (u128)1 << 127;
      u128 edge[] = { top, top - 1, s64 - 1, s64, s64 + 1,
                      h127 - 1, h127, h127 + 1, 0, 1, 2, 3 };
      /* xorshift64: unsigned BY INTENT -- the wrap is the generator */
      uint64_t x = 0x9E3779B97F4A7C15ULL;
      long checked = 0;
      for (int i = 0; i < 200000 + 12; i++) {
         u128 v;
         if (i < 12) {
            v = edge[i];
         } else {
            x ^= x << 13; x ^= x >> 7; x ^= x << 17;
            uint64_t y = x;
            x ^= x << 13; x ^= x >> 7; x ^= x << 17;
            int bits = 1 + (int)(y % 128);
            v = ((u128)x << 64) | y;
            if (bits < 128) v &= ((u128)1 << bits) - 1;
         }
         u128_to_mpz(z, v);
         mpz_sqrt(r, z);
         u128_to_mpz(w, isqrt_u128(v));
         if (mpz_cmp(r, w) != 0) {
            u128_str(v, buf);
            printf("  FAIL isqrt_u128(%s)\n", buf);
            fails++;
            break;
         }
         checked++;
      }
      printf("  isqrt_u128: %ld values exact\n", checked);

      /* curve(n) exact for every int64 n >= 0, incl. INT64_MAX */
      int64_t ns[] = { INT64_MAX, INT64_MAX - 1, (int64_t)1 << 62,
                       3037000499LL, 0, 1 };
      for (int i = 0; i < 6; i++) {
         mpz_set_si(w, ns[i]);
         curve_mpz(r, w);
         u128_to_mpz(z, curve(ns[i]));
         if (mpz_cmp(r, z) != 0) {
            printf("  FAIL curve(%lld) != curve_mpz\n",
                   (long long)ns[i]);
            fails++;
         }
      }
      /* n_at needs p < 2^127.  curve(INT64_MAX) = 2^127 - 2^64 + 1 is
       * below it, so the round trip must hold for EVERY int64 n >= 0,
       * right up to INT64_MAX. */
      for (int64_t n = INT64_MAX; n > INT64_MAX - 2000; n -= 97) {
         u128 p = curve(n);
         if (p >= h127 || n_at(p) != n || n_at(p - 1) != n - 1) {
            printf("  FAIL n_at top-of-int64 n=%lld\n", (long long)n);
            fails++;
            break;
         }
      }
      if (curve(INT64_MAX) != h127 - ((u128)1 << 64) + 1) {
         printf("  FAIL curve(INT64_MAX) != 2^127 - 2^64 + 1\n");
         fails++;
      }
      mpz_clears(z, r, w, NULL);
   }

   /* 4. the wrapper must agree with the canonical form exactly */
   printf("is_consec_sq wrapper == on_curve\n");
   {
      mpz_t m2;
      mpz_init(m2);
      for (u128 v = 1; v < 200000; v++) {
         u128_str(v, buf);
         mpz_set_str(q, buf, 10);
         int a = on_curve(q, m, s) ? 1 : 0;
         int b = is_consec_sq(q, m2) ? 1 : 0;
         if (a != b || (a && mpz_cmp(m, m2) != 0)) {
            printf("  FAIL wrapper differs at v=%s\n", buf);
            fails++;
            break;
         }
      }
      mpz_clear(m2);
   }

   /* compute_n_bounds: check the INVARIANT directly rather than
    * against another implementation.  n_min must be the first index
    * whose curve value has d digits and n_max the last, so the
    * neighbours just outside must fall in the adjacent decades. */
   printf("compute_n_bounds: edges are exact, d = 1..400\n");
   {
      mpz_t nmin, nmax, p, lo, hi, t;
      mpz_inits(nmin, nmax, p, lo, hi, t, NULL);
      for (int d = 1; d <= 400; d++) {
         compute_n_bounds(d, nmin, nmax);
         mpz_ui_pow_ui(lo, 10, (unsigned long)(d - 1));
         mpz_ui_pow_ui(hi, 10, (unsigned long)d);

         curve_mpz(p, nmin);
         if (mpz_cmp(p, lo) < 0 || mpz_cmp(p, hi) >= 0) {
            printf("  FAIL d=%d: curve(n_min) not d digits\n", d);
            fails++;
         }
         if (mpz_sgn(nmin) > 0) {          /* n_min-1 must fall short */
            mpz_sub_ui(t, nmin, 1);
            curve_mpz(p, t);
            if (mpz_cmp(p, lo) >= 0) {
               printf("  FAIL d=%d: n_min is not the first\n", d);
               fails++;
            }
         }
         curve_mpz(p, nmax);
         if (mpz_cmp(p, lo) < 0 || mpz_cmp(p, hi) >= 0) {
            printf("  FAIL d=%d: curve(n_max) not d digits\n", d);
            fails++;
         }
         mpz_add_ui(t, nmax, 1);           /* n_max+1 must overshoot */
         curve_mpz(p, t);
         if (mpz_cmp(p, hi) < 0) {
            printf("  FAIL d=%d: n_max is not the last\n", d);
            fails++;
         }
      }
      /* d=1 is the case the old "+1" got wrong: curve(0) = 1 */
      compute_n_bounds(1, nmin, nmax);
      if (mpz_sgn(nmin) != 0) {
         printf("  FAIL d=1: n_min should be 0 (curve(0) = 1)\n");
         fails++;
      }
      mpz_clears(nmin, nmax, p, lo, hi, t, NULL);
   }

   printf("reverse_str\n");
   {
      char out[32];
      reverse_str("12345", out, 5);
      if (strcmp(out, "54321")) { printf("  FAIL rev\n"); fails++; }
      reverse_str("10500", out, 5);
      if (strcmp(out, "00501")) { printf("  FAIL rev z\n"); fails++; }
      reverse_str("7", out, 1);
      if (strcmp(out, "7")) { printf("  FAIL rev 1\n"); fails++; }
   }

   mpz_clears(q, m, s, NULL);
   printf(fails ? "\n%d FAILURE(S)\n" : "\nALL PASS\n", fails);
   return fails ? 1 : 0;
}
