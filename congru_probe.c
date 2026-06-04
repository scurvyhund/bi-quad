/*
 * congru_probe.c — Hunt for a CONGRUENCE OBSTRUCTION to bi-quadratic emirps.
 *
 * For modulus M and digit-length d we ask: does there exist a d-digit pattern
 * (a_{d-1}..a_0) such that BOTH
 *    p   = sum a_i*10^i        is on the curve mod M   (p mod M in C_M)
 *    rev = sum a_i*10^(d-1-i)  is on the curve mod M   (rev mod M in C_M)
 * where C_M = { 2x^2+2x+1 mod M }.  Leading/trailing digits a_0,a_{d-1} must be
 * odd & nonzero (both p and rev are odd curve numbers).
 *
 * Exact reachability DP over (p mod M, rev mod M) — the reversal is captured
 * exactly (each digit contributes 10^i to p and 10^(d-1-i) to rev). NO carries.
 *
 * If 0 reachable states have both coords in C_M -> OBSTRUCTION at (M,d):
 *   a pencil-and-paper proof that no d-digit bi-quadratic emirp exists.
 * The dream: an M that obstructs ALL large d -> non-existence theorem.
 *
 * Build: gcc congru_probe.c -o congru_probe -O2 -std=c99 -Wall
 */
#include <stdio.h>
#include <stdlib.h>

static long pow10mod(int e, long M){ long r=1%M, b=10%M; while(e>0){ if(e&1) r=r*b%M; b=b*b%M; e>>=1; } return r; }

/* C_M membership */
static char* curve_set(long M){
    char*C=calloc(M,1);
    for(long x=0;x<M;x++){ long v=((2*x%M)*x%M + 2*x + 1)%M; C[v]=1; }
    return C;
}

/* returns number of reachable (p,rev) pairs with both coords on curve.
 * 0 => obstruction at (M,d). */
static long feasible_count(long M, int d, char*C){
    long MM = M*M;
    char *vis = calloc(MM,1);
    long *cur = malloc(MM*sizeof(long)), *nxt = malloc(MM*sizeof(long));
    long ncur, nnxt;
    /* start: (p=0,rev=0) before any digit */
    cur[0]=0; ncur=1; vis[0]=1;
    for(int i=0;i<d;i++){
        long wp = pow10mod(i, M);          /* weight of digit i in p */
        long wr = pow10mod(d-1-i, M);       /* weight of digit i in rev */
        int dlo=0, dhi=9, odd=0;
        if(i==0 || i==d-1){ odd=1; }        /* a_0, a_{d-1} odd & nonzero */
        nnxt=0;
        /* need a fresh visited for the new layer */
        char *vis2 = calloc(MM,1);
        for(long k=0;k<ncur;k++){
            long st=cur[k]; long P=st/M, R=st%M;
            for(int a=(odd?1:dlo); a<=dhi; a+=(odd?2:1)){
                long P2=(P + a*wp)%M, R2=(R + a*wr)%M;
                long s2=P2*M+R2;
                if(!vis2[s2]){ vis2[s2]=1; nxt[nnxt++]=s2; }
            }
        }
        free(vis); vis=vis2;
        long*tmp=cur; cur=nxt; nxt=tmp; ncur=nnxt;
    }
    long cnt=0;
    for(long k=0;k<ncur;k++){ long st=cur[k]; if(C[st/M] && C[st%M]) cnt++; }
    free(vis); free(cur); free(nxt);
    return cnt;
}

int main(void){
    long M[] = {3,9,11,33,99,101,1001,1111,7,13,91,77,143,41,37,111,999,121,1221,239,
                /* powers of 2 and 5 — the curve's strong low-digit structure */
                4,8,16,32,64,128,256,512,1024, 5,25,125,625, 10,20,40,80,100,200,400,1000,
                /* mixed / bigger composites */
                88,176,275,1375,891,1089,1100,1331};
    int nM = sizeof(M)/sizeof(M[0]);
    int dmin=8, dmax=30;

    printf("\n  Congruence-obstruction hunt: rev(2n^2+2n+1)=2m^2+2m+1 mod M\n");
    printf("  For each M, the digit-lengths d in [%d,%d] that are OBSTRUCTED (0 feasible)\n", dmin,dmax);
    printf("  ============================================================================\n");
    for(int j=0;j<nM;j++){
        long m=M[j];
        char*C=curve_set(m);
        int csz=0; for(long x=0;x<m;x++) csz+=C[x];
        printf("  M=%4ld  |C_M|=%3d : ", m, csz);
        int any=0;
        for(int d=dmin; d<=dmax; d++){
            long f = feasible_count(m,d,C);
            if(f==0){ printf("%d ", d); any=1; }
        }
        if(!any) printf("(none)");
        printf("\n");
        free(C);
    }
    printf("\n  (an obstruction here = a proof, no compute needed, for that d)\n\n");
    return 0;
}
