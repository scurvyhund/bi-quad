/* 
 * mod_obstruct.c -- Modular obstruction search for bi-quadratic emirps
 * For each digit count d and depth k, determines whether a d-digit
 * converse prime pair (p, rev(p)) with both p and rev(p) of the form
 * 2n^2 + 2n + 1 is structurally possible, based on modular constraints
 * on the first and last k digits.
 *
 * Default gcc compiler cmd: 
 *
 * $> gcc prog.c -o prog -O3 -march=znver2 -mtune=znver2 -std=c99 -Wall
 *    -fopenmp -lgmp 
 * 
 * Build:  make
 * Usage:  ./mod_obstruct [max_d] [max_k] [min_k] [min_d]
 *
 *         defaults: max_d=50, max_k=6, min_k=3, min_d=0
 *         min_k/min_d set where the run STARTS.  An interrupted run
 *         resumes when re-invoked with the SAME four arguments, from
 *         mod_obstruct_D<max_d>_K<max_k>_k<min_k>_d<min_d>.ckpt in the
 *         current directory; any other invocation ignores that file.
 *
 * Algorithm:
 *  
 * For p = 2n^2+2n+1 and q = rev(p) = 2m^2+2m+1:
 *   
 *   - last  k digits of p  -> determined by n mod 10^k
 *   - first k digits of q = reverse(last k of p) -> must be achievable
 *   - last  k digits of q  = reverse(first k of p) -> must be valid ending
 *   - first k digits of p -> determined by magnitude of n
 *
 * Both the p-side and q-side constraints are checked.
 *
 * Memory-efficient design (supports k up to ~13):
 *
 * - is_valid_ending stored as bitset (1 bit/entry vs 1 byte)
 *   - endings[] array eliminated -- computed inline via __int128
 *   - Phase 2b reverse-residue lookup replaced by Hensel lifting
 *   - valid_firsts[] right-sized to actual count (not mod-sized)
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <omp.h>
#include <gmp.h>

#include "curve_gmp.h"   /* compute_n_bounds */

#define DEFAULT_MAX_D  50
#define DEFAULT_MAX_K   6
#define NUM_THREADS     8
#define CKPT_LEGACY    "mod_obstruct.ckpt"   /* pre-2026-11-19 name */
#define CKPT_MAGIC     "MODOBS2"
/* Bump when a change alters which survivors are counted, so a
 * checkpoint from an older binary is REJECTED rather than resumed into
 * a log whose earlier lines that binary wrote.  "hensel-u128" = after
 * 417ca34; every earlier binary could make false obstructions at k>=9. */
#define CKPT_BUILD     "hensel-u128"

/* Argument ceilings.  MAX_K: every fixed-width step is now exact far
 * past this (u128 lift, long mod to 10^18); the binding limit is
 * memory -- two 10^k-bit bitsets plus valid_firsts, ~6 GB at k=10,
 * 1.25 TB of bitset alone at k=13, where calloc fails loudly.  k >= 1:
 * k=0 makes mod = 1, every ending 0, and a FALSE "d=1 OBSTRUCTION"
 * (p=1 is a real survivor) -- 2026-11-19, docs/audit_2026-11.md. */
#define MAX_K          13
#define MAX_D_ARG      200

// Bitset macros -- compact bool array using 1 bit per entry 
#define BITSET_WORDS(n) (((n) + 63) / 64)
#define BITSET_SET(bs, i) ((bs)[(i) >> 6] |= (1ULL << ((i) & 63)))
#define BITSET_GET(bs, i) (((bs)[(i) >> 6] >> ((i) & 63)) & 1)

/*
 * Checkpointing.  The checkpoint is named after the WHOLE invocation --
 * max_d, max_k AND the starting min_k, min_d -- so a run resumes only
 * from its own checkpoint.  Keyed by (max_d, max_k) alone, an
 * interrupted sanity run (50 6 6 11, which never terminates) left
 * "50 6 6 16" in mod_obstruct.ckpt, and the next plain "50 6" silently
 * started at k=6 d=16, skipping k=3..5 and d=7..15 (2026-11-19).
 *
 * The file holds the NEXT unprocessed (k, d).  Anything malformed,
 * from another build, or naming a position outside the run is REJECTED
 * LOUDLY and the run starts at its beginning.  Deleted on completion.
 */
