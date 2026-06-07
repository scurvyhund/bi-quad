# Completing the square: turning `n² + n` into a perfect-square trinomial

> Plain-text / Unicode math (no LaTeX), so it renders identically on GitHub,
> GitLab, and Codeberg. Background note for the curve `p = 2n² + 2n + 1`, whose
> `n² + n` core is one completed square away from `(n + ½)²`.

To form a perfect-square trinomial from the expression `n² + n`, you add a
specific constant term to balance it.

### The general rule
To complete the square on an expression of the form `x² + bx`, **add the square
of half the coefficient of the middle term**:

    Term to add = (b/2)²

---

### Step-by-step application

**1. Identify the coefficient (b).**
In `n² + n`, the coefficient of the middle term (`n`) is implicitly **1**:

    b = 1

**2. Divide the coefficient by 2.**

    1 ÷ 2 = ½

**3. Square the result.**

    (½)² = ¼

**4. Add this term to the expression.**
Adding `¼` forms a perfect-square trinomial:

    n² + n + ¼

---

### Verifying the result
Factor it to check:

    n² + n + ¼  =  (n + ½)(n + ½)  =  (n + ½)²

### Summary
To turn `n² + n` into a perfect-square trinomial, the missing term is **¼**:

    n² + n + ¼  =  (n + ½)²

---

So for `n² − n` the last term would be −¼ and the trinomial −¼ … regardless of
the coefficient of `n²`? Say we had `3n² − n`?

Two distinct concepts are tangled here: the **sign** of the term and the
**coefficient** of the leading term (`n²`).

### 1. Correction on `n² − n`

A small misunderstanding: the term is **not** `−¼`. **You always square** the
value you compute, and a square is never negative.

For `n² − n`:
1. **Identify b:** the coefficient of `n` is `−1`.
2. **Divide by 2:** `−1 ÷ 2 = −½`.
3. **Square it:** `(−½)² = +¼`.

So the perfect trinomial for `n² − n` is:

    n² − n + ¼

Factoring, the sign inside the brackets matches the sign of the middle term:

    (n − ½)²

**Key takeaway:** even when the middle term is negative (`−n`), the number you
add to complete the square is **always positive** (`+¼`).

---

### 2. The coefficient of `n²`

**Yes — the coefficient of `n²` matters immensely.** If it is anything other
than `1` (e.g. `3n²`), you cannot apply "divide by 2 and square" to the whole
expression directly; factor out the leading coefficient first.

**Example: `3n² − n`** — factor out the leading coefficient:

1. Pull the `3` out of the variable terms (dividing `−n` by `3` gives `−⅓n`):

       3(n² − ⅓n)

2. Apply the rule **inside the parentheses**:
   - coefficient of `n` inside: `−⅓`
   - divide by 2: `−⅙`
   - square it: `(−⅙)² = 1/36`

3. Add that inside the parentheses:

       3(n² − ⅓n + 1/36)

4. The expression inside is now a perfect square:

       3(n − ⅙)²

**Summary of differences:**

| Expression | Term to add | Perfect-square trinomial form |
| :--- | :--- | :--- |
| `n² − n` | **+¼** | `n² − n + ¼` |
| `3n² − n` | **+1/36** (inside the factor) | `3(n² − ⅓n + 1/36)` |

**The rule, generalized:** for `ax² + bx`, the term to add to complete the
square is

    (b / 2a)²

— the leading coefficient `a` now sits in the denominator.
