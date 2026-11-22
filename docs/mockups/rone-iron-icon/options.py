"""RONE Iron icon - new directions (mockup). Same family frame as RoneIron/tools/make_icon.py."""
import pathlib, sys, math
import numpy as np
from PIL import Image, ImageChops, ImageDraw, ImageFilter, ImageFont

OUT = pathlib.Path(__file__).resolve().parent
NEON = (229, 82, 255)
S = 4

def frame(n):
    im = Image.new("RGBA", (n, n), (0, 0, 0, 0)); d = ImageDraw.Draw(im)
    r = int(n * 0.22); pad = int(n * 0.04)
    d.rounded_rectangle([pad, pad, n - pad, n - pad], radius=r, fill=(20, 22, 26, 255))
    ring = Image.new("RGBA", (n, n), (0, 0, 0, 0))
    ImageDraw.Draw(ring).rounded_rectangle([pad + n*0.02]*2 + [n - pad - n*0.02]*2, radius=int(r*0.9),
                                           outline=NEON + (255,), width=int(n*0.022))
    im.alpha_composite(Image.blend(Image.new("RGBA", (n, n)), ring.filter(ImageFilter.GaussianBlur(n*0.02)), 0.55))
    im.alpha_composite(ring)
    fpad = pad + int(n*0.075)
    ImageDraw.Draw(im).rounded_rectangle([fpad, fpad, n-fpad, n-fpad], radius=int(r*0.7), fill=(11, 13, 16, 255))
    # soft orchid bloom in the middle of the face, like Flanger / Stucker
    bloom = Image.new("RGBA", (n, n)); ImageDraw.Draw(bloom).ellipse([n*0.28, n*0.28, n*0.72, n*0.72], fill=NEON + (60,))
    face = Image.new("L", (n, n)); ImageDraw.Draw(face).rounded_rectangle([fpad, fpad, n-fpad, n-fpad], radius=int(r*0.7), fill=255)
    bloom = bloom.filter(ImageFilter.GaussianBlur(n*0.09)); bloom.putalpha(ImageChops.multiply(bloom.split()[3], face))
    im.alpha_composite(bloom)
    return im, face

def metal(mask, n, top, bot, hot=0.55):
    """fill a mask white-hot at the top fading to orchid, with a neon glow behind"""
    ys = np.clip((np.arange(n, dtype=np.float64) - top) / max(1, bot - top), 0, 1)
    t = np.clip(ys / hot, 0, 1)[:, None] * np.ones((1, n))
    g = np.zeros((n, n, 4), np.uint8)
    for j in range(3): g[..., j] = (255*(1-t) + NEON[j]*t).astype(np.uint8)
    g[..., 3] = np.array(mask)
    body = Image.fromarray(g, "RGBA")
    glow = Image.new("RGBA", (n, n), NEON + (0,)); glow.putalpha(mask.filter(ImageFilter.GaussianBlur(n*0.03)))
    out = Image.new("RGBA", (n, n))
    out.alpha_composite(Image.blend(Image.new("RGBA", (n, n)), glow, 0.9))
    out.alpha_composite(body)
    return out

def tube(mask, n):
    """neon tube look (Hook's J / Flanger's infinity): orchid with glow + a pale core"""
    glow = Image.new("RGBA", (n, n), NEON + (0,)); glow.putalpha(mask.filter(ImageFilter.GaussianBlur(n*0.035)))
    body = Image.new("RGBA", (n, n), NEON + (0,)); body.putalpha(mask)
    core_m = mask.filter(ImageFilter.MinFilter(int(n*0.03) | 1)).filter(ImageFilter.GaussianBlur(n*0.008))
    core = Image.new("RGBA", (n, n), (255, 200, 255, 0)); core.putalpha(core_m.point(lambda v: int(v*0.75)))
    out = Image.new("RGBA", (n, n))
    out.alpha_composite(glow); out.alpha_composite(glow); out.alpha_composite(body); out.alpha_composite(core)
    return out

# ---------- A: a vocal phrase (a filled waveform) chopped into four slices with clear gaps, nudged up / down
def env(u):
    return (0.55*math.exp(-((u-0.30)/0.16)**2) + 0.85*math.exp(-((u-0.62)/0.17)**2) + 0.08) * (u*(1-u)*4)**0.35