static char g_ckpt[128];
static int  g_max_d, g_max_k, g_min_k, g_min_d;

static void ckpt_init(int max_d, int max_k, int min_k, int min_d) {
   g_max_d = max_d; g_max_k = max_k; g_min_k = min_k; g_min_d = min_d;
   snprintf(g_ckpt, sizeof(g_ckpt), "mod_obstruct_D%d_K%d_k%d_d%d.ckpt",
            max_d, max_k, min_k, min_d);
}

/* Atomic: write .tmp, then rename.  Every step checked -- a failed
 * write leaves the previous checkpoint in place, and must say so. */
static void write_checkpoint(int k, int d) {
   char tmp[160];
   snprintf(tmp, sizeof(tmp), "%s.tmp", g_ckpt);
   FILE *f = fopen(tmp, "w");
   if (!f) {
      fprintf(stderr, "  WARNING: cannot open checkpoint %s\n", tmp);
      return;
   }
   int bad = fprintf(f, "%s %s %d %d %d %d\n%d %d\n", CKPT_MAGIC,
                     CKPT_BUILD, g_max_d, g_max_k, g_min_k, g_min_d,
                     k, d) < 0;
   bad |= (fflush(f) != 0);
   bad |= (fclose(f) != 0);
   if (bad || rename(tmp, g_ckpt) != 0) {
      fprintf(stderr, "  WARNING: checkpoint write FAILED for %s at "
              "k=%d d=%d -- previous checkpoint (if any) left in place\n",
              g_ckpt, k, d);
      remove(tmp);
   }
}

/* 1 = valid, next position in *k, *d;  0 = no checkpoint;
 * -1 = present but rejected (reason printed). */
static int read_checkpoint(int *k, int *d) {
   FILE *f = fopen(g_ckpt, "r");
   if (!f) return 0;
   char magic[16], build[32];
   int cd, ck, cmk, cmd, nk, nd;
   int got = fscanf(f, "%15s %31s %d %d %d %d %d %d", magic, build,
                    &cd, &ck, &cmk, &cmd, &nk, &nd);
   fclose(f);
   const char *why = NULL;
   if (got != 8)                           why = "malformed";
   else if (strcmp(magic, CKPT_MAGIC))     why = "wrong format";
   else if (strcmp(build, CKPT_BUILD))     why = "written by another build";
   else if (cd != g_max_d || ck != g_max_k
            || cmk != g_min_k || cmd != g_min_d)
                                           why = "different run parameters";
   else if (nk < g_min_k || nk > g_max_k + 1
            || nd < 0 || nd > g_max_d + 1
            || (nk == g_min_k && nd != 0 && nd < g_min_d))
                                           why = "position outside the run";
   if (why) {
      fprintf(stderr, "  *** CHECKPOINT REJECTED: %s (%s) -- starting "
              "this run from its beginning\n", g_ckpt, why);
      fflush(stderr);
      return -1;
   }
   *k = nk;
   *d = nd;
   return 1;
}

/* 
 * Compute (2r^2 + 2r + 1) mod m.
 * Uses 64-bit arithmetic when mod <= 10^9 (k <= 9), since
 * 2r^2 fits in unsigned long long.  Falls back to __int128
 * for k >= 10 where r can exceed ~3x10^9. 
 * 
 */
static inline long ending_for_residue(long r, long m) {
   if (__builtin_expect(m <= 1000000000L, 1)) {
      unsigned long long rr = (unsigned long long)r;
      return (long)((2*rr*rr + 2*rr + 1) % (unsigned long long)m);
   }
   __int128 rr = (__int128)r;
   return (long)((2*rr*rr + 2*rr + 1) % (__int128)m);
}

// Reverse a k-digit number (leading zeros preserved in digit sense). 
static long reverse_k(long val, int k) {
   long result = 0;
   
   for (int i = 0; i < k; i++) {
      result = result * 10 + (val % 10);
      val /= 10;
   }
   return result;
}

