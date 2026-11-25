import math
import os
import struct

OUT_DIR = r"C:\Gamma\mods\Project INVERNO - DynSnow\gamedata\textures\wm"
THM_SRC = r"C:\Gamma\mods\300- Blood pool with trails and steps - xcvb\gamedata\textures\wm\wm_blood_step_l_0.thm"

os.makedirs(OUT_DIR, exist_ok=True)
with open(THM_SRC, "rb") as f:
    THM_BYTES = f.read()

W, H = 256, 256

# Exact hardware-neutral 2x-multiplicative block from X-Ray's wm_blood_step_l_0.dds:
# alpha = 0 AND interpolated color = RGB(127.5, 127.5, 127.5) -> 100% invisible border!
NEUTRAL_DXT5_BLOCK = struct.pack("<BB6sHHI", 0, 0, b"\x00" * 6, 0x83D0, 0x7C0F, 0xFFFFFFFF)


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
    # If entire 4x4 block has zero mask weight, emit exact neutral 0.5-grey / 0-alpha block!
    max_a = max(p[3] for p in pixels)
    if max_a <= 2:
        return NEUTRAL_DXT5_BLOCK

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
        if abs(pixels[0][0] - 128) <= 4 and abs(pixels[0][1] - 128) <= 4 and abs(pixels[0][2] - 128) <= 4:
            return alpha_bytes + struct.pack("<HHI", 0x83D0, 0x7C0F, 0xFFFFFFFF)
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


def shade_mult2x_snow(depth, rim, grain):
    """
    Calibrated for X-Ray's 2x multiplicative wallmark blending (dst * src * 2):
    - Outside print (depth=0, rim=0): EXACT RGB(128, 128, 128), A=0 -> 100% invisible quad!
    - Inside compressed sole (depth->1): deep icy blue-grey shadow RGB(52, 68, 96) -> crisp 3D cavity on bright snow!
    - Raised snow rim (rim->1): sunlit crest RGB(150, 155, 164) on one side + shadow on inner lip!
    """
    w = clamp(max(depth, rim), 0.0, 1.0)
    if w <= 0.008:
        return (128, 128, 128, 0)

    # Target color at full depth vs full rim around 128 neutral midpoint
    # Cavity darkens white snow to cool deep blue-shadow; rim slightly brightens snow crest
    r = 128.0 - depth * (78.0 - (grain - 0.5) * 10.0) + rim * 20.0
    g = 128.0 - depth * (60.0 - (grain - 0.5) * 10.0) + rim * 24.0
    b = 128.0 - depth * (32.0 - (grain - 0.5) * 8.0) + rim * 30.0

    ri = int(round(clamp(r, 36.0, 168.0)))
    gi = int(round(clamp(g, 48.0, 172.0)))
    bi = int(round(clamp(b, 72.0, 180.0)))
    ai = int(round(clamp(w * 255.0, 0.0, 245.0)))
    return (ri, gi, bi, ai)


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
            r_rad = math.hypot(nx, ny)
            if r_rad > 0.90:
                row.append((128, 128, 128, 0))
                continue
            grain = fbm(x * 0.14, y * 0.14)
            dist, cx, cy = boot_sdf(nx, ny, is_right=is_right)
            d_warped = dist + (grain - 0.5) * 0.08
            inside = 1.0 - smoothstep(-0.14, 0.03, d_warped)
            arch_bridge = 1.0 - 0.30 * math.exp(-((cy - 0.08) / 0.15) ** 2)
            lug_wave = 0.5 + 0.5 * math.sin(cy * 38.0 + abs(cx) * 6.0)
            tread_depth = inside * arch_bridge * (0.76 + 0.24 * lug_wave)
            rim = math.exp(-((d_warped - 0.09) / 0.11) ** 2) * (0.55 + 0.35 * grain) * (1.0 - inside)
            border_fade = 1.0 - smoothstep(0.68, 0.88, r_rad)
            row.append(shade_mult2x_snow(tread_depth * border_fade, rim * border_fade, grain))
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
            r_rad = math.hypot(nx, ny)
            if r_rad > 0.90:
                row.append((128, 128, 128, 0))
                continue
            grain = fbm(x * 0.15 + 11.3, y * 0.15 + 7.1)
            min_d = min(math.hypot((nx - px) / rx, (ny - py) / ry) - 1.0 for px, py, rx, ry in pads)
            d_warped = min_d + (grain - 0.5) * 0.11
            inside = 1.0 - smoothstep(-0.20, 0.05, d_warped)
            rim = math.exp(-((d_warped - 0.10) / 0.14) ** 2) * 0.65 * (1.0 - inside)
            border_fade = 1.0 - smoothstep(0.68, 0.88, r_rad)
            row.append(shade_mult2x_snow(inside * 0.95 * border_fade, rim * border_fade, grain))
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
            if r > 0.90:
                row.append((128, 128, 128, 0))
                continue
            ang = math.atan2(ny, nx)
            grain = fbm(x * 0.11 + 31.0, y * 0.11 + 19.0)
            rays = 0.5 + 0.5 * math.sin(ang * 9.0 + grain * 4.0)
            d = r - (0.46 + (grain - 0.5) * 0.14 + rays * 0.05)
            inside = (1.0 - smoothstep(-0.26, 0.04, d)) * (0.70 + 0.30 * grain)
            rim = math.exp(-((d - 0.09) / 0.13) ** 2) * 0.65 * (1.0 - inside)
            border_fade = 1.0 - smoothstep(0.66, 0.88, r)
            row.append(shade_mult2x_snow(inside * border_fade, rim * border_fade, grain))
        grid.append(row)
    return grid


