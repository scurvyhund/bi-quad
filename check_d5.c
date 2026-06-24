/* check_d5.c — Enumerate all 5-digit bi-quadratic emirp candidates.
 * Shows every n where p=2n²+2n+1 has 5 digits and rev(p) is also
 * of the form 2m²+2m+1.
 *
 * Build: gcc -O2 -std=c99 -o check_d5 check_d5.c -lgmp
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <gmp.h>

/* Reverse a decimal string */
static void reverse_str(const char *src, char *dst, int len)
{
   for (int i = 0; i < len; i++)
      dst[i] = src[len - 1 - i];
   dst[len] = '\0';
}

/* Check if q = 2m²+2m+1 for some integer m >= 0.
 * Requires 2q-1 to be a perfect square, then m = (sqrt(2q-1) - 1) / 2. */
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
   printf("\n  5-Digit Bi-Quadratic Emirp Candidate Enumeration\n");
   printf("====================================================\n\n");

   /* For d=5: 10000 <= 2n²+2n+1 <= 99999
    * n_min = 70 (2·70²+2·70+1 = 9941 → 4 digits, so 71)
    * Let's compute precisely */
   int n_min = 0, n_max = 0;
   for (int n = 0; n < 1000; n++) {
      long p = 2L * n * n + 2 * n + 1;
      if (p >= 10000 && n_min == 0) n_min = n;
      if (p <= 99999) n_max = n;
   }

   printf("  n range for 5-digit p: [%d, %d]\n", n_min, n_max);
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

      /* Skip palindromes */
      if (strcmp(p_str, rev_str) == 0)
         continue;

      /* Skip if rev(p) has leading zero (would be fewer digits) */
      if (rev_str[0] == '0')
         continue;

      long q = atol(rev_str);
      mpz_set_ui(q_mpz, q);

      if (is_consec_sq(q_mpz, m_out)) {
         long m = mpz_get_ui(m_out);
         bool p_prime = mpz_probab_prime_p(q_mpz, 25) > 0;

         mpz_t p_mpz;
         mpz_init_set_ui(p_mpz, p);
         bool p_is_prime = mpz_probab_prime_p(p_mpz, 25) > 0;
         mpz_clear(p_mpz);

         const char *status;
         if (p_is_prime && p_prime)
            status = "BOTH PRIME — CONVERSE PAIR!";
         else if (p_is_prime)
            status = "p prime, rev(p) composite";
         else if (p_prime)
            status = "p composite, rev(p) prime";
         else
            status = "both composite";

         printf("  n=%-4d  p=%-7ld  q=%-7ld  m=%-4ld  %s\n",
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
