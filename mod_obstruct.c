/* mod_obstruct.c — Modular obstruction search for bi-quadratic emirps
 *
 * For each digit count d and depth k, determines whether a d-digit
 * converse prime pair (p, rev(p)) with both p and rev(p) of the form
 * 2n² + 2n + 1 is structurally possible, based on modular constraints
 * on the first and last k digits.
 *
 * Build:  make
 * Usage:  ./mod_obstruct [max_d] [max_k]
 *         defaults: max_d=50, max_k=6
 *
 * Algorithm:
 *   For p = 2n²+2n+1 and q = rev(p) = 2m²+2m+1:
 *   - last k digits of p  → determined by n mod 10^k
 *   - first k digits of q = reverse(last k of p) → must be achievable
 *   - last k digits of q  = reverse(first k of p) → must be valid ending
 *   - first k digits of p → determined by magnitude of n
 *
 *   Both the p-side and q-side constraints are checked.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <omp.h>
#include <gmp.h>

#define DEFAULT_MAX_D  50
#define DEFAULT_MAX_K   6
#define NUM_THREADS     8

/* Reverse a k-digit number (leading zeros preserved in digit sense). */
static long reverse_k(long val, int k)
{
    long result = 0;
    for (int i = 0; i < k; i++) {
        result = result * 10 + (val % 10);
        val /= 10;
    }
    return result;
}

/* Compare function for qsort/bsearch on longs */
static int cmp_long(const void *a, const void *b)
{
    long la = *(const long *)a;
    long lb = *(const long *)b;
    return (la > lb) - (la < lb);
}

/* Check if sorted array contains any value in [lo, hi] */
static bool sorted_has_value_in_range(const long *arr, int len,
                                      long lo, long hi)
{
    if (len == 0 || lo > hi)
        return false;

    /* Binary search for first element >= lo */
    int left = 0, right = len;
    while (left < right) {
        int mid = left + (right - left) / 2;
        if (arr[mid] < lo)
            left = mid + 1;
        else
            right = mid;
    }
    return (left < len && arr[left] <= hi);
}

/* Compute n_min, n_max for d-digit numbers of the form 2n²+2n+1. */
static void compute_n_bounds(int d, mpz_t n_min, mpz_t n_max)
{
    mpz_t target, sq;
    mpz_init(target);
    mpz_init(sq);

    /* n_min: solve 2n²+2n+1 >= 10^(d-1) */
    mpz_ui_pow_ui(target, 10, d - 1);
    mpz_mul_ui(target, target, 2);
    mpz_sub_ui(target, target, 1);
    mpz_sqrt(sq, target);
    mpz_sub_ui(sq, sq, 1);
    mpz_fdiv_q_ui(n_min, sq, 2);
    mpz_add_ui(n_min, n_min, 1);

    /* Verify n_min produces d digits (guards against sqrt rounding) */
    mpz_t check, lo_bound;
    mpz_init(check);
    mpz_init(lo_bound);
    mpz_ui_pow_ui(lo_bound, 10, d - 1);
    mpz_mul(check, n_min, n_min);
    mpz_mul_ui(check, check, 2);
    mpz_addmul_ui(check, n_min, 2);
    mpz_add_ui(check, check, 1);
    if (mpz_cmp(check, lo_bound) < 0)
        mpz_add_ui(n_min, n_min, 1);
    mpz_clear(check);
    mpz_clear(lo_bound);

    /* n_max: solve 2n²+2n+1 < 10^d */
    mpz_ui_pow_ui(target, 10, d);
    mpz_mul_ui(target, target, 2);
    mpz_sub_ui(target, target, 1);
    mpz_sqrt(sq, target);
    mpz_sub_ui(sq, sq, 1);
    mpz_fdiv_q_ui(n_max, sq, 2);

    mpz_clear(target);
    mpz_clear(sq);
}

/* Compute first-k-digit prefix of p = 2n²+2n+1 for given n.
 * Returns floor(p / 10^(d-k)).
 */
static long compute_first_k(mpz_t n, int d, int k, mpz_t tmp_p, mpz_t tmp_pow)
{
    /* p = 2n² + 2n + 1 */
    mpz_mul(tmp_p, n, n);
    mpz_mul_ui(tmp_p, tmp_p, 2);
    mpz_addmul_ui(tmp_p, n, 2);
    mpz_add_ui(tmp_p, tmp_p, 1);

    /* divide by 10^(d-k) */
    mpz_ui_pow_ui(tmp_pow, 10, d - k);
    mpz_tdiv_q(tmp_p, tmp_p, tmp_pow);

    return mpz_get_ui(tmp_p);
}

/* Find first n >= lo with n ≡ r (mod stride) */
static void first_n_with_residue(mpz_t result, mpz_t lo,
                                 long r, long stride)
{
    unsigned long lo_mod = mpz_fdiv_ui(lo, (unsigned long)stride);
    long gap = (((long)r - (long)lo_mod) % stride + stride) % stride;
    mpz_add_ui(result, lo, (unsigned long)gap);
}

