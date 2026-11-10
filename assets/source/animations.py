"""Animation sprite sheets (horizontal strips) + atlas.json."""
import os, math, json
import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageEnhance
from scipy import ndimage
from common import *
import static_assets as SA

AD = os.path.join(OUT, "anim")
os.makedirs(AD, exist_ok=True)
ATLAS = {}


def save_strip(name, frames, fps, loop, anchor, note=""):
    w, h = frames[0].size
    strip = Image.new("RGBA", (w * len(frames), h), (0, 0, 0, 0))
    for i, f in enumerate(frames):
        strip.alpha_composite(f, (i * w, 0))
    strip.save(f"{AD}/{name}.png", optimize=True)
    ATLAS[name] = dict(file=f"anim/{name}.png", frame_w=w, frame_h=h, frames=len(frames),
                       fps=fps, loop=loop, anchor=anchor, note=note)


def brighten(img, k):
    a = np.array(img).astype(float)
    a[..., :3] = a[..., :3] + (255 - a[..., :3]) * k
    return Image.fromarray(np.clip(a, 0, 255).astype(np.uint8), "RGBA")


def puff(canvas_size, cx, cy, r, color, alpha):
    w, h = canvas_size
    yy, xx = np.mgrid[0:h, 0:w]
    d = np.hypot(xx - cx, yy - cy) / max(r, 1)
    a = np.clip(1 - d, 0, 1) ** 1.6 * alpha
    return to_img(np.ones((h, w, 3)) * np.array(color), a)


