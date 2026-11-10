"""Static sprites: bricks, cracks, paddle, ball, power-ups, UI, background."""
import os, math, json
import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont
from scipy import ndimage
from common import *

BW, BH = 128, 48

PAL = {
    "sandstone": dict(dark=(132, 88, 48), light=(226, 180, 118), hits=1),
    "limestone": dict(dark=(128, 120, 104), light=(222, 212, 188), hits=2),
    "granite":   dict(dark=(46, 50, 60), light=(122, 128, 140), hits=3),
    "jade":      dict(dark=(14, 78, 52), light=(96, 196, 136), hits=1),
    "gold":      dict(dark=(128, 78, 14), light=(255, 214, 96), hits=1),
    "obsidian":  dict(dark=(12, 10, 18), light=(62, 52, 78), hits=0),
}
GLYPHS = {
    "sandstone": ["sun", "waves", "steps", "spiral"],
    "limestone": ["eye", "meander", "crescent", "diamond"],
    "granite":   ["steps", "spiral", "sun", "waves"],
    "jade":      ["diamond", "eye"],
    "gold":      ["sun", "eye"],
    "obsidian":  ["diamond"],
}


def ensure(*p):
    d = os.path.join(OUT, *p)
    os.makedirs(d, exist_ok=True)
    return d


def stone_shape(W, H, seed, rough=True, chips=True):
    S = SS
    m = rounded_mask(W, H, 6 * S, inset=2 * S)
    if rough:
        dist = ndimage.distance_transform_edt(m > 0.5)
        n = fbm(H, W, 6 * S, 3, seed + 7)
        m = np.clip((dist - n * 2.2 * S) / (1.0 * S), 0, 1)
    if chips:
        r = np.random.default_rng(seed)
        im = Image.fromarray((m * 255).astype(np.uint8))
        d = ImageDraw.Draw(im)
        for _ in range(r.integers(1, 3)):
            corner = r.integers(0, 4)
            cx = 2 * S if corner in (0, 2) else W - 2 * S
            cy = 2 * S if corner in (0, 1) else H - 2 * S
            sx = 1 if corner in (0, 2) else -1
            sy = 1 if corner in (0, 1) else -1
            a, b = r.uniform(5, 11) * S, r.uniform(4, 9) * S
            d.polygon([(cx - sx * 2, cy - sy * 2), (cx + sx * a, cy - sy * 2), (cx + sx * a * 0.4, cy + sy * b * 0.5),
                       (cx - sx * 2, cy + sy * b)], fill=0)
        m = ndimage.gaussian_filter(np.array(im) / 255.0, 0.6 * S)
    return m


def stone_texture(W, H, dark, light, seed, strata=False, speckle=0.0, veins=False):
    t = fbm(H, W, 30 * SS, 5, seed)
    t = (t - t.min()) / (t.max() - t.min() + 1e-6)
    t = 0.25 + 0.6 * t
    if strata:
        y = np.arange(H)[:, None] / H
        warp = fbm(H, W, 40 * SS, 2, seed + 3)
        t += 0.09 * np.sin((y * 9 + warp * 3) * math.pi)
    if veins:
        v = fbm(H, W, 24 * SS, 4, seed + 11)
        v = np.exp(-((v - 0.5) / 0.03) ** 2)
        t += 0.35 * v
    if speckle:
        r = np.random.default_rng(seed + 5)
        sp = r.random((H, W))
        sp = ndimage.gaussian_filter((sp > 0.985).astype(float), 0.6 * SS)
        sp2 = ndimage.gaussian_filter((r.random((H, W)) > 0.985).astype(float), 0.6 * SS)
        t += speckle * (sp * 2.5 - sp2 * 2.5)
    rgb = lerp_rgb(dark, light, np.clip(t, 0, 1.2))
    y = np.linspace(1.08, 0.9, H)[:, None, None]
    return rgb * y


