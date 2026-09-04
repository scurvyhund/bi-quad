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

.PHONY: all pals test testcurve clean

all: $(TARGET)

$(TARGET): mod_obstruct.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

mod_debug: mod_obstruct.c
	$(CC) $(DBG) $(CFLAGS) -o $@ $< $(LDLIBS)
test: $(TARGET)
	./$(TARGET) 30 5

# --- palindrome tools (share curve.h; header-only, no link step) ---
pals: $(PALS)

$(PALS): %: %.c curve.h
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS) -lm

# unit tests for curve.h -- run this after ANY edit to curve.h
test_curve: test_curve.c curve.h
	$(CC) -O2 -std=c99 -Wall -Wextra -o $@ $< -lm

testcurve: test_curve
	./test_curve

clean:
	rm -f $(TARGET) $(PALS) test_curve	
