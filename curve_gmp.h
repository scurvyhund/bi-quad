/* curve_gmp.h
 *
 * The curve-membership test, and the decimal-reversal helper that
 * every emirp tool needs, for the GMP side of the project.
 *
 * Membership: a value q lies on p = 2m^2 + 2m + 1 exactly when 2q-1
 * is a perfect square whose root is odd; then m = (sqrt(2q-1) - 1)/2.
 * This is THE predicate the whole emirp search rests on -- it decides
 * whether a number is on the curve at all.  It previously existed in
 * five copies (check_d5, check_d7, check_d9, check_survivors as
 * is_consec_sq, hunt.c as on_curve), byte-identical in four of them.
 * Identical copies are the state divergence starts from, and a
 * membership test that drifts in one copy fails SILENTLY: the tool
 * simply stops finding things.
 *
 * Kept separate from curve.h so that curve.h stays GMP-free -- its
 * own unit test links without -lgmp.
 *
 * Header-only for the same reason as curve.h: on_curve sits in the
 * inner loop of a multi-day hunt.  See the NOTE at the top of curve.h.
 *
 * Build: no separate compile step -- #include "curve_gmp.h", link -lgmp.
 */

#ifndef BQ_CURVE_GMP_H
#define BQ_CURVE_GMP_H

#include <stdbool.h>
#include <gmp.h>

/* Is val on the curve?  If so store its index in m and return true.
 *
 * CANONICAL FORM.  Takes its scratch mpz_t from the caller: hunt.c
 * calls this billions of times, and an mpz_init/mpz_clear pair per
 * call would be pure overhead in that loop.  Cold callers should use
 * is_consec_sq() below rather than manage a scratch by hand.
 *
 * The negative guard matters only in principle -- mpz_perfect_square_p
 * already rejects negatives, and val = 0 is the only input that could
 * reach it -- but it costs nothing and states the domain. */
static inline bool on_curve(const mpz_t val, mpz_t m, mpz_t s) {
   mpz_mul_ui(s, val, 2);
   mpz_sub_ui(s, s, 1);
   if (mpz_sgn(s) < 0) return false;
   if (!mpz_perfect_square_p(s)) return false;
   mpz_sqrt(s, s);
   if (mpz_even_p(s)) return false;     /* root must be odd: s = 2m+1 */
   mpz_sub_ui(s, s, 1);
   mpz_fdiv_q_ui(m, s, 2);
   return true;
}

/* Same test, allocating its own scratch.  For the cold enumerators
 * (check_d5/d7/d9, check_survivors) where one mpz_init per hit is
 * irrelevant and a borrowed scratch would only be a trap.
 *
 * The four copies this replaces tested the root's parity as
 * "is s-1 divisible by 2" rather than "is s odd" -- the same test
 * written the other way round -- and used mpz_divexact_ui where
 * on_curve uses mpz_fdiv_q_ui, which agree exactly when the dividend
 * is even, as it is here. */
static inline bool is_consec_sq(mpz_t q, mpz_t m_out) {
   mpz_t s;
   mpz_init(s);
   bool ok = on_curve(q, m_out, s);
   mpz_clear(s);
   return ok;
}

/* Reverse a decimal string of length len into dst (len+1 bytes). */
static inline void reverse_str(const char *src, char *dst, int len) {
   for (int i = 0; i < len; i++)
      dst[i] = src[len - 1 - i];
   dst[len] = '\0';
}

/* p = 2n^2 + 2n + 1, on mpz. */
static inline void curve_mpz(mpz_t p, const mpz_t n) {
   mpz_mul(p, n, n);
   mpz_mul_ui(p, p, 2);
   mpz_addmul_ui(p, n, 2);
   mpz_add_ui(p, p, 1);
}

/* The n-range whose curve values have exactly d decimal digits:
 * n_min = least n with curve(n) >= 10^(d-1),
 * n_max = greatest n with curve(n) <  10^d.
 *
 * SELF-CORRECTING.  The sqrt only SEEDS the search; the bounds are
 * then walked to their true edges by evaluating curve() directly, so
 * the answer does not depend on the seed being right.  This costs
 * nothing -- it is called once per digit-length, never in a hot loop.
 *
 * The three copies this replaces (hunt.c, mod_obstruct.c and its
 * _bkup) instead computed n_min = floor((sqrt(2*10^(d-1) - 1) - 1)/2)
 * + 1 and defended it with a comment reading "guards against sqrt
 * rounding".  There is no sqrt rounding to guard against: mpz_sqrt is
 * exact integer arithmetic.  Measured over d = 1..400 that guard
 * fired zero times, and the two live copies agreed with each other at
 * every d -- so this change is behaviour-preserving everywhere except
 * d = 1, where the unconditional "+1" pushed n_min to 1 and silently
 * dropped n = 0 (p = 1, which is genuinely on the curve).
 *
 * That precise failure has bitten this project before: check_d7.c
 * records an n_min hardcoded to 710 behind a guard that could never
 * fire, which dropped the true first 7-digit n = 707 and its two
 * successors.  Walking to the edge removes the whole class. */
static inline void compute_n_bounds(int d, mpz_t n_min, mpz_t n_max) {
   mpz_t lo, hi, p, t;
   mpz_inits(lo, hi, p, t, NULL);
   mpz_ui_pow_ui(lo, 10, (unsigned long)(d - 1));
   mpz_ui_pow_ui(hi, 10, (unsigned long)d);

   /* seed n ~ sqrt(p/2), deliberately biased low, then walk up */
   mpz_mul_ui(t, lo, 2);
   mpz_sqrt(t, t);
   mpz_fdiv_q_ui(n_min, t, 2);
   if (mpz_cmp_ui(n_min, 2) > 0) mpz_sub_ui(n_min, n_min, 2);
   else mpz_set_ui(n_min, 0);
   for (;;) {
      curve_mpz(p, n_min);
      if (mpz_cmp(p, lo) >= 0) break;
      mpz_add_ui(n_min, n_min, 1);
   }
   for (;;) {                      /* and back down to the true edge */
      if (mpz_sgn(n_min) == 0) break;
      mpz_sub_ui(t, n_min, 1);
      curve_mpz(p, t);
      if (mpz_cmp(p, lo) < 0) break;
      mpz_set(n_min, t);
   }

   /* seed biased high, then walk down */
   mpz_mul_ui(t, hi, 2);
   mpz_sqrt(t, t);
   mpz_fdiv_q_ui(n_max, t, 2);
   mpz_add_ui(n_max, n_max, 2);
   for (;;) {
      curve_mpz(p, n_max);
      if (mpz_cmp(p, hi) < 0) break;
      if (mpz_sgn(n_max) == 0) break;
      mpz_sub_ui(n_max, n_max, 1);
   }
   for (;;) {
      mpz_add_ui(t, n_max, 1);
      curve_mpz(p, t);
      if (mpz_cmp(p, hi) >= 0) break;
      mpz_set(n_max, t);
   }

   mpz_clears(lo, hi, p, t, NULL);
}

#endif /* BQ_CURVE_GMP_H */
