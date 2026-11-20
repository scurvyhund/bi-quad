# Makefile for bi-quad — Modular obstruction search
#
# Build:  make
# Test:   make test       (quick run: d=30, k=5)
# Clean:  make clean

CC       = gcc
DBG	 = -gdwarf-5
CFLAGS   = -O3 -march=znver2 -mtune=znver2 -std=c99 -Wall -Wextra -fopenmp
LDLIBS   = -lgmp

TARGET   = mod_obstruct
PALS     = palsplit palcurve palbrute
CHECKS   = check_d5 check_d7 check_d9 check_survivors
TESTS    = test_curve test_curve_gmp test_palpred test_palpred_noasm

.PHONY: all pals checks test tests sanitize clean palpred_drift

all: $(TARGET)

$(TARGET): mod_obstruct.c curve_gmp.h
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

mod_debug: mod_obstruct.c curve_gmp.h
	$(CC) $(DBG) $(CFLAGS) -o $@ $< $(LDLIBS)
test: $(TARGET)
	./$(TARGET) 30 5

# --- palindrome tools (share curve.h; header-only, no link step) ---
pals: $(PALS)

$(PALS): %: %.c curve.h
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS) -lm

# --- GMP emirp tools (share curve_gmp.h) ---
checks: $(CHECKS) hunt

$(CHECKS): %: %.c curve_gmp.h
	$(CC) -O2 -std=c99 -Wall -Wextra -o $@ $< $(LDLIBS)

# GUARD: the resweep chain (scripts/resweep_tail.sh) invokes ./hunt
# FRESH for each d, so replacing this binary mid-chain destroys the
# single-binary provenance that is the entire point of the run.
# chmod a-w does NOT protect it -- gcc unlinks and recreates the
# output, and the directory is writable. Tested 2026-09-06.
hunt: hunt.c curve_gmp.h
	@if pgrep -x hunt >/dev/null 2>&1; then \
	  echo "*** REFUSING to rebuild hunt: one is RUNNING."; \
	  echo "***"; \
	  echo "*** The resweep chain re-invokes ./hunt per digit-length."; \
	  echo "*** Replacing it now voids the run's provenance."; \
	  echo "***   watch:  tail -f logs/hunt_resweep_2026-09-05.log"; \
	  echo "***   build elsewhere if you must:"; \
	  echo "***     $(CC) $(CFLAGS) -o hunt_new hunt.c $(LDLIBS)"; \
	  exit 1; \
	fi
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

# --- unit tests: run after ANY edit to curve.h / curve_gmp.h ---
test_curve: test_curve.c curve.h
	$(CC) -O2 -std=c99 -Wall -Wextra -o $@ $< -lm

test_curve_gmp: test_curve_gmp.c curve.h curve_gmp.h
	$(CC) -O2 -std=c99 -Wall -Wextra -o $@ $< $(LDLIBS) -lm

test_palpred: test_palpred.c curve.h
	$(CC) -O2 -std=c99 -Wall -Wextra -o $@ $< -lm

test_palpred_noasm: test_palpred.c curve.h
	$(CC) -O2 -std=c99 -Wall -Wextra -DPALBRUTE_NO_ASM -o $@ $< -lm

# test_palpred holds COPIES of palbrute's predicates; fail if they drift.
PB_FAST = /^static int is_pal_fast\(u128 x, int d\) \{/,/^}/
PB_DIVQ = /^\#if defined\(__x86_64__\) && !defined\(PALBRUTE_NO_ASM\)/,/^\#endif/
palpred_drift: palbrute.c test_palpred.c
	@awk '$(PB_FAST)' palbrute.c    > .pb_fast.a
	@awk '$(PB_FAST)' test_palpred.c > .pb_fast.b
	@awk '$(PB_DIVQ)' palbrute.c    > .pb_divq.a
	@awk '$(PB_DIVQ)' test_palpred.c > .pb_divq.b
	@cmp -s .pb_fast.a .pb_fast.b && cmp -s .pb_divq.a .pb_divq.b \
	   && test -s .pb_fast.a && test -s .pb_divq.a \
	   || { echo "FAIL: test_palpred.c copy of palbrute's predicates has DRIFTED"; \
	        rm -f .pb_fast.? .pb_divq.?; exit 1; }
	@rm -f .pb_fast.? .pb_divq.?
	@echo "palpred copy: identical to palbrute.c"

tests: $(TESTS) palpred_drift
	./test_curve
	./test_curve_gmp
	./test_palpred
	./test_palpred_noasm

# --- sanitizer pass: run before any long job, and after header edits ---
# clang, not gcc: gcc cannot trap UNSIGNED wraps at all, and this box's
# gcc lacks libasan/libubsan.  halt_on_error makes any report fail make.
# The unit tests must run SILENT here (no runtime errors on a clean tree).
SANCC    = clang
SANFLAGS = -O1 -g -std=c99 -Wall -Wextra -fno-omit-frame-pointer \
           -fsanitize=address,undefined,signed-integer-overflow,unsigned-integer-overflow
SANENV   = UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1
SAN_BINS = $(addsuffix _san,$(TESTS))

%_san: %.c curve.h curve_gmp.h
	$(SANCC) $(SANFLAGS) -o $@ $< $(LDLIBS) -lm

test_palpred_noasm_san: test_palpred.c curve.h
	$(SANCC) $(SANFLAGS) -DPALBRUTE_NO_ASM -o $@ $< -lm

sanitize: $(SAN_BINS)
	$(SANENV) ./test_curve_san
	$(SANENV) ./test_curve_gmp_san
	$(SANENV) ./test_palpred_san
	$(SANENV) ./test_palpred_noasm_san

clean:
	rm -f $(TARGET) $(PALS) $(CHECKS) hunt $(TESTS) $(SAN_BINS)	