# ------------------------------------------------------------------ shatter
def shatter(src, canvas=(320, 256), center=(160, 112), n_chunks=11, frames=18, seed=1,
            dust=(190, 150, 100), pebble=(110, 80, 50), impact=None, sparkle=None,
            speed=(90, 230), up=170, gravity=1500, flash=True):
    r = np.random.default_rng(seed)
    a = np.array(src).astype(float)
    h, w = a.shape[:2]
    if impact is None:
        impact = (w * r.uniform(0.35, 0.65), h * 0.95)
    # voronoi seeds, denser near impact
    pts = []
    for i in range(n_chunks):
        if i < n_chunks // 3:
            pts.append((impact[0] + r.normal(0, w * 0.12), impact[1] + r.normal(-h * 0.3, h * 0.2)))
        else:
            pts.append((r.uniform(0, w), r.uniform(0, h)))
    pts = np.array(pts)
    yy, xx = np.mgrid[0:h, 0:w]
    dd = (xx[..., None] - pts[:, 0]) ** 2 + (yy[..., None] - pts[:, 1]) ** 2
    lab = dd.argmin(-1)
    chunks = []
    for i in range(n_chunks):
        m = (lab == i)
        if (m & (a[..., 3] > 20)).sum() < 6: continue
        ys, xs = np.where(m)
        y0, y1, x0, x1 = ys.min(), ys.max() + 1, xs.min(), xs.max() + 1
        c = a[y0:y1, x0:x1].copy()
        mm = m[y0:y1, x0:x1]
        er = ndimage.binary_erosion(mm, iterations=1)
        edge = mm & ~er
        c[..., :3][edge] *= 0.55
        c[..., 3] *= mm
        img = Image.fromarray(np.clip(c, 0, 255).astype(np.uint8), "RGBA")
        cxp, cyp = (x0 + x1) / 2, (y0 + y1) / 2
        dv = np.array([cxp - impact[0], cyp - impact[1]])
        dv = dv / (np.linalg.norm(dv) + 1e-6)
        sp = r.uniform(*speed)
        v = dv * sp + np.array([r.normal(0, 40), -up * r.uniform(0.6, 1.2)])
        chunks.append(dict(img=img, pos=np.array([cxp - w / 2, cyp - h / 2]), v=v,
                           rot=0.0, vr=r.uniform(-540, 540)))
    peb = [dict(p=np.array([r.uniform(-w / 2, w / 2), r.uniform(-h / 2, h / 2)]),
                v=np.array([r.normal(0, 260), -r.uniform(150, 420)]), s=r.uniform(2, 4.5))
           for _ in range(18)]
    puffs = [dict(p=(r.uniform(-w * 0.45, w * 0.45), r.uniform(-h * 0.3, h * 0.4)), r0=r.uniform(8, 16),
                  r1=r.uniform(34, 60), drift=r.uniform(-30, -10)) for _ in range(7)]
    sparks = []
    if sparkle:
        sparks = [dict(p=(r.uniform(-w * 0.7, w * 0.7), r.uniform(-h * 1.4, h * 0.8)), t0=r.integers(1, 7),
                       life=r.integers(4, 8), size=int(r.uniform(18, 34))) for _ in range(14)]
    out = []
    dt = 1 / 30
    for f in range(frames):
        cv = Image.new("RGBA", canvas, (0, 0, 0, 0))
        t = f * dt
        fade = 1.0 if f < frames * 0.55 else max(0, 1 - (f - frames * 0.55) / (frames * 0.45))
        # dust behind
        for pf in puffs:
            k = min(1, f / (frames * 0.6))
            rad = pf["r0"] + (pf["r1"] - pf["r0"]) * (1 - (1 - k) ** 2)
            al = 0.5 * (1 - k) ** 1.3 if f > 0 else 0
            if al > 0.01:
                cv.alpha_composite(puff(canvas, center[0] + pf["p"][0], center[1] + pf["p"][1] + pf["drift"] * t,
                                        rad, dust, al))
        if f == 0:
            img = brighten(src, 0.75 if flash else 0)
            cv.alpha_composite(img, (int(center[0] - w / 2), int(center[1] - h / 2)))
        else:
            tt = t - dt * 0.5
            for c in chunks:
                p = c["pos"] + c["v"] * tt + np.array([0, 0.5 * gravity * tt * tt])
                ang = c["vr"] * tt
                im = c["img"]
                if f == 1 and flash: im = brighten(im, 0.35)
                im = im.rotate(ang, resample=Image.BICUBIC, expand=True)
                if fade < 1:
                    arr = np.array(im); arr[..., 3] = (arr[..., 3] * fade).astype(np.uint8)
                    im = Image.fromarray(arr, "RGBA")
                x = int(center[0] + p[0] - im.width / 2); y = int(center[1] + p[1] - im.height / 2)
                cv.alpha_composite(im, (x, y)) if 0 <= x and 0 <= y and x + im.width <= canvas[0] and y + im.height <= canvas[1] \
                    else paste_clip(cv, im, x, y)
            d = ImageDraw.Draw(cv)
            for pb in peb:
                p = pb["p"] + pb["v"] * tt + np.array([0, 0.5 * gravity * 1.2 * tt * tt])
                x, y = center[0] + p[0], center[1] + p[1]
                al = int(255 * fade)
                d.ellipse([x - pb["s"] / 2, y - pb["s"] / 2, x + pb["s"] / 2, y + pb["s"] / 2],
                          fill=tuple(pebble) + (al,))
        for s in sparks:
            age = f - s["t0"]
            if 0 <= age < s["life"]:
                k = age / s["life"]
                sz = max(4, int(s["size"] * math.sin(k * math.pi)))
                st = star4(sz, (255, 236, 160))
                cv.alpha_composite(st, (int(center[0] + s["p"][0] - sz / 2), int(center[1] + s["p"][1] - sz / 2)))
        out.append(cv)
    return out


def paste_clip(cv, im, x, y):
    tmp = Image.new("RGBA", cv.size, (0, 0, 0, 0))
    tmp.paste(im, (x, y), im)
    cv.alpha_composite(tmp)


# ------------------------------------------------------------------ fire colour map
def fire_color(dens):
    stops = [(0.0, (0, 0, 0, 0)), (0.15, (220, 70, 10, 0.0)), (0.3, (245, 95, 20, 0.75)),
             (0.5, (255, 120, 20, 0.95)), (0.72, (255, 200, 70, 1)), (1.0, (255, 250, 220, 1))]
    d = np.clip(dens, 0, 1)
    rgb = np.zeros(d.shape + (3,)); al = np.zeros(d.shape)
    for (p0, c0), (p1, c1) in zip(stops[:-1], stops[1:]):
        m = (d >= p0) & (d <= p1)
        t = ((d - p0) / (p1 - p0))[m]
        for k in range(3):
            rgb[..., k][m] = c0[k] + (c1[k] - c0[k]) * t
        al[m] = c0[3] + (c1[3] - c0[3]) * t
    return rgb, al


