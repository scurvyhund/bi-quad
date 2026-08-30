/* 
 * mod_obstruct.c — Modular obstruction search for bi-quadratic emirps
 * For each digit count d and depth k, determines whether a d-digit
 * converse prime pair (p, rev(p)) with both p and rev(p) of the form
 * 2n² + 2n + 1 is structurally possible, based on modular constraints
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
 *         min_k/min_d allow resuming an interrupted run
 *
 * Algorithm:
 *  
 * For p = 2n²+2n+1 and q = rev(p) = 2m²+2m+1:
 *   
 *   - last  k digits of p  → determined by n mod 10^k
 *   - first k digits of q = reverse(last k of p) → must be achievable
 *   - last  k digits of q  = reverse(first k of p) → must be valid ending
 *   - first k digits of p → determined by magnitude of n
 *
 * Both the p-side and q-side constraints are checked.
 *
 * Memory-efficient design (supports k up to ~13):
 *
 * - is_valid_ending stored as bitset (1 bit/entry vs 1 byte)
 *   - endings[] array eliminated — computed inline via __int128
 *   - Phase 2b reverse-residue lookup replaced by Hensel lifting
 *   - valid_firsts[] right-sized to actual count (not mod-sized)
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <omp.h>
#include <gmp.h>

#define DEFAULT_MAX_D  50
#define DEFAULT_MAX_K   6
#define NUM_THREADS     8
#define CKPT_FILE      "mod_obstruct.ckpt"

// Bitset macros — compact bool array using 1 bit per entry 
#define BITSET_WORDS(n) (((n) + 63) / 64)
#define BITSET_SET(bs, i) ((bs)[(i) >> 6] |= (1ULL << ((i) & 63)))
#define BITSET_GET(bs, i) (((bs)[(i) >> 6] >> ((i) & 63)) & 1)

/* 
 * Write checkpoint atomically: write to tmp file, then rename.
 * Stores the next (k, d) to process so we can resume there. 
 * 
 */
static void write_checkpoint(int max_d, int max_k, int k, int d) {
   FILE *f = fopen(CKPT_FILE ".tmp", "w");
   if (f) {
      fprintf(f, "%d %d %d %d\n", max_d, max_k, k, d);
      fclose(f);
      rename(CKPT_FILE ".tmp", CKPT_FILE);
   }
}

/* 
 * Read checkpoint and set min_k/min_d if it matches current run params.
 * Returns true if checkpoint was loaded successfully. 
 *
 */
static bool read_checkpoint(int max_d, int max_k, int *min_k, int *min_d) {
   FILE *f = fopen(CKPT_FILE, "r");
   if (!f) return false;

   int ck_max_d, ck_max_k, ck_k, ck_d;
   if (fscanf(f, "%d %d %d %d", &ck_max_d, &ck_max_k, &ck_k, &ck_d) == 4
      && ck_max_d == max_d && ck_max_k == max_k) {
      fclose(f);
      *min_k = ck_k;
      *min_d = ck_d;
      return true;
   }
   fclose(f);
   return false;
}

/* 
 * Compute (2r² + 2r + 1) mod m.
 * Uses 64-bit arithmetic when mod ≤ 10^9 (k ≤ 9), since
 * 2r² fits in unsigned long long.  Falls back to __int128
 * for k ≥ 10 where r can exceed ~3×10^9. 
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

// Compute n_min, n_max for d-digit numbers of the form 2n²+2n+1. 
static void compute_n_bounds(int d, mpz_t n_min, mpz_t n_max) {
   mpz_t target, sq;
   mpz_init(target);
   mpz_init(sq);

   // n_min: solve 2n²+2n+1 >= 10^(d-1) 
   mpz_ui_pow_ui(target, 10, d - 1);
   mpz_mul_ui(target, target, 2);
   mpz_sub_ui(target, target, 1);
   mpz_sqrt(sq, target);
   mpz_sub_ui(sq, sq, 1);
   mpz_fdiv_q_ui(n_min, sq, 2);
   mpz_add_ui(n_min, n_min, 1);

   // Verify n_min produces d digits (guards against sqrt rounding) 
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

   // n_max: solve 2n²+2n+1 < 10^d 
   mpz_ui_pow_ui(target, 10, d);
   mpz_mul_ui(target, target, 2);
   mpz_sub_ui(target, target, 1);
   mpz_sqrt(sq, target);
   mpz_sub_ui(sq, sq, 1);
   mpz_fdiv_q_ui(n_max, sq, 2);

   mpz_clear(target);
   mpz_clear(sq);
}

/* 
 * Compute first-k-digit prefix of p = 2n²+2n+1 for given n.
 * Returns floor(p / 10^(d-k)).
 *
 */

