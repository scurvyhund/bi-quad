#!/usr/bin/env python3
"""Render the ending distribution of palindromic curve-values to a PNG.
Pillow only (no matplotlib). Data: docs/palindrome_insights.md corpus.
Build: python3 gen_ending_figure.py  ->  docs/palindrome_endings.png"""
from PIL import Image, ImageDraw, ImageFont

# (label, count, kind)  kind: 'gold' holds the primes; 'teal' eligible; 'div5'
DATA = [("…13", 11, "gold"), ("…21", 4, "teal"), ("…01", 2, "teal"),
        ("…61", 2, "teal"), ("…41", 2, "teal"), ("…81", 1, "teal"),
        ("…5", 11, "div5")]

W, H = 1200, 760
BG = (252, 252, 250); INK = (34, 34, 40); MUT = (120, 120, 130)
GRID = (225, 225, 230)
TEAL = (38, 140, 150); GOLD = (232, 170, 30); RED = (208, 70, 70)
COL = {"teal": TEAL, "gold": GOLD, "div5": RED}
img = Image.new("RGB", (W, H), BG); d = ImageDraw.Draw(img)

def font(sz, bold=False):
    paths = ["/usr/share/fonts/dejavu-sans-fonts/DejaVuSans%s.ttf",
             "/usr/share/fonts/dejavu/DejaVuSans%s.ttf",
             "/usr/share/fonts/truetype/dejavu/DejaVuSans%s.ttf"]
    suf = "-Bold" if bold else ""
    for p in paths:
        try: return ImageFont.truetype(p % suf, sz)
        except Exception: pass
    return ImageFont.load_default()

def text(x, y, s, fnt, fill=INK, anchor="la"):
    d.text((x, y), s, font=fnt, fill=fill, anchor=anchor)

text(W / 2, 40, "Endings of palindromic curve-values on  p = 2n²+2n+1",
     font(34, True), INK, "ma")
text(W / 2, 86, "…13 is the densest prime-eligible ending — and holds both "
     "large prime palindromes (313, 3187813)", font(19), MUT, "ma")

# axes
ax_l, ax_r = 120, W - 60
A_t, A_b = 170, 620
ymax = 12
def BY(c): return A_b - c / ymax * (A_b - A_t)
for c in range(0, ymax + 1, 2):
    d.line([ax_l, BY(c), ax_r, BY(c)], fill=GRID)
    text(ax_l - 14, BY(c), str(c), font(17), MUT, "rm")
text(ax_l - 66, (A_t + A_b) / 2, "count", font(19), INK, "mm")
d.line([ax_l, A_b, ax_r, A_b], fill=(180, 180, 190), width=2)

n = len(DATA); span = ax_r - ax_l; step = span / n
for i, (lab, cnt, kind) in enumerate(DATA):
    cx = ax_l + step * i + step / 2; bw = step * 0.5
    d.rectangle([cx - bw / 2, BY(cnt), cx + bw / 2, A_b], fill=COL[kind])
    text(cx, BY(cnt) - 8, str(cnt), font(22, True), INK, "mb")
    sub = "÷5, composite" if kind == "div5" else "prime-eligible"
    text(cx, A_b + 12, lab, font(24, True),
         (150, 110, 10) if kind == "gold" else INK, "ma")
    text(cx, A_b + 46, sub, font(14), MUT, "ma")
    if kind == "gold":
        text(cx, BY(cnt) - 40, "313, 3187813", font(15), (150, 110, 10), "mb")

# legend
ly = 690
for col, lab, x in [(GOLD, "holds the prime palindromes", 120),
                    (TEAL, "prime-eligible (composite)", 500),
                    (RED, "÷5 — trivially composite", 850)]:
    d.rectangle([x, ly - 8, x + 20, ly + 8], fill=col)
    text(x + 30, ly, lab, font(16), INK, "lm")

img.save("docs/palindrome_endings.png")
print("wrote docs/palindrome_endings.png", img.size)