/* Find last n <= hi with n ≡ r (mod stride) */
static void last_n_with_residue(mpz_t result, mpz_t hi,
                                long r, long stride)
{
    unsigned long hi_mod = mpz_fdiv_ui(hi, (unsigned long)stride);
    long gap = (((long)hi_mod - (long)r) % stride + stride) % stride;
    mpz_sub_ui(result, hi, (unsigned long)gap);
}

/* Solve 2m² + 2m + 1 ≡ target (mod 10^k) via Hensel lifting.
 * Returns number of solutions stored in sols[].
 * Max solutions bounded by ~4 * 2^(k-1).
 */
#define MAX_HENSEL_SOLS 4096
static int solve_residues(long target, int k, long *sols)
{
    long tmp[MAX_HENSEL_SOLS];
    int count = 0;

    /* Base: solutions mod 10 */
    long t10 = target % 10;
    for (long r = 0; r < 10; r++) {
        if ((2*r*r + 2*r + 1) % 10 == t10)
            sols[count++] = r;
    }

    /* Hensel lift: mod 10^j → mod 10^(j+1) */
    long pow10j = 10;
    for (int j = 1; j < k; j++) {
        long next_mod = pow10j * 10;
        long target_next = target % next_mod;
        int new_count = 0;

        for (int i = 0; i < count; i++) {
            long r = sols[i];
            for (long c = 0; c < 10; c++) {
                long r_new = r + c * pow10j;
                long long rn = (long long)r_new;
                long long fr = (2*rn*rn + 2*rn + 1) % (long long)next_mod;
                if (fr == target_next && new_count < MAX_HENSEL_SOLS)
                    tmp[new_count++] = r_new;
            }
        }

        /* Swap: copy tmp → sols */
        for (int i = 0; i < new_count; i++)
            sols[i] = tmp[i];
        count = new_count;
        pow10j = next_mod;
    }

    return count;
}

