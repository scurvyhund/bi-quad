/* curve.h
 *
 * Shared primitives for the bi-quad palindrome tools (palsplit,
 * palcurve, palbrute).  Header-only by design -- see NOTE below.
 *
 * Everything here is arithmetic on the curve family
 *
 *     p = A*m^2 + B*m + C
 *
 * of which the project curve p = 2n^2 + 2n + 1 = n^2 + (n+1)^2 is the
 * case (A,B,C) = (2,2,1).  Decimal helpers (pow10 table, digit access,
 * reversal, u128 printing) live here too: every tool needs them and
 * three hand-written copies of a digit loop is three chances to differ.
 *
 * NOTE -- header-only, not the usual .h/.c pair.
 *   curve(), curve_mod(), digit_at() and isqrt_u128() are called in
 *   the innermost loop tens of millions of times per run.  Putting
 *   them in a separate translation unit would cost the inlining that
 *   the whole O(10^(d/4)) result rides on.  They are therefore
 *   'static inline' in the header.  The project style rule (factor
 *   shared logic into .h/.c and link both) is deliberately relaxed
 *   here for that reason, and only here.
 *
 * NOTE -- what is deliberately NOT shared.
 *   palbrute.c keeps its OWN palindrome predicate.  palbrute exists to
 *   cross-check palsplit; a cross-check that reuses the code it is
 *   checking proves nothing.  See docs/palindrome_split_search.md.
 *   Do not "clean that up".
 *
 * Build: no separate compile step -- #include "curve.h".
 *        Needs -lm (sqrtl, used only as an isqrt seed).
 */

#ifndef BQ_CURVE_H
#define BQ_CURVE_H

#include <stdint.h>
#include <string.h>
#include <math.h>

/* Unsigned ONLY as a fixed-width decimal container: no arithmetic in
 * this header can go negative, and we need the full 128-bit range.
 * Every loop counter and index in the callers stays signed, per the
 * project's integer-wraparound policy. */
typedef unsigned __int128 u128;

/* Extent of the pow10 table.  10^37 is the largest power of ten for
 * which 2p still fits a u128 (2*10^37 < 3.40e38), which is what the
 * band arithmetic needs.  This is the TABLE size only -- a tool's own
 * maximum d is its own business and may be smaller (palcurve caps at
 * 33 so that 4*A*p also fits).  Do not conflate the two. */
#define BQ_MAX_D   37

/* buffer big enough for any u128 in decimal (39 digits) plus NUL */
#define BQ_STRLEN  44

static u128 bq_pow10[BQ_MAX_D + 2];

static inline void init_pow10(void) {
   bq_pow10[0] = 1;
   for (int i = 1; i <= BQ_MAX_D + 1; i++) {
      bq_pow10[i] = bq_pow10[i - 1] * 10;
   }
}

/* 10^e without needing the table initialised (cold paths only) */
static inline u128 ipow10(int e) {
   u128 r = 1;
   while (e-- > 0) r *= 10;
   return r;
}

/* Exact integer sqrt of a u128.  Seeded from long double, refined by
 * Newton, then adjusted exactly -- so the result is exact regardless
 * of how good the seed was.  Comparisons use division, never x*x,
 * which would overflow near the top of the range.
 *
 * This is what removes floating point from the band edge.  An earlier
 * palsplit estimated the edge in long double and needed an 80-bit
 * mantissa to stay safe; the margin turned out to be ~1.3x, not the
 * 1000x first claimed, and no regression at d<=27 could have caught a
 * miss.  See docs/palindrome_split_search.md sec. 4. */
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

/* ---- the project curve, p = 2n^2 + 2n + 1 ------------------------ */

static inline u128 curve(int64_t n) {
   u128 v = (u128)n;
   return 2 * v * v + 2 * v + 1;
}

static inline int64_t curve_mod(int64_t r, int64_t m) {
   u128 v = (u128)r;
   return (int64_t)((2 * v * v + 2 * v + 1) % (u128)m);
}

/* Largest n with curve(n) <= p, i.e. the band edge.  Inverting
 * 2n^2+2n+1 = p gives n = (sqrt(2p-1) - 1)/2, exact here because
 * isqrt_u128 is exact.  Callers still widen by a guard band and
 * re-test curve(n) directly, so this need not be tight. */
static inline int64_t n_at(u128 p) {
   if (p == 0) return 0;
   u128 s = isqrt_u128(2 * p - 1);
   return (s > 0) ? (int64_t)((s - 1) / 2) : 0;
}

/* ---- the general curve, p = A*m^2 + B*m + C ---------------------- */

static inline u128 curve_abc(int64_t m, int64_t a, int64_t b, int64_t c) {
   u128 v = (u128)m;
   return (u128)a * v * v + (u128)b * v + (u128)c;
}

static inline int64_t curve_abc_mod(int64_t r, int64_t md,
                                    int64_t a, int64_t b, int64_t c) {
   return (int64_t)(curve_abc(r, a, b, c) % (u128)md);
}

/* Band edge for the general curve, via the quadratic formula:
 *   A m^2 + B m + C = p  ->  m = (-B + sqrt(B^2 - 4AC + 4Ap)) / 2A
 * 4Ap dominates the constant offset, so all terms stay non-negative.
 * Requires 4*A*p to fit a u128.  For A <= 3 that binds at p < 2.83e37
 * (d <= 37); palcurve caps at d = 33, which is conservative on purpose
 * -- see the comment on its MAX_D. */
static inline int64_t m_at(u128 p, int64_t a, int64_t b, int64_t c) {
   u128 disc = 4 * (u128)a * p + (u128)(b * b);
   u128 sub  = 4 * (u128)a * (u128)c;
   disc = (disc > sub) ? disc - sub : 0;
   u128 s = isqrt_u128(disc);
   if (s < (u128)b) return 0;
   return (int64_t)((s - (u128)b) / (2 * (u128)a));
}

/* ---- decimal helpers -------------------------------------------- */

/* digit i of p counting from the units place (i=0).  Needs the pow10
 * table, so call init_pow10() first. */
static inline int digit_at(u128 p, int i) {
   return (int)((p / bq_pow10[i]) % 10);
}

/* does the d-digit value p read the same both ways?  Digit-indexed
 * form, used by the split searches.  palbrute deliberately uses a
 * different implementation -- see the NOTE at the top. */
static inline int is_pal_d(u128 p, int d) {
   for (int i = 0; i < d / 2; i++) {
      if (digit_at(p, i) != digit_at(p, d - 1 - i)) return 0;
   }
   return 1;
}

/* the low t digits of v, reversed -- i.e. the high t digits that a
 * palindrome ending in v must have */
static inline int64_t rev_digits(int64_t v, int t) {
   int64_t h = 0;
   for (int i = 0; i < t; i++) {
      h = h * 10 + v % 10;
      v /= 10;
   }
   return h;
}

/* decimal form of v into buf, which must hold BQ_STRLEN bytes */
static inline void u128_str(u128 v, char *buf) {
   char tmp[BQ_STRLEN];
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

#endif /* BQ_CURVE_H */
