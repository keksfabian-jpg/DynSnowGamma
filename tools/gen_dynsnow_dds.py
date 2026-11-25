import math
import os
import struct

OUT_DIR = r"C:\Gamma\mods\Project INVERNO - DynSnow\gamedata\textures\wm"
THM_SRC = r"C:\Gamma\mods\300- Blood pool with trails and steps - xcvb\gamedata\textures\wm\wm_blood_step_l_0.thm"

os.makedirs(OUT_DIR, exist_ok=True)

with open(THM_SRC, "rb") as f:
    THM_BYTES = f.read()

W, H = 256, 256


def clamp(x, lo=0.0, hi=1.0):
    return lo if x < lo else (hi if x > hi else x)


def smoothstep(e0, e1, x):
    t = clamp((x - e0) / (e1 - e0) if e1 != e0 else 0.0)
    return t * t * (3.0 - 2.0 * t)


def hash2(x, y):
    n = math.sin(x * 127.1 + y * 311.7) * 43758.5453
    return n - math.floor(n)


def vnoise(x, y):
    ix, iy = math.floor(x), math.floor(y)
    fx, fy = x - ix, y - iy
    ux = fx * fx * (3.0 - 2.0 * fx)
    uy = fy * fy * (3.0 - 2.0 * fy)
    a = hash2(ix, iy)
    b = hash2(ix + 1, iy)
    c = hash2(ix, iy + 1)
    d = hash2(ix + 1, iy + 1)
    return (a * (1 - ux) + b * ux) * (1 - uy) + (c * (1 - ux) + d * ux) * uy


def fbm(x, y):
    return 0.55 * vnoise(x, y) + 0.30 * vnoise(x * 2.13, y * 2.13) + 0.15 * vnoise(x * 4.37, y * 4.37)


def rgb565(r, g, b):
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)


