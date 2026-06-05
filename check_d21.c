/* check_d21.c — Sample and verify d=21 residue processing with timing
 *
 * Tests 100 random n values in the d=21 range to estimate per-residue
 * computation time and extrapolate to full d=21 run time.
 *
 * Build: gcc -O2 -std=c99 -o check_d21 check_d21.c -lgmp
 * Run: ./check_d21
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <gmp.h>

int main(void) {
    printf("\n  21-Digit Bi-Quadratic Residue Timing Analysis\n");
    printf("===============================================\n\n");

    mpz_t n_min, n_max, p, temp, mod, check, lo_bound;
    mpz_init(n_min);
    mpz_init(n_max);
    mpz_init(p);
    mpz_init(temp);
    mpz_init(mod);
    mpz_init(check);
    mpz_init(lo_bound);

    /* Calculate n_min: smallest n where 2n²+2n+1 >= 10^20 */
    mpz_ui_pow_ui(temp, 10, 20);
    mpz_mul_ui(temp, temp, 2);
    mpz_sub_ui(temp, temp, 1);
    mpz_sqrt(n_min, temp);
    mpz_sub_ui(n_min, n_min, 1);
    mpz_fdiv_q_ui(n_min, n_min, 2);
    mpz_add_ui(n_min, n_min, 1);

    /* Verify n_min produces 20+ digits (guard against sqrt rounding) */
    mpz_ui_pow_ui(lo_bound, 10, 19);
    mpz_mul(check, n_min, n_min);
    mpz_mul_ui(check, check, 2);
    mpz_addmul_ui(check, n_min, 2);
    mpz_add_ui(check, check, 1);

    if (mpz_cmp(check, lo_bound) < 0)
        mpz_add_ui(n_min, n_min, 1);

    /* Calculate n_max: largest n where 2n²+2n+1 < 10^21 */
    mpz_ui_pow_ui(temp, 10, 21);
    mpz_mul_ui(temp, temp, 2);
    mpz_sub_ui(temp, temp, 1);
    mpz_sqrt(n_max, temp);
    mpz_sub_ui(n_max, n_max, 1);
    mpz_fdiv_q_ui(n_max, n_max, 2);

    gmp_printf("  n_min: %Zd\n", n_min);
    gmp_printf("  n_max: %Zd\n", n_max);

    /* Range size */
    mpz_sub(temp, n_max, n_min);
    unsigned long range_size = mpz_get_ui(temp);
    printf("  Range size: %lu n values\n\n", range_size);

    /* Set mod = 10^10 for residue testing */
    mpz_ui_pow_ui(mod, 10, 10);

    /* Time 100 random samples */
    printf("  Computing 100 random d=21 residues...\n");

    srand(time(NULL));
    mpz_t n_sample;
    mpz_init(n_sample);

    clock_t start = clock();

    int valid_count = 0;
    for (int i = 0; i < 100; i++) {
        /* Random n in [n_min, n_max] */
        unsigned long offset = rand() % (range_size + 1);
        mpz_set(n_sample, n_min);
        mpz_add_ui(n_sample, n_sample, offset);

        /* Compute p = 2n²+2n+1 */
        mpz_mul(p, n_sample, n_sample);
        mpz_mul_ui(p, p, 2);
        mpz_addmul_ui(p, n_sample, 2);
        mpz_add_ui(p, p, 1);

        /* Verify 21 digits */
        long p_digits = mpz_sizeinbase(p, 10);
        if (p_digits == 21)
            valid_count++;

        /* Compute p mod 10^10 */
        mpz_mod(temp, p, mod);
    }

    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;

    printf("  Valid d=21 residues: %d / 100\n", valid_count);
    printf("  Time for 100 residues: %.4f seconds\n", elapsed);
    printf("  Average per residue: %.6f seconds\n\n", elapsed / 100.0);

    /* Extrapolate to 10 billion residues */
    double per_residue = elapsed / 100.0;
    double total_seconds = per_residue * 10000000000.0;
    double total_hours = total_seconds / 3600.0;
    double total_days = total_hours / 24.0;

    printf("  EXTRAPOLATION (10B residues):\n");
    printf("  ==============================\n");
    printf("  Per-residue time: %.6f sec\n", per_residue);
    printf("  Total time: %.1f seconds\n", total_seconds);
    printf("  Total time: %.1f hours\n", total_hours);
    printf("  Total time: %.2f days\n\n", total_days);

    printf("  NOTE: This is GMP arithmetic only (no filter, no p/q checks).\n");
    printf("  Actual mod_obstruct d=21 will be faster (~40% residues skipped by div-5 filter).\n");
    printf("===============================================\n\n");

    mpz_clear(n_min);
    mpz_clear(n_max);
    mpz_clear(p);
    mpz_clear(temp);
    mpz_clear(mod);
    mpz_clear(check);
    mpz_clear(lo_bound);
    mpz_clear(n_sample);

    return 0;
}
