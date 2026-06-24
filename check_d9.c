/* check_d9.c — Enumerate all 9-digit bi-quadratic emirp candidates.
 *
 * Build: gcc -O2 -std=c99 -o check_d9 check_d9.c -lgmp
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <gmp.h>

static void reverse_str(const char *src, char *dst, int len)
{
   for (int i = 0; i < len; i++)
      dst[i] = src[len - 1 - i];
   dst[len] = '\0';
}

static bool is_consec_sq(mpz_t q, mpz_t m_out)
{
   mpz_t disc;
   mpz_init(disc);

   mpz_mul_ui(disc, q, 2);
   mpz_sub_ui(disc, disc, 1);

   if (mpz_perfect_square_p(disc)) {
      mpz_sqrt(disc, disc);
      mpz_sub_ui(disc, disc, 1);
      if (mpz_divisible_ui_p(disc, 2)) {
         mpz_divexact_ui(m_out, disc, 2);
         mpz_clear(disc);
         return true;
      }
   }

   mpz_clear(disc);
   return false;
}

int main(void)
{
   printf("\n  9-Digit Bi-Quadratic Emirp Candidate Enumeration\n");
   printf("====================================================\n\n");

   /* Compute n range for d=9 */
   mpz_t n_mpz, p_mpz, q_mpz, m_out;
   mpz_init(n_mpz);
   mpz_init(p_mpz);
   mpz_init(q_mpz);
   mpz_init(m_out);

   /* n_min: smallest n where 2n²+2n+1 >= 10^8 */
   long n_min = 0, n_max = 0;
   for (long n = 7000; n < 8000; n++) {
      long p = 2L * n * n + 2 * n + 1;
      if (p >= 100000000L) { n_min = n; break; }
   }
   for (long n = 22400; n > 22000; n--) {
      long p = 2L * n * n + 2 * n + 1;
      if (p <= 999999999L) { n_max = n; break; }
   }

   printf("  n range for 9-digit p: [%ld, %ld]\n", n_min, n_max);
   printf("  Total n values: %ld\n\n", n_max - n_min + 1);

   int matches = 0;

   printf("  %-8s  %-12s  %-12s  %-8s  %s\n",
         "n", "p", "rev(p)", "m", "Status");
   printf("  %-8s  %-12s  %-12s  %-8s  %s\n",
         "--------", "------------", "------------",
         "--------", "-------------------------");

   for (long n = n_min; n <= n_max; n++) {
      long p = 2L * n * n + 2 * n + 1;

      char p_str[32], rev[32];
      sprintf(p_str, "%ld", p);
      int len = strlen(p_str);
      reverse_str(p_str, rev, len);

      if (strcmp(p_str, rev) == 0)
         continue;
      if (rev[0] == '0')
         continue;

      long q = atol(rev);
      mpz_set_ui(q_mpz, q);

      if (is_consec_sq(q_mpz, m_out)) {
         long m = mpz_get_ui(m_out);

         mpz_set_ui(p_mpz, p);
         bool p_prime = mpz_probab_prime_p(p_mpz, 25) > 0;
         bool q_prime = mpz_probab_prime_p(q_mpz, 25) > 0;

         const char *status;
         if (p_prime && q_prime)
            status = "BOTH PRIME — CONVERSE PAIR!";
         else if (p_prime)
            status = "p prime, rev(p) composite";
         else if (q_prime)
            status = "p composite, rev(p) prime";
         else
            status = "both composite";

         printf("  n=%-6ld  p=%-10ld  q=%-10ld  m=%-6ld  %s\n",
               n, p, q, m, status);
         matches++;
      }
   }

   printf("\n  Total structural matches (both consec-sq): %d\n", matches);
   printf("====================================================\n\n");

   mpz_clear(n_mpz);
   mpz_clear(p_mpz);
   mpz_clear(q_mpz);
   mpz_clear(m_out);

   return 0;
}
