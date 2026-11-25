# ❄️ DynSnow for S.T.A.L.K.E.R. G.A.M.M.A. (`DynSnowGamma` v1.1)

An interactive, ultra-lightweight **3D dynamic snow deformation, weapon snow accumulation, and environmental interaction** addon for **S.T.A.L.K.E.R. G.A.M.M.A.** (Anomaly 1.5.3 DX11 + Screen Space Shaders 23.5), inspired by **dynSnow** from Garry's Mod.

Designed with a tiny **`0.32 MB` VRAM footprint** to run at 60 FPS even on handheld APUs while bringing winter overhauls like **Project: INVERNO** to life.

---

## ✨ Features (v1.1)

- **🏔️ 3D Parallax Snow Trench Deformation (`dynsnow_common.h`):**
  Passes a live world-space breadcrumb polyline (`shader_param_2` & `shader_param_5`) into the DX11 terrain raymarcher to carve real 3D sunken snow furrows with raised side berms, 3D slope normals, and deep arctic-blue self-shadowing that gradually snow back over when standing still.
- **🔫 Dynamic 3D Snow & Frost on Weapons & Hands:**
  Upward-facing 3D surfaces (`RDrops.w`) of your weapon barrel, receiver, optics, and gloves accumulate model-locked snow crystals and rime frost outdoors in falling snow, shake off slightly from recoil/barrel heat when firing, and melt into glistening droplets indoors or next to a burning campfire!
- **🔥 Campfire Snow Melting & Gradual Re-Snowing:**
  Lit campfires slowly melt surrounding snow outward (~2.2m radius) into dark wet thawed earth and slush (suppressing snow footprints inside the warm mud zone). Once extinguished, fresh snowfall gradually buries the firepit again from the outer rim inward.
- **🥾 Zero-Border Directional Bootprints & Mutant Paw Trails:**
  Player, nearby stalkers (within 35m), and mutants leave directional 2x-multiplicative hardware-neutral DXT5 tracks in snow — species-scaled from small *Tushkano* & *Blind Dog* paws up to massive *Chimera* & *Pseudogiant* prints.
- **🌨️ 100% Continuous Outdoor Snowfall Hook:**
  Non-destructive runtime hook for **Project: INVERNO** keeps rich snowfall (*Snowflakes*, *Heavy Snow*, *Blizzard*) active outdoors without dry clear-weather gaps, while automatically pausing indoors and underground.
- **🌲 Environmental Interactions (Craters & Tree Snow Cascades):**
  Jump landings, ground shots, and explosions churn craters & snow dust; hitting snow-laden tree canopies shakes falling snow cascades loose from branches.

---

## 📦 Installation (Mod Organizer 2 / G.A.M.M.A.)

1. Download this repository as a `.zip` (**Code → Download ZIP**).
2. In **Mod Organizer 2**, click **Install a new mod from an archive** and select the downloaded `.zip`.
3. Place **`DynSnowGamma`** at the **very bottom** of your MO2 mod list (below *Screen Space Shaders 23* and *Project: INVERNO*).
4. *(Recommended)* In **MCM → Screen Space Shaders → Terrain**, set **Distance** and **POM Range** to `22` and **POM Height** to `0.07` for full 3D snow depth.
5. Delete `Anomaly\appdata\shaders_cache` once before launching so the DX11 shaders compile cleanly.
6. Configure all features live in-game via **Options → MCM → `!DynSnow`**.

---

## 🇩🇪 Kurzbeschreibung (Deutsch)

**DynSnowGamma (v1.1)** bringt interaktiven 3D-Schnee nach Vorbild von *Garry's Mod dynSnow* nach **S.T.A.L.K.E.R. G.A.M.M.A.**:
- **Echte 3D-Schneespuren & Verformung** per DX11-Parallax-Raymarching inkl. aufgeworfener Schneeränder
- **Dynamischer Schnee & Frost auf Waffen & Handschuhen** (sammelt sich draußen im Schneefall, schüttelt sich beim Schießen leicht ab und schmilzt drinnen oder am Lagerfeuer weg)
- **Lagerfeuer-Schneeschmelze & Wieder-Zuschneien** rund um aktive/erloschene Feuerstellen
- **Richtungsgetreue Stiefel- & Mutanten-Pfotenabdrücke** im Umkreis von 35 m ohne Kastenränder
- **100 % Dauer-Schneefall** im Freien + **Schneekrater & Baum-Schneelawinen** bei Beschuss/Explosionen
- Komplett im Spiel einstellbar über **Optionen → MCM → `!DynSnow`**.

---

## 📜 License

Code, configuration, and procedural texture generator (`tools/gen_dynsnow_dds.py`) are released under the [MIT License](LICENSE). HUD shader integration builds on open-source X-Ray Monolith / SSS 23.5 hooks.