def loop_noise(h, w, scale, seed, nframes, speed_rows):
    """Noise that scrolls upward and loops after nframes."""
    P = speed_rows * nframes
    N = fbm(h + 2 * P, w, scale, 4, seed)
    frames = []
    for f in range(nframes):
        off = f * speed_rows
        ys = (np.arange(h) + P - off)  # scroll upward => content moves up: sample lower rows
        # blend two copies one period apart for seamless loop
        wgt = off / P
        frames.append(N[ys] * (1 - wgt) + N[ys + P] * wgt)
    return frames


def torch_flame(w=64, h=128, nf=8):
    W, H = w * SS, h * SS
    nfr = loop_noise(H, W, 12 * SS, 77, nf, int(H / nf * 0.9))
    sway = loop_noise(H, 1, 30 * SS, 78, nf, int(H / nf * 0.5))
    yy, xx = np.mgrid[0:H, 0:W]
    out = []
    for f in range(nf):
        yn = yy / H                     # 0 top .. 1 bottom
        sx = (sway[f][:, 0:1] - 0.5) * W * 0.25 * (1 - yn)
        xn = (xx - W / 2 - sx) / (W / 2)
        base = 0.86
        prof = np.clip((yn - 0.02) / (base - 0.02), 0, 1)       # 0 at tip, 1 near base
        width = 0.8 * np.sqrt(prof) * np.clip((1 - (yn - base) / 0.14), 0, 1) ** 0.5
        shape = np.clip(1 - np.abs(xn) / (width + 1e-3), 0, 1)
        dens = shape ** 0.8 * (0.55 + 0.75 * nfr[f]) * (0.35 + 0.75 * prof)
        dens = dens - (1 - prof) * 0.25
        rgb, al = fire_color(dens * 0.82)
        glow = ndimage.gaussian_filter(np.clip(dens, 0, 1), 6 * SS) * 0.45
        al = np.maximum(al, glow)
        rgb = np.where((al > glow + 1e-6)[..., None], rgb, np.array([255, 130, 40]))
        out.append(down(to_img(rgb, al), w, h))
    return out


def fireball(size=96, nf=8, ball=None):
    S = size * SS
    nfr = loop_noise(S, S, 7 * SS, 55, nf, int(S / nf * 0.8))
    yy, xx = np.mgrid[0:S, 0:S]
    cx, cy = S / 2, S * 0.64
    out = []
    for f in range(nf):
        dx = (xx - cx) / (S * 0.16)
        dy = (yy - cy) / (S * 0.16)
        stretch = np.where(dy < 0, 1 / (1 + (-dy) * 0.0) , 1)
        dyy = np.where(dy < 0, dy / 2.1, dy)  # elongate upward (trail)
        r = np.hypot(dx * (1 + np.clip(-dy, 0, 3) * 0.25), dyy)
        dens = np.clip(1.15 - r * 0.62, 0, 1.2) * (0.4 + 0.8 * nfr[f])
        rgb, al = fire_color(dens)
        img = down(to_img(rgb, al), size, size)
        if ball is not None:
            b = ball.resize((30, 30), Image.LANCZOS)
            arr = np.array(b).astype(float)
            arr[..., 0] = np.clip(arr[..., 0] * 1.05 + 20, 0, 255)
            arr[..., 1] *= 0.75; arr[..., 2] *= 0.4
            b = Image.fromarray(arr.astype(np.uint8), "RGBA")
            img.alpha_composite(b, (int(size / 2 - 15), int(size * 0.64 - 15)))
        out.append(img)
    return out


# ------------------------------------------------------------------ small effects
def impact_dust(size=96, nf=8, color=(200, 165, 120), seed=3):
    r = np.random.default_rng(seed)
    puffs = [dict(a=r.uniform(-math.pi * 0.95, -math.pi * 0.05), sp=r.uniform(14, 34), r0=r.uniform(5, 9),
                  r1=r.uniform(14, 22)) for _ in range(6)]
    pebs = [dict(a=r.uniform(-math.pi * 0.9, -math.pi * 0.1), sp=r.uniform(30, 44), s=r.uniform(2, 3.5))
            for _ in range(6)]
    out = []
    c = (size / 2, size * 0.7)
    for f in range(nf):
        k = f / (nf - 1)
        cv = Image.new("RGBA", (size, size), (0, 0, 0, 0))
        for p in puffs:
            e = 1 - (1 - k) ** 2
            x = c[0] + math.cos(p["a"]) * p["sp"] * e; y = c[1] + math.sin(p["a"]) * p["sp"] * e
            cv.alpha_composite(puff((size, size), x, y, p["r0"] + (p["r1"] - p["r0"]) * e, color, 0.8 * (1 - k) ** 1.2))
        d = ImageDraw.Draw(cv)
        for p in pebs:
            x = c[0] + math.cos(p["a"]) * p["sp"] * k
            y = c[1] + math.sin(p["a"]) * p["sp"] * k + 40 * k * k
            d.ellipse([x - p["s"] / 2, y - p["s"] / 2, x + p["s"] / 2, y + p["s"] / 2],
                      fill=(120, 90, 60, int(255 * (1 - k))))
        out.append(cv)
    return out


