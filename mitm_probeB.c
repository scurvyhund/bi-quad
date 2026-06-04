/*
 * mitm_probeB.c — Does the two-ended digit-DP (approach B) actually prune?
 *
 * Build p from both ends. At the midpoint we've fixed the top j and bottom j
 * digits (j = d/4). A partial string (T,B) is ALIVE iff it is consistent with
 *   - some n: p=2n^2+2n+1 has top j = T, bottom j = B          ("p on curve")
 *   - some m: q=2m^2+2m+1 reverses to a p with that same (T,B)  ("rev(p) on curve")
 *
 * The count of alive midpoint strings = B's meet-in-the-middle cost.
 *   ~ sqrt(range)  -> B prunes, MITM-able.
 *   ~ range        -> B is brute force in disguise.
 *
 * We get the ground-truth sets by enumerating n and m (feasible at small d).
 *
 * Build: gcc mitm_probeB.c -o mitm_probeB -O2 -std=c99 -Wall
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

typedef unsigned __int128 u128;
typedef uint64_t u64;

static u128 curve(u128 n){ return 2*n*n + 2*n + 1; }
static u128 ipow10(int e){ u128 r=1; while(e-->0) r*=10; return r; }
static u128 isqrt128(u128 x){
    if(x<2) return x;
    u128 lo=1, hi=((u128)1<<64);
    while(lo<hi){ u128 mid=lo+(hi-lo+1)/2; if(mid<=x/mid) lo=mid; else hi=mid-1; }
    return lo;
}
static u64 rev_j(u64 x,int j){ u64 r=0; while(j-->0){ r=r*10+x%10; x/=10; } return r; }

/* bit array helpers */
static inline void setbit(u64*bs,u64 i){ bs[i>>6]|=(1ULL<<(i&63)); }
static inline int  getbit(u64*bs,u64 i){ return (bs[i>>6]>>(i&63))&1; }

int main(void){
    printf("\n  Approach-B probe: midpoint survivor count (two-ended digit-DP)\n");
    printf("  j = d/4 digits fixed at each end.  alive = consistent with p AND rev(p) on curve\n");
    printf("  =================================================================================\n");
    printf("   d   j   range(brute)   sqrt(range)   alive(B cost)   verdict\n");
    printf("  ---------------------------------------------------------------------------------\n");
    for(int d=8; d<=16; d+=2){
        int j = d/4; if(j<1) j=1;
        u128 L=ipow10(d-1), H=ipow10(d);
        u128 P10j = ipow10(j), Pdj = ipow10(d-j);
        u128 s=isqrt128(2*L-1); u128 nmin=(s>0)?(s-1)/2:0;
        while(curve(nmin)<L) nmin++;  while(nmin>0&&curve(nmin-1)>=L) nmin--;
        s=isqrt128(2*H-1); u128 nmax=(s-1)/2;
        while(curve(nmax)>=H) nmax--; while(curve(nmax+1)<H) nmax++;
        u64 range=(u64)(nmax-nmin)+1;

        u64 sigspace=(u64)ipow10(2*j);
        u64 words=(sigspace>>6)+1;
        u64 *P=calloc(words,8), *Q=calloc(words,8);
        if(!P||!Q){ printf("  d=%2d alloc fail\n",d); free(P);free(Q); continue; }

        /* p-side: each n -> sig(p) = topj*10^j + botj */
        for(u128 n=nmin;n<=nmax;n++){
            u128 p=curve(n);
            u64 topj=(u64)(p/Pdj), botj=(u64)(p%P10j);
            setbit(P, topj*(u64)P10j + botj);
        }
        /* q-side: each m -> the p-sig that rev(q) would have */
        for(u128 m=nmin;m<=nmax;m++){
            u128 q=curve(m);
            u64 qbot=(u64)(q%P10j), qtop=(u64)(q/Pdj);
            u64 topj=rev_j(qbot,j);     /* top j of p = reverse(bottom j of q) */
            u64 botj=rev_j(qtop,j);     /* bottom j of p = reverse(top j of q) */
            setbit(Q, topj*(u64)P10j + botj);
        }
        /* alive = |P AND Q| */
        u64 alive=0;
        for(u64 w=0;w<words;w++) alive+=__builtin_popcountll(P[w]&Q[w]);
        free(P); free(Q);

        double sq=sqrt((double)range);
        const char*verdict = (alive < 4*sq) ? "PRUNES (~sqrt!)"
                            : (alive > range/4) ? "~BRUTE (no win)" : "partial";
        printf("   %2d  %2d   %10llu   %10.0f   %12llu   %s\n",
               d,j,(unsigned long long)range,sq,(unsigned long long)alive,verdict);
    }
    printf("\n");
    return 0;
}