int main(int argc, char *argv[])
{
    int max_d = (argc > 1) ? atoi(argv[1]) : DEFAULT_MAX_D;
    int max_k = (argc > 2) ? atoi(argv[2]) : DEFAULT_MAX_K;

    printf("\n");
    printf("  Modular Obstruction Search for Bi-Quadratic Emirps\n");
    printf("=====================================================\n");
    printf("  max_d = %d    max_k = %d\n", max_d, max_k);
    printf("=====================================================\n\n");

    for (int k = 3; k <= max_k; k++) {
        long mod = 1;
        for (int i = 0; i < k; i++)
            mod *= 10;

        long prefix_min = mod / 10;   /* 10^(k-1) */

        /* Phase 1: Build VALID_ENDINGS and reverse lookup.
         * For each valid ending, store which residues produce it. */
        bool *is_valid_ending = calloc(mod, sizeof(bool));
        long *endings = malloc(mod * sizeof(long));  /* ending for each r */
        int num_endings = 0;

        for (long n = 0; n < mod; n++) {
            unsigned long long nn = (unsigned long long)n;
            long e = (long)((2*nn*nn + 2*nn + 1) % (unsigned long long)mod);
            endings[n] = e;
            if (!is_valid_ending[e]) {
                is_valid_ending[e] = true;
                num_endings++;
            }
        }

        /* Phase 2: Build VALID_FIRSTS as a sorted array for fast
         * range queries. A first-k prefix f is valid if reverse_k(f)
         * is a valid ending (ensures last-k of q is achievable). */
        long *valid_firsts = malloc(mod * sizeof(long));
        int num_firsts = 0;
        bool *is_valid_first = calloc(mod, sizeof(bool));

        for (long e = 0; e < mod; e++) {
            if (!is_valid_ending[e])
                continue;
            long f = reverse_k(e, k);
            if (f >= prefix_min && !is_valid_first[f]) {
                is_valid_first[f] = true;
                valid_firsts[num_firsts++] = f;
            }
        }
        qsort(valid_firsts, num_firsts, sizeof(long), cmp_long);

        printf("  k=%d: valid_endings = %d / %ld (%.2f%%)  "
               "valid_firsts = %d\n",
               k, num_endings, mod,
               100.0 * num_endings / mod, num_firsts);
        printf("  ---------------------------------------------------\n");

        /* Phase 3: For each digit count d, check feasibility.
         *
         * For each residue r (p-side):
         *   1. Compute range of achievable first-k prefixes for p
         *   2. Check if any valid first falls in that range (p-side pass)
         *   3. For each matching first-k prefix f:
         *      - q's last-k = reverse_k(f) → find m residues producing this
         *      - q's first-k = reverse_k(ending_of_p)
         *      - Check if any such m achieves that first-k prefix (q-side pass)
         *
         * The residue loop is parallelized with OpenMP. Each thread
         * gets its own GMP variables to avoid sharing.
         */
        mpz_t n_min, n_max;
        mpz_init(n_min);
        mpz_init(n_max);

        omp_set_num_threads(NUM_THREADS);
        int obstruction_count = 0;

        for (int d = k + 1; d <= max_d; d++) {

            compute_n_bounds(d, n_min, n_max);

            int survivors = 0;

            #pragma omp parallel reduction(+:survivors)
            {
                /* Per-thread GMP variables */
                mpz_t t_first, t_last, t_p, t_pow, t_mfirst, t_mlast;
                mpz_init(t_first);
                mpz_init(t_last);
                mpz_init(t_p);
                mpz_init(t_pow);
                mpz_init(t_mfirst);
                mpz_init(t_mlast);

                #pragma omp for schedule(dynamic, 1024)
                for (long r = 0; r < mod; r++) {
                    long p_ending = endings[r];

                    /* First n ≡ r (mod 10^k) in [n_min, n_max] */
                    first_n_with_residue(t_first, n_min, r, mod);
                    if (mpz_cmp(t_first, n_max) > 0)
                        continue;

                    /* Last n ≡ r (mod 10^k) in [n_min, n_max] */
                    last_n_with_residue(t_last, n_max, r, mod);

                    /* Compute range of first-k prefixes achievable by p */
                    long fk_min = compute_first_k(t_first, d, k, t_p, t_pow);
                    long fk_max = compute_first_k(t_last, d, k, t_p, t_pow);

                    /* Check p-side: any valid first in [fk_min, fk_max]? */
                    if (!sorted_has_value_in_range(valid_firsts, num_firsts,
                                                   fk_min, fk_max))
                        continue;

                    /* P-side passed. Now check q-side for each valid first
                     * in [fk_min, fk_max]. */
                    bool q_ok = false;

                    /* Find valid firsts in [fk_min, fk_max] */
                    int left = 0, right = num_firsts;
                    while (left < right) {
                        int mid = left + (right - left) / 2;
                        if (valid_firsts[mid] < fk_min)
                            left = mid + 1;
                        else
                            right = mid;
                    }

                    for (int fi = left;
                         fi < num_firsts && valid_firsts[fi] <= fk_max;
                         fi++) {
                        long f = valid_firsts[fi];

                        /* q's last-k digits = reverse_k(f) */
                        long q_ending = reverse_k(f, k);

                        /* q's first-k digits = reverse_k(p_ending) */
                        long q_first = reverse_k(p_ending, k);

                        /* q_first must be a valid k-digit prefix */
                        if (q_first < prefix_min)
                            continue;

                        if (!is_valid_ending[q_ending])
                            continue;

                        /* Solve for m residues on the fly */
                        long m_sols[MAX_HENSEL_SOLS];
                        int qe_count = solve_residues(q_ending, k, m_sols);

                        for (int mi = 0; mi < qe_count; mi++) {
                            long m_r = m_sols[mi];

                            first_n_with_residue(t_mfirst, n_min, m_r, mod);
                            if (mpz_cmp(t_mfirst, n_max) <= 0) {
                                last_n_with_residue(t_mlast, n_max, m_r, mod);

                                long mk_min = compute_first_k(t_mfirst, d, k,
                                                              t_p, t_pow);
                                long mk_max = compute_first_k(t_mlast, d, k,
                                                              t_p, t_pow);

                                if (q_first >= mk_min && q_first <= mk_max) {
                                    q_ok = true;
                                }
                            }

                            if (q_ok)
                                break;
                        }

                        if (q_ok)
                            break;
                    }

                    if (q_ok)
                        survivors++;
                }

                mpz_clear(t_first);
                mpz_clear(t_last);
                mpz_clear(t_p);
                mpz_clear(t_pow);
                mpz_clear(t_mfirst);
                mpz_clear(t_mlast);
            }

            const char *tag = "";
            if (survivors == 0) {
                tag = "  *** OBSTRUCTION ***";
                obstruction_count++;
            } else if (survivors == mod) {
                tag = "  (saturated)";
            }

            printf("    d=%2d  survivors = %6d%s\n", d, survivors, tag);
            fflush(stdout);

            /* If fully saturated, all larger d will also saturate — skip */
            if (survivors == mod) {
                printf("    d=%2d..%2d  (skipped — saturated)\n",
                       d + 1, max_d);
                break;
            }
        }

        printf("\n  k=%d summary: %d obstructions found out of %d "
               "digit counts tested\n\n",
               k, obstruction_count, max_d - k);

        free(is_valid_ending);
        free(is_valid_first);
        free(endings);
        free(valid_firsts);

        mpz_clear(n_min);
        mpz_clear(n_max);
    }

    printf("=====================================================\n");
    printf("  Search complete.\n");
    printf("=====================================================\n\n");

    return 0;
}
