/*
 * hunt.c -- Direct brute-force hunt for bi-quadratic emirps.
 *
 * For each digit count d, enumerate EVERY n with p = 2n^2+2n+1
 * having d digits, form q = rev(p), and test the full conditions:
 *
 *   (1) q lies on the curve: q = 2m^2+2m+1  (on_curve check)
 *   (2) p is prime
 *   (3) q is prime
 *   (4) p != q  (genuine emirp, not a palindrome)
 *
 * A hit on (1) alone = a "survivor". Palindromic survivors (p==q)
 * are tracked separately from emirp candidates (p!=q). A hit on all
 * four conditions = a bi-quadratic emirp. Hits are printed immediately
 * to stdout with fflush so no result is lost on crash or kill.
 *
 * Checkpointing: every BLOCK_SIZE iterations the run saves progress
 * atomically to hunt_d<NN>_<CKPT_TAG>.ckpt (tmp + rename). On restart
 * the run resumes from the last checkpoint, restoring cumulative
 * counts. The checkpoint file is deleted on clean completion.
 *
 * The checkpoint is keyed to EVERYTHING that defines the count, not
 * just d: the filter set (CKPT_TAG, also in the file name), n_min and
 * the range.  A file that is malformed or does not match is REJECTED
 * LOUDLY on stderr and the d restarts from zero.  The tag is in the
 * name so binaries with different filters (hunt_noopt, pre-5f5fc4f
 * builds: plain hunt_d<NN>.ckpt) can never resume from each other's
 * counts; a legacy-named file is reported, never read or touched.
 * Change CKPT_TAG whenever the filters change what is counted.
 *
 * Build:  gcc hunt.c -o hunt -O3 -march=znver2 -std=c99 -Wall \
 *             -fopenmp -lgmp
 * Usage:  ./hunt [min_d] [max_d]      (defaults 13 17)
 *         1 <= min_d <= max_d <= HUNT_MAX_D (38); else exit 1.
 *
 * Width ceiling: p, q, n live in GMP and have no limit.  The only
 * fixed-width values are the loop index i and the block bounds, all
 * signed long.  At d=38 the n-range is < 2^63 with headroom for
 * blk + BLOCK_SIZE; at d=39 it is not.  Hence HUNT_MAX_D = 38 --
 * enforced at entry, and re-checked per d via mpz_fits_slong_p.
 * (Academic: d=38 is ~10^19 iterations.)
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <omp.h>
#include <gmp.h>

#include "curve_gmp.h"   /* on_curve, compute_n_bounds */

#define NUM_THREADS  8
#ifndef BLOCK_SIZE                 /* -D override for resume tests */
#define BLOCK_SIZE   5000000000L    /* 5 B iters per checkpoint block */
#endif
#define CKPT_TAG     "skip10x11"    /* VALID_NMOD + even-d VALID_NMOD11 */
#define CKPT_MAGIC   "HUNTCKPT2"
#define HUNT_MAX_D   38   /* long-index ceiling; see header */

/* n%10 in {1,3,6,8} -> p ends in 5 -> composite; skip those.
 * NOTE: LOSSY for counts -- see docs/skip_optimization.md. */
static const int VALID_NMOD[10] = {1,0,1,0,1,1,0,1,0,1};

/* EVEN d only: a converse pair needs p mod 11 in {3,5,6,8}
 * (docs/mod11_converse_constraint.md). n mod 11 determines p mod 11
 * via p = 2n^2+2n+1, so the condition becomes a lookup on n:
 *
 *   n%11 :  0  1  2  3  4  5  6  7  8  9 10
 *   p%11 :  1  5  2  3  8  6  8  3  2  5  1
 *   keep :  .  y  .  y  y  y  y  y  .  y  .
 *
 * Skips 4/11 = 36.4% of even-d iterations. Unlike VALID_NMOD this is
 * COUNT-PRESERVING: at even d both ends of every converse pair lie in
 * {3,5,6,8} (the flavours are (6,5),(5,6),(8,3),(3,8)), so no survivor
 * is dropped, and no even-d palindrome exists to drop. Independent of
 * VALID_NMOD since gcd(10,11) = 1. */
static const int VALID_NMOD11[11] = {0,1,0,1,1,1,1,1,0,1,0};