def impact_spark(size=64, nf=8, seed=5):
    r = np.random.default_rng(seed)
    rays = [(r.uniform(0, 2 * math.pi), r.uniform(0.6, 1.0)) for _ in range(9)]
    out = []
    S = size * SS
    for f in range(nf):
        k = (f + 1) / nf
        im = Image.new("RGBA", (S, S), (0, 0, 0, 0)); d = ImageDraw.Draw(im)
        c = S / 2
        for a, L in rays:
            r0 = S * 0.5 * L * max(0, k - 0.35) * 1.3
            r1 = S * 0.5 * L * min(1, k * 1.6)
            al = int(255 * (1 - k) ** 0.7)
            d.line([c + math.cos(a) * r0, c + math.sin(a) * r0, c + math.cos(a) * r1, c + math.sin(a) * r1],
                   fill=(255, 220, 130, al), width=int(2.2 * SS))
        if f < 3:
            rr = S * (0.16 - f * 0.04)
            d.ellipse([c - rr, c - rr, c + rr, c + rr], fill=(255, 250, 220, int(230 - f * 70)))
        im = im.filter(ImageFilter.GaussianBlur(0.6 * SS))
        out.append(im.resize((size, size), Image.LANCZOS))
    return out


def hit_flash(nf=6, w=128, h=48):
    out = []
    m = rounded_mask(w, h, 6, inset=2)
    for a in [0.85, 0.6, 0.4, 0.24, 0.12, 0.0][:nf]:
        out.append(to_img(np.ones((h, w, 3)) * 255, m * a))
    return out


def clang(src, nf=8):
    out = []
    a0 = np.array(src).astype(float)
    h, w = a0.shape[:2]
    yy, xx = np.mgrid[0:h, 0:w]
    for f in range(nf):
        a = a0.copy()
        pos = -0.3 + 1.6 * f / (nf - 1)
        band = np.exp(-(((xx / w) * 0.75 + (yy / h) * 0.25 - pos) / 0.07) ** 2)
        a[..., :3] += band[..., None] * np.array([255, 220, 160]) * 0.75
        if f == 0: a[..., :3] += (255 - a[..., :3]) * 0.5
        out.append(Image.fromarray(np.clip(a, 0, 255).astype(np.uint8), "RGBA"))
    return out


def ball_glow_pulse(nf=8, size=96):
    out = []
    for f in range(nf):
        k = 0.5 + 0.5 * math.sin(f / nf * 2 * math.pi)
        out.append(SA.make_glow(size, (255, 170, 60), 2.0 - 0.3 * k, 0.55 + 0.3 * k))
    return out


