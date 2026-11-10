"""Shared helpers for the Ancient Temple asset generator."""
import math
import numpy as np
from PIL import Image, ImageDraw, ImageFilter
from scipy import ndimage

OUT = "/home/claude/temple/assets"
SS = 3  # supersampling factor


def value_noise(h, w, scale, seed):
    r = np.random.default_rng(seed)
    gh, gw = int(h / scale) + 3, int(w / scale) + 3
    g = r.random((gh, gw))
    ys = np.arange(h) / scale
    xs = np.arange(w) / scale
    y0 = ys.astype(int); x0 = xs.astype(int)
    fy = ys - y0; fx = xs - x0
    fy = fy * fy * (3 - 2 * fy); fx = fx * fx * (3 - 2 * fx)
    a = g[y0][:, x0]; b = g[y0][:, x0 + 1]
    c = g[y0 + 1][:, x0]; d = g[y0 + 1][:, x0 + 1]
    fx = fx[None, :]; fy = fy[:, None]
    return (a * (1 - fx) + b * fx) * (1 - fy) + (c * (1 - fx) + d * fx) * fy


def fbm(h, w, scale, octaves, seed, gain=0.5):
    tot = np.zeros((h, w)); amp = 1.0; s = scale; norm = 0
    for o in range(octaves):
        tot += amp * value_noise(h, w, max(s, 1), seed + o * 101)
        norm += amp; amp *= gain; s /= 2
    return tot / norm


def lerp_rgb(c1, c2, t):
    c1 = np.array(c1, float); c2 = np.array(c2, float)
    return c1 + (c2 - c1) * t[..., None]


def rounded_mask(w, h, r, inset=0):
    m = Image.new("L", (w, h), 0)
    ImageDraw.Draw(m).rounded_rectangle([inset, inset, w - 1 - inset, h - 1 - inset], r, fill=255)
    return np.array(m) / 255.0


def to_img(rgb, alpha):
    rgb = np.clip(rgb, 0, 255).astype(np.uint8)
    a = np.clip(alpha * 255, 0, 255).astype(np.uint8)
    return Image.fromarray(np.dstack([rgb, a]), "RGBA")


def down(img, w, h):
    return img.resize((w, h), Image.LANCZOS)


def bevel_shading(mask, width, strength=0.35):
    """Return a shading multiplier from a mask: lit top-left, shaded bottom-right."""
    dist = ndimage.distance_transform_edt(mask > 0.5)
    edge = np.clip(1 - dist / width, 0, 1)
    gy, gx = np.gradient(ndimage.gaussian_filter(mask.astype(float), width / 3))
    # light from top-left: gradient pointing into shape from top-left edges is +x,+y
    light = (gx + gy)
    light = light / (np.abs(light).max() + 1e-6)
    return 1 + strength * light * 2.2 * edge + 0.0


def carve(rgb, glyph_mask, depth_px, dark=0.55):
    """Carve a recessed glyph into rgb (float array). glyph_mask 0..1."""
    g = glyph_mask
    sh = ndimage.shift(g, (depth_px, depth_px), order=1)
    inner_shadow = np.clip(g - sh, 0, 1)        # top-left inner wall -> dark
    lip_light = np.clip(sh - g, 0, 1)           # just outside bottom-right -> bright
    out = rgb * (1 - g[..., None] * (1 - dark))
    out = out * (1 - inner_shadow[..., None] * 0.45)
    out = out + lip_light[..., None] * 40
    return out


def emboss_raise(rgb, mask, d, amt=60):
    """Raised relief (light top-left)."""
    sh_a = ndimage.shift(mask, (d, d), order=1)
    sh_b = ndimage.shift(mask, (-d, -d), order=1)
    light = np.clip(mask - sh_a, 0, 1)
    dark = np.clip(mask - sh_b, 0, 1)
    return rgb + light[..., None] * amt - dark[..., None] * amt