/* Atomic checkpoint write: write to .tmp then rename over target.
 * Every step is checked: a failed write leaves the PREVIOUS checkpoint
 * in place, so it must be reported -- silently, a restart would redo
 * work the heartbeat said was saved. */
static void write_ckpt(const char *path, int d, const char *nmin_s,
                        long range, long blk_end,
                        long surv, long elig,
                        long emrp, long pals) {
   char tmp[128];
   if (snprintf(tmp, sizeof(tmp), "%s.tmp", path)
         >= (int)sizeof(tmp)) {
      fprintf(stderr, "  WARNING: ckpt path too long: %s\n", path);
      return;
   }
   FILE *f = fopen(tmp, "w");
   if (!f) {
      fprintf(stderr, "  WARNING: cannot open ckpt %s\n", tmp);
      return;
   }
   int bad = fprintf(f, "%s %s %d %s %ld\n%ld %ld %ld %ld %ld\n",
                     CKPT_MAGIC, CKPT_TAG, d, nmin_s, range,
                     blk_end, surv, elig, emrp, pals) < 0;
   bad |= (fflush(f) != 0);
   bad |= (fclose(f) != 0);
   if (bad || rename(tmp, path) != 0) {
      fprintf(stderr,
         "  WARNING: ckpt write FAILED for %s at i=%ld -- previous "
         "checkpoint (if any) left in place\n", path, blk_end);
      remove(tmp);
   }
}

/* Read a checkpoint.  Returns 1 = valid (outputs set), 0 = no file,
 * -1 = present but rejected (reason already printed, LOUDLY). */
static int read_ckpt(const char *path, int d, const char *nmin_s,
                     long range, long *ci, long *cs, long *ce,
                     long *cemrp, long *cpal) {
   FILE *cf = fopen(path, "r");
   if (!cf) return 0;
   char magic[16], tag[32], nmin_f[64];
   int fd;
   long frange;
   int got = fscanf(cf, "%15s %31s %d %63s %ld %ld %ld %ld %ld %ld",
                    magic, tag, &fd, nmin_f, &frange,
                    ci, cs, ce, cemrp, cpal);
   fclose(cf);
   const char *why = NULL;
   if (got != 10)                         why = "malformed";
   else if (strcmp(magic, CKPT_MAGIC))    why = "wrong format/version";
   else if (strcmp(tag, CKPT_TAG))        why = "different filter tag";
   else if (fd != d)                      why = "different d";
   else if (strcmp(nmin_f, nmin_s))       why = "different n_min";
   else if (frange != range)              why = "different range";
   else if (*ci < 0 || *ci > range)       why = "position out of range";
   else if (*cs < 0 || *ce < 0 || *cemrp < 0 || *cpal < 0
            || *ce > *cs || *cpal > *cs || *cemrp > *cs)
                                          why = "inconsistent counts";
   if (why) {
      fprintf(stderr,
         "  *** CHECKPOINT REJECTED: %s (%s) -- d=%d restarts from "
         "ZERO; file will be overwritten at the first block\n",
         path, why, d);
      fflush(stderr);
      return -1;
   }
   return 1;
}

/* Parse a digit count; false on junk, trailing text or out-of-range. */
static bool parse_d(const char *s, int *out) {
   char *end;
   errno = 0;
   long v = strtol(s, &end, 10);
   if (errno || end == s || *end != '\0') return false;
   if (v < 1 || v > HUNT_MAX_D) return false;
   *out = (int)v;
   return true;
}

