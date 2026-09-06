def isprime(n):
    if n < 2: return False
    for q in (2,3,5,7,11,13,17,19,23,29,31,37):
        if n % q == 0: return n == q
    d, s = n-1, 0
    while d % 2 == 0: d //= 2; s += 1
    for a in (2,3,5,7,11,13,17,19,23,29,31,37):
        x = pow(a, d, n)
        if x in (1, n-1): continue
        for _ in range(s-1):
            x = x*x % n
            if x == n-1: break
        else: return False
    return True
LIM = 10**8
weak, strong = [], []
n = 1
while True:
    p = 2*n*n + 2*n + 1
    if p > LIM: break
    if isprime(p):
        r = int(str(p)[::-1])
        if r != p and isprime(r):
            weak.append(p)
            # on curve?  2r-1 must be an odd perfect square
            t = 2*r - 1
            s = int(t**0.5)
            while s*s < t: s += 1
            if s*s == t:
                strong.append(p)
    n += 1
print("limit", LIM)
print("weak  (p on curve, rev(p) prime):", len(weak), weak[:12])
print("strong(both on curve)           :", len(strong), strong)