// Check if sorted array contains any value in [lo, hi] 
static bool sorted_has_value_in_range(const long *arr, long len,
                             long lo, long hi) {
   if (len == 0 || lo > hi)
      return false;

   // Binary search for first element >= lo 
   long left = 0, right = len;

   while (left < right) {
      long mid = left + (right - left) / 2;
   
      if (arr[mid] < lo)
         left = mid + 1;
      else
         right = mid;
   }
   return (left < len && arr[left] <= hi);
}

// Compute n_min, n_max for d-digit numbers of the form 2n^2+2n+1. 

/* 
 * Compute first-k-digit prefix of p = 2n^2+2n+1 for given n.
 * Returns floor(p / 10^(d-k)).
 *
 */

static long compute_first_k(mpz_t n, int d, int k, mpz_t tmp_p, mpz_t tmp_pow)
{
   // p = 2n^2 + 2n + 1 
   mpz_mul(tmp_p, n, n);
   mpz_mul_ui(tmp_p, tmp_p, 2);
   mpz_addmul_ui(tmp_p, n, 2);
   mpz_add_ui(tmp_p, tmp_p, 1);

   // divide by 10^(d-k) 
   mpz_ui_pow_ui(tmp_pow, 10, d - k);
   mpz_tdiv_q(tmp_p, tmp_p, tmp_pow);

   return mpz_get_ui(tmp_p);
}

// Find first n >= lo with n == r (mod stride) 
static void first_n_with_residue(mpz_t result, mpz_t lo,
                         long r, long stride) 
{
   unsigned long lo_mod = mpz_fdiv_ui(lo, (unsigned long)stride);
   long gap = (((long)r - (long)lo_mod) % stride + stride) % stride;
   
   mpz_add_ui(result, lo, (unsigned long)gap);
}

/*
 * Solve 2m^2 + 2m + 1 == target (mod 10^k) by digit-wise lifting.
 * Returns the number of solutions stored in sols[].
 *
 * EXACT SOLUTION BOUND.  Mod 2^k there are exactly 4 solutions (k>=2):
 * m(m+1) == (t-1)/2 (mod 2^(k-1)) has 2 roots mod 2, each lifting
 * uniquely (derivative 2m+1 is odd), and each doubles mod 2^k.  Mod 5^k
 * the equation is (2m+1)^2 == 2t-1, whose root count peaks at
 * max(2*5^floor((k-1)/2), 5^floor(k/2)).  So at most
 *
 *     hensel_cap(k) = 4 * max(2*5^floor((k-1)/2), 5^floor(k/2))
 *
 * -- 5000 at k=9, 12500 at k=10, 125000 at k=13.  The intermediate
 * lists (mod 10^j, j<k) obey the same bound for j, which is smaller.
 *
 * HISTORY (2026-11-19, docs/audit_2026-11.md).  This used a fixed
 * MAX_HENSEL_SOLS = 4096 that SILENTLY dropped solutions past it (k=9:
 * targets with 5000 solutions kept 4096), and formed 2*r^2 in int64,
 * which overflows once r > ~2^31 -- the last lift at k=10 and every
 * lift past it.  Both only DROP m-residues, so both could only produce
 * FALSE OBSTRUCTIONS.  At k=10 all 6 true d=21 survivors lost their
 * m-residue this way, which is what made landscape_k10.log report
 * "d=21 OBSTRUCTION".  Now the lift is exact u128 and the capacity is
 * the proven bound; exceeding it is a fatal error, never a truncation.
 */
static long hensel_cap(int k) {
   long a = 2, b = 1;
   for (int i = 0; i < (k - 1) / 2; i++) a *= 5;
   for (int i = 0; i < k / 2; i++)       b *= 5;
   return 4 * (a > b ? a : b);
}