int main(int argc, char *argv[]) {
   int min_d = 13, max_d = 17;
   if (argc > 3
         || (argc > 1 && !parse_d(argv[1], &min_d))
         || (argc > 2 && !parse_d(argv[2], &max_d))
         || min_d > max_d) {
      fprintf(stderr,
         "usage: %s [min_d] [max_d]   1 <= min_d <= max_d <= %d\n",
         argv[0], HUNT_MAX_D);
      return 1;
   }

   printf("\n  Bi-Quadratic Emirp HUNT (direct brute force)\n");
   printf("  ============================================\n");
   printf("  d = %d .. %d\n\n", min_d, max_d);
   fflush(stdout);

   for (int d = min_d; d <= max_d; d++) {
      mpz_t n_min, n_max;
      mpz_inits(n_min, n_max, NULL);
      compute_n_bounds(d, n_min, n_max);

      mpz_t rng;
      mpz_init(rng);
      mpz_sub(rng, n_max, n_min);
      if (!mpz_fits_slong_p(rng)
            || mpz_get_si(rng) > LONG_MAX - 1 - BLOCK_SIZE) {
         /* Unreachable while HUNT_MAX_D is right; refuse, never skip. */
         gmp_fprintf(stderr,
            "  d=%2d: range %Zd overflows long -- HUNT_MAX_D wrong\n",
            d, rng);
         mpz_clears(n_min, n_max, rng, NULL);
         return 1;
      }
      long range = mpz_get_si(rng) + 1;
      mpz_clear(rng);

      char ckpt[64], legacy[64];
      snprintf(ckpt, sizeof(ckpt), "hunt_d%02d_%s.ckpt", d, CKPT_TAG);
      snprintf(legacy, sizeof(legacy), "hunt_d%02d.ckpt", d);
      char *nmin_s = mpz_get_str(NULL, 10, n_min);
      {
         FILE *lf = fopen(legacy, "r");
         if (lf) {
            fclose(lf);
            fprintf(stderr,
               "  NOTE: legacy %s present (hunt_noopt or a pre-5f5fc4f "
               "build) -- NOT read, NOT touched\n", legacy);
         }
      }

      long start_i        = 0;
      long survivors      = 0;
      long elig_survivors = 0;
      long emirps         = 0;
      long palindromes    = 0;

      /* Load checkpoint if present */
      {
         long ci, cs, ce, cemrp, cpal;
         if (read_ckpt(ckpt, d, nmin_s, range,
                       &ci, &cs, &ce, &cemrp, &cpal) == 1) {
            start_i        = ci;
            survivors      = cs;
            elig_survivors = ce;
            emirps         = cemrp;
            palindromes    = cpal;
            printf(
               "  d=%2d: resuming from i=%ld/%ld (%.1f%%)"
               "  surv=%ld pals=%ld emrp=%ld\n",
               d, start_i, range, 100.0 * start_i / range,
               survivors, palindromes, emirps);
            fflush(stdout);
         }
      }

      /* Fuse both residue filters into one table indexed by i % 110.
       * 110 = lcm(10,11), so i % 110 fixes both n%10 and n%11 (n =
       * n_min + i). One modulo and one lookup in the inner loop
       * instead of two of each, and the even-d test is hoisted out
       * entirely -- odd d pays nothing for the mod-11 filter. */
      int nmod_base   = (int)mpz_fdiv_ui(n_min, 10);
      int nmod11_base = (int)mpz_fdiv_ui(n_min, 11);
      bool even_d     = (d % 2 == 0);

      int keep_n[110];
      for (int j = 0; j < 110; j++)
         keep_n[j] = VALID_NMOD[(nmod_base + j) % 10]
                  && (!even_d
                      || VALID_NMOD11[(nmod11_base + j) % 11]);

      double t0 = omp_get_wtime();

      /* Outer block loop (sequential) -- parallel region per block.
       * A complete block is the checkpoint unit. */
      for (long blk = start_i; blk < range; blk += BLOCK_SIZE) {
         long blk_end = blk + BLOCK_SIZE;
         if (blk_end > range) blk_end = range;

         #pragma omp parallel num_threads(NUM_THREADS)
         {
            mpz_t n, p, q, m, s;
            mpz_inits(n, p, q, m, s, NULL);
            char *pbuf = malloc(d + 2);
            char *qbuf = malloc(d + 2);   /* d digits + NUL + sign */
            if (!pbuf || !qbuf) {
               fprintf(stderr, "  d=%d: malloc failed\n", d);
               exit(1);
            }

            #pragma omp for schedule(dynamic, 100000)
            for (long i = blk; i < blk_end; i++) {
               if (!keep_n[i % 110]) continue;
               mpz_add_ui(n, n_min, (unsigned long)i);

               mpz_mul(p, n, n);
               mpz_mul_ui(p, p, 2);
               mpz_addmul_ui(p, n, 2);
               mpz_add_ui(p, p, 1);

               mpz_get_str(pbuf, 10, p);
               int len = (int)strlen(pbuf);
               for (int a = 0; a < len; a++)
                  qbuf[a] = pbuf[len - 1 - a];
               qbuf[len] = '\0';
               mpz_set_str(q, qbuf, 10);

               if (!on_curve(q, m, s))
                  continue;

               bool pal  = (mpz_cmp(p, q) == 0);
               bool elig = (mpz_fdiv_ui(p, 5) != 0)
                        && (mpz_fdiv_ui(q, 5) != 0);

               /* Even-d mod-11 invariant.
                * See docs/mod11_converse_constraint.md. Since 10 = -1
                * (mod 11), rev(p) = (-1)^(d-1) * p, so at EVEN d a
                * converse pair forces a^2 + b^2 = 9 (mod 11) where
                * a^2 = 2p-1, b^2 = 2q-1. Consequences:
                *   - palindrome (b == a) needs a^2 = 10, which is not
                *     a square mod 11 -> NO even-d palindrome lies on
                *     this curve at all;
                *   - otherwise p mod 11 must be 3, 5, 6 or 8.
                * A survivor here is an exact converse pair (q is the
                * literal digit reversal and on_curve(q) passed), so
                * this holds at every d -- no guard needed. A hit is a
                * bug in this hunt, not a discovery. Reported loudly
                * rather than aborted: a 100+ hour run should not die
                * on a diagnostic. */
               if (d % 2 == 0) {
                  unsigned long r11 = mpz_fdiv_ui(p, 11);
                  if (pal || (r11 != 3 && r11 != 5
                              && r11 != 6 && r11 != 8)) {
                     #pragma omp critical (mod11)
                     {
                        fprintf(stderr,
                           "  *** BUG d=%d: even-d survivor breaks the "
                           "mod-11 invariant (p mod 11 = %lu, pal=%d)\n"
                           "      p = ", d, r11, (int)pal);
                        mpz_out_str(stderr, 10, p);
                        fprintf(stderr, "\n");
                        fflush(stderr);
                     }
                  }
               }

               #pragma omp atomic
               survivors++;
               if (pal) {
                  #pragma omp atomic
                  palindromes++;
               }
               if (elig) {
                  #pragma omp atomic
                  elig_survivors++;
               }

               bool full = !pal
                        && mpz_probab_prime_p(p, 40)
                        && mpz_probab_prime_p(q, 40);
               if (full) {
                  #pragma omp atomic
                  emirps++;
               }

               /* Print hit immediately -- never buffer results. */
               #pragma omp critical (stdout)
               {
                  const char *lbl =
                     full ? "*** EMIRP ***" :
                     pal  ? "palindrome   " :
                            "survivor     ";
                  printf("  %s  n=", lbl);
                  mpz_out_str(stdout, 10, n);
                  printf("  p=");
                  mpz_out_str(stdout, 10, p);
                  if (!pal) {
                     printf("  q=");
                     mpz_out_str(stdout, 10, q);
                  }
                  printf("\n");
                  fflush(stdout);
               }
            }

            mpz_clears(n, p, q, m, s, NULL);
            free(pbuf); free(qbuf);
         } /* end omp parallel */

         write_ckpt(ckpt, d, nmin_s, range, blk_end,
                    survivors, elig_survivors, emirps, palindromes);
         fprintf(stderr,
            "  d=%2d  ckpt %ld/%ld (%.1f%%)"
            "  surv=%ld  pals=%ld  emrp=%ld  [%.2f h]\n",
            d, blk_end, range,
            100.0 * blk_end / range,
            survivors, palindromes, emirps,
            (omp_get_wtime() - t0) / 3600.0);
      } /* end block loop */

      double dt = omp_get_wtime() - t0;
      printf(
         "  d=%2d  range=%ld  survivors(raw)=%ld"
         "  palindromes=%ld  prime-eligible=%ld"
         "  EMIRPS=%ld   [%.2f h]\n",
         d, range, survivors,
         palindromes, elig_survivors,
         emirps, dt / 3600.0);
      fflush(stdout);

      remove(ckpt);
      free(nmin_s);
      mpz_clears(n_min, n_max, NULL);
   }

   printf("\n  Hunt complete.\n\n");
   return 0;
}