def opt_a(n):
    im, face = frame(n)
    x0, x1, cy = n*0.22, n*0.78, n*0.50
    m = Image.new("L", (n, n)); d = ImageDraw.Draw(m)
    N = 400; top = []; bot = []
    for i in range(N+1):
        u = i/N; x = x0 + (x1-x0)*u
        a = env(u) * (0.72 + 0.28*abs(math.sin(u*math.pi*11))) * n*0.25
        top.append((x, cy-a)); bot.append((x, cy+a))
    d.polygon(top + bot[::-1], fill=255)
    cuts = [0.27, 0.50, 0.72]; gw = n*0.028
    edges = [x0-1] + [x0+(x1-x0)*c for c in cuts] + [x1+1]
    nud = [-0.03, 0.035, -0.035, 0.025]
    out = Image.new("L", (n, n))
    for k in range(4):
        a = edges[k] + (gw/2 if k else 0); b = edges[k+1] - (gw/2 if k < 3 else 0)
        sl = Image.new("L", (n, n)); ImageDraw.Draw(sl).rectangle([a, 0, b, n], fill=255)
        out = ImageChops.lighter(out, ImageChops.offset(ImageChops.multiply(m, sl), 0, int(nud[k]*n)))
    art = metal(out, n, n*0.24, n*0.76, hot=0.55)
    art.putalpha(ImageChops.multiply(art.split()[3], face)); im.alpha_composite(art)
    return im

# ---------- B: a microphone whose head is chopped into three slices, the middle one thrown aside
def opt_b(n):
    im, face = frame(n)
    cx = n*0.5; top, bot = n*0.20, n*0.55; hw = n*0.12
    cap = Image.new("L", (n, n)); ImageDraw.Draw(cap).rounded_rectangle([cx-hw, top, cx+hw, bot], radius=int(hw), fill=255)
    g1, g2 = top + (bot-top)*0.37, top + (bot-top)*0.64; gw = n*0.026
    edges = [(0, g1-gw/2), (g1+gw/2, g2-gw/2), (g2+gw/2, n)]
    shift = [-n*0.012, n*0.05, -n*0.012]
    head = Image.new("L", (n, n))
    for (a, b), sx in zip(edges, shift):
        sl = Image.new("L", (n, n)); ImageDraw.Draw(sl).rectangle([0, a, n, b], fill=255)
        head = ImageChops.lighter(head, ImageChops.offset(ImageChops.multiply(cap, sl), int(sx), 0))
    stand = Image.new("L", (n, n)); sd = ImageDraw.Draw(stand); lw = int(n*0.03)
    ar = n*0.185; acy = n*0.43
    sd.arc([cx-ar, acy-ar, cx+ar, acy+ar], 15, 165, fill=255, width=lw)
    sd.line([cx, acy+ar-lw/2, cx, n*0.74], fill=255, width=lw)
    sd.rounded_rectangle([cx-n*0.10, n*0.735, cx+n*0.10, n*0.735+lw], radius=lw//2, fill=255)
    art = tube(stand, n)
    art.alpha_composite(metal(head, n, top, bot, hot=0.8))
    art.putalpha(ImageChops.multiply(art.split()[3], face)); im.alpha_composite(art)
    return im

# ---------- C: a neon voice line (a tube, like Grain / Analyzer) cut into three pieces that step apart
def opt_c(n):
    im, face = frame(n)
    x0, x1, cy = n*0.23, n*0.77, n*0.50
    pts = []
    N = 300
    for i in range(N+1):
        u = i/N; a = env(u)*n*0.22
        pts.append((x0+(x1-x0)*u, cy - a*math.sin(u*math.pi*5)))
    cuts = [0.30, 0.62]; gap = 0.035
    rng = [(0, cuts[0]-gap), (cuts[0]+gap, cuts[1]-gap), (cuts[1]+gap, 1)]
    nud = [-0.035, 0.035, -0.02]
    m = Image.new("L", (n, n)); lw = int(n*0.042)
    for (a, b), dy in zip(rng, nud):
        seg = [(x, y+dy*n) for (x, y), k in zip(pts, range(N+1)) if a <= k/N <= b]
        piece = Image.new("L", (n, n)); pd = ImageDraw.Draw(piece); rr = lw/2
        for p, q in zip(seg, seg[1:]):
            steps = max(1, int(math.hypot(q[0]-p[0], q[1]-p[1]) / 2))
            for s in range(steps):
                x = p[0] + (q[0]-p[0])*s/steps; y = p[1] + (q[1]-p[1])*s/steps
                pd.ellipse([x-rr, y-rr, x+rr, y+rr], fill=255)
        m = ImageChops.lighter(m, piece)
    art = tube(m, n)
    art.putalpha(ImageChops.multiply(art.split()[3], face)); im.alpha_composite(art)
    return im

OPTS = {"A": opt_a, "B": opt_b, "C": opt_c}

def render(fn, size): return fn(size*S).resize((size, size), Image.LANCZOS)

if __name__ == "__main__":
    for k, fn in OPTS.items():
        render(fn, 512).save(OUT / f"option-{k}.png")
    print("ok")