static int solve_residues(long target, int k, long *sols, long *tmp,
                          long cap) {
   long count = 0;

   // Base: solutions mod 10
   long t10 = target % 10;

   for (long r = 0; r < 10; r++) {
      if ((2*r*r + 2*r + 1) % 10 == t10)
         sols[count++] = r;
   }

   // Lift: mod 10^j -> mod 10^(j+1), exact in u128 (r < 10^13 here)
   long pow10j = 10;

   for (int j = 1; j < k; j++) {
      long next_mod = pow10j * 10;
      long target_next = target % next_mod;
      long new_count = 0;

      for (long i = 0; i < count; i++) {
         long r = sols[i];

         for (long c = 0; c < 10; c++) {
            long r_new = r + c * pow10j;
            unsigned __int128 rn = (unsigned __int128)r_new;
            long fr = (long)((2*rn*rn + 2*rn + 1)
                             % (unsigned __int128)next_mod);

            if (fr == target_next) {
               if (new_count >= cap) {
                  fprintf(stderr, "FATAL: solve_residues: more than %ld "
                          "solutions at k=%d (target %ld) -- the proven "
                          "bound is wrong; results would be truncated\n",
                          cap, k, target);
                  exit(1);
               }
               tmp[new_count++] = r_new;
            }
         }
      }

      // Swap: copy tmp -> sols
      for (long i = 0; i < new_count; i++)
         sols[i] = tmp[i];
      count = new_count;
      pow10j = next_mod;
   }

   return (int)count;
}

/* Whole-string int parse: false on empty, junk or trailing text. */
static bool parse_int(const char *s, int *out) {
   char *end;
   errno = 0;
   long v = strtol(s, &end, 10);
   if (errno || end == s || *end != '\0' || v < INT_MIN || v > INT_MAX)
      return false;
   *out = (int)v;
   return true;
}

