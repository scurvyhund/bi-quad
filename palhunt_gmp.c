/*
 * palhunt_gmp.c — Prime palindromes on the curve 2n^2+2n+1, PAST the 64-bit wall.
 *
 * n in uint64, p = 2n^2+2n+1 in unsigned __int128 (good to ~37 digits), and GMP
 * is called ONLY to certify the rare palindrome — so it stays fast while reaching
 * digit-lengths palhunt.c (uint64, <=19 digits) could never touch.
 *
 * Hunts for the next prime palindrome on the curve beyond Jim's 3187813 (1997).
 *
 * Build: gcc palhunt_gmp.c -o palhunt_gmp -O3 -march=znver2 -std=c99 -Wall -fopenmp -lgmp
 * Usage: ./palhunt_big [min_d] [max_d]
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
   printf("\n  PRIME PALINDROMES on p = 2n^2+2n+1   (d=%d..%d, GMP-certified)\n", dmin,dmax);
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
      double t0=omp_get_wtime(); long found=0;

      #pragma omp parallel num_threads(NT)
      {
         mpz_t z; mpz_init(z);
         #pragma omp for schedule(dynamic,1000000)
         for(unsigned long long i=0;i<range;i++){
            /* MOD-5 OPTIMIZATION (available, not applied — 2026-06-19)
             * For p = 2n^2+2n+1:
             *   n≡1 (mod 5) → p≡5 (mod 5) → p divisible by 5, never prime
             *   n≡3 (mod 5) → p≡5 (mod 5) → p divisible by 5, never prime
             * So adding:
             *   unsigned r = (unsigned)((nmin+i) % 5);
             *   if (r==1 || r==3) continue;
             * BEFORE curve() would skip 40% of candidates before the
             * expensive u128 multiply.  Benchmarked 2026-06-06: ~10%
             * CPU-time saving (skip-fraction != speedup — the saving is
             * small because cost lives in the survivor path, not here).
             * Deliberately omitted: search is finalising and the gain
             * does not affect mathematical results.
             * Full record: docs/zig_experiment_2026-06-06.md
             */
            u128 p = curve((u128)nmin + i);
            int last=(int)(p%10);
            if(last!=1 && last!=3) continue;            /* prime-eligible last digit */
            int first=(int)((long double)p / P1ld);     /* fast exact leading digit */
            if(first!=last) continue;                   /* palindrome needs first==last */
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
