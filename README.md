# ❄️ DynSnow for S.T.A.L.K.E.R. G.A.M.M.A. (`DynSnowGamma`)

An interactive, ultra-lightweight dynamic snow deformation & environmental interaction addon for **S.T.A.L.K.E.R. G.A.M.M.A.** (and Anomaly 1.5.3 DX11 + Screen Space Shaders), inspired by **dynSnow** from Garry's Mod.

Built from scratch as a **100% clean-room, non-destructive addon** (`0.32 MB` total VRAM footprint!) designed to run at locked 60 FPS even on handheld APUs while bringing winter overhauls like **Project: INVERNO** to life.

---

## ✨ Features

- **🥾 3D-Shaded Directional Bootprints (Player & Stalkers):**
  Leaves left and right lugged combat boot impressions in snow aligned to movement heading, including doorway snow-tracking (carries snow 3–4 steps indoors before boots dry off).
- **🐾 Species-Scaled Mutant Tracks:**
  Nearby mutants (within 35m) leave scaled paw & claw trails in the snow — from small *Tushkano* & *Blind Dog* prints up to massive *Chimera* & *Pseudogiant* tracks so you can hunt or track threats across the Zone.
- **🔥 Campfire Snow Melting & Gradual Re-Snowing:**
  Lit campfires slowly melt surrounding snow outward into dark thawed earth and wet slush (suppressing snow footprints inside the warm mud ring). Once a campfire goes out, falling snow gradually buries the firepit again from the outer rim inward!
- **🌨️ Dynamic Snow Fill-In & 100% Continuous Snowfall Option:**
  Footprints and craters naturally fill back in faster during heavy snow/blizzards (~45s) and linger longer during light snow (~140s). Includes an optional runtime hook for **Project: INVERNO** to keep outdoor snowfall active 100% of the time without dry clear-weather gaps.
- **🌲 Environmental Interactions (Craters & Tree Snow Shake):**
  Jumping/sprinting kicks up snow dust, bullets & explosions churn craters into the snow, and shooting or blasting near tree canopies shakes falling snow cascades loose from branches.
- **🌙 Day/Night-Aware Snow Decal Shader:**
  Custom hybrid DX11 wallmark pixel shader (`effects_wallmark_blood.ps`) keeps Screen Space Shaders blood decals 100% intact while shading snow tracks and campfire earth dynamically with live sky hemisphere, sun, and hearth bounce lighting (never glows in the dark at night!).

---

## 📦 Installation (Mod Organizer 2 / G.A.M.M.A.)

1. Download this repository as a `.zip` (`Code -> Download ZIP`) or grab the latest release archive.
2. In **Mod Organizer 2**, click **Install a new mod from an archive** and select the downloaded `.zip`.
3. Place **`DynSnowGamma`** at the **very bottom** of your MO2 mod list (below *Screen Space Shaders* and below *Project: INVERNO* or your winter texture overhaul).
4. Delete `Anomaly\appdata\shaders_cache` once before launching so the hybrid wallmark shader compiles cleanly.
5. Configure everything live in-game via **Options → MCM → `!DynSnow`**.

---

## 🇩🇪 Kurzbeschreibung (Deutsch)

**DynSnowGamma** bringt interaktiven Schnee nach Vorbild von *Garry's Mod dynSnow* nach **S.T.A.L.K.E.R. G.A.M.M.A.**:
- **3D-Schnee-Fußspuren & Mutanten-Pfotenabdrücke** (inkl. Verfolgung von Stalkern & Mutanten im Schnee)
- **Dynamische Lagerfeuer-Schneeschmelze:** Brennende Lagerfeuer schmelzen den Schnee rundherum langsam zu dunkler, feuchter Erde weg; erlischt das Feuer, schneit die Stelle nach und nach wieder zu
- **100 % Dauer-Schneefall-Option** (nahtloses Wechseln zwischen leichtem Schneerieseln, dicken Flocken, starkem Schneefall & Blizzard)
- **Umwelt-Interaktion:** Explosions- & Einschlagskrater im Schnee sowie herabfallende Schneelawinen von getroffenen Baumkronen
- **Extrem leichtgewichtig:** Nur ~320 KB VRAM-Bedarf, komplett über **MCM (`!DynSnow`)** einstellbar.

---

## 📜 License

Released under the [MIT License](LICENSE). All scripts, shaders, and procedurally generated DXT5 textures (`tools/gen_dynsnow_dds.py`) in this repository are original works and redistribute zero third-party assets.