int main(int argc, char *argv[]) {
   int max_d = DEFAULT_MAX_D, max_k = DEFAULT_MAX_K;
   int min_k = 3, min_d = 0;
   bool args_ok = (argc <= 5)
      && (argc <= 1 || parse_int(argv[1], &max_d))
      && (argc <= 2 || parse_int(argv[2], &max_k))
      && (argc <= 3 || parse_int(argv[3], &min_k))
      && (argc <= 4 || parse_int(argv[4], &min_d));
   if (!args_ok || max_d < 1 || max_d > MAX_D_ARG
         || min_k < 1 || max_k > MAX_K || min_k > max_k
         || min_d < 0 || min_d > max_d) {
      fprintf(stderr,
         "usage: %s [max_d] [max_k] [min_k] [min_d]\n"
         "  1 <= max_d <= %d,  1 <= min_k <= max_k <= %d,"
         "  0 <= min_d <= max_d\n", argv[0], MAX_D_ARG, MAX_K);
      return 1;
   }

   /* Resume only from THIS invocation's own checkpoint. */
   ckpt_init(max_d, max_k, min_k, min_d);
   {
      FILE *lf = fopen(CKPT_LEGACY, "r");
      if (lf) {
         fclose(lf);
         fprintf(stderr, "  NOTE: legacy %s present (pre-2026-11-19 "
                 "build) -- NOT read, NOT touched\n", CKPT_LEGACY);
      }
   }
   const int run_k = min_k, run_d = min_d;   /* as invoked */
   int ck_k, ck_d;
   bool from_ckpt = (read_checkpoint(&ck_k, &ck_d) == 1);
   if (from_ckpt) {
      min_k = ck_k;
      min_d = ck_d;
   }

   printf("\n");
   printf("  Modular Obstruction Search for Bi-Quadratic Emirps\n");
   printf("=====================================================\n");
   printf("  max_d = %d    max_k = %d", max_d, max_k);

   if (from_ckpt)
      printf("    (resuming from checkpoint at k=%d, d=%d; run began "
             "k=%d, d=%d)", min_k, min_d, run_k, run_d);
   else if (run_k != 3 || run_d != 0)
      printf("    (starting at k=%d, d=%d)", run_k, run_d);

   printf("\n");
   printf("=====================================================\n\n");
   fflush(stdout);

   for (int k = min_k; k <= max_k; k++) {
      
      // Set mod to 10^k power...
      long mod = 1;
      
      for (int i = 0; i < k; i++)
         mod *= 10;

      // 10^(k-1) 
      long prefix_min = mod / 10;   

      /* 
       * Phase 1: Build is_valid_ending bitset.
       * Mark which last-k-digit values are achievable by 2n^2+2n+1.
       * Uses bitset (mod/8 bytes) instead of bool array (mod bytes).
       * endings[] array eliminated -- computed inline via __int128. 
       *
       */
      
      size_t bs_words = BITSET_WORDS(mod);
      uint64_t *is_valid_ending = calloc(bs_words, sizeof(uint64_t));
      
      if (!is_valid_ending) {
         fprintf(stderr, "Failed to allocate is_valid_ending "
               "bitset (%zu MB)\n", bs_words * 8 / (1024*1024));
         return 1;
      }
      long num_endings = 0;

      for (long n = 0; n < mod; n++) {
         long e = ending_for_residue(n, mod);
         /* p = 2n^2+2n+1 is always odd, so it can only end in 1, 3 or 5.
          * An ending of 5 is divisible by 5 -> composite, never prime, so
          * it can never belong to a valid emirp pair. Drop it (and, defen-
          * sively, any ...0) before admitting it to the bitset. This cleans
          * the q-side: valid_firsts is built from reverse_k(valid endings),
          * and Phase 3 checks q_ending against this same bitset. */
         if (e % 10 == 5 || e % 10 == 0)
            continue;
         if (!BITSET_GET(is_valid_ending, e)) {
            BITSET_SET(is_valid_ending, e);
            num_endings++;
         }
      }

      /* 
       * Phase 2: Build VALID_FIRSTS as a sorted array for fast
       * range queries. A first-k prefix f is valid if reverse_k(f)
       * is a valid ending (ensures last-k of q is achievable).
       *
       * Two-pass: count first, then right-size the allocation.
       * Uses a temporary bitset for dedup instead of bool array.
       *
       */
      
      uint64_t *is_valid_first_bs = calloc(bs_words, sizeof(uint64_t));
      
      if (!is_valid_first_bs) {
         fprintf(stderr, "Failed to allocate is_valid_first "
               "bitset (%zu MB)\n", bs_words * 8 / (1024*1024));
         free(is_valid_ending);
         return 1;
      }

      long num_firsts = 0;

      // Pass 1: count unique valid firsts 
      for (long e = 0; e < mod; e++) {
      
         if (!BITSET_GET(is_valid_ending, e))
            continue;
         
         long f = reverse_k(e, k);
         
         if (f >= prefix_min && !BITSET_GET(is_valid_first_bs, f)) {
            BITSET_SET(is_valid_first_bs, f);
            num_firsts++;
         }
      }

      // Pass 2: allocate right-sized array and fill 
      long *valid_firsts = malloc(num_firsts * sizeof(long));
      
      if (!valid_firsts) {
         fprintf(stderr, "Failed to allocate valid_firsts "
               "(%ld entries, %zu MB)\n",
               num_firsts, num_firsts * sizeof(long) / (1024*1024));
         
         free(is_valid_first_bs);
         free(is_valid_ending);
         return 1;
      }

      long fill_idx = 0;
      
      for (long f = prefix_min; f < mod; f++) {
      
         if (BITSET_GET(is_valid_first_bs, f))
            valid_firsts[fill_idx++] = f;
      }
      free(is_valid_first_bs);

      // valid_firsts is already sorted (filled in ascending order) 

      printf("  k=%d: valid_endings = %ld / %ld (%.2f%%)  "
            "valid_firsts = %ld\n",
            k, num_endings, mod,
            100.0 * num_endings / mod, num_firsts);
      printf("  ---------------------------------------------------\n");
      fflush(stdout);

      /* 
       * Phase 3: For each digit count d, check feasibility.
       *
       * For each residue r (p-side):
       *   1. Compute range of achievable first-k prefixes for p
       *   2. Check if any valid first falls in that range (p-side pass)
       *   3. For each matching first-k prefix f:
       *      - q's last-k = reverse_k(f) -> find m residues via Hensel
       *      - q's first-k = reverse_k(ending_of_p)
       *      - Chk if any such m achieves that first-k prefix (q-side pass)
       *
       * The residue loop is parallelized with OpenMP. Each thread
       * gets its own GMP variables to avoid sharing.
       *
       */
      
      mpz_t n_min, n_max;
      mpz_init(n_min);
      mpz_init(n_max);

      omp_set_num_threads(NUM_THREADS);
      int obstruction_count = 0;
      int tested_count = 0;
      const long hcap = hensel_cap(k);

      int d_start = k + 1;
      
      if (k == min_k && min_d > d_start)
         d_start = min_d;

      for (int d = d_start; d <= max_d; d++) {

         compute_n_bounds(d, n_min, n_max);

         /* 64-bit: counts reach 10^k, past INT_MAX at k>=10 */
         long survivors = 0;

         /* Shared progress counters (atomic). g_scanned = TRUE aggregate
          * residues examined across ALL threads -- robust to load imbalance,
          * unlike the old per-thread r%100M checkpoint that went silent
          * whenever one thread fell behind. g_heavy = residues that reach
          * the expensive Hensel/q-side work, the real cost driver and the
          * phase we were previously completely blind to. */
         long g_scanned = 0;
         long g_heavy   = 0;
         /* Time-based heartbeat state (shared): guarantees a progress line
          * every ~20s of WALL time regardless of per-residue cost. The old
          * count-based thresholds were useless at d=21, where a single heavy
          * residue costs ~100ms and a thread needs hours to reach a 2M-iter
          * flush -- so both counters sat at 0 for 20+ min while all 8 cores
          * ground away. cur_r in the output shows position in [0,mod). */
         double hb_t0         = omp_get_wtime();
         /* hb_last is read OUTSIDE the (hb) critical section by every
          * thread and written inside it, so both sides are atomic: a plain
          * concurrent read/write is a data race (UB).  TSan with Archer
          * did not report it (2026-11-19) -- dynamic tools see only what
          * they observe; the language rules decide. */
         double hb_last       = hb_t0;
         long   hb_last_heavy = 0;

         #pragma omp parallel reduction(+:survivors)
         {
            // Per-thread GMP variables 
            mpz_t t_first, t_p, t_pow, t_mfirst, t_n, t_m;
            mpz_init(t_first);
            mpz_init(t_p);
            mpz_init(t_pow);
            mpz_init(t_mfirst);
            mpz_init(t_n);
            mpz_init(t_m);

            /* Hensel buffers, once per thread: at k=13 each holds 125000
             * longs (1 MB), too big for a thread stack. */
            long *m_sols = malloc((size_t)hcap * sizeof(long));
            long *h_tmp  = malloc((size_t)hcap * sizeof(long));
            if (!m_sols || !h_tmp) {
               fprintf(stderr, "FATAL: cannot allocate Hensel buffers "
                       "(2 x %ld longs)\n", hcap);
               exit(1);
            }

            /* per-thread tally, flushed to g_scanned in batches */
            long loc_scanned = 0;

            /* dynamic scheduling: the per-residue cost is highly skewed
             * (most continue cheaply, a few do expensive Hensel/GMP work),
             * so fixed static blocks load-imbalance badly. A 100K chunk
             * keeps scheduling overhead negligible (~100K chunks total)
             * while letting idle threads steal the costly tail. */
            #pragma omp for schedule(dynamic, 100000)
            for (long r = 0; r < mod; r++) {

               /* Aggregate progress: flush a thread-local tally into the
                * shared counter every 2M iterations (keeps atomics rare),
                * then emit one line per 100M residues of TRUE total
                * progress -- accurate regardless of load imbalance. */
               if (++loc_scanned >= 200000) {
                  #pragma omp atomic
                  g_scanned += loc_scanned;
                  loc_scanned = 0;
                  /* time-based heartbeat (covers cheap regions, where
                   * this flush path fires often) */
                  double hb_seen;    /* unlocked: atomic read */
                  #pragma omp atomic read
                  hb_seen = hb_last;
                  if (omp_get_wtime() - hb_seen >= 20.0) {
                     #pragma omp critical (hb)
                     if (omp_get_wtime() - hb_last >= 20.0) {
                        double n2 = omp_get_wtime();
                        long sc, hv;
                        #pragma omp atomic read
                        sc = g_scanned;
                        #pragma omp atomic read
                        hv = g_heavy;
                        double rate = (hv - hb_last_heavy) / (n2 - hb_last);
                        fprintf(stderr,
                           "  d=%2d HB t=%6.0fs  heavy=%ld (%.0f/s)  "
                           "scanned~%ldM  cur_r=%ld (%.2f%%)\n",
                           d, n2 - hb_t0, hv, rate, sc / 1000000,
                           r, 100.0 * (double)r / (double)mod);
                        fflush(stderr);
                        #pragma omp atomic write
                        hb_last = n2;
                        hb_last_heavy = hv;
                     }
                  }
               }

               /*
                * First n == r (mod 10^k) in [n_min, n_max] --
                * check this BEFORE computing the ending to
                * skip residues with no n values cheaply
                * (at k=10 d=11, eliminates 99.999% of r).
                *
                */

               first_n_with_residue(t_first, n_min, r, mod);
      
               if (mpz_cmp(t_first, n_max) > 0)
                  continue;

               long p_ending = ending_for_residue(r, mod);

               /* p's own last-k digits must be a prime-eligible ending.
                * p_ending is achievable by construction, so this is
                * purely the cleanliness test: without it, residues where
                * p ends in 5 (composite p) are counted as spurious
                * survivors. Mirrors the q_ending check below. */
               if (!BITSET_GET(is_valid_ending, p_ending))
                  continue;

               /* q's first-k digits = reverse_k(p_ending) -- fixed for
                * this residue (depends only on r), so compute once. */
               long q_first = reverse_k(p_ending, k);

               if (q_first < prefix_min)
                  continue;

               /*
                * EXACT per-residue check (replaces the old interval
                * [fk_min,fk_max] over-approximation). Enumerate every
                * actual n-value for this residue -- t_first, +mod, +2*mod,
                * ... <= n_max -- and test its EXACT p first-k prefix.
                *
                * The interval was correct only when a residue had a
                * single n-value (range < mod, i.e. d<=20 at k=10). At
                * d>=21 a residue has 2+ n-values whose p-prefixes are far
                * apart, so the interval admitted prefixes no real n
                * produces (wrong counts) AND forced a giant valid_firsts
                * sweep (intractable). Enumerating the few real n-values is
                * exact and cheap, and collapses to the identical single
                * point check for d<=20 -- so d<=20 results are unchanged.
                */
               bool surv = false;

               for (mpz_set(t_n, t_first);
                   mpz_cmp(t_n, n_max) <= 0;
                   mpz_add_ui(t_n, t_n, (unsigned long)mod)) {

                  /* exact first-k prefix of p = 2n^2+2n+1 for this n */
                  long f = compute_first_k(t_n, d, k, t_p, t_pow);

                  /* p-side: this prefix must be a valid_first (so q's
                   * last-k is achievable). Exact membership = [f,f]. */
                  if (!sorted_has_value_in_range(valid_firsts, num_firsts,
                                          f, f))
                     continue;

                  /* q's last-k digits = reverse_k(f) */
                  long q_ending = reverse_k(f, k);
                  if (!BITSET_GET(is_valid_ending, q_ending))
                     continue;

                  /* q-side Hensel work -- count it + time-based heartbeat */
                  {
                     #pragma omp atomic
                     g_heavy++;
                     double hb_seen;    /* unlocked: atomic read */
                     #pragma omp atomic read
                     hb_seen = hb_last;
                     if (omp_get_wtime() - hb_seen >= 20.0) {
                        #pragma omp critical (hb)
                        if (omp_get_wtime() - hb_last >= 20.0) {
                           double n2 = omp_get_wtime();
                           long sc, hv;
                           #pragma omp atomic read
                           sc = g_scanned;
                           #pragma omp atomic read
                           hv = g_heavy;
                           double rate = (hv - hb_last_heavy) / (n2 - hb_last);
                           fprintf(stderr,
                              "  d=%2d HB t=%6.0fs  heavy=%ld (%.0f/s)  "
                              "scanned~%ldM  cur_r=%ld (%.2f%%)\n",
                              d, n2 - hb_t0, hv, rate, sc / 1000000,
                              r, 100.0 * (double)r / (double)mod);
                           fflush(stderr);
                           #pragma omp atomic write
                           hb_last = n2;
                           hb_last_heavy = hv;
                        }
                     }
                  }

                  /* Solve for m-residues achieving q_ending, then test
                   * whether any ACTUAL m-value yields q first-k == q_first. */
                  int qe_count = solve_residues(q_ending, k, m_sols,
                                                h_tmp, hcap);

                  for (int mi = 0; mi < qe_count && !surv; mi++) {
                     long m_r = m_sols[mi];

                     first_n_with_residue(t_mfirst, n_min, m_r, mod);

                     for (mpz_set(t_m, t_mfirst);
                         mpz_cmp(t_m, n_max) <= 0;
                         mpz_add_ui(t_m, t_m, (unsigned long)mod)) {
                        long mk = compute_first_k(t_m, d, k, t_p, t_pow);
                        if (mk == q_first) { surv = true; break; }
                     }
                  }

                  if (surv)
                     break;
               }

               if (surv) {
                  survivors++;

                  /* Even-d mod-11 invariant -- VALID ONLY WHEN d <= 2k.
                   * See docs/mod11_converse_constraint.md.
                   *
                   * A survivor here is a residue class whose p matches
                   * on its first k and last k digits. Those cover all
                   * d digits only when d <= 2k; past that the middle
                   * digits are free, q need not equal rev(p), and
                   * p mod 11 is unconstrained -- so the check MUST NOT
                   * run there or it fires on legitimate candidates.
                   * Within d <= 2k the survivor is an exact converse
                   * pair and p mod 11 must be 3, 5, 6 or 8.
                   *
                   * Diagnostic only: printed, never fatal. Aborting a
                   * worker mid-run would discard the survivor count
                   * this pass is about to report. */
                  if (d % 2 == 0 && d <= 2 * k) {
                     unsigned long nr = mpz_fdiv_ui(t_n, 11);
                     unsigned long r11 =
                        (2 * nr * nr + 2 * nr + 1) % 11;
                     if (r11 != 3 && r11 != 5
                         && r11 != 6 && r11 != 8) {
                        #pragma omp critical (mod11)
                        {
                           fprintf(stderr,
                              "  *** BUG d=%2d k=%d: survivor breaks "
                              "the mod-11 invariant (p mod 11 = %lu, "
                              "r=%ld)\n", d, k, r11, r);
                           fflush(stderr);
                        }
                     }
                  }
               }
            }

            mpz_clear(t_first);
            mpz_clear(t_p);
            mpz_clear(t_pow);
            mpz_clear(t_mfirst);
            mpz_clear(t_n);
            mpz_clear(t_m);
            free(m_sols);
            free(h_tmp);
         }

         const char *tag = "";
         
         /* With the prime-eligibility filter, only residues whose p_ending
          * avoids last digit 0/5 can survive, so the saturated maximum is
          * 3*mod/5, not mod (10^k is divisible by 5, so this is exact). */
         long sat_level = mod / 5 * 3;

         if (survivors == 0) {
            tag = "  *** OBSTRUCTION ***";
            obstruction_count++;
         } else if (survivors == sat_level) {
            tag = "  (saturated)";
         }

         tested_count++;
         printf("    d=%2d  survivors = %6ld%s\n", d, survivors, tag);
         fflush(stdout);

         // If saturated, all larger d will also saturate -- skip
         if (survivors == sat_level) {
            printf("    d=%2d..%2d  (skipped — saturated)\n",
                  d + 1, max_d);
          
            write_checkpoint(k + 1, 0);
            break;
         }

         // Checkpoint: next d-value to process 
         write_checkpoint(k, d + 1);
      }

      printf("\n  k=%d summary: %d obstructions found out of %d "
            "digit counts tested\n\n",
            k, obstruction_count, tested_count);

      // Checkpoint: advance to next k 
      write_checkpoint(k + 1, 0);

      free(valid_firsts);
      free(is_valid_ending);

      mpz_clear(n_min);
      mpz_clear(n_max);
   }

   printf("=====================================================\n");
   printf("  Search complete.\n");
   printf("=====================================================\n\n");

   remove(g_ckpt);
   return 0;
}
