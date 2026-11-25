# ❄️ DynSnow for S.T.A.L.K.E.R. G.A.M.M.A. (`DynSnowGamma` v1.2)

An interactive, **zero-VRAM (`0 KB` textures!) 3D volumetric snow deformation, subtle weapon frost dusting, and smooth campfire snow-melting** addon for **S.T.A.L.K.E.R. G.A.M.M.A.** (Anomaly 1.5.3 DX11 + Screen Space Shaders 23.5 + Project: INVERNO), inspired by **dynSnow** from Garry's Mod.

Engineered with a single-pass per-pixel spatial prefilter (`DynSnowContext`) to run at a locked **60 FPS** even on 4 GB VRAM handheld APUs.

---

## ✨ Features (v1.2)

- **🏔️ Persistent 5+ Minute Multi-Actor 3D Snow Trenches (Player, Stalkers & Mutants):**
  Streams a live `30m × 30m` window of world-space snow trails (`25` connected 3D capsule nodes with collinear polyline simplification covering `40–65m` of visible tracks, backed by an unlimited world-space history) into DX11 terrain raymarching.
  - Both the **Player** and nearby **Stalkers & Mutants (within 40m)** carve real 3D sunken snow trenches with raised side berms and 3D slope normals.
  - Trails persist for **5–6 minutes (`>= 300s`)** and **slowly fill back in smoothly from oldest start to newest end** (`31` interpolated depth levels along every segment).
  - Pure 3D volumetric deformation — no fake 2D footprint decals.
- **🔫 Subtle, Authentic Crystalline Frost & Snow Dusting on Weapons:**
  Uses pixel-level bumped world-space normals (`Nw_hud.y`), bump height cavities (`S.height`), and weapon texture UVs (`I.tcdh.xy`) so delicate rime frost and tiny snow crystals settle naturally on top ridges, grooves, and sights while keeping ~75% of dark gunmetal/wood and 94%+ of normal map detail crisp — without whitewashing sleeves or hands. Shakes slightly on recoil and melts indoors or near lit campfires.
- **🔥 Butter-Smooth 3D Campfire Snow Melting & Re-Snowing:**
  Supports up to **2 simultaneous nearby campfires** driven by continuous per-frame (`dt`) unquantized float radii (`30 Hz` shader sync) and a wide radial slush-to-soil thermal falloff. Lit campfires fluidly melt surrounding 3D snow outward; extinguished fires smoothly snow back over from the outer rim inward.
- **🌨️ 100% Continuous Outdoor Snowfall Hook & Tree Branch Cascades:**
  Keeps rich snowfall (*Snowflakes*, *Heavy Snow*, *Blizzard*) active outdoors in **Project: INVERNO** without dry clear-weather gaps, while sprinting, landings, gunshots, and explosions kick up snow dust and shake snow cascades from tree canopies.
- **🛡️ Zero Mod Conflicts:**
  Leaves `shader_param_5..8` completely untouched so **3DSS Scopes**, **Boomsticks & Sharpsticks Lasers**, and **Beef's NVGs** work side-by-side without interference.

---

## 📦 Installation (Mod Organizer 2 / G.A.M.M.A.)

1. Download this repository as a `.zip` (**Code → Download ZIP**).
2. In **Mod Organizer 2**, click **Install a new mod from an archive** and select the downloaded `.zip`.
3. Place **`DynSnowGamma`** at the **very bottom** of your MO2 mod list (below *Screen Space Shaders 23* and *Project: INVERNO*).
4. Delete `Anomaly\appdata\shaders_cache` once before launching so the DX11 shaders compile cleanly.
5. Configure all features live in-game via **Options → MCM → `!DynSnow`**.

---

## 🇩🇪 Kurzbeschreibung (Deutsch)

**DynSnowGamma (v1.2)** bringt echte volumetrische 3D-Schneeverformung nach Vorbild von *Garry's Mod dynSnow* nach **S.T.A.L.K.E.R. G.A.M.M.A.**:
- **5+ Minuten persistente 3D-Schneegräben für Spieler, Stalker & Mutanten** (reines DX11-Parallax-Raymarching ohne flache 2D-Fußspuren; Gräben schneien langsam und fließend von Anfang bis Ende wieder zu)
- **Dezenter, kristalliner Raureif & feiner Schneestaub auf Waffen** im Stil von *Project: INVERNO* (ohne weiße Hände/Ärmel; schüttelt sich beim Schießen leicht ab und taut drinnen/am Feuer weg)
- **Komplett stufenloses, flüssiges Lagerfeuer-Schmelzen & Wieder-Zuschneien** per Echtzeit-Float-Interpolation
- **100 % Dauer-Schneefall** im Freien + **Baum-Schneelawinen** bei Beschuss/Explosionen
- Einstellbar im Spiel über **Optionen → MCM → `!DynSnow`**.

---

## 📜 License

Released under the [MIT License](LICENSE).