static long compute_first_k(mpz_t n, int d, int k, mpz_t tmp_p, mpz_t tmp_pow)
{
   // p = 2n² + 2n + 1 
   mpz_mul(tmp_p, n, n);
   mpz_mul_ui(tmp_p, tmp_p, 2);
   mpz_addmul_ui(tmp_p, n, 2);
   mpz_add_ui(tmp_p, tmp_p, 1);

   // divide by 10^(d-k) 
   mpz_ui_pow_ui(tmp_pow, 10, d - k);
   mpz_tdiv_q(tmp_p, tmp_p, tmp_pow);

   return mpz_get_ui(tmp_p);
}

// Find first n >= lo with n ≡ r (mod stride) 
static void first_n_with_residue(mpz_t result, mpz_t lo,
                         long r, long stride) 
{
   unsigned long lo_mod = mpz_fdiv_ui(lo, (unsigned long)stride);
   long gap = (((long)r - (long)lo_mod) % stride + stride) % stride;
   
   mpz_add_ui(result, lo, (unsigned long)gap);
}

/* 
 * Solve 2m² + 2m + 1 ≡ target (mod 10^k) via Hensel lifting.
 * Returns number of solutions stored in sols[].
 * Max solutions bounded by ~4 * 2^(k-1).
 *
 */

#define MAX_HENSEL_SOLS 4096

static int solve_residues(long target, int k, long *sols) {
   long tmp[MAX_HENSEL_SOLS];
   int count = 0;

   // Base: solutions mod 10 
   long t10 = target % 10;

   for (long r = 0; r < 10; r++) {
      
      if ((2*r*r + 2*r + 1) % 10 == t10)
         sols[count++] = r;
   }

   // Hensel lift: mod 10^j → mod 10^(j+1) 
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

      // Swap: copy tmp → sols 
      for (int i = 0; i < new_count; i++)
         sols[i] = tmp[i];
      count = new_count;
      pow10j = next_mod;
   }

   return count;
}

