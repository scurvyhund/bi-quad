/*
 * mitm_probe.c — Fingerprint probe for the meet-in-the-middle idea.
 *
 * Split n = a*10^t + b.  Then  p = 2n^2+2n+1  expands (binomial!) to
 *   2a^2*10^2t + 2a*10^t  +  4ab*10^t  +  (2b^2+2b+1)
 * Theory says:
 *   - low  t  digits of p are a PURE function of b   (the a-terms vanish mod 10^t)
 *   - the cross term 4ab*10^t reaches up to ~position 3t below the top
 *   - so the TOP ~(d-3t) digits should be ~pure-a (up to carries)
 *   - the MIDDLE ~2t digits are coupled (bilinear in a,b)
 *
 * This measures the ACTUAL widths (worst case over samples) so we know whether
 * the clean zones are fat and the coupled band thin (MITM flies) or not.
 *
 * Build: gcc mitm_probe.c -o mitm_probe -O2 -std=c99 -Wall
 */
#include <stdio.h>
#include <stdint.h>

typedef unsigned __int128 u128;

static u128 curve(u128 n){ return 2*n*n + 2*n + 1; }
static int  ndig(u128 x){ int n=0; if(!x) return 1; while(x){n++; x/=10;} return n; }
static u128 ipow10(int e){ u128 r=1; while(e-->0) r*=10; return r; }

static u128 isqrt128(u128 x){
    if(x<2) return x;
    u128 lo=1, hi=((u128)1<<64);
    while(lo<hi){ u128 mid=lo+(hi-lo+1)/2; if(mid <= x/mid) lo=mid; else hi=mid-1; }
    return lo;
}
/* matching low decimal digits of x and y */
static int match_low(u128 x,u128 y){
    int c=0;
    for(;;){ if(x%10!=y%10) break; c++; x/=10; y/=10; if(!x&&!y) break; if(c>80) break; }
    return c;
}
/* matching high decimal digits, both assumed to have exactly D digits */
static int match_high(u128 x,u128 y,int D){
    int dx[48],dy[48];
    for(int i=0;i<D;i++){ dx[i]=(int)(x%10); x/=10; dy[i]=(int)(y%10); y/=10; }
    int c=0; for(int i=D-1;i>=0;i--){ if(dx[i]==dy[i]) c++; else break; }
    return c;
}

int main(void){
    printf("\n  MITM fingerprint probe   (split n = a*10^t + b,  balanced t = d/4)\n");
    printf("  p = 2n^2+2n+1.  pureB = clean low digits (fn of b), pureA = clean high (fn of a)\n");
    printf("  ============================================================================\n");
    printf("   d    t   pureB   pureA   coupled    | theory: B=t, A~d-3t, mid~2t\n");
    printf("  ----------------------------------------------------------------------------\n");
    for(int d=8; d<=30; d+=2){
        int t = d/4; if(t<1) t=1;
        u128 pw = ipow10(t);
        u128 L = ipow10(d-1), H = ipow10(d);
        u128 s = isqrt128(2*L-1); u128 nmin = (s>0)?(s-1)/2:0;
        while(curve(nmin) <  L) nmin++;
        while(nmin>0 && curve(nmin-1) >= L) nmin--;
        s = isqrt128(2*H-1); u128 nmax = (s-1)/2;
        while(curve(nmax) >= H) nmax--;
        while(curve(nmax+1) < H) nmax++;
        u128 amin = nmin/pw, amax = nmax/pw;

        /* pureB: hold b, vary a to the extremes; min clean low-digit run */
        int pureB = 999;
        for(int bi=0; bi<8; bi++){
            u128 b = (pw*(u128)bi)/8;
            u128 a1=amin+1, a2=amax-1;
            u128 n1=a1*pw+b, n2=a2*pw+b;
            if(n1<nmin||n1>nmax||n2<nmin||n2>nmax||n1==n2) continue;
            int m = match_low(curve(n1), curve(n2));
            if(m<pureB) pureB=m;
        }
        /* pureA: hold a, vary b to the extremes; min clean high-digit run */
        int pureA = 999;
        u128 amid=(amin+amax)/2;
        for(int ai=-3; ai<=3; ai++){
            u128 a = amid + (u128)ai;
            u128 n1=a*pw+0, n2=a*pw+(pw-1);
            if(n1<nmin||n1>nmax||n2<nmin||n2>nmax) continue;
            u128 p1=curve(n1), p2=curve(n2);
            if(ndig(p1)!=d || ndig(p2)!=d) continue;
            int m = match_high(p1,p2,d);
            if(m<pureA) pureA=m;
        }
        if(pureB==999) pureB=-1;
        if(pureA==999) pureA=-1;
        int coupled = d - (pureB<0?0:pureB) - (pureA<0?0:pureA);
        printf("   %2d   %2d   %5d   %5d   %7d    | B=%d  A~%d  mid~%d\n",
               d, t, pureB, pureA, coupled, t, d-3*t, 2*t);
    }
    printf("\n");
    return 0;
}