def generate_campfire_melt():
    """
    2x-multiplicative thawed warm earth decal for static meshes around campfires:
    - Outside melt circle: EXACT RGB(128, 128, 128), A=0 -> zero square border!
    - Inside melt circle: warm dark brown mud RGB(38, 30, 22) -> turns white snow into dark thawed earth!
    """
    grid = []
    for y in range(H):
        ny = (y / (H - 1)) * 2.0 - 1.0
        row = []
        for x in range(W):
            nx = (x / (W - 1)) * 2.0 - 1.0
            r = math.hypot(nx, ny)
            if r > 0.88:
                row.append((128, 128, 128, 0))
                continue
            coarse = fbm(x * 0.055 + 5.7, y * 0.055 + 9.2)
            fine = fbm(x * 0.22 + 17.4, y * 0.22 + 3.8)
            edge_r = 0.60 + (coarse - 0.5) * 0.18 + (fine - 0.5) * 0.06
            soil_w = (1.0 - smoothstep(edge_r - 0.18, edge_r + 0.06, r)) * (1.0 - smoothstep(0.66, 0.86, r))
            if soil_w <= 0.008:
                row.append((128, 128, 128, 0))
                continue
            target_r = 38.0 + fine * 18.0
            target_g = 30.0 + fine * 15.0
            target_b = 24.0 + fine * 14.0
            ri = int(round(128.0 + (target_r - 128.0) * soil_w))
            gi = int(round(128.0 + (target_g - 128.0) * soil_w))
            bi = int(round(128.0 + (target_b - 128.0) * soil_w))
            ai = int(round(clamp(soil_w * 240.0, 0.0, 240.0)))
            row.append((ri, gi, bi, ai))
        grid.append(row)
    return grid


write_dds_and_thm("wm_dynsnow_boot_l", generate_boot(is_right=False))
write_dds_and_thm("wm_dynsnow_boot_r", generate_boot(is_right=True))
write_dds_and_thm("wm_dynsnow_paw", generate_paw())
write_dds_and_thm("wm_dynsnow_crater", generate_crater())
write_dds_and_thm("wm_dynsnow_melt", generate_campfire_melt())