def crack_mask(W, H, seed, n_main=2, branch=0.35, start=None):
    r = np.random.default_rng(seed)
    im = Image.new("L", (W, H), 0)
    d = ImageDraw.Draw(im)
    S = SS
    if start is None:
        start = (W * r.uniform(0.35, 0.65), H * r.uniform(0.3, 0.7))

    def walk(x, y, ang, length, width, depth):
        pts = [(x, y)]
        steps = int(length / (4 * S))
        for i in range(steps):
            ang += r.normal(0, 0.45)
            x += math.cos(ang) * 4 * S; y += math.sin(ang) * 4 * S
            pts.append((x, y))
            if depth < 2 and r.random() < branch * 0.25:
                walk(x, y, ang + r.choice([-1, 1]) * r.uniform(0.6, 1.2), length * 0.45, width * 0.7, depth + 1)
        d.line(pts, fill=255, width=max(1, int(width)), joint="curve")

    for k in range(n_main):
        ang = r.uniform(0, 2 * math.pi)
        walk(start[0], start[1], ang, r.uniform(0.35, 0.6) * W, 1.9 * S, 0)
        walk(start[0], start[1], ang + math.pi + r.normal(0, 0.4), r.uniform(0.2, 0.4) * W, 1.6 * S, 0)
    return np.array(im.filter(ImageFilter.GaussianBlur(0.35 * S))) / 255.0, start


def apply_cracks(rgb, alpha, cm):
    sh = ndimage.shift(cm, (1.2 * SS, 1.2 * SS), order=1)
    rgb = rgb * (1 - cm[..., None] * 0.78)
    rgb = rgb + np.clip(sh - cm, 0, 1)[..., None] * 45
    return rgb, alpha


def finish(rgb, mask, W, H, w, h):
    shade = bevel_shading(mask, 4 * SS, 0.32)
    rgb = rgb * shade[..., None]
    return down(to_img(rgb, mask), w, h)


def make_brick(kind, gname, seed, cracks=0, w=BW, h=BH, return_layers=False):
    W, H = w * SS, h * SS
    p = PAL[kind]
    smooth = kind in ("gold", "obsidian", "jade")
    mask = stone_shape(W, H, seed, rough=not smooth, chips=not smooth)
    if smooth:
        mask = rounded_mask(W, H, 7 * SS, inset=2 * SS)
        mask = ndimage.gaussian_filter(mask, 0.5 * SS)
    rgb = stone_texture(W, H, p["dark"], p["light"], seed,
                        strata=kind in ("sandstone", "limestone"),
                        speckle=0.25 if kind == "granite" else 0.06,
                        veins=kind == "jade")
    g = glyph(gname, W, H, int(2.6 * SS))
    if kind == "gold":
        # metallic: sharpen contrast, specular band, raised glyph + inner frame
        lum = rgb.mean(-1, keepdims=True)
        rgb = lerp_rgb(p["dark"], p["light"], np.clip(lum[..., 0] / 255 * 1.3 - 0.15, 0, 1))
        y = np.arange(H)[:, None] / H
        band = np.exp(-((y - 0.3) / 0.12) ** 2) * 70 + np.exp(-((y - 0.78) / 0.08) ** 2) * 25
        rgb = rgb + band[..., None] * np.array([1, 0.95, 0.75])
        frame = rounded_mask(W, H, 5 * SS, inset=6 * SS) - rounded_mask(W, H, 4 * SS, inset=8 * SS)
        rgb = emboss_raise(rgb, np.clip(frame, 0, 1), 1.0 * SS, 55)
        rgb = emboss_raise(rgb, g, 1.2 * SS, 70)
    elif kind == "obsidian":
        y = np.arange(H)[:, None] / H
        rgb = rgb + (np.exp(-((y - 0.28) / 0.1) ** 2) * 40)[..., None]
        frame = np.clip(rounded_mask(W, H, 7 * SS, inset=2 * SS) - rounded_mask(W, H, 4 * SS, inset=8 * SS), 0, 1)
        bronze = stone_texture(W, H, (96, 56, 24), (226, 160, 82), seed + 99)
        bronze = bronze + band_spec(H, W)
        rgb = rgb * (1 - frame[..., None]) + bronze * frame[..., None]
        rgb = emboss_raise(rgb, frame, 1.0 * SS, 45)
        # rivets
        riv = Image.new("L", (W, H), 0); dr = ImageDraw.Draw(riv)
        for (x, yy) in [(5 * SS, 5 * SS), (W - 5 * SS, 5 * SS), (5 * SS, H - 5 * SS), (W - 5 * SS, H - 5 * SS)]:
            pass
        for x in (W * 0.5,):
            pass
        rivm = np.array(riv) / 255.0
        gl = g * 0.9
        rgb = carve(rgb, gl, 1.0 * SS, dark=0.5)
        # faint purple glint in glyph
        rgb = rgb + (gl[..., None] * np.array([40, 20, 70]))
    elif kind == "jade":
        y = np.arange(H)[:, None] / H
        rgb = rgb + (np.exp(-((y - 0.25) / 0.1) ** 2) * 55)[..., None] * np.array([0.8, 1, 0.9])
        rgb = carve(rgb, g, 1.3 * SS, dark=0.6)
    else:
        rgb = carve(rgb, g, 1.4 * SS, dark=0.62)
    cms = []
    if cracks:
        cm1, st = crack_mask(W, H, seed + 1000, n_main=1, branch=0.3)
        cms.append(cm1)
        total = cm1
        if cracks >= 2:
            cm2, _ = crack_mask(W, H, seed + 2000, n_main=2, branch=0.6, start=st)
            total = np.clip(cm1 + cm2, 0, 1)
            # knock a chunk off a corner
            r = np.random.default_rng(seed + 3)
            im = Image.fromarray((mask * 255).astype(np.uint8)); d = ImageDraw.Draw(im)
            cx = W - 2 * SS if r.random() < 0.5 else 2 * SS
            sx = -1 if cx > W / 2 else 1
            d.polygon([(cx, H * 0.15), (cx + sx * 16 * SS, H * 0.05), (cx + sx * 9 * SS, H * 0.55),
                       (cx, H * 0.7)], fill=0)
            mask = ndimage.gaussian_filter(np.array(im) / 255.0, 0.5 * SS)
        rgb, mask = apply_cracks(rgb, mask, total)
    img = finish(rgb, mask, W, H, w, h)
    return img


