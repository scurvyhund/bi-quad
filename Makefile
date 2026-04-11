# Makefile for bi-quad — Modular obstruction search
#
# Build:  make
# Test:   make test       (quick run: d=30, k=5)
# Clean:  make clean

CC       = gcc
CFLAGS   = -O3 -march=znver2 -mtune=znver2 -std=c99 -Wall -Wextra -fopenmp
LDLIBS   = -lgmp

TARGET   = mod_obstruct

.PHONY: all test debug clean

all: $(TARGET)

$(TARGET): mod_obstruct.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

mod_obstruct_debug: mod_obstruct.c
	$(CC) -gdwarf-5 $(CFLAGS) -DDEBUG -o $@ $< $(LDLIBS)

test: $(TARGET)
	./$(TARGET) 30 5

debug: mod_obstruct_debug
	@echo "=== mod_obstruct invariant test (k<=4, d<=10) ==="
	@./mod_obstruct_debug 10 4 2>&1 | grep -E "\*\*\* BUG" \
	    && echo "*** INVARIANT VIOLATIONS FOUND ***" \
	    || echo "=== all invariants clean ==="

clean:
	rm -f $(TARGET) mod_obstruct_debug
