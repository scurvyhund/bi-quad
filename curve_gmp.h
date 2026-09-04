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

#endif /* BQ_CURVE_GMP_H */
