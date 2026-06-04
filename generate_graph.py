#!/usr/bin/env python3
"""Render the BigFermat findings on the curve p = 2n^2+2n+1 to a PNG (Pillow only)."""
import math
from PIL import Image, ImageDraw, ImageFont

# ---------- data ----------
prime_pals = [(1, 5, 1), (9, 181, 3), (12, 313, 3), (1262, 3187813, 7)]  # (n, p, d)
landscape = [(7,5),(8,2),(9,0),(10,0),(11,0),(12,0),(13,4),(14,6),(15,5),
             (16,0),(17,1),(18,0),(19,0),(20,0),(21,0),(22,0)]
OBSTR = {9,10,11,12,16,18,19,20,21,22}
NEW = -1  # d=22 since PROVEN obstructed by exhaustive brute force (raw=0, EMIRPS=0)

# ---------- canvas ----------
W, H = 1500, 1560
BG=(252,252,250); INK=(34,34,40); GRID=(225,225,230); MUT=(120,120,130)
TEAL=(38,140,150); RED=(208,70,70); GOLD=(232,170,30); ORANGE=(230,130,40)
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
text(W/2, 90, "what an obstruction sieve + exhaustive search found  ·  BigFermat, 2026-06-04",
     F(22), MUT, "ma")

# ================= PANEL A : the curve, log-log =================
ax_l, ax_r = 150, W-90
A_t, A_b = 150, 690
text(ax_l, A_t-34, "① The curve and its prime palindromes  (log–log)", FB(26), INK, "la")

# axis ranges: x=log10(n) 0..11 ; y=log10(p) 0..22.5
xlo,xhi = 0,11.0; ylo,yhi = 0,22.5
def AX(lx): return ax_l + (lx-xlo)/(xhi-xlo)*(ax_r-ax_l)
def AY(ly): return A_b - (ly-ylo)/(yhi-ylo)*(A_b-A_t)

# frontier shading (explored to d=22  ->  n ~ 7e10, log ~ 10.85)
d.rectangle([AX(0),A_t,AX(10.85),A_b], fill=FRONT)
# gridlines
for gx in range(0,12,1):
    d.line([AX(gx),A_t,AX(gx),A_b], fill=GRID)
    text(AX(gx),A_b+8,sup(gx) if gx else "1",F(15),MUT,"ma")
for gy in range(0,23,5):
    d.line([ax_l,AY(gy),ax_r,AY(gy)], fill=GRID)
    text(ax_l-10,AY(gy),sup(gy) if gy else "1",F(15),MUT,"rm")
text((ax_l+ax_r)/2, A_b+38, "n", F(20), INK, "ma")
text(ax_l-86, (A_t+A_b)/2, "p", F(20), INK, "mm")
d.rectangle([ax_l,A_t,ax_r,A_b], outline=(180,180,190))

# the curve polyline
pts=[]
for i in range(0,441):
    lx = xlo + (xhi-xlo)*i/440
    n = 10**lx
    p = 2*n*n+2*n+1
    pts.append((AX(lx), AY(math.log10(p))))
d.line(pts, fill=TEAL, width=4)

# frontier line + label
d.line([AX(10.85),A_t,AX(10.85),A_b], fill=ORANGE, width=2)
text(AX(10.85)-8, A_t+8, "explored to d=22 →", F(16), ORANGE, "ra")

# "desert" annotation along the upper curve
text(AX(6.8), AY(11.5), "no prime palindrome", F(19), MUT, "mm")
text(AX(6.8), AY(10.3), "(confirmed d = 8 … 21)", F(17), MUT, "mm")

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
     "3187813  (d=7) — the largest, since 1997", F(18), (150,110,10), "lm")
text(AX(0)+14, AY(math.log10(5))-6, "5, 181, 313", F(15), (150,110,10), "lb")
text(ax_l+14, A_t+14, "★ prime palindrome on the curve   (only four exist, all d ≤ 7)",
     F(17), (150,110,10), "la")

# ================= PANEL B : obstruction landscape =================
B_t, B_b = 840, 1380
bx_l, bx_r = 150, W-90
text(bx_l, B_t-34, "② Obstruction landscape by digit-length d  (emirp survivors)", FB(26), INK,"la")

dmin,dmax = 7,22; ymax=7.6
def BX(dd):
    # centered columns
    span=(bx_r-bx_l); step=span/(dmax-dmin+1)
    return bx_l + step*(dd-dmin) + step/2
def BY(c): return B_b - c/ymax*(B_b-B_t)
step=(bx_r-bx_l)/(dmax-dmin+1)

# proven band  d=7..22
d.rectangle([BX(7)-step/2, B_t, BX(22)+step/2, B_b], fill=BAND)
text((BX(7)+BX(22))/2, B_t+12,
     "PROVEN: no bi-quadratic emirp with ≤ 22 digits", FB(20), (40,120,90), "ma")

# y grid
for c in range(0,7):
    d.line([bx_l,BY(c),bx_r,BY(c)], fill=GRID)
    text(bx_l-12,BY(c),str(c),F(16),MUT,"rm")
text(bx_l-70,(B_t+B_b)/2,"survivors",F(19),INK,"mm")
d.line([bx_l,B_b,bx_r,B_b], fill=(180,180,190), width=2)

for (dd,c) in landscape:
    cx=BX(dd); bw=step*0.52
    if dd in OBSTR:
        # obstruction: red X on the baseline
        r=11
        d.line([cx-r,B_b-r,cx+r,B_b+r],fill=RED,width=4)
        d.line([cx-r,B_b+r,cx+r,B_b-r],fill=RED,width=4)
    else:
        col = ORANGE if dd==NEW else TEAL
        d.rectangle([cx-bw/2,BY(c),cx+bw/2,B_b], fill=col)
        text(cx,BY(c)-8,str(c),FB(20),col,"mb")
    text(cx,B_b+12,str(dd),F(18),INK if dd!=NEW else ORANGE,"ma")
text((bx_l+bx_r)/2,B_b+44,"digit-length  d",F(20),INK,"ma")

# star over d=7 (prime palindrome lives there) — well above the "5" count
star(BX(7), BY(5)-52, 12, GOLD)

# callouts — over the empty d=17..21 obstruction zone (clear of tall bars & band text)
cox = (BX(17)+BX(21))/2
text(cox, B_t+72, "all survivors so far are composite", F(18), TEAL, "ma")
text(cox, B_t+96, "→ zero actual emirps found", F(18), TEAL, "ma")
# (d=22 is now a proven obstruction — drawn as a red ✗, no special marker)

# legend
ly=B_b+80; lx=bx_l
def chip(x,y,col,kind):
    if kind=="x":
        d.line([x,y-8,x+16,y+8],fill=col,width=4); d.line([x,y+8,x+16,y-8],fill=col,width=4)
    elif kind=="star": star(x+8,y,9,col)
    else: d.rectangle([x,y-8,x+16,y+8],fill=col)
items=[(TEAL,"box","survivors (candidates, all composite)"),
       (RED,"x","obstruction — proven impossible"),
       (GOLD,"star","prime palindrome present")]
for col,k,lab in items:
    chip(lx,ly,col,k); text(lx+28,ly,lab,F(17),INK,"lm"); lx+=330 if len(lab)<34 else 360
    if lx>bx_r-200: lx=bx_l; ly+=34

img.save("docs/biquad_curve_landscape.png")
print("wrote docs/biquad_curve_landscape.png", img.size)