int main(int argc, char *argv[]) {
   int max_d = (argc > 1) ? atoi(argv[1]) : DEFAULT_MAX_D;
   int max_k = (argc > 2) ? atoi(argv[2]) : DEFAULT_MAX_K;
   int min_k = (argc > 3) ? atoi(argv[3]) : 3;
   int min_d = (argc > 4) ? atoi(argv[4]) : 0;

   // Auto-resume from checkpoint if no explicit min_k/min_d given 
   bool from_ckpt = false;
   if (argc <= 3) {
      from_ckpt = read_checkpoint(max_d, max_k, &min_k, &min_d);
   }

   printf("\n");
   printf("  Modular Obstruction Search for Bi-Quadratic Emirps\n");
   printf("=====================================================\n");
   printf("  max_d = %d    max_k = %d", max_d, max_k);

   if (min_k > 3 || min_d > 0) {
      printf("    (resuming from k=%d, d=%d", min_k, min_d);
      printf(from_ckpt ? " — from checkpoint)" : ")");
   }

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
       * Mark which last-k-digit values are achievable by 2n²+2n+1.
       * Uses bitset (mod/8 bytes) instead of bool array (mod bytes).
       * endings[] array eliminated — computed inline via __int128. 
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
       *      - q's last-k = reverse_k(f) → find m residues via Hensel
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

      int d_start = k + 1;
      
      if (k == min_k && min_d > d_start)
         d_start = min_d;

      for (int d = d_start; d <= max_d; d++) {

         compute_n_bounds(d, n_min, n_max);

         /* 64-bit: counts reach 10^k, past INT_MAX at k>=10 */
         long survivors = 0;

         /* Shared progress counters (atomic). g_scanned = TRUE aggregate
          * residues examined across ALL threads — robust to load imbalance,
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
          * flush — so both counters sat at 0 for 20+ min while all 8 cores
          * ground away. cur_r in the output shows position in [0,mod). */
         double hb_t0         = omp_get_wtime();
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
                * progress — accurate regardless of load imbalance. */
               if (++loc_scanned >= 200000) {
                  #pragma omp atomic
                  g_scanned += loc_scanned;
                  loc_scanned = 0;
                  /* time-based heartbeat (covers cheap regions, where
                   * this flush path fires often) */
                  if (omp_get_wtime() - hb_last >= 20.0) {
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
                        hb_last = n2;
                        hb_last_heavy = hv;
                     }
                  }
               }

               /*
                * First n ≡ r (mod 10^k) in [n_min, n_max] —
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

               /* q's first-k digits = reverse_k(p_ending) — fixed for
                * this residue (depends only on r), so compute once. */
               long q_first = reverse_k(p_ending, k);

               if (q_first < prefix_min)
                  continue;

               /*
                * EXACT per-residue check (replaces the old interval
                * [fk_min,fk_max] over-approximation). Enumerate every
                * actual n-value for this residue — t_first, +mod, +2*mod,
                * ... <= n_max — and test its EXACT p first-k prefix.
                *
                * The interval was correct only when a residue had a
                * single n-value (range < mod, i.e. d<=20 at k=10). At
                * d>=21 a residue has 2+ n-values whose p-prefixes are far
                * apart, so the interval admitted prefixes no real n
                * produces (wrong counts) AND forced a giant valid_firsts
                * sweep (intractable). Enumerating the few real n-values is
                * exact and cheap, and collapses to the identical single
                * point check for d<=20 — so d<=20 results are unchanged.
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

                  /* q-side Hensel work — count it + time-based heartbeat */
                  {
                     #pragma omp atomic
                     g_heavy++;
                     if (omp_get_wtime() - hb_last >= 20.0) {
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
                           hb_last = n2;
                           hb_last_heavy = hv;
                        }
                     }
                  }

                  /* Solve for m-residues achieving q_ending, then test
                   * whether any ACTUAL m-value yields q first-k == q_first. */
                  long m_sols[MAX_HENSEL_SOLS];
                  int qe_count = solve_residues(q_ending, k, m_sols);

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

         printf("    d=%2d  survivors = %6ld%s\n", d, survivors, tag);
         fflush(stdout);

         // If saturated, all larger d will also saturate — skip
         if (survivors == sat_level) {
            printf("    d=%2d..%2d  (skipped — saturated)\n",
                  d + 1, max_d);
          
            write_checkpoint(max_d, max_k, k + 1, 0);
            break;
         }

         // Checkpoint: next d-value to process 
         write_checkpoint(max_d, max_k, k, d + 1);
      }

      printf("\n  k=%d summary: %d obstructions found out of %d "
            "digit counts tested\n\n",
            k, obstruction_count, max_d - k);

      // Checkpoint: advance to next k 
      write_checkpoint(max_d, max_k, k + 1, 0);

      free(valid_firsts);
      free(is_valid_ending);

      mpz_clear(n_min);
      mpz_clear(n_max);
   }

   printf("=====================================================\n");
   printf("  Search complete.\n");
   printf("=====================================================\n\n");

   remove(CKPT_FILE);
   return 0;
}