# ------------------------------------------------------------------ glyphs
def glyph(name, w, h, lw):
    """Abstract carved glyphs, drawn on an L image (w,h)."""
    im = Image.new("L", (w, h), 0)
    d = ImageDraw.Draw(im)
    cx, cy = w / 2, h / 2
    s = min(w, h)
    if name == "sun":
        r = s * 0.22
        d.ellipse([cx - r, cy - r, cx + r, cy + r], outline=255, width=lw)
        d.ellipse([cx - r * .35, cy - r * .35, cx + r * .35, cy + r * .35], fill=255)
        for i in range(8):
            a = i * math.pi / 4
            d.line([cx + math.cos(a) * r * 1.35, cy + math.sin(a) * r * 1.35,
                    cx + math.cos(a) * r * 1.85, cy + math.sin(a) * r * 1.85], fill=255, width=lw)
    elif name == "spiral":
        pts = []
        for i in range(160):
            t = i / 159 * 3.2 * math.pi
            r = s * 0.04 + s * 0.06 * t / math.pi
            pts.append((cx + math.cos(t) * r * 1.2, cy + math.sin(t) * r))
        d.line(pts, fill=255, width=lw, joint="curve")
    elif name == "eye":
        ew, eh = s * 0.95, s * 0.32
        d.chord([cx - ew / 2, cy - eh, cx + ew / 2, cy + eh * 0.9], 200, 340, outline=255, width=lw)
        d.arc([cx - ew / 2, cy - eh * 1.4, cx + ew / 2, cy + eh * 0.55], 20, 160, fill=255, width=lw)
        r = eh * 0.45
        d.ellipse([cx - r, cy - r * 0.9, cx + r, cy + r * 1.1], fill=255)
    elif name == "waves":
        for row in (-1, 1):
            pts = []
            for i in range(120):
                x = cx - s * 0.75 + i / 119 * s * 1.5
                pts.append((x, cy + row * s * 0.14 + math.sin(i / 119 * 4 * math.pi) * s * 0.09))
            d.line(pts, fill=255, width=lw, joint="curve")
    elif name == "steps":
        bw = s * 0.28
        for k in range(3):
            x0 = cx - bw * (3 - k) * 0.6
            x1 = cx + bw * (3 - k) * 0.6
            y1 = cy + s * 0.3 - k * s * 0.2
            d.rectangle([x0, y1 - s * 0.2 + lw * 0.5, x1, y1], outline=255, width=lw)
    elif name == "crescent":
        r = s * 0.3
        d.ellipse([cx - r, cy - r, cx + r, cy + r], fill=255)
        d.ellipse([cx - r * 0.55, cy - r * 1.05, cx + r * 1.35, cy + r * 0.85], fill=0)
        for dx in (-1.9, 1.9):
            rr = s * 0.06
            d.ellipse([cx + dx * r - rr, cy - rr, cx + dx * r + rr, cy + rr], fill=255)
    elif name == "diamond":
        r = s * 0.3
        for k in (1.0, 0.55):
            d.polygon([(cx, cy - r * k), (cx + r * k * 1.6, cy), (cx, cy + r * k), (cx - r * k * 1.6, cy)],
                      outline=255, width=lw)
        rr = s * 0.05
        d.ellipse([cx - rr, cy - rr, cx + rr, cy + rr], fill=255)
    elif name == "meander":  # greek key style band
        u = s * 0.16
        x = cx - u * 4.5
        pts = [(x, cy + u * 1.5)]
        for k in range(3):
            x0 = x + k * u * 3
            pts += [(x0, cy - u * 1.5), (x0 + u * 2, cy - u * 1.5), (x0 + u * 2, cy + u * .5),
                    (x0 + u, cy + u * .5), (x0 + u, cy - u * .5)]
            pts += [(x0 + u, cy - u * .5)]
            pts += [(x0 + u * 3, cy + u * 1.5)] if k < 2 else [(x0 + u * 3, cy + u * 1.5)]
        d.line(pts, fill=255, width=lw, joint="curve")
    return np.array(im.filter(ImageFilter.GaussianBlur(lw * 0.15))) / 255.0


def star4(size, color=(255, 240, 180), a=1.0):
    """4-point sparkle sprite."""
    S = size * SS
    yy, xx = np.mgrid[0:S, 0:S] - (S - 1) / 2
    r = np.hypot(xx, yy) / (S / 2)
    arms = np.exp(-np.abs(xx) / (S * 0.03)) * np.exp(-np.abs(yy) / (S * 0.28)) + \
        np.exp(-np.abs(yy) / (S * 0.03)) * np.exp(-np.abs(xx) / (S * 0.28))
    core = np.exp(-(r / 0.18) ** 2)
    al = np.clip((arms + core) * a, 0, 1) * np.clip(1 - r, 0, 1) ** 0.5
    rgb = np.ones((S, S, 3)) * np.array(color)
    rgb = rgb + (255 - rgb) * core[..., None]
    return down(to_img(rgb, al), size, size)


def soft_dot(size, color, hardness=2.0):
    yy, xx = np.mgrid[0:size, 0:size] - (size - 1) / 2
    r = np.hypot(xx, yy) / (size / 2)
    al = np.clip(1 - r, 0, 1) ** hardness
    rgb = np.ones((size, size, 3)) * np.array(color)
    return to_img(rgb, al)