def band_spec(H, W):
    y = np.arange(H)[:, None] / H
    return (np.exp(-((y - 0.3) / 0.1) ** 2) * 50)[..., None] * np.ones(3)


def crack_overlay(stage, seed=4242, w=BW, h=BH):
    W, H = w * SS, h * SS
    cm1, st = crack_mask(W, H, seed, n_main=1, branch=0.3)
    cm = cm1
    if stage >= 2:
        cm2, _ = crack_mask(W, H, seed + 1, n_main=2, branch=0.6, start=st)
        cm = np.clip(cm1 + cm2, 0, 1)
    sh = ndimage.shift(cm, (1.2 * SS, 1.2 * SS), order=1)
    lip = np.clip(sh - cm, 0, 1)
    clip = rounded_mask(W, H, 6 * SS, inset=4 * SS)
    rgb = np.zeros((H, W, 3)) + np.array([25, 16, 8])
    rgb = rgb * (1 - lip[..., None]) + np.array([255, 235, 200]) * lip[..., None]
    a = np.clip(cm * 0.85 + lip * 0.35, 0, 1) * clip
    return down(to_img(rgb, a), w, h)


# ------------------------------------------------------------------ paddle
def make_paddle(w, h=48, glow=0.0):
    W, H = w * SS, h * SS
    cap = 30 * SS
    body = rounded_mask(W, H, H // 2, inset=2 * SS)
    rgb = stone_texture(W, H, (120, 92, 62), (214, 182, 136), 77 + w, strata=True)
    gold = stone_texture(W, H, (130, 80, 16), (255, 216, 100), 31)
    gold = gold + band_spec(H, W) * np.array([1, 0.9, 0.6])
    xs = np.arange(W)[None, :] * np.ones((H, 1))
    capm = ((xs < cap) | (xs > W - cap)).astype(float)
    capm = ndimage.gaussian_filter(capm, 0.6 * SS)
    rgb = rgb * (1 - capm[..., None]) + gold * capm[..., None]
    # gold seam lines next to caps
    seam = (np.abs(xs - cap) < 1.6 * SS) | (np.abs(xs - (W - cap)) < 1.6 * SS)
    rgb = emboss_raise(rgb, ndimage.gaussian_filter(seam.astype(float), 0.5 * SS), 1 * SS, 50)
    # center groove line + inlaid sun
    groove = np.zeros((H, W))
    groove[int(H * 0.5 - 0.8 * SS):int(H * 0.5 + 0.8 * SS), cap + 8 * SS: W - cap - 8 * SS] = 1
    gw = 40 * SS
    gm = np.zeros((H, W))
    gsun = glyph("sun", gw, H, int(2.4 * SS))
    x0 = W // 2 - gw // 2
    gm[:, x0:x0 + gw] = gsun
    clear = np.zeros((H, W)); clear[:, x0 - 4 * SS:x0 + gw + 4 * SS] = 1
    groove = ndimage.gaussian_filter(groove * (1 - clear), 0.4 * SS)
    rgb = carve(rgb, groove, 1 * SS, 0.6)
    inlay = np.array([255, 196, 70]) + glow * np.array([0, 50, 120])
    rgb = carve(rgb, gm, 1 * SS, 0.7)
    rgb = rgb * (1 - gm[..., None] * 0.8) + gm[..., None] * inlay * (0.8 + 0.4 * glow)
    shade = bevel_shading(body, 6 * SS, 0.4)
    rgb = rgb * shade[..., None]
    if glow > 0:
        # bloom around inlay
        bl = ndimage.gaussian_filter(gm, 5 * SS) * 3 * glow
        rgb = rgb + bl[..., None] * np.array([255, 190, 80])
    return down(to_img(rgb, body), w, h)


# ------------------------------------------------------------------ ball
def make_ball(d=32, fire=False):
    D = d * SS
    yy, xx = np.mgrid[0:D, 0:D]
    c = (D - 1) / 2
    nx = (xx - c) / (D / 2 - SS); ny = (yy - c) / (D / 2 - SS)
    r2 = nx ** 2 + ny ** 2
    inside = np.clip((1 - np.sqrt(r2)) * D / 2 / SS * 1.2, 0, 1)
    nz = np.sqrt(np.clip(1 - r2, 0, 1))
    L = np.array([-0.45, -0.55, 0.7]); L /= np.linalg.norm(L)
    diff = np.clip(nx * L[0] + ny * L[1] + nz * L[2], 0, 1)
    tex = fbm(D, D, 8 * SS, 3, 5)
    base_d = np.array([150, 54, 8]); base_l = np.array([255, 178, 52])
    rgb = base_d + (base_l - base_d) * (diff * 0.85 + tex[..., None] * 0.25)[..., None] if False else \
        lerp_rgb(base_d, base_l, np.clip(diff * 0.85 + tex * 0.25, 0, 1))
    # carved ring
    rr = np.sqrt(r2)
    ring = np.exp(-((rr - 0.55) / 0.06) ** 2) * 0.8
    rgb = rgb * (1 - ring[..., None] * 0.3)
    H = (L + np.array([0, 0, 1])); H /= np.linalg.norm(H)
    spec = np.clip(nx * H[0] + ny * H[1] + nz * H[2], 0, 1) ** 40
    rgb = rgb + spec[..., None] * 255
    rim = np.clip(1 - nz, 0, 1) ** 3 * 120
    rgb = rgb + rim[..., None] * np.array([1, 0.6, 0.2])
    return down(to_img(rgb, inside), d, d)


def make_glow(size, color, power=2.2, strength=1.0):
    yy, xx = np.mgrid[0:size, 0:size] - (size - 1) / 2
    r = np.hypot(xx, yy) / (size / 2)
    a = np.clip(1 - r, 0, 1) ** power * strength
    return to_img(np.ones((size, size, 3)) * np.array(color), a)


# ------------------------------------------------------------------ power-ups
def tablet_base(s=64, seed=3):
    S = s * SS
    m = rounded_mask(S, S, 12 * SS, inset=2 * SS)
    m = ndimage.gaussian_filter(m, 0.5 * SS)
    rgb = stone_texture(S, S, (70, 44, 24), (150, 104, 62), seed, strata=True)
    rim = np.clip(rounded_mask(S, S, 12 * SS, inset=2 * SS) - rounded_mask(S, S, 9 * SS, inset=7 * SS), 0, 1)
    gold = stone_texture(S, S, (130, 80, 16), (255, 216, 100), seed + 9) + band_spec(S, S)
    rgb = rgb * (1 - rim[..., None]) + gold * rim[..., None]
    rgb = emboss_raise(rgb, rim, 1 * SS, 40)
    return rgb, m


def symbol(name, s=64):
    S = s * SS
    im = Image.new("L", (S, S), 0); d = ImageDraw.Draw(im)
    c = S / 2; u = S / 64
    lw = int(5 * u)
    if name == "expand":
        d.line([c - 18 * u, c, c + 18 * u, c], fill=255, width=lw)
        d.polygon([(c - 24 * u, c), (c - 14 * u, c - 9 * u), (c - 14 * u, c + 9 * u)], fill=255)
        d.polygon([(c + 24 * u, c), (c + 14 * u, c - 9 * u), (c + 14 * u, c + 9 * u)], fill=255)
    elif name == "shrink":
        for sx in (-1, 1):
            d.line([c + sx * 24 * u, c, c + sx * 10 * u, c], fill=255, width=lw)
            d.polygon([(c + sx * 4 * u, c), (c + sx * 13 * u, c - 9 * u), (c + sx * 13 * u, c + 9 * u)], fill=255)
    elif name == "multiball":
        for (x, y) in [(c, c - 9 * u), (c - 10 * u, c + 7 * u), (c + 10 * u, c + 7 * u)]:
            d.ellipse([x - 7 * u, y - 7 * u, x + 7 * u, y + 7 * u], fill=255)
    elif name == "slow":
        d.polygon([(c - 13 * u, c - 18 * u), (c + 13 * u, c - 18 * u), (c, c)], outline=255, width=int(3.5 * u))
        d.polygon([(c - 13 * u, c + 18 * u), (c + 13 * u, c + 18 * u), (c, c)], fill=255)
        d.line([c - 16 * u, c - 19 * u, c + 16 * u, c - 19 * u], fill=255, width=lw)
        d.line([c - 16 * u, c + 19 * u, c + 16 * u, c + 19 * u], fill=255, width=lw)
    elif name == "fireball":
        def drop(R, cx, cy, m=2.2, lean=0.0):
            pts = []
            for i in range(90):
                t = i / 89 * 2 * math.pi
                x = R * 0.78 * math.sin(t) * math.sin(t / 2) ** m
                y = -R * math.cos(t)
                pts.append((cx + x + lean * (y - R) * 0.25, cy + y))
            return pts
        d.polygon(drop(23 * u, c, c + 2 * u, 2.4, 0.15), fill=255)
        d.polygon(drop(10 * u, c + 1 * u, c + 12 * u, 2.0), fill=0)
    elif name == "life":
        r = 9 * u
        d.ellipse([c - r, c - 20 * u, c + r, c - 20 * u + 2 * r], outline=255, width=lw)
        d.line([c, c - 2 * u, c, c + 22 * u], fill=255, width=lw)
        d.line([c - 14 * u, c + 4 * u, c + 14 * u, c + 4 * u], fill=255, width=lw)
    elif name == "laser":
        d.polygon([(c + 5 * u, c - 24 * u), (c - 12 * u, c + 3 * u), (c - 1 * u, c + 3 * u), (c - 6 * u, c + 24 * u),
                   (c + 12 * u, c - 5 * u), (c + 1 * u, c - 5 * u)], fill=255)
    elif name == "barrier":
        for y in (-13, 0, 13):
            d.line([c - 22 * u, c + y * u, c + 22 * u, c + y * u], fill=255, width=int(3.5 * u))
        for y, xs in ((-6.5, (-8, 12)), (6.5, (-14, 4, 18))):
            for x in xs:
                d.line([c + x * u, c + (y - 6.5) * u, c + x * u, c + (y + 6.5) * u], fill=255, width=int(3.5 * u))
    elif name == "catch":
        d.arc([c - 20 * u, c - 16 * u, c + 20 * u, c + 22 * u], 0, 180, fill=255, width=lw)
        d.ellipse([c - 8 * u, c - 16 * u, c + 8 * u, c], fill=255)
    elif name == "bonus":
        pts = []
        for i in range(10):
            a = -math.pi / 2 + i * math.pi / 5
            r = (24 if i % 2 == 0 else 10) * u
            pts.append((c + math.cos(a) * r, c + math.sin(a) * r))
        d.polygon(pts, fill=255)
    return np.array(im.filter(ImageFilter.GaussianBlur(0.4 * SS))) / 255.0


SYMBOL_TINT = {"expand": (120, 230, 140), "shrink": (240, 110, 90), "multiball": (255, 210, 90),
               "slow": (120, 200, 255), "fireball": (255, 120, 40), "life": (255, 120, 150),
               "laser": (190, 140, 255), "barrier": (90, 220, 210), "catch": (235, 235, 205), "bonus": (255, 225, 120)}


def make_powerup(name, s=64, shine=None):
    rgb, m = tablet_base(s, seed=sum(map(ord, name)))
    S = s * SS
    g = symbol(name, s)
    tint = np.array(SYMBOL_TINT[name])
    # inner recess
    rgb = carve(rgb, ndimage.gaussian_filter(g, 0.3 * SS) * 0.0, 1, 1)
    col = tint * 0.75 + 60
    sym = np.ones((S, S, 3)) * col
    sym = emboss_raise(sym, g, 1.0 * SS, 60)
    glow = ndimage.gaussian_filter(g, 3 * SS)[..., None] * tint * 0.9
    rgb = rgb + glow
    rgb = rgb * (1 - g[..., None]) + sym * g[..., None]
    shade = bevel_shading(m, 4 * SS, 0.35)
    rgb = rgb * shade[..., None]
    if shine is not None:
        yy, xx = np.mgrid[0:S, 0:S] / S
        band = np.exp(-(((xx + yy) / 2 - shine) / 0.06) ** 2)
        rgb = rgb + band[..., None] * 140 * m[..., None]
    return down(to_img(rgb, m), s, s)


# ------------------------------------------------------------------ UI
def make_plaque(w, h, seed=8):
    W, H = w * SS, h * SS
    m = rounded_mask(W, H, 14 * SS, inset=2 * SS)
    rgb = stone_texture(W, H, (54, 36, 22), (118, 84, 54), seed, strata=True)
    rim = np.clip(rounded_mask(W, H, 14 * SS, inset=2 * SS) - rounded_mask(W, H, 10 * SS, inset=9 * SS), 0, 1)
    gold = stone_texture(W, H, (130, 80, 16), (255, 216, 100), seed + 9) + band_spec(H, W)
    rgb = rgb * (1 - rim[..., None]) + gold * rim[..., None]
    rgb = emboss_raise(rgb, rim, 1 * SS, 40)
    inner = rounded_mask(W, H, 8 * SS, inset=12 * SS)
    rgb = rgb * (1 - 0.25 * ndimage.gaussian_filter(inner, 2 * SS))[..., None]
    shade = bevel_shading(m, 5 * SS, 0.35)
    return down(to_img(rgb * shade[..., None], m), w, h)


# ------------------------------------------------------------------ background
def make_background(W=1920, H=1080):
    r = np.random.default_rng(11)
    # wall blocks
    mortar = Image.new("L", (W, H), 0); d = ImageDraw.Draw(mortar)
    tone = np.zeros((H, W))
    tone_img = Image.new("L", (W, H), 128); dt = ImageDraw.Draw(tone_img)
    rowh = 92
    glyph_spots = []
    y = 104
    row = 0
    while y < H:
        x = -r.integers(0, 200)
        while x < W:
            bw = int(r.integers(170, 290))
            dt.rectangle([x, y, x + bw, y + rowh], fill=int(r.integers(95, 160)))
            d.rectangle([x, y, x + bw, y + rowh], outline=255, width=5)
            if r.random() < 0.14 and 140 < x < W - 300:
                glyph_spots.append((x + bw // 2, y + rowh // 2))
            x += bw
        y += rowh; row += 1
    mort = ndimage.gaussian_filter(np.array(mortar) / 255.0, 1.4)
    blockm = 1 - mort
    tone = np.array(tone_img) / 255.0
    n = fbm(H, W, 40, 6, 21)
    t = np.clip(0.55 * n + 0.45 * tone, 0, 1)
    rgb = lerp_rgb((26, 18, 14), (92, 70, 52), t)
    # strata + speckle
    rgb *= (1 + 0.05 * np.sin(np.arange(H)[:, None] / 7.0 + fbm(H, W, 120, 2, 3) * 6))[..., None]
    gy, gx = np.gradient(ndimage.gaussian_filter(blockm, 2.5))
    light = (gx * 0.6 + gy) * 6
    rgb *= (1 + np.clip(light, -0.6, 0.6))[..., None]
    rgb *= (1 - 0.65 * mort)[..., None]
    # faint glyphs on some blocks
    gl = np.zeros((H, W))
    names = ["sun", "eye", "spiral", "steps", "waves", "crescent", "diamond"]
    for i, (gx0, gy0) in enumerate(glyph_spots):
        gw_, gh_ = 150, 84
        gm = glyph(names[i % len(names)], gw_, gh_, 4)
        x0, y0 = gx0 - gw_ // 2, gy0 - gh_ // 2
        if x0 < 0 or y0 < 0 or x0 + gw_ > W or y0 + gh_ > H: continue
        gl[y0:y0 + gh_, x0:x0 + gw_] = np.maximum(gl[y0:y0 + gh_, x0:x0 + gw_], gm)
    rgb = carve(rgb, gl * 0.6, 2, dark=0.7)

    # ---- pillars
    PW = 100
    xs = np.arange(W)[None, :] * np.ones((H, 1))
    ys = np.arange(H)[:, None] * np.ones((1, W))
    pil = np.zeros((H, W))
    pil[104:, :PW] = 1; pil[104:, W - PW:] = 1
    ptex = stone_texture(H, W, (70, 52, 36), (186, 150, 108), 61, strata=False) if False else \
        lerp_rgb((70, 52, 36), (190, 152, 110), np.clip(fbm(H, W, 60, 5, 61) * 1.1, 0, 1))
    # cylinder shading + flutes
    lx = np.where(xs < PW, xs / PW, (xs - (W - PW)) / PW)
    cyl = np.sin(np.clip(lx, 0, 1) * math.pi) ** 0.6 * 0.9 + 0.25
    cyl *= np.where(xs < PW, 1 + 0.25 * (1 - lx), 1 + 0.25 * lx)  # lit from play-field side a bit
    flute = 1 - 0.28 * (np.cos(lx * math.pi * 2 * 5) * 0.5 + 0.5) ** 3
    prgb = ptex * (cyl * flute)[..., None]
    # capital & base bands
    for (y0, y1) in [(104, 150), (H - 70, H)]:
        band = (ys >= y0) & (ys < y1)
        prgb[band] = (ptex[band] * 1.15)
    caps = np.zeros((H, W))
    for yb in (104, 150, H - 70):
        caps[(ys >= yb - 3) & (ys < yb + 3)] = 1
    pil_s = ndimage.gaussian_filter(pil, 1.0)
    rgb = rgb * (1 - pil_s[..., None]) + prgb * pil_s[..., None]
    rgb = rgb * (1 - 0.5 * (caps * pil)[..., None])
    # pillar inner shadow cast on wall
    shadow = np.zeros((H, W))
    shadow[104:, PW:PW + 40] = np.linspace(1, 0, 40)[None, :]
    shadow[104:, W - PW - 40:W - PW] = np.linspace(0, 1, 40)[None, :]
    rgb *= (1 - 0.55 * shadow)[..., None]

    # ---- frieze (HUD band)
    fr = (ys < 104)
    ftex = lerp_rgb((40, 26, 16), (118, 86, 54), np.clip(fbm(H, W, 50, 5, 71), 0, 1))
    meander = np.zeros((H, W))
    tile = glyph("meander", 200, 104, 5)
    sunt = glyph("sun", 104, 104, 4)
    x = 0; k = 0
    while x < W:
        if k % 3 == 1:
            meander[0:104, x:x + 104] = sunt[:, :min(104, W - x)]; x += 104
        else:
            meander[0:104, x:x + 200] = tile[:, :min(200, W - x)]; x += 200
        k += 1
    frgb = carve(ftex, meander * 0.9, 2, dark=0.55)
    frgb *= np.clip(1.15 - 0.4 * (ys / 104), 0.7, 1.2)[..., None]
    rgb = np.where(fr[..., None], frgb, rgb)
    # gold trim under frieze
    trim = ((ys >= 96) & (ys < 108)).astype(float)
    goldt = lerp_rgb((120, 72, 14), (255, 214, 100), np.clip(fbm(H, W, 20, 3, 5) * 1.2, 0, 1))
    goldt = goldt * (1 + 0.5 * np.exp(-((ys - 99) / 2.5) ** 2))[..., None]
    rgb = rgb * (1 - trim[..., None]) + goldt * trim[..., None]
    rgb *= (1 - 0.6 * np.exp(-((ys - 112) / 5) ** 2) * (ys > 107))[..., None]

    # ---- sconces (static part) at pillar torch positions
    sconces = [(PW // 2, 560), (W - PW // 2, 560)]
    sc = Image.new("L", (W, H), 0); ds = ImageDraw.Draw(sc)
    for (cx, cy) in sconces:
        ds.ellipse([cx - 40, cy - 14, cx + 40, cy + 2], fill=255)
        ds.polygon([(cx - 40, cy - 6), (cx + 40, cy - 6), (cx + 24, cy + 30), (cx - 24, cy + 30)], fill=255)
        ds.rectangle([cx - 8, cy + 30, cx + 8, cy + 90], fill=255)
        ds.ellipse([cx - 18, cy + 80, cx + 18, cy + 104], fill=255)
    scm = ndimage.gaussian_filter(np.array(sc) / 255.0, 0.8)
    bronze = lerp_rgb((70, 40, 16), (210, 146, 70), np.clip(fbm(H, W, 10, 3, 9) * 1.2, 0, 1))
    bronze = emboss_raise(bronze, scm, 2, 60)
    rgb = rgb * (1 - scm[..., None]) + bronze * scm[..., None]

    # ---- lighting
    L = np.zeros((H, W)) + 0.34
    for (cx, cy) in sconces:
        dd = np.hypot(xs - cx, (ys - cy) * 1.1)
        L += 0.9 * np.exp(-(dd / 380) ** 2) + 0.5 * np.exp(-(dd / 110) ** 2)
    dd = np.hypot((xs - W / 2) / 1.6, ys - 200)
    L += 0.3 * np.exp(-(dd / 520) ** 2)
    # bottom darkness (the pit)
    L *= np.clip(1.25 - (ys / H) ** 2.2 * 1.05, 0.12, 1.3)
    warm = np.array([1.12, 0.98, 0.82])
    rgb = rgb * L[..., None] * warm
    # vignette
    v = np.hypot((xs - W / 2) / W, (ys - H / 2) / H)
    rgb *= np.clip(1.15 - v * 0.9, 0, 1.2)[..., None]
    # floating dust grain
    grain = r.normal(0, 3.0, (H, W, 1))
    rgb += grain
    img = to_img(rgb, np.ones((H, W)))
    return img.convert("RGB"), sconces


def main():
    meta = {"bricks": {}}
    bd = ensure("bricks")
    seed = 100
    for kind, glyphs in GLYPHS.items():
        meta["bricks"][kind] = {"hits": PAL[kind]["hits"], "variants": []}
        for gi, gname in enumerate(glyphs):
            seed += 17
            base = f"{kind}_{chr(97 + gi)}"
            make_brick(kind, gname, seed).save(f"{bd}/{base}.png")
            files = [f"{base}.png"]
            hits = PAL[kind]["hits"]
            for c in range(1, hits):
                make_brick(kind, gname, seed, cracks=c).save(f"{bd}/{base}_crack{c}.png")
                files.append(f"{base}_crack{c}.png")
            meta["bricks"][kind]["variants"].append(files)
            meta["bricks"][kind].setdefault("seeds", []).append([gname, seed])
    cd = ensure("cracks")
    for st in (1, 2):
        crack_overlay(st).save(f"{cd}/crack_overlay_{st}.png")

    pd = ensure("paddle")
    for name, w in [("short", 176), ("normal", 256), ("long", 352)]:
        make_paddle(w).save(f"{pd}/paddle_{name}.png")

    bl = ensure("ball")
    make_ball(32).save(f"{bl}/ball.png")
    make_glow(96, (255, 170, 60), 2.0, 0.75).save(f"{bl}/ball_glow.png")
    make_glow(16, (255, 200, 110), 1.6, 0.9).save(f"{bl}/trail_particle.png")

    pu = ensure("powerups")
    for n in SYMBOL_TINT:
        make_powerup(n).save(f"{pu}/powerup_{n}.png")

    ui = ensure("ui")
    make_plaque(400, 80).save(f"{ui}/panel.png")
    make_plaque(960, 240, seed=12).save(f"{ui}/banner.png")
    make_ball(28).save(f"{ui}/life_icon.png")

    bg = ensure("background")
    img, sconces = make_background()
    img.save(f"{bg}/background.png", optimize=True)
    make_glow(512, (255, 150, 60), 1.8, 0.55).save(f"{bg}/torch_glow.png")
    meta["torches"] = [{"x": x, "y": y} for (x, y) in sconces]
    json.dump(meta, open("/home/claude/temple/static_meta.json", "w"), indent=1)


if __name__ == "__main__":
    main()
