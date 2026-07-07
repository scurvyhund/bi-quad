# Numeric Gatekeeper Optimization — Eliminating String Conversion from Hot Loop

## Problem

`gmp_sprintf(cand_str, "%Zd", candidate)` is called for every candidate
(5.3 trillion times in the 10^28 run). Full string conversion of 28-digit
GMP integers is expensive and likely the cause of 50% CPU stall observed
via memory/cache pressure.

## Solution

Move the gatekeeper check to pure GMP arithmetic. Only convert to string
for the ~0.06% of candidates that pass the gatekeeper (3.2B out of 5.3T).

## Key Insight

Each zone has a fixed digit count (`zones[z].digits`), so `10^(digits-2)`
can be precomputed once per zone rather than per iteration.

---

## Diff

### 1. Add `mpz_t` for the power-of-10 divisor (per-thread GMP variables)

```diff
 // Per-thread GMP variables
-mpz_t n_z, candidate, reversed_num, disc, n1_out, n2_out;
+mpz_t n_z, candidate, reversed_num, disc, n1_out, n2_out, pow10;
 mpz_init(n_z);
 mpz_init(candidate);
 mpz_init(reversed_num);
 mpz_init(disc);
 mpz_init(n1_out);
 mpz_init(n2_out);
+mpz_init(pow10);
```

### 2. Precompute `10^(digits-2)` at the top of each zone loop

```diff
 // Zone-aware iteration
 for (int z = 0; z < num_zones; z++) {
     uint64_t zone_start = zones[z].n_min;
     uint64_t zone_end   = zones[z].n_max;

+    // Precompute 10^(d-2) for extracting first 2 digits
+    // All candidates in a zone have the same digit count
+    int zone_digits = zones[z].digits;
+    mpz_ui_pow_ui(pow10, 10, (unsigned long)(zone_digits - 2));
+
     // Compute first n for this thread in this zone
```

### 3. Replace string-based gatekeeper with numeric gatekeeper

```diff
             st.candidates_generated++;

-            // Step 2: Convert to string (stack buffer)
-            gmp_sprintf(cand_str, "%Zd", candidate);
-            int len = strlen(cand_str);
-
-            // Step 3: Gatekeeper filter (safety net)
-            if (!is_valid_candidate(cand_str, len))
+            // Step 2: Numeric gatekeeper (no string conversion)
+            //
+            // Last 2 digits via modular arithmetic
+            unsigned long last2 = mpz_fdiv_ui(candidate, 100);
+            unsigned long last_digit = last2 % 10;
+            if ((last_digit != 1 && last_digit != 3) || (last2 % 4 != 1))
+                continue;
+
+            // First 2 digits via integer division by 10^(d-2)
+            mpz_tdiv_q(n_z, candidate, pow10);  // reuse n_z as temp
+            unsigned long first2 = mpz_get_ui(n_z);
+            if (first2 != 10 && first2 != 12 && first2 != 14 &&
+                first2 != 16 && first2 != 18 && first2 != 31)
                 continue;
             st.gatekeeper_passed++;
```

### 4. Defer string conversion to AFTER gatekeeper passes

```diff
             st.gatekeeper_passed++;

+            // Convert to string only for candidates that passed gatekeeper
+            gmp_sprintf(cand_str, "%Zd", candidate);
+            int len = strlen(cand_str);
+
             // Step 4: Palindrome branch
             if (is_palindrome_str(cand_str, len)) {
```

### 5. Restore n_z before next iteration's candidate computation

The change in Step 3 reuses `n_z` as a temporary for the division result.
This is safe because `n_z` is set fresh at the top of each iteration:

```c
// Step 1: Compute p = 2n^2 + 2n + 1
mpz_set_ui(n_z, n);       // <-- n_z is overwritten here every iteration
```

No change needed — the existing `mpz_set_ui(n_z, n)` at the top of the loop
already resets it.

### 6. Clean up the new mpz_t at thread exit

```diff
 mpz_clear(n_z);
 mpz_clear(candidate);
 mpz_clear(reversed_num);
 mpz_clear(disc);
 mpz_clear(n1_out);
 mpz_clear(n2_out);
+mpz_clear(pow10);
```

---

## Why This Works

| Step | Before | After |
|------|--------|-------|
| Gatekeeper (every candidate) | `gmp_sprintf` full 28-digit string conversion | `mpz_fdiv_ui` (mod 100) + `mpz_tdiv_q` (div by precomputed constant) |
| String conversion | 5.3T calls | ~3.2B calls (0.06% of before) |

- `mpz_fdiv_ui(candidate, 100)` extracts last 2 digits — single-limb mod,
  essentially free
- `mpz_tdiv_q(result, candidate, pow10)` extracts first 2 digits — GMP integer
  division by a precomputed power of 10, much cheaper than full decimal
  string conversion
- `pow10` is computed once per zone, not per iteration
- The check-last-digits-first ordering provides early exit: most candidates
  fail on last-digit check before even computing the first-2-digit division

## Expected Impact

~1600x fewer string conversions in the hot loop. The two GMP integer operations
replacing it (`mpz_fdiv_ui` + `mpz_tdiv_q`) are significantly cheaper than
`gmp_sprintf` which must perform full base-10 conversion with memory allocation
for the output string.

Estimated wall-time reduction: substantial (the exact factor depends on what
fraction of per-iteration cost was the string conversion vs. the candidate
computation itself — benchmarking will tell).

## Also Consider

- **`NUM_THREADS 8`** instead of 16: since SMT gives no benefit for this
  workload, saves thread management overhead
- **Fix "CPU time" label** (line 567): `omp_get_wtime()` is wall time, not
  CPU time
