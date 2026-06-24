/*
 * hunt.c — Direct brute-force hunt for bi-quadratic emirps.
 *
 * For each digit count d, enumerate EVERY n with p = 2n^2+2n+1 having d digits,
 * form q = rev(p), and test the full (exact) conditions:
 *
 *   (1) q lies on the curve: q = 2m^2+2m+1 for some integer m
 *       <=> 2q-1 is a perfect square S and m = (sqrt(S)-1)/2 is a non-neg int
 *   (2) p is prime
 *   (3) q is prime
 *   (4) p != q  (genuine emirp, not a palindrome)
 *
 * A hit on (1) alone = a "survivor" (should match mod_obstruct's count at
 * k >= ceil(d/2), an independent cross-check). A hit on all four = an actual
 * bi-quadratic emirp.
 *
 * This is tractable only for small d (range = n_max-n_min is ~2M..216M for
 * d=13..17). For large d use the modular sieve (mod_obstruct) instead.
 *
 * Build:  gcc hunt.c -o hunt -O3 -march=znver2 -std=c99 -Wall -fopenmp -lgmp
 * Usage:  ./hunt [min_d] [max_d]      (defaults 13 17)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <omp.h>
#include <gmp.h>

#define NUM_THREADS 8
#define MAX_HITS    1024

/* n_min, n_max for d-digit p = 2n^2+2n+1  (same logic as mod_obstruct.c) */
static void compute_n_bounds(int d, mpz_t n_min, mpz_t n_max) {
   mpz_t target, sq, check, lo;
   mpz_inits(target, sq, check, lo, NULL);

   mpz_ui_pow_ui(target, 10, d - 1);          /* 10^(d-1) */
   mpz_mul_ui(target, target, 2);
   mpz_sub_ui(target, target, 1);
   mpz_sqrt(sq, target);
   mpz_sub_ui(sq, sq, 1);
   mpz_fdiv_q_ui(n_min, sq, 2);
   mpz_add_ui(n_min, n_min, 1);

   mpz_ui_pow_ui(lo, 10, d - 1);
   mpz_mul(check, n_min, n_min);
   mpz_mul_ui(check, check, 2);
   mpz_addmul_ui(check, n_min, 2);
   mpz_add_ui(check, check, 1);
   if (mpz_cmp(check, lo) < 0)
      mpz_add_ui(n_min, n_min, 1);

   mpz_ui_pow_ui(target, 10, d);              /* 10^d */
   mpz_mul_ui(target, target, 2);
   mpz_sub_ui(target, target, 1);
   mpz_sqrt(sq, target);
   mpz_sub_ui(sq, sq, 1);
   mpz_fdiv_q_ui(n_max, sq, 2);

   mpz_clears(target, sq, check, lo, NULL);
}

/* If val = 2m^2+2m+1 for an integer m>=0, store m and return true. */
static bool on_curve(const mpz_t val, mpz_t m, mpz_t s) {
   /* 2*val - 1 must be a perfect square */
   mpz_mul_ui(s, val, 2);
   mpz_sub_ui(s, s, 1);
   if (mpz_sgn(s) < 0) return false;
   if (!mpz_perfect_square_p(s)) return false;
   mpz_sqrt(s, s);                 /* s = sqrt(2val-1) = 2m+1, must be odd */
   if (mpz_even_p(s)) return false;
   mpz_sub_ui(s, s, 1);
   mpz_fdiv_q_ui(m, s, 2);         /* m = (sqrt-1)/2 */
   return true;
}

