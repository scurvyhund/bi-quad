#!/usr/bin/env python3
"""Render the BigFermat findings on the curve p = 2n^2+2n+1 to a PNG (Pillow only)."""
import math
from PIL import Image, ImageDraw, ImageFont

# ---------- data ----------
prime_pals = [(1, 5, 1), (9, 181, 3), (12, 313, 3), (1262, 3187813, 7)]  # (n, p, d)
# The ONE bi-quadratic emirp: 12641 (n=79) <-> 14621 (n=85), at d=5.
emirp_pts = [(79, 12641), (85, 14621)]
# survivors = raw count of n whose p AND rev(p) both lie on the curve, per d
# (authoritative: GMP brute force hunt.c, exhaustive, every n; d=5..24)
landscape = [(5,6),(6,0),(7,7),(8,2),(9,6),(10,0),(11,5),(12,2),(13,4),(14,6),
             (15,6),(16,2),(17,1),(18,0),(19,3),(20,0),(21,7),(22,0),(23,3),(24,2)]
OBSTR = {6,10,18,20,22}      # zero curve-reversal pairs at all (true obstructions)
EMIRP_D = 5                  # the only emirp lives here (12641 <-> 14621)

# ---------- canvas ----------
W, H = 1500, 1560
BG=(252,252,250); INK=(34,34,40); GRID=(225,225,230); MUT=(120,120,130)
TEAL=(38,140,150); RED=(208,70,70); GOLD=(232,170,30); ORANGE=(230,130,40)
EMR=(150,45,160)  # the emirp — its own distinct colour
BAND=(232,244,238); FRONT=(245,238,228)
img = Image.new("RGB",(W,H),BG); d = ImageDraw.Draw(img)

def font(sz, bold=False):
    paths = ["/usr/share/fonts/dejavu-sans-fonts/DejaVuSans%s.ttf",
             "/usr/share/fonts/dejavu/DejaVuSans%s.ttf",
             "/usr/share/fonts/truetype/dejavu/DejaVuSans%s.ttf"]
    suf = "-Bold" if bold else ""
    for p in paths:
        try: return ImageFont.truetype(p % suf, sz)
        except Exception: pass
    return ImageFont.load_default()
F=lambda s:font(s); FB=lambda s:font(s,True)
def sup(k):
    S="⁰¹²³⁴⁵⁶⁷⁸⁹"
    return "10"+"".join(S[int(c)] for c in str(k))

def text(x,y,s,fnt,fill=INK,anchor="la"):
    d.text((x,y),s,font=fnt,fill=fill,anchor=anchor)

# ================= header =================
text(W/2, 40, "The curve  p = 2n² + 2n + 1  =  n² + (n+1)²", FB(40), INK, "ma")
text(W/2, 90, "what an obstruction sieve + exhaustive brute force found  ·  BigFermat, 2026-06-05",
     F(22), MUT, "ma")

# ================= PANEL A : the curve, log-log =================
ax_l, ax_r = 150, W-90
A_t, A_b = 188, 700
text(ax_l, A_t-34, "① The curve, its palindromes, and the lone emirp  (log–log)", FB(26), INK, "la")

# axis ranges: x=log10(n) 0..12 ; y=log10(p) 0..24.5
xlo,xhi = 0,12.0; ylo,yhi = 0,24.5
def AX(lx): return ax_l + (lx-xlo)/(xhi-xlo)*(ax_r-ax_l)
def AY(ly): return A_b - (ly-ylo)/(yhi-ylo)*(A_b-A_t)

# frontier shading (exhaustively searched to d=24  ->  n ~ 7e11, log ~ 11.85)
LXF = 11.85
d.rectangle([AX(0),A_t,AX(LXF),A_b], fill=FRONT)
# gridlines
for gx in range(0,13,1):
    d.line([AX(gx),A_t,AX(gx),A_b], fill=GRID)
    text(AX(gx),A_b+8,sup(gx) if gx else "1",F(15),MUT,"ma")
for gy in range(0,25,5):
    d.line([ax_l,AY(gy),ax_r,AY(gy)], fill=GRID)
    text(ax_l-10,AY(gy),sup(gy) if gy else "1",F(15),MUT,"rm")
text((ax_l+ax_r)/2, A_b+38, "n", F(20), INK, "ma")
text(ax_l-86, (A_t+A_b)/2, "p", F(20), INK, "mm")
d.rectangle([ax_l,A_t,ax_r,A_b], outline=(180,180,190))

# the curve polyline
pts=[]
for i in range(0,481):
    lx = xlo + (xhi-xlo)*i/480
    n = 10**lx
    p = 2*n*n+2*n+1
    pts.append((AX(lx), AY(math.log10(p))))
d.line(pts, fill=TEAL, width=4)

# frontier line + label
d.line([AX(LXF),A_t,AX(LXF),A_b], fill=ORANGE, width=2)
text(AX(LXF)-8, A_t+8, "exhaustively searched to d=24 →", F(16), ORANGE, "ra")

# "desert" annotation along the upper curve
text(AX(7.2), AY(12.5), "no prime palindrome", F(19), MUT, "mm")
text(AX(7.2), AY(11.1), "(confirmed d = 8 … 21)", F(17), MUT, "mm")

