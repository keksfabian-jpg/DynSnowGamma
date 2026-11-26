# ❄️ DynSnow for S.T.A.L.K.E.R. G.A.M.M.A. (`DynSnowGamma` v1.7 — Full-Map Edition)

An interactive, **zero-VRAM (`0 KB` textures!) full-map 3D volumetric snow deformation, subtle weapon frost dusting, smooth campfire snow-melting, and independent NPC vs. World-Object snow control** addon for **S.T.A.L.K.E.R. G.A.M.M.A.** (Anomaly 1.5.3 DX11 + Screen Space Shaders 23.5 + Project: INVERNO), inspired by **dynSnow** from Garry's Mod.

Engineered with a single-pass per-pixel spatial prefilter (`DynSnowContext`), camera-invariant world-locked coordinates, and a dual-scale bit-exact 37-float Anchor + Landmark Delta stream to run at a locked **60 FPS** even on 4 GB VRAM handheld APUs.

---

## ✨ Features (v1.7)

- **🗺️ True Map-Wide Visibility (`0 m .. 1,500+ m` across `3.2 km × 3.2 km` — Zero Camera Shift, Zero Distance Cutoff!):**
  Encodes absolute `3,200 m × 3,200 m` world-space coordinates via a dual-scale bit-exact **37-float Anchor + Landmark Delta stream** (`19.5 cm` fine steps + `73.8 m` consolidated straight landmarks, zero torus wrapping, zero corner-cutting, **0.00 mm shift when walking or looking around!**) and renders 3D snow trenches, sunlit side berms, and melted campfires across **all three** terrain LOD tiers (`deffer_terrain_high_flat_d.ps`, `deffer_terrain_mid_flat.ps`, and `deffer_terrain_low_flat.ps` + `deffer_impl_flat.ps`).
  - **100% Camera-Rotation-Invariant + 3DSS Sniper Scope Support:** Looking around (`360°`) never evicts, reorders, or shifts nearby/distant trails, while aiming down sights (including **3DSS Picture-in-Picture scopes** and binoculars) automatically prioritizes distant Stalkers & Mutants under your crosshair across hillsides and valleys, fading smoothly from start to end over **5–6 minutes (`>= 305s`)**.
- **🧍🚗 Independent MCM Sliders for NPC/Mutant Snow vs. Vehicle/World-Object Snow:**
  Distinguishes skeletal-skinned characters (`SKIN_0..SKIN_4`: Stalkers, NPCs, Mutants) from static & rigid world geometry (cars, trucks, BTRs, roofs, barrels, crates, buildings) directly in the DX11 vertex/pixel pipeline so you can turn down or completely disable (`0.0`) snow on NPCs & Mutants while keeping snow on vehicles & roofs untouched!
- **🔫 Subtle, Authentic Crystalline Frost & Snow Dusting on Weapons:**
  Uses pixel-level bumped world-space normals (`Nw_hud.y`), bump height cavities (`S.height`), and weapon texture UVs (`I.tcdh.xy`) so delicate rime frost and tiny snow crystals settle naturally on top ridges, grooves, and sights without whitewashing sleeves or hands.
- **🔥 Butter-Smooth Full-Map 3D Campfire Snow Melting & Re-Snowing:**
  Supports up to **2 simultaneous campfires** anywhere on a `3.2 km` map, driven by continuous per-frame (`dt`) unquantized float radii (`30 Hz` shader sync).
- **🌨️ 100% Continuous Outdoor Snowfall Hook, Tree Cascades & Full Underground Lab Support:**
  Keeps rich snowfall (*Snowflakes*, *Heavy Snow*, *Blizzard*) active outdoors in **Project: INVERNO** while automatically disabling snow across all underground labs & bunkers.

---

## 📦 Installation (Mod Organizer 2 / G.A.M.M.A.)

1. Download this repository as a `.zip` (**Code → Download ZIP**).
2. In **Mod Organizer 2**, click **Install a new mod from an archive** and select the downloaded `.zip`.
3. Place **`DynSnowGamma`** at the **very bottom** of your MO2 mod list (below *Screen Space Shaders 23* and *Project: INVERNO*).
4. Delete `Anomaly\appdata\shaders_cache` once before launching so the DX11 shaders compile cleanly.
5. Configure all features live in-game via **Options → MCM → `!DynSnow`**.

### 🔧 Separate Compatibility Patch (Quark Engine / Standalone Anomaly without SSS 23)
- **Standard G.A.M.M.A. users do NOT need this patch.**
- If you are playing on **Quark Engine**, **Vanilla Anomaly**, or a custom modpack **without Screen Space Shaders 23** (and get a `CBlender_deffer_model::Compile` shader error on startup), install the separate patch archive [`patches/DynSnowGamma_Quark_NoSSS_Patch.zip`](patches/DynSnowGamma_Quark_NoSSS_Patch.zip) **below** `DynSnowGamma` in Mod Organizer 2 and clear `appdata\shaders_cache`.

---

## 🇩🇪 Kurzbeschreibung (Deutsch)

**DynSnowGamma (v1.7 — Full-Map Edition)** bringt echte volumetrische 3D-Schneeverformung nach Vorbild von *Garry's Mod dynSnow* nach **S.T.A.L.K.E.R. G.A.M.M.A.**:
- **Quer über die gesamte Map sichtbar (`0 m bis 1.500+ m` ohne Distanzlimit)** über alle drei Terrain-LOD-Stufen (*High*, *Mid* & *Low*)
- **Getrennte MCM-Regler für NPCs/Mutanten vs. Autos/Dächer/Weltobjekte** (`0.0 = komplett aus` auf Charakteren/Mutanten, während Schnee auf Fahrzeugen & Dächern erhalten bleibt)
- **5+ Minuten persistente 3D-Schneegräben** für Spieler sowie alle aktiven Stalker & Mutanten auf der gesamten Karte
- **Dezenter, kristalliner Raureif & feiner Schneestaub auf Waffen** + **stufenloses Lagerfeuer-Schmelzen**
- **Separater Patch (`patches/DynSnowGamma_Quark_NoSSS_Patch.zip`)** für Spieler mit *Quark Engine* / ohne *Screen Space Shaders 23*

---

## 📜 License

Released under the [MIT License](LICENSE).
