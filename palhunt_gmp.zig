// palhunt_gmp.zig — Zig port of palhunt_gmp.c (bi-quad experiment, 2026-06-06).
//
// Faithful port for a head-to-head against the C version. Demonstrates the three
// things that matter for bi-quad-type work:
//   1. native u128            (C needed a hand-rolled hi/lo splitter just to PRINT)
//   2. GMP via @cImport       (the paramount C-interop requirement — no bindings)
//   3. std.Thread             (the OpenMP-pragma rebuild: Zig has no OpenMP)
//
// Targets Zig 0.14.x (where @cImport still works directly; 0.16 moved it to the
// build system). Build:
//   zig build-exe palhunt_gmp.zig -O ReleaseFast -lc -lgmp -mcpu=znver2 \
//     -femit-bin=palhunt_zig
// Usage: ./palhunt_zig [min_d] [max_d]

const std = @import("std");
const c = @cImport({
    @cInclude("gmp.h");
});

const NT: usize = 8;

fn ipow10(e: i32) u128 {
    var r: u128 = 1;
    var k = e;
    while (k > 0) : (k -= 1) r *= 10;
    return r;
}

fn isqrt128(x: u128) u128 {
    if (x < 2) return x;
    var lo: u128 = 1;
    var hi: u128 = @as(u128, 1) << 64;
    while (lo < hi) {
        const m = lo + (hi - lo + 1) / 2;
        if (m <= x / m) {
            lo = m;
        } else {
            hi = m - 1;
        }
    }
    return lo;
}

fn curve(n: u128) u128 {
    return 2 * n * n + 2 * n + 1;
}

fn isPal(x: u128) bool {
    var r: u128 = 0;
    var t: u128 = x;
    while (t != 0) {
        r = r * 10 + (t % 10);
        t /= 10;
    }
    return r == x;
}

fn toMpz(z: *c.mpz_t, x: u128) void {
    c.mpz_set_ui(z, @intCast(x >> 64));
    c.mpz_mul_2exp(z, z, 64);
    c.mpz_add_ui(z, z, @truncate(x));
}

const CHUNK: u64 = 1_000_000; // matches C's OpenMP schedule(dynamic, 1000000)

const Ctx = struct {
    nmin: u128,
    range: u64,
    d: i32,
    p1ld: c_longdouble, // (long double)10^(d-1) — match C's x87 leading-digit trick
    found: *std.atomic.Value(i64),
    next: *std.atomic.Value(u64), // shared dynamic chunk dispenser (the OpenMP rebuild)
};

fn worker(ctx: *const Ctx) void {
    var z: c.mpz_t = undefined;
    c.mpz_init(&z);
    defer c.mpz_clear(&z);

    while (true) {
        const start = ctx.next.fetchAdd(CHUNK, .monotonic); // grab next chunk dynamically
        if (start >= ctx.range) break;
        var end = start + CHUNK;
        if (end > ctx.range) end = ctx.range;

        var i: u64 = start;
        while (i < end) : (i += 1) {
            const n = ctx.nmin + i;
            const p = curve(n);
            const last: u8 = @intCast(p % 10);
            if (last != 1 and last != 3) continue; // prime-eligible last digit
            const first: u8 = @intFromFloat(@as(c_longdouble, @floatFromInt(p)) / ctx.p1ld); // x87 leading digit
            if (first != last) continue; // palindrome needs first == last
            if (!isPal(p)) continue;
            toMpz(&z, p);
            if (c.mpz_probab_prime_p(&z, 40) != 0) {
                _ = ctx.found.fetchAdd(1, .monotonic);
                const s = c.mpz_get_str(null, 10, &z);
                std.debug.print(
                    "   *** PRIME PALINDROME ***  d={d}  n={d}  p={s}\n",
                    .{ ctx.d, n, std.mem.span(s) },
                );
            }
        }
    }
}

pub fn main() !void {
    const stdout = std.io.getStdOut().writer();

    var it = std.process.args();
    _ = it.next(); // skip argv[0]
    const dmin: i32 = if (it.next()) |a| try std.fmt.parseInt(i32, a, 10) else 1;
    const dmax: i32 = if (it.next()) |a| try std.fmt.parseInt(i32, a, 10) else 7;

    try stdout.print(
        "\n  PRIME PALINDROMES on p = 2n^2+2n+1   (d={d}..{d}, GMP-certified) [zig]\n",
        .{ dmin, dmax },
    );
    try stdout.print(
        "  =================================================================\n",
        .{},
    );

    var d = dmin;
    while (d <= dmax) : (d += 1) {
        const L = ipow10(d - 1);
        const H = ipow10(d);
        const P1 = ipow10(d - 1);

        var s = isqrt128(2 * L - 1);
        var nmin: u128 = if (s > 0) (s - 1) / 2 else 0;
        while (curve(nmin) < L) nmin += 1;
        while (nmin > 0 and curve(nmin - 1) >= L) nmin -= 1;

        s = isqrt128(2 * H - 1);
        var nmax: u128 = (s - 1) / 2;
        while (curve(nmax) >= H) nmax -= 1;
        while (curve(nmax + 1) < H) nmax += 1;

        const range: u64 = @intCast(nmax - nmin + 1);

        var timer = try std.time.Timer.start();
        var found = std.atomic.Value(i64).init(0);
        var next = std.atomic.Value(u64).init(0);
        const ctx = Ctx{ .nmin = nmin, .range = range, .d = d, .p1ld = @floatFromInt(P1), .found = &found, .next = &next };

        var threads: [NT]std.Thread = undefined;
        for (0..NT) |t| threads[t] = try std.Thread.spawn(.{}, worker, .{&ctx});
        for (0..NT) |t| threads[t].join();

        const secs = @as(f64, @floatFromInt(timer.read())) / 1e9;
        try stdout.print(
            "   d={d}  range={d}  found={d}   [{d:.1}s]\n",
            .{ d, range, found.load(.monotonic), secs },
        );
    }
    try stdout.print("\n", .{});
}