# prime palindrome stars
def star(cx,cy,r,fill):
    pp=[]
    for k in range(10):
        ang=-math.pi/2+k*math.pi/5
        rr=r if k%2==0 else r*0.42
        pp.append((cx+rr*math.cos(ang), cy+rr*math.sin(ang)))
    d.polygon(pp, fill=fill, outline=(150,110,10))
for (n,p,dd) in prime_pals:
    cx,cy = AX(math.log10(n)), AY(math.log10(p))
    star(cx,cy,11,GOLD)
text(AX(math.log10(1262))+18, AY(math.log10(3187813)),
     "3187813  (d=7) — the largest palindrome, since 1997", F(18), (150,110,10), "lm")
text(AX(0)+14, AY(math.log10(5))-6, "5, 181, 313", F(15), (150,110,10), "lb")

# the ONE bi-quadratic emirp (diamond markers)
def diamond(cx,cy,r,fill,outline):
    d.polygon([(cx,cy-r),(cx+r,cy),(cx,cy+r),(cx-r,cy)], fill=fill, outline=outline)
for (nn,pp) in emirp_pts:
    cx,cy = AX(math.log10(nn)), AY(math.log10(pp))
    diamond(cx,cy,9,EMR,(90,20,100))
text(AX(math.log10(85))+18, AY(math.log10(14621)),
     "12641 ⟷ 14621  (d=5) — the only bi-quadratic emirp", F(18), EMR, "lm")

text(ax_l+14, A_t+14, "★ prime palindrome (only four, all d ≤ 7)    ◆ bi-quadratic emirp (only one)",
     F(17), INK, "la")

# ================= PANEL B : obstruction landscape =================
B_t, B_b = 840, 1380
bx_l, bx_r = 150, W-90
text(bx_l, B_t-34, "② Survivor landscape by digit-length d  (n with p & rev(p) both on curve)", FB(24), INK,"la")

dmin,dmax = 5,24; ymax=8.0
def BX(dd):
    # centered columns
    span=(bx_r-bx_l); step=span/(dmax-dmin+1)
    return bx_l + step*(dd-dmin) + step/2
def BY(c): return B_b - c/ymax*(B_b-B_t)
step=(bx_r-bx_l)/(dmax-dmin+1)

# result band  d=5..24
d.rectangle([BX(5)-step/2, B_t, BX(24)+step/2, B_b], fill=BAND)
text((BX(5)+BX(24))/2, B_t+12,
     "12641 ⟷ 14621  is the ONLY bi-quadratic emirp through 24 digits", FB(20), (40,120,90), "ma")

# y grid
for c in range(0,8):
    d.line([bx_l,BY(c),bx_r,BY(c)], fill=GRID)
    text(bx_l-12,BY(c),str(c),F(16),MUT,"rm")
text(bx_l-70,(B_t+B_b)/2,"survivors",F(19),INK,"mm")
d.line([bx_l,B_b,bx_r,B_b], fill=(180,180,190), width=2)

for (dd,c) in landscape:
    cx=BX(dd); bw=step*0.52
    if dd in OBSTR:
        # obstruction: red X on the baseline
        r=10
        d.line([cx-r,B_b-r,cx+r,B_b+r],fill=RED,width=4)
        d.line([cx-r,B_b+r,cx+r,B_b-r],fill=RED,width=4)
    else:
        col = EMR if dd==EMIRP_D else TEAL
        d.rectangle([cx-bw/2,BY(c),cx+bw/2,B_b], fill=col)
        text(cx,BY(c)-8,str(c),FB(18),col,"mb")
    text(cx,B_b+12,str(dd),F(15),EMR if dd==EMIRP_D else INK,"ma")
text((bx_l+bx_r)/2,B_b+44,"digit-length  d",F(20),INK,"ma")

# star over d=7 (the palindrome 3187813 lives there)
star(BX(7), BY(2)-44, 12, GOLD)

# emirp callout pointing at the d=5 column
text(BX(5)+step*0.55, BY(6)-30, "the only emirp:", F(15), EMR, "lb")
text(BX(5)+step*0.55, BY(6)-12, "12641 ⟷ 14621", FB(15), EMR, "lb")

# callouts — over the empty mid zone
cox = (BX(14)+BX(20))/2
text(cox, B_t+74, "every other survivor through d=24 is composite", F(18), TEAL, "ma")
text(cox, B_t+98, "→ exactly one emirp, at d=5", F(18), EMR, "ma")

# legend
ly=B_b+80; lx=bx_l
def chip(x,y,col,kind):
    if kind=="x":
        d.line([x,y-8,x+16,y+8],fill=col,width=4); d.line([x,y+8,x+16,y-8],fill=col,width=4)
    elif kind=="star": star(x+8,y,9,col)
    elif kind=="diamond": diamond(x+8,y,9,col,(90,20,100))
    else: d.rectangle([x,y-8,x+16,y+8],fill=col)
items=[(EMR,"diamond","emirp 12641⟷14621 (d=5)"),
       (TEAL,"box","survivors (all composite)"),
       (RED,"x","obstruction (no candidates)"),
       (GOLD,"star","prime palindrome")]
for col,k,lab in items:
    chip(lx,ly,col,k); text(lx+28,ly,lab,F(16),INK,"lm"); lx+=330
    if lx>bx_r-300: lx=bx_l; ly+=34

img.save("docs/biquad_curve_landscape.png")
print("wrote docs/biquad_curve_landscape.png", img.size)
