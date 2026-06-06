/*
 * palhunt_opt.c — OPTIMIZED palhunt_gmp.c (bi-quad experiment, 2026-06-06).
 *
 * Identical math to palhunt_gmp.c, but the divide-bound inner loop is fixed:
 * the original computed p = curve(n) and then `p % 10` (a 128-bit __udivti3) on
 * EVERY iteration just to get the last digit. But the last digit of
 * p = 2n^2+2n+1 depends only on n mod 5:
 *
 *     n mod 5 :  0  1  2  3  4
 *     last(p) :  1  5  3  5  1
 *
 * So we keep n mod 5 with cheap 64-bit arithmetic (one u128%5 per d, outside the
 * loop), skip n ≡ 1,3 (last digit 5 -> composite) WITHOUT computing p, and never
 * do a 128-bit modulo in the hot path. This is the digit-ending "proof by
 * construction" applied to the brute inner loop.
 *
 * Build: gcc palhunt_opt.c -o palhunt_opt_c -O3 -march=znver2 -std=c99 -Wall -fopenmp -lgmp
 * Usage: ./palhunt_opt_c [min_d] [max_d]
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <omp.h>
#include <gmp.h>

typedef unsigned __int128 u128;
#define NT 8

static u128 ipow10(int e){ u128 r=1; while(e-->0) r*=10; return r; }
static u128 isqrt128(u128 x){
    if(x<2) return x;
    u128 lo=1, hi=((u128)1<<64);
    while(lo<hi){ u128 m=lo+(hi-lo+1)/2; if(m<=x/m) lo=m; else hi=m-1; }
    return lo;
}
static u128 curve(u128 n){ return 2*n*n + 2*n + 1; }
static int  is_pal(u128 x){ u128 r=0,t=x; while(t){ r=r*10+(int)(t%10); t/=10; } return r==x; }
static void to_mpz(mpz_t z,u128 x){
    uint64_t hi=(uint64_t)(x>>64), lo=(uint64_t)x;
    mpz_set_ui(z,hi); mpz_mul_2exp(z,z,64); mpz_add_ui(z,z,lo);
}

int main(int argc,char**argv){
    int dmin = argc>1?atoi(argv[1]):1, dmax = argc>2?atoi(argv[2]):7;
    printf("\n  PRIME PALINDROMES on p = 2n^2+2n+1   (d=%d..%d, GMP-certified) [opt-c]\n", dmin,dmax);
    printf("  =================================================================\n");
    for(int d=dmin; d<=dmax; d++){
        u128 L=ipow10(d-1), H=ipow10(d), P1=ipow10(d-1);
        long double P1ld=(long double)P1;
        u128 s=isqrt128(2*L-1); u128 nmin=(s>0)?(s-1)/2:0;
        while(curve(nmin)<L) nmin++;
        while(nmin>0&&curve(nmin-1)>=L) nmin--;
        s=isqrt128(2*H-1); u128 nmax=(s-1)/2;
        while(curve(nmax)>=H) nmax--;
        while(curve(nmax+1)<H) nmax++;
        unsigned long long range=(unsigned long long)(nmax-nmin)+1;
        int nmin_mod5=(int)(nmin%5);              /* one u128%5, outside the loop */
        double t0=omp_get_wtime(); long found=0;

        #pragma omp parallel num_threads(NT)
        {
            mpz_t z; mpz_init(z);
            #pragma omp for schedule(dynamic,1000000)
            for(unsigned long long i=0;i<range;i++){
                unsigned r=(unsigned)((nmin_mod5+i)%5); /* cheap 64-bit, no __udivti3 */
                if(r==1 || r==3) continue;              /* last digit 5 -> composite */
                int last=(r==2)?3:1;                    /* r in {0,2,4} -> 1,3,1 */
                u128 p = curve((u128)nmin + i);
                int first=(int)((long double)p / P1ld); /* fast exact leading digit */
                if(first!=last) continue;               /* palindrome needs first==last */
                if(!is_pal(p)) continue;
                to_mpz(z,p);
                if(mpz_probab_prime_p(z,40)){
                    #pragma omp atomic
                    found++;
                    #pragma omp critical
                    { gmp_printf("   *** PRIME PALINDROME ***  d=%d  n=%llu  p=%Zd\n",
                                 d,(unsigned long long)((u128)nmin+i),z); fflush(stdout); }
                }
            }
            mpz_clear(z);
        }
        printf("   d=%2d  range=%llu  found=%ld   [%.1fs]\n",
               d,range,found,omp_get_wtime()-t0);
        fflush(stdout);
    }
    printf("\n");
    return 0;
}
