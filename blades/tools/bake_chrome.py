#!/usr/bin/env python3
"""Trace blade chrome and crop art from References/games tab.png. No generated art."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REF = ROOT.parent / "References" / "games tab.png"
N = 24

from reference_io import decode, write_png

w, h, rows = decode(REF)

def pix(x, y):
    if x < 0:
        x = 0
    if x >= w:
        x = w - 1
    if y < 0:
        y = 0
    if y >= h:
        y = h - 1
    i = x * 3
    r = rows[y]
    return (r[i], r[i + 1], r[i + 2])

def lum(c):
    return (c[0] * 3 + c[1] * 4 + c[2]) // 8

def chroma(c):
    return max(c) - min(c)

def is_glass(c):
    r, g, b = c
    return g > r + 28 and g > b + 28 and g > 70

def glass_bounds(y):
    run = None
    first = last = None
    row = rows[y]
    for x in range(w):
        i = x * 3
        c = (row[i], row[i + 1], row[i + 2])
        if is_glass(c):
            if run is None:
                run = x
        else:
            if run is not None and x - run >= 12:
                if first is None:
                    first = run
                last = x - 1
            run = None
    if run is not None and w - run >= 12:
        if first is None:
            first = run
        last = w - 1
    return first, last

raw_g = [glass_bounds(y) for y in range(h)]

def median_edge(y, which):
    vals = []
    for k in range(y - 4, y + 5):
        if 0 <= k < h and raw_g[k][which] is not None:
            vals.append(raw_g[k][which])
    if not vals:
        return None
    vals.sort()
    return vals[len(vals) // 2]

sm = []
for y in range(h):
    if raw_g[y][0] is None:
        sm.append((None, None))
    else:
        sm.append((median_edge(y, 0), median_edge(y, 1)))

def rail_edges(y, gl, gr):
    bg = lum(pix(16, y))
    groove = None
    best = 999
    for x in range(36, max(37, gl - 2)):
        c = pix(x, y)
        L = lum(c)
        if chroma(c) < 40 and L < best:
            best = L
            groove = x
    if groove is None or best >= bg - 16:
        groove = max(40, gl - 70)
    bgR = lum(pix(1268, y))
    outer = gr
    for x in range(gr + 1, 1268):
        if abs(lum(pix(x, y)) - bgR) > 22:
            outer = x
    return groove, outer

edges = []
for y in range(h):
    gl, gr = sm[y]
    if gl is None:
        edges.append((0, 0, 0, 0))
    else:
        ll, rr = rail_edges(y, gl, gr)
        edges.append((gl, gr, ll, rr))

def med_e(y, which):
    vals = [edges[k][which] for k in range(max(0, y - 3), min(h, y + 4)) if edges[k][1] > edges[k][0]]
    if not vals:
        return edges[y][which]
    vals.sort()
    return vals[len(vals) // 2]

edges = [(med_e(y, 0), med_e(y, 1), med_e(y, 2), med_e(y, 3)) if edges[y][1] > edges[y][0] else (0, 0, 0, 0) for y in range(h)]

# Content that must not tint the glass: the photo, the logo, and non-glass pixels.
mask = bytearray(w * h)
for y in range(h):
    gl, gr, _ll, _rr = edges[y]
    if gr <= gl:
        continue
    dirty = [0] * w
    for x in range(gl, gr + 1):
        if not is_glass(pix(x, y)):
            dirty[x] = 1
    if 140 <= y <= 365:
        for x in range(690, min(gr, 1078)):
            dirty[x] = 1
    if 42 <= y <= 112:
        for x in range(820, min(gr, 1110)):
            dirty[x] = 1
    for x in range(w):
        if not dirty[x]:
            continue
        for d in range(-3, 4):
            xx = x + d
            if 0 <= xx < w:
                mask[y * w + xx] = 1

def clean_glass(y, gl, gr):
    pts = []
    x = gl
    while x <= gr:
        if not mask[y * w + x] and is_glass(pix(x, y)):
            pts.append((x, pix(x, y)))
        x += 3
    return pts

def interp(pts, x, fallback):
    if not pts:
        return fallback
    if x <= pts[0][0]:
        return pts[0][1]
    if x >= pts[-1][0]:
        return pts[-1][1]
    lo, hi = 0, len(pts) - 1
    while hi - lo > 1:
        mid = (lo + hi) // 2
        if pts[mid][0] < x:
            lo = mid
        else:
            hi = mid
    x0, c0 = pts[lo]
    x1, c1 = pts[hi]
    if x1 == x0:
        return c0
    u = (x - x0) / (x1 - x0)
    return tuple(int(c0[k] + (c1[k] - c0[k]) * u) for k in range(3))

def clean_metal(y, x0, x1):
    pts = []
    x = x0
    while x <= x1:
        c = pix(x, y)
        if chroma(c) < 42 and lum(c) > 168:
            pts.append((x, c))
        x += 2
    return pts

glass_s = []
left_s = []
right_s = []
for y in range(h):
    gl, gr, ll, rr = edges[y]
    if gr <= gl:
        glass_s.append([(0, 0, 0)] * N)
        left_s.append([(170, 172, 168)] * N)
        right_s.append([(170, 172, 168)] * N)
        continue
    gp = clean_glass(y, gl, gr)
    fb = pix((gl + gr) // 2, y)
    glass_s.append([interp(gp, gl + (gr - gl) * i // (N - 1), fb) for i in range(N)])
    lp = clean_metal(y, ll, gl)
    left_s.append([interp(lp, ll + (gl - ll) * i // (N - 1), pix(ll, y)) for i in range(N)])
    rp = clean_metal(y, gr, rr)
    right_s.append([interp(rp, gr + (rr - gr) * i // (N - 1), pix(rr, y)) for i in range(N)])

def vmedian(src, rad):
    out = []
    for y in range(h):
        row = []
        for i in range(N):
            br, bg, bb = [], [], []
            for k in range(y - rad, y + rad + 1):
                if 0 <= k < h:
                    c = src[k][i]
                    br.append(c[0])
                    bg.append(c[1])
                    bb.append(c[2])
            br.sort()
            bg.sort()
            bb.sort()
            m = len(br) // 2
            row.append((br[m], bg[m], bb[m]))
        out.append(row)
    return out

glass_s = vmedian(glass_s, 14)
left_s = vmedian(left_s, 1)
right_s = vmedian(right_s, 1)

# Preview of the baked chrome, so the silhouette can be checked without the Mac window.
img = []
for y in range(h):
    gl, gr, ll, rr = edges[y]
    bg = pix(12, y)
    line = bytearray(w * 3)
    for x in range(w):
        line[x * 3 : x * 3 + 3] = bytes(bg)
    def paint(x0, x1, cols):
        span = x1 - x0
        if span <= 1:
            return
        for x in range(max(0, x0), min(w - 1, x1) + 1):
            f = (x - x0) * (N - 1) / span
            i = int(f)
            if i >= N - 1:
                c = cols[-1]
            else:
                u = f - i
                a, b = cols[i], cols[i + 1]
                c = tuple(int(a[k] + (b[k] - a[k]) * u) for k in range(3))
            line[x * 3 : x * 3 + 3] = bytes(c)
    if gr > gl:
        paint(ll, gl, left_s[y])
        paint(gl, gr, glass_s[y])
        paint(gr, rr, right_s[y])
    img.append(line)
out_dir = ROOT / "build" / "refcheck"
out_dir.mkdir(parents=True, exist_ok=True)
write_png(out_dir / "chrome.png", w, h, img)

def c_bytes(name, blob):
    lines = [f"const unsigned char {name}[] = {{"]
    row = []
    for i, b in enumerate(blob):
        row.append(f"{b}")
        if len(row) == 24:
            lines.append("\t" + ", ".join(row) + ",")
            row = []
    if row:
        lines.append("\t" + ", ".join(row))
    lines.append("};")
    return "\n".join(lines)

def c_ushorts(name, vals):
    lines = [f"const unsigned short {name}[720] = {{"]
    row = []
    for v in vals:
        row.append(str(v))
        if len(row) == 20:
            lines.append("\t" + ", ".join(row) + ",")
            row = []
    if row:
        lines.append("\t" + ", ".join(row))
    lines.append("};")
    return "\n".join(lines)

def c_ints(name, vals):
    lines = [f"const int {name}[{len(vals)}] = {{"]
    row = []
    for v in vals:
        row.append(str(v))
        if len(row) == 16:
            lines.append("\t" + ", ".join(row) + ",")
            row = []
    if row:
        lines.append("\t" + ", ".join(row))
    lines.append("};")
    return "\n".join(lines)

def rail_span(y, x0, x1):
    # Rails are static reference textures, including their original lettering.
    # Removing dark pixels also destroyed grooves and left partial ghost glyphs.
    return bytes(channel for x in range(x0, x1) for channel in pix(x, y))


bg_blob = bytearray()
for y in range(h):
    bg_blob.extend(pix(12, y))
glass_blob = bytearray()
left_blob = bytearray()
right_blob = bytearray()
left_off = [0]
right_off = [0]
for y in range(h):
    gl, gr, ll, rr = edges[y]
    for c in glass_s[y]:
        glass_blob.extend(c)
    if gr <= gl:
        left_off.append(len(left_blob))
        right_off.append(len(right_blob))
        continue
    left_blob.extend(rail_span(y, ll, gl))
    right_blob.extend(rail_span(y, gr, rr))
    left_off.append(len(left_blob))
    right_off.append(len(right_blob))

data = []
data.append("/* Traced from References/games tab.png. Generated by tools/bake_chrome.py. */")
data.append(c_ushorts("chrome_gl", [e[0] for e in edges]))
data.append(c_ushorts("chrome_gr", [e[1] for e in edges]))
data.append(c_ushorts("chrome_ll", [e[2] for e in edges]))
data.append(c_ushorts("chrome_rr", [e[3] for e in edges]))
data.append(c_bytes("chrome_bg", bg_blob))
data.append(c_bytes("chrome_glass", glass_blob))
data.append(c_ints("chrome_left_off", left_off))
data.append(c_bytes("chrome_left_px", left_blob))
data.append(c_ints("chrome_right_off", right_off))
data.append(c_bytes("chrome_right_px", right_blob))
(ROOT / "src" / "chrome_data.c").write_text("\n\n".join(data) + "\n")
print("chrome_data", (ROOT / "src" / "chrome_data.c").stat().st_size)

def crop(x, y, cw, ch):
    blob = bytearray()
    for yy in range(y, y + ch):
        for xx in range(x, x + cw):
            blob.extend(pix(xx, yy))
    return blob

sprites = []
icon_y = (140, 210, 280, 350, 420, 490)
for i, y0 in enumerate(icon_y):
    sprites.append((f"icon{i}", 212, y0, 64, 60, crop(212, y0, 64, 60)))

# Logo: silver and light pixels in the header's right side.
xs, ys = [], []
for yy in range(48, 108):
    for xx in range(800, 1110):
        c = pix(xx, yy)
        if not is_glass(c) and lum(c) > 120:
            xs.append(xx)
            ys.append(yy)
if xs:
    lx, ly, lw, lh = min(xs) - 4, min(ys) - 4, max(xs) - min(xs) + 8, max(ys) - min(ys) + 8
else:
    lx, ly, lw, lh = 860, 50, 220, 56
print("logo", lx, ly, lw, lh)
sprites.append(("logo", lx, ly, lw, lh, crop(lx, ly, lw, lh)))

sx, sy, sw, sh = 710, 138, 360, 206
print("shot", sx, sy, sw, sh)
sprites.append(("shot", sx, sy, sw, sh, crop(sx, sy, sw, sh)))

# ESRB badge: near-black block under the description.
xs, ys = [], []
for yy in range(500, 640):
    for xx in range(680, 860):
        c = pix(xx, yy)
        if c[0] < 30 and c[1] < 30 and c[2] < 30:
            xs.append(xx)
            ys.append(yy)
if xs:
    ex, ey = min(xs) - 2, min(ys) - 2
    ew, eh = max(xs) - min(xs) + 6, max(ys) - min(ys) + 6
else:
    ex, ey, ew, eh = 730, 530, 48, 64
print("esrb", ex, ey, ew, eh)
sprites.append(("esrb", ex, ey, ew, eh, crop(ex, ey, ew, eh)))

# The panel is a translucent plate. A tight text bbox clips the rounded ends.
cx, cy, cw, ch = 796, 518, 268, 78
print("content", cx, cy, cw, ch)
sprites.append(("content", cx, cy, cw, ch, crop(cx, cy, cw, ch)))

def cluster(pred, x0, x1, y0, y1, pad=2):
    xs, ys = [], []
    for yy in range(y0, y1):
        for xx in range(x0, x1):
            if pred(pix(xx, yy)):
                xs.append(xx)
                ys.append(yy)
    if not xs:
        return None
    return min(xs) - pad, min(ys) - pad, max(xs) - min(xs) + 1 + pad * 2, max(ys) - min(ys) + 1 + pad * 2

# Full orbs, including a little of the glass around them, placed back at these origins.
buttons = [
    ("btn_y", 230, 606, 34, 34),
    ("btn_x", 212, 630, 34, 34),
    ("btn_b", 1044, 598, 32, 32),
    ("btn_a", 1046, 632, 34, 34),
]
for name, x, y, cw, ch in buttons:
    print(name, x, y, cw, ch)
    sprites.append((name, x, y, cw, ch, crop(x, y, cw, ch)))

# The Games word lies on glass rather than the silver rail. Keep this tight
# static reference patch separate from the selectable, live list content.
sprites.append(("games_label", 138, 140, 40, 116, crop(138, 140, 40, 116)))

parts = ["#include \"sprites_ref.h\"", "", "#include \"draw.h\"", "", "/* Crops from References/games tab.png. */"]
table = []
for name, x, y, cw, ch, blob in sprites:
    parts.append(f"static const unsigned char spr_{name}[] = {{")
    row = []
    for b in blob:
        row.append(str(b))
        if len(row) == 24:
            parts.append("\t" + ", ".join(row) + ",")
            row = []
    if row:
        parts.append("\t" + ", ".join(row))
    parts.append("};")
    table.append(f"\t{{ {cw}, {ch}, spr_{name} }},")

parts.append("static const struct { int w, h; const unsigned char *px; } spr_tab[] = {")
parts.extend(table)
parts.append("};")
parts.append("""
void sprite_blit(int id, int x, int y)
{
	if (id < 0 || id >= SPR_COUNT)
		return;
	draw_rgb(x, y, spr_tab[id].w, spr_tab[id].h, spr_tab[id].px);
}
void sprite_blit_scaled(int id, int x, int y, int w, int h)
{
	int row, col;
	if (id < 0 || id >= SPR_COUNT || w <= 0 || h <= 0)
		return;
	for (row = 0; row < h; row++) {
		for (col = 0; col < w; col++) {
			int sx = col * spr_tab[id].w / w;
			int sy = row * spr_tab[id].h / h;
			const unsigned char *p = spr_tab[id].px + (sy * spr_tab[id].w + sx) * 3;
			draw_blend(x + col, y + row, p[0], p[1], p[2], 255);
		}
	}
}

""")
(ROOT / "src" / "sprites_ref.c").write_text("\n".join(parts) + "\n")
print("sprites", (ROOT / "src" / "sprites_ref.c").stat().st_size)
print("placements")
for name, x, y, cw, ch, _b in sprites:
    print(f"  {name} {x},{y} {cw}x{ch}")
