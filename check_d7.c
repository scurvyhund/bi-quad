/* check_d7.c — Enumerate all 7-digit bi-quadratic emirp candidates.
 * Shows every n where p=2n²+2n+1 has 7 digits and rev(p) is also
 * of the form 2m²+2m+1.
 *
 * Build: gcc -O2 -std=c99 -o check_d7 check_d7.c -lgmp
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <gmp.h>

#include "curve_gmp.h"   /* on_curve / is_consec_sq, reverse_str */

int main(void)
{
   printf("\n  7-Digit Bi-Quadratic Emirp Candidate Enumeration\n");
   printf("====================================================\n\n");

   /* For d=7: 1000000 <= 2n²+2n+1 <= 9999999.
    * Both ends are computed. n_min used to be hardcoded to 710 with a
    * guard (n_min == 0) that could never fire, so the true first
    * 7-digit n = 707 and its two successors were never enumerated. */
   int n_min = 0, n_max = 0;
   for (int n = 0; n < 3162; n++) {
      long p = 2L * n * n + 2 * n + 1;
      if (p >= 1000000 && n_min == 0) n_min = n;
      if (p <= 9999999) n_max = n;
   }

   if (n_min == 0 || n_max == 0) {
      fprintf(stderr, "error: could not bracket the 7-digit n range\n");
      return 1;
   }

   printf("  n range for 7-digit p: [%d, %d]\n", n_min, n_max);
   printf("  Total n values: %d\n\n", n_max - n_min + 1);

   mpz_t q_mpz, m_out;
   mpz_init(q_mpz);
   mpz_init(m_out);

   int matches = 0;

   printf("  %-6s  %-8s  %-8s  %-6s  %s\n",
         "n", "p", "rev(p)", "m", "Status");
   printf("  %-6s  %-8s  %-8s  %-6s  %s\n",
         "------", "--------", "--------", "------", "----------");

   for (int n = n_min; n <= n_max; n++) {
      long p = 2L * n * n + 2 * n + 1;

      char p_str[32], rev_str[32];
      sprintf(p_str, "%ld", p);
      int len = strlen(p_str);
      reverse_str(p_str, rev_str, len);

      /* Skip if rev(p) has leading zero (would be fewer digits) */
      if (rev_str[0] == '0')
         continue;

      long q = atol(rev_str);
      mpz_set_ui(q_mpz, q);

      /* Skip palindromes (p == q) */
      if (p == q)
         continue;

      if (is_consec_sq(q_mpz, m_out)) {
         long m = mpz_get_ui(m_out);
         bool q_prime = mpz_probab_prime_p(q_mpz, 25) > 0;

         mpz_t p_mpz;
         mpz_init_set_ui(p_mpz, p);
         bool p_prime = mpz_probab_prime_p(p_mpz, 25) > 0;
         mpz_clear(p_mpz);

         const char *status;
         if (p_prime && q_prime)
            status = "BOTH PRIME — CONVERSE PAIR!";
         else if (p_prime)
            status = "p prime, rev(p) composite";
         else if (q_prime)
            status = "p composite, rev(p) prime";
         else
            status = "both composite";

         printf("  n=%-6d  p=%-9ld  q=%-9ld  m=%-4ld  %s\n",
               n, p, q, m, status);
         matches++;
      }
   }

   printf("\n  Total structural matches (both consec-sq): %d\n", matches);
   printf("====================================================\n\n");

   mpz_clear(q_mpz);
   mpz_clear(m_out);

   return 0;
}
