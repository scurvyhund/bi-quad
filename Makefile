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
TESTS    = test_curve test_curve_gmp test_palpred

.PHONY: all pals checks test tests clean

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

tests: $(TESTS)
	./test_curve
	./test_curve_gmp
	./test_palpred

clean:
	rm -f $(TARGET) $(PALS) $(CHECKS) hunt $(TESTS)	