def main():
    meta = json.load(open("/home/claude/temple/static_meta.json"))
    B = os.path.join(OUT, "bricks")
    dust_col = {"sandstone": (205, 160, 105), "limestone": (210, 200, 180), "granite": (130, 135, 145),
                "jade": (120, 200, 150), "gold": (255, 210, 110), "obsidian": (90, 80, 110)}
    peb_col = {"sandstone": (120, 80, 45), "limestone": (140, 130, 112), "granite": (60, 64, 74),
               "jade": (30, 110, 70), "gold": (200, 140, 30), "obsidian": (30, 25, 40)}
    for kind in ["sandstone", "limestone", "granite", "jade", "gold"]:
        hits = meta["bricks"][kind]["hits"]
        last = f"{kind}_a" + (f"_crack{hits - 1}" if hits > 1 else "")
        src = Image.open(f"{B}/{last}.png").convert("RGBA")
        fr = shatter(src, seed=sum(map(ord, kind)) * 7, dust=dust_col[kind], pebble=peb_col[kind],
                     sparkle=kind in ("gold", "jade"))
        save_strip(f"brick_shatter_{kind}", fr, 30, False, [160, 112],
                   "Play when brick is destroyed. anchor = brick centre inside frame.")
    save_strip("brick_hit_flash", hit_flash(), 30, False, [64, 24],
               "Overlay on any brick when hit but not destroyed (additive or normal blend). Combine with shake offsets.")
    ATLAS["brick_hit_flash"]["shake_offsets"] = [[0, 0], [3, -1], [-3, 1], [2, 0], [-1, 0], [0, 0]]
    obs = Image.open(f"{B}/obsidian_a.png").convert("RGBA")
    save_strip("brick_clang_obsidian", clang(obs), 30, False, [64, 24],
               "Indestructible brick hit: metal shine sweep. Replace brick sprite while playing.")
    save_strip("impact_dust", impact_dust(), 24, False, [48, 67],
               "Spawn at the ball contact point on a brick. Rotate so 'up' faces away from the brick.")
    save_strip("impact_spark", impact_spark(), 30, False, [32, 32],
               "Ball hits wall or paddle.")
    pg = []
    for name, w in [("short", 176), ("normal", 256), ("long", 352)]:
        frames = [SA.make_paddle(w, glow=g) for g in [0.35, 0.8, 1.0, 0.8, 0.55, 0.35, 0.18, 0.06]]
        save_strip(f"paddle_glow_{name}", frames, 24, False, [w // 2, 24],
                   "Paddle sun inlay lights up when the ball bounces off it.")
    save_strip("torch_flame", torch_flame(), 12, True, [32, 118],
               "Looping torch flame. Place anchor on top of each wall sconce. Pulse torch_glow.png alpha 0.8-1.0 along with it.")
    ATLAS["torch_flame"]["positions"] = meta["torches"]
    ball = Image.open(os.path.join(OUT, "ball/ball.png")).convert("RGBA")
    save_strip("ball_fire", fireball(ball=ball), 15, True, [48, 61],
               "Fireball power-up ball. Flame trail points UP in the sprite: rotate so it points opposite the velocity.")
    save_strip("ball_glow_pulse", ball_glow_pulse(), 12, True, [48, 48],
               "Soft halo drawn under the ball (additive).")
    crumble = shatter(ball.resize((32, 32)), canvas=(128, 128), center=(64, 40), n_chunks=7, frames=14, seed=9,
                      dust=(230, 170, 80), pebble=(200, 110, 30), impact=(16, 4), speed=(40, 90), up=60,
                      gravity=500)
    save_strip("ball_lost", crumble, 30, False, [64, 40], "Ball falls out of play: sun-stone crumbles.")
    for n in SA.SYMBOL_TINT:
        frames = [SA.make_powerup(n, shine=-0.15 + 1.3 * i / 7) for i in range(8)] + [SA.make_powerup(n)] * 4
        save_strip(f"powerup_{n}", frames, 15, True, [32, 32], "Falling power-up tablet with light shimmer.")
    # level-clear golden sparkle burst (reuse gold brick shatter style sparkles only)
    sp = []
    r = np.random.default_rng(2)
    stars = [dict(p=(r.uniform(20, 236), r.uniform(20, 236)), t0=r.integers(0, 10), life=r.integers(5, 9),
                  s=int(r.uniform(20, 48))) for _ in range(22)]
    for f in range(16):
        cv = Image.new("RGBA", (256, 256), (0, 0, 0, 0))
        for s in stars:
            age = f - s["t0"]
            if 0 <= age < s["life"]:
                sz = max(4, int(s["s"] * math.sin(age / s["life"] * math.pi)))
                cv.alpha_composite(star4(sz), (int(s["p"][0] - sz / 2), int(s["p"][1] - sz / 2)))
        sp.append(cv)
    save_strip("sparkle_burst", sp, 24, False, [128, 128], "Gold sparkles: combos, level clear, bonus pickups.")
    # ambient particles (static)
    soft_dot(8, (255, 225, 170), 1.5).save(f"{AD}/../background/dust_mote.png")
    soft_dot(6, (255, 150, 50), 1.2).save(f"{AD}/../background/ember.png")
    json.dump(ATLAS, open(os.path.join(OUT, "atlas.json"), "w"), indent=2)


if __name__ == "__main__":
    main()