int main(int argc, char *argv[]) {
   int min_d = (argc > 1) ? atoi(argv[1]) : 13;
   int max_d = (argc > 2) ? atoi(argv[2]) : 17;

   printf("\n  Bi-Quadratic Emirp HUNT (direct brute force)\n");
   printf("  ============================================\n");
   printf("  d = %d .. %d\n\n", min_d, max_d);
   fflush(stdout);

   for (int d = min_d; d <= max_d; d++) {
      mpz_t n_min, n_max;
      mpz_inits(n_min, n_max, NULL);
      compute_n_bounds(d, n_min, n_max);

      /* range as a long for the parallel loop (fits for d<=~20) */
      mpz_t rng;
      mpz_init(rng);
      mpz_sub(rng, n_max, n_min);
      if (!mpz_fits_slong_p(rng)) {
         gmp_printf("  d=%2d: range too large for brute force (%Zd) — skip\n",
                  d, rng);
         mpz_clears(n_min, n_max, rng, NULL);
         continue;
      }
      long range = mpz_get_si(rng) + 1;       /* inclusive */
      mpz_clear(rng);

      long survivors = 0;       /* q on curve (raw) */
      long elig_survivors = 0;  /* q on curve AND p,q prime-eligible (matches sieve) */
      long emirps    = 0;       /* all four conditions */
      double t0 = omp_get_wtime();

      /* collect hit n-values (curve survivors) as strings for reporting */
      char  *hit_buf[MAX_HITS];
      int    hit_kind[MAX_HITS];   /* 1=survivor only, 2=full emirp */
      int    n_hits = 0;

      #pragma omp parallel num_threads(NUM_THREADS)
      {
         mpz_t n, p, q, m, s;
         mpz_inits(n, p, q, m, s, NULL);
         char *pbuf = malloc(d + 2);
         char *qbuf = malloc(d + 2);

         #pragma omp for schedule(dynamic, 100000)
         for (long i = 0; i < range; i++) {
            mpz_add_ui(n, n_min, (unsigned long)i);

            /* p = 2n^2 + 2n + 1 */
            mpz_mul(p, n, n);
            mpz_mul_ui(p, p, 2);
            mpz_addmul_ui(p, n, 2);
            mpz_add_ui(p, p, 1);

            /* q = reverse digits of p */
            mpz_get_str(pbuf, 10, p);
            int len = (int)strlen(pbuf);
            for (int a = 0; a < len; a++)
               qbuf[a] = pbuf[len - 1 - a];
            qbuf[len] = '\0';
            mpz_set_str(q, qbuf, 10);

            /* (1) q on curve? */
            if (!on_curve(q, m, s))
               continue;

            #pragma omp atomic
            survivors++;

            /* prime-eligibility (composite-5) filter: matches the sieve's
             * count. p or q divisible by 5 can never be prime. p,q are odd,
             * so this is just "last digit == 5". */
            bool elig = (mpz_fdiv_ui(p, 5) != 0) && (mpz_fdiv_ui(q, 5) != 0);
            if (elig) {
               #pragma omp atomic
               elig_survivors++;
            }

            /* full test: p,q prime and p != q */
            bool full = (mpz_cmp(p, q) != 0)
                    && mpz_probab_prime_p(p, 40)
                    && mpz_probab_prime_p(q, 40);
            if (full) {
               #pragma omp atomic
               emirps++;
            }

            #pragma omp critical (hits)
            {
               if (n_hits < MAX_HITS) {
                  hit_buf[n_hits] = malloc(d + 4);
                  mpz_get_str(hit_buf[n_hits], 10, n);
                  hit_kind[n_hits] = full ? 2 : (elig ? 1 : 0);
                  n_hits++;
               }
            }
         }
         mpz_clears(n, p, q, m, s, NULL);
         free(pbuf); free(qbuf);
      }

      double dt = omp_get_wtime() - t0;
      printf("  d=%2d  range=%ld  survivors(raw)=%ld  prime-eligible=%ld  EMIRPS=%ld   [%.1fs]\n",
            d, range, survivors, elig_survivors, emirps, dt);
      for (int h = 0; h < n_hits; h++) {
         const char *lbl = hit_kind[h] == 2 ? "*** EMIRP ***"
                     : hit_kind[h] == 1 ? "survivor     "
                     :                    "(p|q div 5)  ";
         printf("        %s  n=%s\n", lbl, hit_buf[h]);
         free(hit_buf[h]);
      }
      fflush(stdout);

      mpz_clears(n_min, n_max, NULL);
   }

   printf("\n  Hunt complete.\n\n");
   return 0;
}
