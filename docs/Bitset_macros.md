# Bitset Macros — How They Work and Why

## The Macros

```c
#define BITSET_WORDS(n) (((n) + 63) / 64)
#define BITSET_SET(bs, i) ((bs)[(i) >> 6] |= (1ULL << ((i) & 63)))
#define BITSET_GET(bs, i) (((bs)[(i) >> 6] >> ((i) & 63)) & 1)
```

The bitset is an array of `uint64_t` words — each word holds 64 entries (bits).
For entry `i`:

## Locating the Word

`i >> 6` (same as `i / 64`) gives the array index. Entries 0–63 live in
`bs[0]`, entries 64–127 in `bs[1]`, etc.

## Locating the Bit Within That Word

`i & 63` (same as `i % 64`) gives the bit position (0–63) within that word.

## Concrete Example: entry i = 200

```
i >> 6  = 200 / 64 = 3          → word bs[3]
i & 63  = 200 % 64 = 8          → bit 8

bs[3]:  bit 63                                    bit 8         bit 0
        ┌───┬───┬───┬─── ··· ───┬───┬───┬───┬───┬───┬─── ··· ───┬───┐
        │   │   │   │           │   │ ← │   │   │   │           │   │
        └───┴───┴───┴─── ··· ───┴───┴───┴───┴───┴───┴─── ··· ───┴───┘
                                      ^
                                   entry 200
```

## SET — `bs[i >> 6] |= (1ULL << (i & 63))`

Creates a mask with only bit 8 set (`0x0000000000000100`), then ORs it into
the word. Turns that one bit on, leaves everything else untouched.

## GET — `(bs[i >> 6] >> (i & 63)) & 1`

Shifts the word right by 8 so bit 8 lands at position 0, then masks with
`& 1` to extract just that one bit. Returns 0 or 1.

## WORDS — `((n) + 63) / 64`

Ceiling division. For k=10, `n = 10^10 = 10,000,000,000` entries needs
`156,250,000` words × 8 bytes = **1.25 GB**. Compare to a `bool` array
which would be 10 GB — an 8x savings.

## Usage in Phase 1

```c
for (long n = 0; n < mod; n++) {
    long e = ending_for_residue(n, mod);   // e.g. e = 200
    if (!BITSET_GET(is_valid_ending, e)) { // check bit 8 of word 3
        BITSET_SET(is_valid_ending, e);    // flip bit 8 of word 3 on
        num_endings++;
    }
}
```

Each residue `n` produces an ending value `e`. The bitset acts as a
deduplicated boolean lookup — "has this ending been seen?" — at 1 bit per
possible ending value instead of 1 byte.

## Why Macros Instead of Functions

Macros expand inline at the call site — no function call overhead, no stack
frame, no register spills. The compiler sees the raw bit operations directly
and can optimize them in context (fold constants, keep the word in a register
across a loop iteration, etc.).

For something called billions of times in a tight inner loop (Phase 1
iterates 10 billion times at k=10), even a tiny per-call overhead adds up.
An `inline` function *hints* the compiler to do the same thing, but it's not
guaranteed — the compiler can ignore it. Macros guarantee zero overhead.

The tradeoff is the usual macro downsides (no type safety, potential
double-evaluation of arguments), but these macros are safe because the
arguments are always simple variables or expressions without side effects.
