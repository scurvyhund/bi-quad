/* check_survivors.c — Enumerate all bi-quadratic emirp candidates
 * for a given digit count d.
 *
 * NOTE (2026-06-05): this counts NON-PALINDROME converse candidates — it skips
 * palindromes (p==rev(p)) at the strcmp below. That is the right metric for emirp
 * hunting, but it differs from hunt.c's "survivors(raw)", which ALSO counts the
 * palindromic case. e.g. d=13: this prints 2 (non-pal); hunt raw = 4 (= 2 + 2
 * palindromes). BOTH are correct. Only real limit: 64-bit `long` overflows at
 * d>=19 — use hunt.c (GMP) past there. See docs/session_2026-06-05_emirp_d5_correction.md.
 *
 * Build: gcc -O2 -std=c99 -o check_survivors check_survivors.c -lgmp
 * Usage: ./check_survivors <d>
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

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <digit_count>\n", argv[0]);
        return 1;
    }

    int d = atoi(argv[1]);
    if (d < 2 || d > 18) {
        fprintf(stderr, "d must be 2..18 (long arithmetic range)\n");
        return 1;
    }

    printf("\n  %d-Digit Bi-Quadratic Emirp Candidate Enumeration\n", d);
    printf("====================================================\n\n");

    long lo_bound = 1;
    for (int i = 0; i < d - 1; i++) lo_bound *= 10;
    long hi_bound = lo_bound * 10 - 1;

    /* Find n range */
    long n_min = 0, n_max = 0;
    for (long n = 1; ; n++) {
        long p = 2L * n * n + 2 * n + 1;
        if (p >= lo_bound) { n_min = n; break; }
    }
    for (long n = n_min; ; n++) {
        long p = 2L * n * n + 2 * n + 1;
        if (p > hi_bound) { n_max = n - 1; break; }
    }

    printf("  n range for %d-digit p: [%ld, %ld]\n", d, n_min, n_max);
    printf("  Total n values: %ld\n\n", n_max - n_min + 1);

    mpz_t p_mpz, q_mpz, m_out;
    mpz_init(p_mpz);
    mpz_init(q_mpz);
    mpz_init(m_out);

    int matches = 0;
    int both_prime = 0;

    printf("  %-8s  %-*s  %-*s  %-8s  %s\n",
           "n", d + 2, "p", d + 2, "rev(p)", "m", "Status");
    printf("  %-8s  %-*s  %-*s  %-8s  %s\n",
           "--------", d + 2, "------------", d + 2, "------------",
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
            if (p_prime && q_prime) {
                status = "BOTH PRIME — CONVERSE PAIR!";
                both_prime++;
            } else if (p_prime)
                status = "p prime, rev(p) composite";
            else if (q_prime)
                status = "p composite, rev(p) prime";
            else
                status = "both composite";

            printf("  n=%-6ld  p=%-*ld  q=%-*ld  m=%-6ld  %s\n",
                   n, d + 2, p, d + 2, q, m, status);
            matches++;
        }
    }

    printf("\n  Total structural matches (both consec-sq): %d\n", matches);
    printf("  Converse pairs found: %d\n", both_prime);
    printf("====================================================\n\n");

    mpz_clear(p_mpz);
    mpz_clear(q_mpz);
    mpz_clear(m_out);

    return 0;
}
