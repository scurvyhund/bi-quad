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

.PHONY: all test clean

all: $(TARGET)

$(TARGET): mod_obstruct.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

mod_debug: mod_obstruct.c
	$(CC) $(DBG) $(CFLAGS) -o $@ $< $(LDLIBS)
test: $(TARGET)
	./$(TARGET) 30 5

clean:
	rm -f $(TARGET)	
