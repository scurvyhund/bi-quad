/*
 * palhunt.c — Find PRIME PALINDROMES on the curve p = 2n^2+2n+1 (= n^2+(n+1)^2).
 *
 * Every such p is ≡ 1 (mod 4) — a "4n+1 prime". This enumerates n, forms p,
 * keeps the ones whose decimal digits are a palindrome, and tests primality
 * (deterministic Miller-Rabin, exact for all 64-bit p). Reports every hit.
 *
 * 64-bit: p < 2^64 requires n < ~3.03e9 (p up to ~19-20 digits).
 *
 * Build: gcc palhunt.c -o palhunt -O3 -march=znver2 -std=c99 -Wall -fopenmp
 * Usage: ./palhunt [max_n]      (default 3000000000)
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <omp.h>

#define NUM_THREADS 8

/* 64-bit modular multiply via 128-bit */
static inline uint64_t mulmod(uint64_t a, uint64_t b, uint64_t m) {
    return (uint64_t)(((unsigned __int128)a * b) % m);
}
static uint64_t powmod(uint64_t a, uint64_t e, uint64_t m) {
    uint64_t r = 1; a %= m;
    while (e) { if (e & 1) r = mulmod(r, a, m); a = mulmod(a, a, m); e >>= 1; }
    return r;
}
/* deterministic Miller-Rabin, exact for all n < 2^64 */
static bool is_prime(uint64_t n) {
    if (n < 2) return false;
    for (uint64_t p = 2; p < 40; p++) {
        if (p*p > n) return true;
        if (n % p == 0) return n == p;
    }
    uint64_t d = n - 1; int r = 0;
    while ((d & 1) == 0) { d >>= 1; r++; }
    static const uint64_t bases[] = {2,3,5,7,11,13,17,19,23,29,31,37};
    for (int i = 0; i < 12; i++) {
        uint64_t a = bases[i] % n;
        if (a == 0) continue;
        uint64_t x = powmod(a, d, n);
        if (x == 1 || x == n - 1) continue;
        bool comp = true;
        for (int j = 0; j < r - 1; j++) {
            x = mulmod(x, x, n);
            if (x == n - 1) { comp = false; break; }
        }
        if (comp) return false;
    }
    return true;
}

static inline bool is_palindrome(uint64_t x) {
    uint64_t r = 0, t = x;
    while (t) { r = r * 10 + t % 10; t /= 10; }
    return r == x;
}

int main(int argc, char *argv[]) {
    uint64_t max_n = (argc > 1) ? strtoull(argv[1], NULL, 10) : 3000000000ULL;

    printf("\n  PRIME PALINDROMES on p = 2n^2+2n+1  (n <= %llu)\n",
           (unsigned long long)max_n);
    printf("  ===============================================\n\n");
    fflush(stdout);

    double t0 = omp_get_wtime();
    long count = 0;

    #pragma omp parallel for schedule(dynamic, 1000000) num_threads(NUM_THREADS)
    for (uint64_t n = 1; n <= max_n; n++) {
        uint64_t p = 2*n*n + 2*n + 1;       /* fits in u64 for n < ~3.03e9 */
        if (!is_palindrome(p)) continue;
        if (!is_prime(p)) continue;
        #pragma omp atomic
        count++;
        #pragma omp critical
        {
            int d = 0; uint64_t t = p; while (t) { d++; t /= 10; }
            printf("  n=%-11llu  p=%llu  (%d digits)\n",
                   (unsigned long long)n, (unsigned long long)p, d);
            fflush(stdout);
        }
    }

    printf("\n  found %ld prime palindromes on the curve  [%.1fs]\n\n",
           count, omp_get_wtime() - t0);
    return 0;
}