def encode_dxt5_block(pixels):
    alphas = [p[3] for p in pixels]
    amin, amax = min(alphas), max(alphas)
    if amax == amin:
        a0, a1 = amax, amax
        a_indices = [0] * 16
    else:
        a0, a1 = amax, amin
        palette_a = [
            a0,
            a1,
            (6 * a0 + 1 * a1 + 3) // 7,
            (5 * a0 + 2 * a1 + 3) // 7,
            (4 * a0 + 3 * a1 + 3) // 7,
            (3 * a0 + 4 * a1 + 3) // 7,
            (2 * a0 + 5 * a1 + 3) // 7,
            (1 * a0 + 6 * a1 + 3) // 7,
        ]
        a_indices = [min(range(8), key=lambda idx: abs(a - palette_a[idx])) for a in alphas]

    alpha_bits = 0
    for i in range(16):
        alpha_bits |= (a_indices[i] & 0x7) << (3 * i)
    alpha_bytes = struct.pack("<BB", a0, a1) + alpha_bits.to_bytes(6, "little")

    lums = [p[0] * 0.299 + p[1] * 0.587 + p[2] * 0.114 for p in pixels]
    min_i = min(range(16), key=lambda i: lums[i])
    max_i = max(range(16), key=lambda i: lums[i])
    v0 = rgb565(*pixels[max_i][:3])
    v1 = rgb565(*pixels[min_i][:3])

    if v0 < v1:
        v0, v1 = v1, v0
    elif v0 == v1:
        return alpha_bytes + struct.pack("<HHI", v0, v1, 0)

    c0 = ((v0 >> 11) * 255 // 31, ((v0 >> 5) & 0x3F) * 255 // 63, (v0 & 0x1F) * 255 // 31)
    c1 = ((v1 >> 11) * 255 // 31, ((v1 >> 5) & 0x3F) * 255 // 63, (v1 & 0x1F) * 255 // 31)
    pal = [
        c0,
        c1,
        ((2 * c0[0] + c1[0]) // 3, (2 * c0[1] + c1[1]) // 3, (2 * c0[2] + c1[2]) // 3),
        ((c0[0] + 2 * c1[0]) // 3, (c0[1] + 2 * c1[1]) // 3, (c0[2] + 2 * c1[2]) // 3),
    ]
    c_bits = 0
    for i, p in enumerate(pixels):
        r, g, b, _ = p
        best_c = min(range(4), key=lambda idx: (r - pal[idx][0]) ** 2 + (g - pal[idx][1]) ** 2 + (b - pal[idx][2]) ** 2)
        c_bits |= best_c << (2 * i)

    return alpha_bytes + struct.pack("<HHI", v0, v1, c_bits)


def write_dds_and_thm(name, rgba_grid):
    header = bytearray(128)
    header[0:4] = b"DDS "
    struct.pack_into("<I", header, 4, 124)
    struct.pack_into("<I", header, 8, 0x00081007)
    struct.pack_into("<I", header, 12, H)
    struct.pack_into("<I", header, 16, W)
    struct.pack_into("<I", header, 20, W * H)
    struct.pack_into("<I", header, 24, 1)
    struct.pack_into("<I", header, 28, 1)
    struct.pack_into("<I", header, 76, 32)
    struct.pack_into("<I", header, 80, 0x00000004)
    header[84:88] = b"DXT5"
    struct.pack_into("<I", header, 108, 0x00001000)

    blocks = bytearray()
    for by in range(0, H, 4):
        for bx in range(0, W, 4):
            blk = [rgba_grid[by + py][bx + px] for py in range(4) for px in range(4)]
            blocks.extend(encode_dxt5_block(blk))

    dds_path = os.path.join(OUT_DIR, f"{name}.dds")
    thm_path = os.path.join(OUT_DIR, f"{name}.thm")
    with open(dds_path, "wb") as f:
        f.write(header)
        f.write(blocks)
    with open(thm_path, "wb") as f:
        f.write(THM_BYTES)
    print(f"Wrote {dds_path} ({len(header) + len(blocks)} bytes)")


def shade_snow_field(depth, rim, grain):
    base_r = 122.0 + (1.0 - depth) * 62.0 + rim * 52.0 + (grain - 0.5) * 10.0
    base_g = 144.0 + (1.0 - depth) * 58.0 + rim * 44.0 + (grain - 0.5) * 10.0
    base_b = 178.0 + (1.0 - depth) * 48.0 + rim * 28.0 + (grain - 0.5) * 8.0
    r = int(clamp(base_r, 95, 228))
    g = int(clamp(base_g, 118, 242))
    b = int(clamp(max(base_b, r * 1.12 + 12.0), 140, 255))
    alpha_f = clamp(depth * 0.84 + rim * 0.52, 0.0, 0.90)
    return (r, g, b, int(round(alpha_f * 255.0)))


def boot_sdf(nx, ny, is_right=False):
    if is_right:
        nx = -nx
    curve_x = nx - 0.045 * math.sin(ny * 2.2)
    t = clamp((ny + 0.72) / 1.44, 0.0, 1.0)
    fore = math.exp(-((t - 0.24) / 0.26) ** 2) * 0.33
    heel = math.exp(-((t - 0.82) / 0.22) ** 2) * 0.26
    half_w = 0.16 + fore + heel
    if curve_x > 0:
        half_w -= 0.055 * math.exp(-((t - 0.56) / 0.16) ** 2)
    dy = max(abs(ny) - 0.54, 0.0) / 0.20
    dx = abs(curve_x) / max(half_w, 0.08)
    return math.sqrt(dx * dx + dy * dy) - 1.0, curve_x, ny


def generate_boot(is_right):
    grid = []
    for y in range(H):
        ny = (y / (H - 1)) * 2.0 - 1.0
        row = []
        for x in range(W):
            nx = (x / (W - 1)) * 2.0 - 1.0
            grain = fbm(x * 0.14, y * 0.14)
            dist, cx, cy = boot_sdf(nx, ny, is_right=is_right)
            d_warped = dist + (grain - 0.5) * 0.085
            inside = 1.0 - smoothstep(-0.14, 0.04, d_warped)
            arch_bridge = 1.0 - 0.35 * math.exp(-((cy - 0.08) / 0.15) ** 2)
            lug_wave = 0.5 + 0.5 * math.sin(cy * 38.0 + abs(cx) * 6.0)
            border_lug = smoothstep(-0.22, -0.04, d_warped) * (0.5 + 0.5 * math.sin(cy * 52.0))
            tread_depth = inside * arch_bridge * (0.72 + 0.22 * lug_wave + 0.10 * border_lug)
            rim = math.exp(-((d_warped - 0.09) / 0.13) ** 2) * (0.65 + 0.35 * grain)
            border_fade = 1.0 - smoothstep(0.76, 0.96, math.hypot(nx, ny))
            row.append(shade_snow_field(tread_depth * border_fade, rim * border_fade * (1.0 - inside * 0.7), grain))
        grid.append(row)
    return grid


def generate_paw():
    pads = [
        (0.0, 0.24, 0.30, 0.24),
        (-0.30, -0.14, 0.13, 0.18),
        (-0.11, -0.32, 0.13, 0.19),
        (0.11, -0.32, 0.13, 0.19),
        (0.30, -0.14, 0.13, 0.18),
        (-0.33, -0.38, 0.045, 0.07),
        (-0.12, -0.56, 0.045, 0.07),
        (0.12, -0.56, 0.045, 0.07),
        (0.33, -0.38, 0.045, 0.07),
    ]
    grid = []
    for y in range(H):
        ny = (y / (H - 1)) * 2.0 - 1.0
        row = []
        for x in range(W):
            nx = (x / (W - 1)) * 2.0 - 1.0
            grain = fbm(x * 0.15 + 11.3, y * 0.15 + 7.1)
            min_d = min(math.hypot((nx - px) / rx, (ny - py) / ry) - 1.0 for px, py, rx, ry in pads)
            d_warped = min_d + (grain - 0.5) * 0.12
            inside = 1.0 - smoothstep(-0.20, 0.06, d_warped)
            rim = math.exp(-((d_warped - 0.12) / 0.18) ** 2) * (0.6 + 0.4 * grain)
            border_fade = 1.0 - smoothstep(0.76, 0.96, math.hypot(nx, ny))
            row.append(shade_snow_field(inside * 0.92 * border_fade, rim * border_fade * (1.0 - inside * 0.7), grain))
        grid.append(row)
    return grid


def generate_crater():
    grid = []
    for y in range(H):
        ny = (y / (H - 1)) * 2.0 - 1.0
        row = []
        for x in range(W):
            nx = (x / (W - 1)) * 2.0 - 1.0
            r = math.hypot(nx, ny)
            ang = math.atan2(ny, nx)
            grain = fbm(x * 0.11 + 31.0, y * 0.11 + 19.0)
            rays = 0.5 + 0.5 * math.sin(ang * 9.0 + grain * 4.0)
            d = r - (0.48 + (grain - 0.5) * 0.16 + rays * 0.06)
            inside = (1.0 - smoothstep(-0.28, 0.05, d)) * (0.65 + 0.35 * grain)
            rim = math.exp(-((d - 0.10) / 0.16) ** 2) * (0.55 + 0.45 * grain)
            border_fade = 1.0 - smoothstep(0.74, 0.96, r)
            row.append(shade_snow_field(inside * border_fade, rim * border_fade * (1.0 - inside * 0.6), grain))
        grid.append(row)
    return grid


def generate_campfire_melt():
    """
    Organic thawed dark wet earth/ash patch with a slushy icy meltwater ring at the outer edge.
    Keeps B >= R * 1.15 + 8 so effects_wallmark_blood.ps routes it through the smooth lit DynSnow path!
    """
    grid = []
    for y in range(H):
        ny = (y / (H - 1)) * 2.0 - 1.0
        row = []
        for x in range(W):
            nx = (x / (W - 1)) * 2.0 - 1.0
            r = math.hypot(nx, ny)
            coarse = fbm(x * 0.055 + 5.7, y * 0.055 + 9.2)
            fine = fbm(x * 0.22 + 17.4, y * 0.22 + 3.8)
            # Ragged heat-melted snow boundary
            edge_r = 0.68 + (coarse - 0.5) * 0.22 + (fine - 0.5) * 0.08
            d = r - edge_r

            # Inner thawed wet soil/charcoal zone
            soil_mask = 1.0 - smoothstep(-0.22, 0.02, d)
            # Wet melting slush transition ring right at the receding snow line
            slush_ring = math.exp(-((d + 0.03) / 0.11) ** 2)

            # Dark damp earth/ash (quantization-safe B > R * 1.15) combined with icy slush rim
            soil_r = 34.0 + fine * 20.0 + slush_ring * 52.0
            soil_g = 36.0 + fine * 20.0 + slush_ring * 64.0
            soil_b = 48.0 + fine * 22.0 + slush_ring * 84.0

            ri = int(clamp(soil_r, 24, 135))
            gi = int(clamp(soil_g, 26, 150))
            bi = int(clamp(max(soil_b, ri * 1.18 + 10.0), 40, 185))

            border_fade = 1.0 - smoothstep(0.78, 0.97, r)
            alpha_f = clamp((soil_mask * 0.88 + slush_ring * 0.42) * border_fade, 0.0, 0.90)
            row.append((ri, gi, bi, int(round(alpha_f * 255.0))))
        grid.append(row)
    return grid


write_dds_and_thm("wm_dynsnow_boot_l", generate_boot(is_right=False))
write_dds_and_thm("wm_dynsnow_boot_r", generate_boot(is_right=True))
write_dds_and_thm("wm_dynsnow_paw", generate_paw())
write_dds_and_thm("wm_dynsnow_crater", generate_crater())
write_dds_and_thm("wm_dynsnow_melt", generate_campfire_melt())
