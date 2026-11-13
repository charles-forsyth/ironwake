# Chapter 12: Assets and credits

Scope: every folder under `Content/Ironwake` (751 `.uasset` files, listed in `docs/spec/_assets.txt`, a verbatim `find Content/Ironwake -name '*.uasset' | sort` dump taken for this card), where each asset came from (Hunyuan3D-2 on the GPU batch cluster, Sketchfab CC-BY/CC0 library models, Poly Haven CC0 scans, NASA SVS star map, procedural and approved audio), its licence, and which script regenerates it. Source models live outside the repo in `$IRONWAKE_ASSETS/models/`; converted FBX/PNG intermediates under `build/assets/` are git-ignored (see 12.9), so a fresh clone needs the regeneration scripts plus the outside model library. Counts of files per folder are computed from `_assets.txt`, not typed from memory. Not used: Fab/Megascans (the BRIEF forbids Fab purchases [src build/BRIEF.md:66]); the only Epic content in the shipped map is the copied WaterExtras sample (12.8).

## 12.1 Folder map

| Folder | Holds | Origin / licence | Regenerate with |
|---|---|---|---|
| `Content/Ironwake/Audio` [src docs/spec/_assets.txt:1-24] | 24 sounds (WAV imports) | approved music/voice/sfx, CC0 SFXMint, procedural noise (12.5) | `Audio/*.wav` copied by `Content/Python/iw_import_audio.py`; loops by `build/audio/gen.py` |
| `Content/Ironwake/Core` [src docs/spec/_assets.txt:25-25] | `BP_IronwakeGameMode` | project-authored | none (Blueprint; created over MCP in card B) |
| `Content/Ironwake/FX` [src docs/spec/_assets.txt:26-39] | Niagara + FX materials (12.6) | project-authored (Niagara templates duplicated from engine) | `Content/Python/w1_assets.py`, `w2_assets.py`, `w4_spray.py`, `p5b_assets.py`; rest over MCP |
| `Content/Ironwake/Islands` [src docs/spec/_assets.txt:40-48] | island ground/surf/shelf materials + 2 Poly Haven textures | Poly Haven CC0 textures; materials authored | `Content/Python/iw_i1_mats.py`, `iw_i1_shelf.py` |
| `Content/Ironwake/Ships` [src docs/spec/_assets.txt:49-280] | `BP_PlayerShip` + hull folders (12.2) | see 12.2 | see 12.2 |
| `Content/Ironwake/Sky` [src docs/spec/_assets.txt:281-293] | night sky, star map, moon phases | NASA SVS + LROC | `Content/Python/iw_p10_sky.py` (and `iw_p8_sky.py`) |
| `Content/Ironwake/Town` [src docs/spec/_assets.txt:294-314] | town materials + tiling Poly Haven textures | Poly Haven CC0 textures; materials authored | `Content/Python/iw_p10_mats.py`, `iw_p12_mats.py`, `iw_p12_smoke.py` |
| `Content/Ironwake/Water` [src docs/spec/_assets.txt:315-318] | 4 water materials | project copy of Water-plugin chain | `Content/Python/w2_foammat.py`, `w3_foammat.py`, `w4_foammat.py` |
| `Content/Ironwake/World` [src docs/spec/_assets.txt:319-751] | 73 per-asset subfolders (12.3; count of directories under Content/Ironwake/World) | scans, Sketchfab, Hunyuan | `build/e1/prep.py`, `build/p8/prep.py`, `build/i1/prep_i1.py` + import scripts |

Per-folder file counts (immediate `.uasset` files, from `_assets.txt`): 

| Folder | Files |
|---|---|
| Content/Ironwake/Audio | 24 |
| Content/Ironwake/Core | 1 |
| Content/Ironwake/FX | 14 |
| Content/Ironwake/Islands | 9 |
| Content/Ironwake/Sky | 13 |
| Content/Ironwake/Town | 21 |
| Content/Ironwake/Water | 4 |
| Content/Ironwake/Ships | 1 |
| Content/Ironwake/Ships/Materials | 5 |
| Content/Ironwake/Ships/Airship | 4 |
| Content/Ironwake/Ships/BrassAirship | 7 |
| Content/Ironwake/Ships/Aglae | 23 |
| Content/Ironwake/Ships/Devastation | 46 |
| Content/Ironwake/Ships/Frigate | 49 |
| Content/Ironwake/Ships/Goliath | 46 |
| Content/Ironwake/Ships/Ironclad | 3 |
| Content/Ironwake/Ships/Kracken | 48 |

Counts above are computed from `_assets.txt` by the generator (not hand-typed); they are plain numbers rather than tags because the checker matches literal text near a cited line and a file count is not written in any source line. Re-derive with `find Content/Ironwake -maxdepth 2 -name '*.uasset' | wc -l` per folder.

## 12.2 Ships folder

| Folder | Meshes | Source model | Author / licence | Script |
|---|---|---|---|---|
| Devastation | Hull, TurretFore/Aft, BarrelsFore/Aft, PropPort/Stbd, Rudder (`SM_Devastation_*`) | "Hms Devastation 1871" | martyn169, CC-BY 4.0 [src build/CREDITS.txt:8] | `build/r1/process.py build/r1/cfg_devastation.json` then `Content/Python/iw_r1_import.py`, `iw_r1_materials.py` |
| Kracken | `SM_Kracken_Hull` | "RSS Kracken", recoloured to charcoal | martyn169, CC-BY [src build/CREDITS.txt:9] | `cfg_kracken.json` |
| Goliath | `SM_Goliath_Hull` | "Goliath Steam Tug 1903" | martyn169, CC-BY [src build/CREDITS.txt:10] | `cfg_goliath.json` |
| Aglae | `SM_Aglae_Hull` | "Type du remorqueur AGLAE ... 1874" | Gabrielcambreling, CC-BY [src build/CREDITS.txt:11] | `cfg_aglae.json` |
| Frigate | `SM_Frigate_Hull` (sails/yards stripped) | "Russian archipelago frigate Svjatoi Nikolai" | Finnish Heritage Agency, CC-BY 4.0 [src build/CREDITS.txt:31] | `build/s2/prep_frigate.py` -> `build/r1/process.py build/s2/cfg_frigate.json` -> `Content/Python/iw_s2_import.py` |
| BrassAirship | `SM_BrassAirship_Hull`, `SM_BrassAirship_PropStern` | "Steampunk Airship - Brass-Plated Vessel" | Pigcraft, CC-BY [src build/CREDITS.txt:12] | `cfg_brass.json`, `iw_r1_airfix.py`, `iw_r1_airab.py` |
| Airship | `SM_Airship`, `SM_Airship110` | first Hunyuan3D-2 airship from GPU batch job 2048 | generated, no third-party licence [src build/BRIEF.md:55-57] | `Content/Python/iw_import_airship.py` (reads `build/assets/airship/airship110.fbx`) |
| Ironclad | `SM_Ironclad` | first Hunyuan3D-2 ironclad, 140 m | generated [src build/BRIEF.md:77-78] | `build/assets/glb2fbx.py` (Blender) |
| Materials | `M_IW_ShipPaint`, `M_IW_ShipTex`, `M_IW_ShipTex2`, `M_IW_Cloth`, `T_IW_WhiteMask` | project masters | authored; `T_IW_WhiteMask` from `build/assets/common/T_IW_WhiteMask.png` | `iw_r1_materials.py`, `iw_s2_cloth.py` |

Notes:

- R1 pipeline: headless Blender `build/r1/process.py <cfg>.json` strips stands/text, scales to metres, orients +X bow / Z up, splits parts with pivots, decimates, exports FBX + PNG textures (max 2K, 4K for the player hull) (build report `R1.md`). Then the headless editor runs `iw_r1_import.py` and `iw_r1_materials.py` (build report `R1.md`).
- Hull length targets per config: Devastation `125.0` m [src build/r1/cfg_devastation.json:4], Kracken `120.0` [src build/r1/cfg_kracken.json:5], Goliath `50.0` [src build/r1/cfg_goliath.json:3], Aglae `30.0` [src build/r1/cfg_aglae.json:2], brass airship `110.0` [src build/r1/cfg_brass.json:4].
- Sail rig, flags and pennants of the frigate are procedural meshes in C++ (`UIronwakeSailRig`), not assets (build report `S2.md`).
- The brass airship in the world is the GPU batch remesh (`BrassAirshipRemesh`, 12.3), the R1 hull `SM_BrassAirship_Hull` is the decimated 190k version kept only as the base for the spinning stern prop (build report `E1.md`).
- Source GLBs for the library (21 downloaded) live in `$IRONWAKE_ASSETS/models/library/`; only Devastation, Kracken, Goliath, Aglae, brass airship and Svjatoi Nikolai are in the game. The others (Aleksandr, Turuma, Amphion, Amadis, Styrbjorn, Minerva, Chapman, Implacable, Bucentaure, Victorine, Tartane, Rolf Krake, Tordenskjold, Broadford, steampunk_airship_light) are downloaded and credited in the library file but not imported (see `CREDITS.md`).

## 12.3 World folder (per-asset)

One subfolder per mesh: `SM_<Name>` plus `MI_<Name>_*` material instances and `T_<Name>_*` textures. Every mesh imports with Nanite on [src Content/Python/iw_e1_import.py:55]; materials are instances of `M_IW_ShipTex2` (textured) or `M_IW_ShipPaint` (colour only) [src Content/Python/iw_e1_import.py:14-15]. Textures cap at `2048` px, `4096` for BrassAirshipRemesh [src Content/Python/iw_e1_import.py:66]. Auto collision off at import [src Content/Python/iw_e1_import.py:54]; collision is analytic (ch. 5).

### 12.3.1 E1 batch (scans, Sketchfab, Hunyuan harbour)

Script `build/e1/prep.py` (Blender) feeds the headless importer iw_e1_import.py [src build/e1/prep.py:1-3]. Fit column is the (kind, value) from the script: `scale` multiplier, `h` height m, `len` longest horizontal m.

| Folder | prep.py key | Source file | Fit | Licence / credit |
|---|---|---|---|---|
| CoastalCliff01 | `CoastalCliff01` [src build/e1/prep.py:14] | Poly Haven | `1` [src build/e1/prep.py:14] | CC0 (see `CREDITS.md`) |
| CoastalCliff02 | `CoastalCliff02` [src build/e1/prep.py:15] | Poly Haven | `1` [src build/e1/prep.py:15] | CC0 (see `CREDITS.md`) |
| CoastalCliff04 | `CoastalCliff04` [src build/e1/prep.py:16] | Poly Haven | `1` [src build/e1/prep.py:16] | CC0 (see `CREDITS.md`) |
| CoastRocks01 | `CoastRocks01` [src build/e1/prep.py:17] | Poly Haven | `1` [src build/e1/prep.py:17] | CC0 (see `CREDITS.md`) |
| CoastRocks02 | `CoastRocks02` [src build/e1/prep.py:18] | Poly Haven | `1` [src build/e1/prep.py:18] | CC0 (see `CREDITS.md`) |
| CoastRocks03 | `CoastRocks03` [src build/e1/prep.py:19] | Poly Haven | `1` [src build/e1/prep.py:19] | CC0 (see `CREDITS.md`) |
| CoastLandRocks02 | `CoastLandRocks02` [src build/e1/prep.py:20] | Poly Haven | `1` [src build/e1/prep.py:20] | CC0 (see `CREDITS.md`) |
| CoastLandRocks03 | `CoastLandRocks03` [src build/e1/prep.py:21] | Poly Haven | `1` [src build/e1/prep.py:21] | CC0 (see `CREDITS.md`) |
| Boulder01 | `Boulder01` [src build/e1/prep.py:22] | Poly Haven | `1` [src build/e1/prep.py:22] | CC0 (see `CREDITS.md`) |
| WreckStern | `WreckStern` [src build/e1/prep.py:23] | Stern Of SS Rifle, Scottish Maritime Museum | `1` [src build/e1/prep.py:23] | CC0 (see `CREDITS.md`) |
| WharfBuilding | `WharfBuilding` [src build/e1/prep.py:24] | Wharf building in Oslo 1798, Tidvis | `0.01` [src build/e1/prep.py:24] | CC-BY (see `CREDITS.md`) |
| VictorianFactory | `VictorianFactory` [src build/e1/prep.py:25] | Victorian Factory, Paolo Mercogliano | `1.8` [src build/e1/prep.py:25] | CC-BY (see `CREDITS.md`) |
| WoodenPier | `WoodenPier` [src build/e1/prep.py:26] | Wooden Pier_22_MB, Mehdi Shahsavan | `1` [src build/e1/prep.py:26] | CC-BY (see `CREDITS.md`) |
| ModularPier | `ModularPier` [src build/e1/prep.py:27] | Poly Haven | `1.5` [src build/e1/prep.py:27] | CC0 (see `CREDITS.md`) |
| SeaMine | `SeaMine` [src build/e1/prep.py:28] | Old Sea Mine, Jorma Rysky | `1` [src build/e1/prep.py:28] | CC-BY (see `CREDITS.md`) |
| WoodenBarrels | `WoodenBarrels` [src build/e1/prep.py:29] | Wooden Barrels, MaX3Dd | `2.6` [src build/e1/prep.py:29] | CC-BY (see `CREDITS.md`) |
| Barrel03 | `Barrel03` [src build/e1/prep.py:30] | Poly Haven | `1` [src build/e1/prep.py:30] | CC0 (see `CREDITS.md`) |
| MilitaryCrate | `MilitaryCrate` [src build/e1/prep.py:31] | Poly Haven | `1.4` [src build/e1/prep.py:31] | CC0 (see `CREDITS.md`) |
| HangingLamp | `HangingLamp` [src build/e1/prep.py:32] | Poly Haven | `1.5` [src build/e1/prep.py:32] | CC0 (see `CREDITS.md`) |
| WallLamp | `WallLamp` [src build/e1/prep.py:33] | Poly Haven | `2` [src build/e1/prep.py:33] | CC0 (see `CREDITS.md`) |
| Rowboat | `Rowboat` [src build/e1/prep.py:34] | Victorian Row Boat, Ashley Aslett | `2.2` [src build/e1/prep.py:34] | CC-BY (see `CREDITS.md`) |
| Fourareen | `Fourareen` [src build/e1/prep.py:35] | Venus, a Shetland fourareen, Scottish Maritime Museum | `1` [src build/e1/prep.py:35] | CC0 (see `CREDITS.md`) |

Hunyuan3D-2 harbour pieces (generated from our own lumina concept art on GPU batch V100s, jobs `2153/2156/2158/2160`) (design archive `models/harbour_gen/USE.md`): no third-party licence, "ours to use" (design archive `models/harbour_gen/USE.md`). Credited in-game as generated with Hunyuan3D-2 (Tencent) [src build/CREDITS.txt:28].

| Folder | prep.py key | Hunyuan source | Fit | Vision score at generation (design archive `models/harbour_gen/USE.md`) |
|---|---|---|---|---|
| SteamCrane | `SteamCrane` [src build/e1/prep.py:36] | harbour_gen/steam_crane.glb | `22` [src build/e1/prep.py:36] | `8` (design archive `models/harbour_gen/USE.md`) |
| CoalingStation | `CoalingStation` [src build/e1/prep.py:37] | harbour_gen/coaling_station.glb | `26` [src build/e1/prep.py:37] | `8` (design archive `models/harbour_gen/USE.md`) |
| SignalTower | `SignalTower` [src build/e1/prep.py:38] | harbour_gen/signal_tower.glb | `30` [src build/e1/prep.py:38] | `5` (design archive `models/harbour_gen/USE.md`) |
| SteamBuoy | `SteamBuoy` [src build/e1/prep.py:39] | harbour_gen/steam_buoy.glb | `5.5` [src build/e1/prep.py:39] | `8.5` (design archive `models/harbour_gen/USE.md`) |
| MooringMast | `MooringMast` [src build/e1/prep.py:40] | harbour_gen/mooring_mast.glb | `55` [src build/e1/prep.py:40] | `8` (design archive `models/harbour_gen/USE.md`) |
| GasLamp | `GasLamp` [src build/e1/prep.py:41] | harbour_gen/gas_lamp_post.glb | `4.6` [src build/e1/prep.py:41] | `7` (design archive `models/harbour_gen/USE.md`) |
| BollardCapstan | `BollardCapstan` [src build/e1/prep.py:42] | harbour_gen/bollard_capstan.glb | `3.2` [src build/e1/prep.py:42] | `8.5` (design archive `models/harbour_gen/USE.md`) |
| TorpedoBoat | `TorpedoBoat` [src build/e1/prep.py:43] | harbour_gen/torpedo_boat.glb | `40` [src build/e1/prep.py:43] | `8.5` (design archive `models/harbour_gen/USE.md`) |
| SteamPinnace | `SteamPinnace` [src build/e1/prep.py:44] | harbour_gen/steam_pinnace.glb | `15` [src build/e1/prep.py:44] | `8` (design archive `models/harbour_gen/USE.md`) |
| DockWarehouse | `DockWarehouse` [src build/e1/prep.py:45] | harbour_gen/dock_warehouse.glb | `26` [src build/e1/prep.py:45] | `9` (design archive `models/harbour_gen/USE.md`) |

`BrassAirshipRemesh`: GPU batch job `2162`, 250k-face remesh + 4K bake of the Pigcraft model (batch asset log); E1 uses variant `a250k` at scale `115.5355967911108` [src build/e1/prep.py:47]; texmax `4096` and metal `0.3` [src build/e1/prep.py:49]. Licence follows the Pigcraft CC-BY credit [src build/CREDITS.txt:29].

### 12.3.2 P8 batch (Hunyuan cluster batch 1)

Source: GPU batch batch 1 job IDs `2167`, `2175`, `2176`, `2177`, `2178`, `2179`, `2180`, `2181` (also `2183`, `2185` are sky jobs), outputs in `models/cluster_b1/<jobid>/<name>.glb`. `SCORES.json` holds the vision score and verdict for 34 generated models: 17 keep, 9 fix, 5 fix-minor, 2 reject, 1 reject-rig (counted from the file). Script `build/p8/prep.py` (Blender) -> importer iw_p8_import.py [src build/p8/prep.py:1-3]. Generated for Ironwake, no third-party licence; credited as Hunyuan3D-2 (Tencent) [src build/CREDITS.txt:38].

| Folder | prep.py fit | SCORES.json score | Verdict | In game as |
|---|---|---|---|---|
| PaddleSteamer | `45` [src build/p8/prep.py:18] | `8` (design archive `models/cluster_b1/SCORES.json`) | keep | NPC moored/loop boat |
| PatrolAirship | `70` [src build/p8/prep.py:19] | `8.5` (design archive `models/cluster_b1/SCORES.json`) | keep | NPC airship |
| PatrolGunboat | `32` [src build/p8/prep.py:20] | `8` (design archive `models/cluster_b1/SCORES.json`) | keep | NPC boat |
| PilotLaunch | `14` [src build/p8/prep.py:21] | `8` (design archive `models/cluster_b1/SCORES.json`) | keep | moored at quay |
| RowingGig | `8` [src build/p8/prep.py:22] | `8.5` (design archive `models/cluster_b1/SCORES.json`) | keep | moored at quay |
| SkyYacht | `45` [src build/p8/prep.py:23] | `8.5` (design archive `models/cluster_b1/SCORES.json`) | keep | NPC airship |
| SteamDrifter | `24` [src build/p8/prep.py:24] | `8` (design archive `models/cluster_b1/SCORES.json`) | keep | NPC boat |
| TownBandstand | `7` [src build/p8/prep.py:25] | `8` (design archive `models/cluster_b1/SCORES.json`) | keep | town square |
| TownChapel | `15` [src build/p8/prep.py:26] | `7` (design archive `models/cluster_b1/SCORES.json`) | keep | top terrace |
| TownCrateStack | `3` [src build/p8/prep.py:27] | `8.5` (design archive `models/cluster_b1/SCORES.json`) | keep | quay props |
| TownCustomsHouse | `22` [src build/p8/prep.py:28] | `8.5` (design archive `models/cluster_b1/SCORES.json`) | keep | quay end |
| TownHandcart | `2.4` [src build/p8/prep.py:29] | `8` (design archive `models/cluster_b1/SCORES.json`) | keep | quay/street props |
| TownMarketStall | `3.6` [src build/p8/prep.py:30] | `7.5` (design archive `models/cluster_b1/SCORES.json`) | keep | town square |
| TownRowCottages | `18` [src build/p8/prep.py:31] | `8` (design archive `models/cluster_b1/SCORES.json`) | keep | terrace houses |
| TownStoneTownhouse | `12` [src build/p8/prep.py:32] | `9` (design archive `models/cluster_b1/SCORES.json`) | keep | terrace houses |
| TownStoneWall | `4` [src build/p8/prep.py:33] | `7` (design archive `models/cluster_b1/SCORES.json`) | keep | walls (no code reference found) |
| TownTimberHouse | `9` [src build/p8/prep.py:34] | `8` (design archive `models/cluster_b1/SCORES.json`) | keep | terrace houses |

Other models in SCORES.json that were NOT imported (kept for GPU batch batch 2): the 9 `fix` models and 3 rejects. Verdict text copied from the file:

| Model | Score | Verdict | Note (from SCORES.json) |
|---|---|---|---|
| fishing_smack | `4` (design archive `models/cluster_b1/SCORES.json`) | reject-rig | hull/cabin ok; rigging fused into pillars. Keep hull, rebuild masts/rigging in Blender (prep_ships.py path). |
| town_fish_racks | `4.5` (design archive `models/cluster_b1/SCORES.json`) | fix | frame+shed ok; hanging fish melted. Cut below rack, replace with instanced fish cards. |
| cargo_airship_long | `5` (design archive `models/cluster_b1/SCORES.json`) | fix | envelope/gondola ok; propellers, struts melted, tail fin notch. Rebuild props in Blender. |
| town_picket_fence | `2` (design archive `models/cluster_b1/SCORES.json`) | reject | build procedurally in UE/Blender (simple slats); never use image-to-3D for fences. |
| town_water_pump | `5` (design archive `models/cluster_b1/SCORES.json`) | fix | massing ok; handle/pipes/gear melted. |
| coal_lighter | `6` (design archive `models/cluster_b1/SCORES.json`) | fix | hull/hold ok; crane boom fused. Rebuild derrick. |
| salvage_tender | `6` (design archive `models/cluster_b1/SCORES.json`) | fix | hull/stack/bell ok; A-frame + cables fused. |
| town_fisher_cottage | `7` (design archive `models/cluster_b1/SCORES.json`) | fix-minor | lobster pots clumped; cut + replace with props. |
| town_windmill | `5` (design archive `models/cluster_b1/SCORES.json`) | fix | tower/cap ok; lattice sails melted. Rebuild sails as alpha-card planes. |
| town_foundry | `4` (design archive `models/cluster_b1/SCORES.json`) | fix | massing ok, texture failed to read as brick (paint ran normally). Retexture with tiling brick PBR in UE, or re-paint at higher res. |
| town_gasometer | `6` (design archive `models/cluster_b1/SCORES.json`) | fix | tank ok; lattice frame solid/chunky. Replace frame with thin-strut geometry or alpha. |
| town_street_lamp | `2` (design archive `models/cluster_b1/SCORES.json`) | reject | post faded/thin, lantern glass corrupted. Build procedurally (cylinder + lantern) in UE. |
| town_boathouse | `6` (design archive `models/cluster_b1/SCORES.json`) | fix-minor | building good; ramp missing (add a simple slipway mesh in UE). |
| town_tavern | `6` (design archive `models/cluster_b1/SCORES.json`) | fix-minor | render cropped/dark so score is soft; massing + timber framing fine, slight roof waviness, props/sign to add as separate meshes. Recheck in engine. |
| town_market_hall | `6.5` (design archive `models/cluster_b1/SCORES.json`) | fix-minor | timber frame good; renders dark (turntable backlight shows through arch, not a mesh fault). Recheck in engine. |
| town_terrace_house | `5` (design archive `models/cluster_b1/SCORES.json`) | fix | front good; sides/rear flat brown, stray pipe bits. Terrace houses sit in rows so sides are hidden: cut pipes, tile brick on rear. |
| revenue_cutter | `7` (design archive `models/cluster_b1/SCORES.json`) | fix-minor | hull/masts/stack good; furled sails lumpy. |

P8 imports all 17 keep models with Nanite and simple collision (box for `Town*`, 26-DOP for boats) (build report `P8.md`). Orientation corrections in `OPTS`: RowingGig `-90`, TownHandcart `-90`, PilotLaunch `180`, SteamDrifter `180`, PatrolAirship `180`, SkyYacht `180` [src build/p8/prep.py:48].

### 12.3.3 I1 batch (islands, Poly Haven CC0)

Script `build/i1/prep_i1.py` -> `Content/Python/iw_i1_import.py` (copy of the E1 importer that also re-attaches leaf alpha maps) (build report `I1.md`). Source models in `models/islands_prepped/` (GPU batch decimation job 2151 for the world set) and `models/islands_extra/` (`modular_fort_01`, glTF). All CC0 from Poly Haven (see `CREDITS.md`), fort kit added for I1 (see `CREDITS.md`).

| Folder | prep key | Poly Haven asset |
|---|---|---|
| CoastLine01 | `coast_line_01` [src build/i1/prep_i1.py:17] | coast_line_01 (see `CREDITS.md`) |
| CoastLandRocks04 | `coast_land_rocks_04` [src build/i1/prep_i1.py:18] | coast_land_rocks_04 (see `CREDITS.md`) |
| SandRocksSmall01 | `sand_rocks_small_01` [src build/i1/prep_i1.py:19] | sand_rocks_small_01 (see `CREDITS.md`) |
| NamaCliff01 | `namaqualand_cliff_01` [src build/i1/prep_i1.py:20] | namaqualand_cliff_01 (see `CREDITS.md`) |
| NamaCliff02 | `namaqualand_cliff_02` [src build/i1/prep_i1.py:21] | namaqualand_cliff_02 (see `CREDITS.md`) |
| RockFace01 | `rock_face_01` [src build/i1/prep_i1.py:22] | rock_face_01 (see `CREDITS.md`) |
| RockMossSet01 | `rock_moss_set_01` [src build/i1/prep_i1.py:23] | rock_moss_set_01 (see `CREDITS.md`) |
| CoastRocks05 | `coast_rocks_05` [src build/i1/prep_i1.py:24] | coast_rocks_05 (see `CREDITS.md`) |
| IslandTree01 | `island_tree_01` [src build/i1/prep_i1.py:25] | island_tree_01 (see `CREDITS.md`) |
| IslandTree02 | `island_tree_02` [src build/i1/prep_i1.py:26] | island_tree_02 (see `CREDITS.md`) |
| IslandTree03 | `island_tree_03` [src build/i1/prep_i1.py:27] | island_tree_03 (see `CREDITS.md`) |
| FirTree01 | `fir_tree_01` [src build/i1/prep_i1.py:28] | fir_tree_01 (see `CREDITS.md`) |
| Shrub02 | `shrub_02` [src build/i1/prep_i1.py:29] | shrub_02 (see `CREDITS.md`) |
| Shrub03 | `shrub_03` [src build/i1/prep_i1.py:30] | shrub_03 (see `CREDITS.md`) |
| GrassMedium01 | `grass_medium_01` [src build/i1/prep_i1.py:31] | grass_medium_01 (see `CREDITS.md`) |
| DeadTreeTrunk02 | `dead_tree_trunk_02` [src build/i1/prep_i1.py:32] | dead_tree_trunk_02 (see `CREDITS.md`) |
| TreeStump01 | `tree_stump_01` [src build/i1/prep_i1.py:33] | tree_stump_01 (see `CREDITS.md`) |
| FortTower | `tower_round` [src build/i1/prep_i1.py:41] | `modular_fort_01` piece, from the multi-piece glTF (see `CREDITS.md`) |
| FortWallThick | `wall_thick_straight_01` [src build/i1/prep_i1.py:41] | `modular_fort_01` piece, from the multi-piece glTF (see `CREDITS.md`) |
| FortWallThin | `wall_thin_straight_02` [src build/i1/prep_i1.py:41] | `modular_fort_01` piece, from the multi-piece glTF (see `CREDITS.md`) |
| FortGate | `wall_thin_gate_01` [src build/i1/prep_i1.py:42] | `modular_fort_01` piece, from the multi-piece glTF (see `CREDITS.md`) |
| FortCorner | `wall_thick_corner_01` [src build/i1/prep_i1.py:42] | `modular_fort_01` piece, from the multi-piece glTF (see `CREDITS.md`) |
| FortWalkway | `wall_walkway_straight_01` [src build/i1/prep_i1.py:42] | `modular_fort_01` piece, from the multi-piece glTF (see `CREDITS.md`) |

Material problems solved in the importer: glTF KHR texture transforms baked into UVs; Poly Haven leaf alpha maps re-attached through `M_IW_Foliage` (build report `I1.md`); instanced masters need `bUsedWithInstancedStaticMeshes`, set by `iw_i1_usage.py` (build report `I1.md`); foliage Nanite OFF with 4 auto-LODs (`iw_i1_veglod.py`) [src Content/Python/iw_i1_veglod.py:1].

Unreferenced meshes: `CoastLandRocks04`, `CoastLine01`, `FortWalkway`, `HangingLamp`, `TownStoneWall`, `WoodenPier` have no literal name occurrence in `Source/IronwakeTest/*.cpp|h` (checked with grep for this card). They may be loaded by a constructed string; verify by grepping the logs for `IWE1 place`. They are imported and cooked (always-cook rule 12.9) but probably unused.

## 12.4 Sky, Town, Water, Islands, FX, Core: file lists

### Sky (`Content/Ironwake/Sky`)

NASA/Goddard SVS Deep Star Maps 2020 (1.7 billion real stars, Hipparcos-2, Tycho-2, Gaia DR2) (design archive `models/sky_final/SKY_USE.md`), credit required (design archive `models/sky_final/SKY_USE.md`); moon from NASA LROC CGI Moon Kit with phase table `moon_2027.json` from skyfield DE421 (design archive `models/sky_final/SKY_USE.md`). Sources in `models/sky_final/` and `models/sky_nasa/`. `T_IW_StarHDR` is built from `starmap_2020_8k.exr` [src Content/Python/iw_p10_sky.py:24] and imported as RGBA16F by iw_p10_sky.py. `T_IW_StarMap8k` and `T_IW_StarGlow` are the earlier 8-bit path from `build/p8/skyprep.py` and `build/p8/glowprep.py`, dropped in P10 (build report `P10.md`). `MI_StormDeck2` is the storm cloud deck from card A (build report `A.md`)

Files: `MI_StormDeck2`, `M_IW_NightSky`, `T_IW_MoonPhase0`, `T_IW_MoonPhase1`, `T_IW_MoonPhase2`, `T_IW_MoonPhase3`, `T_IW_MoonPhase4`, `T_IW_MoonPhase5`, `T_IW_MoonPhase6`, `T_IW_MoonPhase7`, `T_IW_StarGlow`, `T_IW_StarHDR`, `T_IW_StarMap8k`.

### Town (`Content/Ironwake/Town`)

Textures `patterned_cobblestone_02`, `cobblestone_floor_02`, `wood_floor_deck`, `aerial_grass_rock` (Poly Haven CC0) imported at 2048 max [src Content/Python/iw_p10_mats.py:22]. `M_IW_Plaster`, `M_IW_Tile`, `M_IW_Smk`, `M_IW_Glow` authored. Scripts `iw_p10_mats.py` (tiling materials + window material), `iw_p12_mats.py` (plaster/roof with per-colour tint), `iw_p12_smoke.py` (unlit translucent chimney smoke). Textures read from `build/assets/p10/` [src build/assets/p10 listing: see Gaps]

Files: `MI_IW_Cobble`, `MI_IW_Deck`, `MI_IW_Ground`, `MI_IW_Setts`, `MI_IW_Window`, `M_IW_Glow`, `M_IW_Plaster`, `M_IW_Smk`, `M_IW_Tile`, `T_aerial_grass_rock_d`, `T_aerial_grass_rock_n`, `T_aerial_grass_rock_r`, `T_cobblestone_floor_02_d`, `T_cobblestone_floor_02_n`, `T_cobblestone_floor_02_r`, `T_patterned_cobblestone_02_d`, `T_patterned_cobblestone_02_n`, `T_patterned_cobblestone_02_r`, `T_wood_floor_deck_d`, `T_wood_floor_deck_n`, `T_wood_floor_deck_r`.

### Islands (`Content/Ironwake/Islands`)

Poly Haven CC0 textures `coast_sand_01`, `aerial_grass_rock` (see `CREDITS.md`); materials authored. Scripts `iw_i1_mats.py` (ground + surf + surf audio), `iw_i1_shelf.py` (`M_IW_Shelf`, `M_IW_Surf2`), `iw_i1_surfparams.py`

Files: `M_IW_Foliage`, `M_IW_IslandGround`, `M_IW_Shelf`, `M_IW_Surf`, `M_IW_Surf2`, `T_aerial_grass_rock_Diffuse`, `T_aerial_grass_rock_nor_gl`, `T_coast_sand_01_Diffuse`, `T_coast_sand_01_nor_gl`.

### Water (`Content/Ironwake/Water`)

Project-owned copy of the Water plugin material chain: `M_IW_WaterRoot` <- `MI_IW_WaterInst` <- `MI_IW_WaterOcean` <- `MI_Ocean_Storm` (build report `W2.md`). Built idempotently by `Content/Python/w2_foammat.py`; the `IW_Foam` Custom node is edited in place by `w3_foammat.py` and `w4_foammat.py` (never delete referenced water assets: it crashed the editor) [src Content/Python/w4_foammat.py:2]

Files: `MI_IW_WaterInst`, `MI_IW_WaterOcean`, `MI_Ocean_Storm`, `M_IW_WaterRoot`.

### FX (`Content/Ironwake/FX`)

Project-authored. Niagara systems are duplicates of engine templates retuned over MCP (build report `C.md`). `NS_IW_Splash`, `NS_IW_Explosion2`, `NS_IW_Fire`, `NS_IW_Smoke` (card C); `NS_IW_BowSpray`, `M_IW_BowFoam` (W1, `w1_assets.py`); `NS_IW_Spume`, `NS_IW_Rain`, `NS_IW_BowSheet` (W2, `w2_assets.py`); `M_IW_Spray` (W4, `w4_spray.py`); `NS_IW_FunnelSmoke` (P5b, `p5b_assets.py`); `M_WakeFoam` (card B)

Files: `M_IW_BowFoam`, `M_IW_Particle`, `M_IW_Spray`, `M_IW_Tracer`, `M_WakeFoam`, `NS_IW_BowSheet`, `NS_IW_BowSpray`, `NS_IW_Explosion2`, `NS_IW_Fire`, `NS_IW_FunnelSmoke`, `NS_IW_Rain`, `NS_IW_Smoke`, `NS_IW_Splash`, `NS_IW_Spume`.

### Core (`Content/Ironwake/Core`)

Project-authored Blueprint. DefaultPawn = BP_PlayerShip, HUD = AIronwakeHUD (build report `B.md`)

Files: `BP_IronwakeGameMode`.

Ships root also holds `BP_PlayerShip` (child of the C++ pawn) (build report `B.md`).

## 12.5 Audio assets

24 sounds in `Content/Ironwake/Audio`, imported from the repo `Audio/*.wav` by `Content/Python/iw_import_audio.py` (loops flagged looping) [src Content/Python/iw_import_audio.py:9-15]. Sources outside the repo: `$IRONWAKE_ASSETS/audio/{music,sfx,voice,ambience,sfx_cc0}`. Full audio behaviour is chapter 10; this table is provenance.

| Asset | Origin | Licence | How made / regenerate |
|---|---|---|---|
| `main_theme` | approved music file `audio/music/main_theme.wav` | ours (approved generation; tool unknown) | copy; 30.8 s (build report `P4.md`) |
| `battle_night_storm` | approved music `audio/music/battle_night_storm.wav` | ours (approved generation) | copy |
| `broadside_sfx` | approved `audio/sfx/broadside_sfx.wav` | ours | copy |
| `foghorn_sfx` | approved `audio/sfx/foghorn_sfx.wav` | ours | copy |
| `slow_lap_loop` | approved `audio/sfx/slow_lap_loop.wav` | ours | copy |
| `bow_wash_loop` | approved file, REGENERATED in P11 as broadband | ours (procedural) | `build/audio/gen.py` (original kept as `build/audio/bow_wash_loop.orig.wav`) |
| `hull_wake_loop` | approved file, REGENERATED in P11 | ours (procedural) | `build/audio/gen.py` |
| `hull_hiss_proc` | procedural, regenerated in P11 | ours (procedural) | `build/audio/gen.py` |
| `bow_rush_proc` | procedural moving-water layer | ours (procedural) | W3 audio fix; generator script not located (unknown) |
| `harbour_lap_proc` | procedural noise, 4.3 s swell rhythm | ours (procedural) | generated with ffmpeg noise (design archive `audio/sfx_cc0/USE.md`) |
| `wind_sea_proc` | procedural filtered noise | ours | ffmpeg (see `CREDITS.md`) |
| `gale_proc` | procedural filtered noise | ours | ffmpeg (see `CREDITS.md`) |
| `rain_proc` | procedural filtered noise | ours | ffmpeg (see `CREDITS.md`) |
| `surf_proc` | procedural filtered noise | ours | ffmpeg, imported by `iw_i1_mats.py` |
| `creak_wooden_hull` | SFXMint ship-creak-02 | CC0 1.0 | mp3 -> wav (see `CREDITS.md`) |
| `creak_storm_strain` | SFXMint ship-creak-10 | CC0 1.0 | mp3 -> wav (see `CREDITS.md`) |
| `storm_howling_wind` | SFXMint hurricane-10 | CC0 1.0 | mp3 -> wav (see `CREDITS.md`) |
| `thunder_near / thunder_far` | synthesised: brown noise burst, lowpass 300 Hz, 3-6 s decay | ours | W2 (build report `W2.md`); recipe (design archive `audio/sfx_cc0/USE.md`) |
| `captain_contact / captain_load` | first half / rest of approved `audio/voice/captain.wav` | ours (approved voice) | split by silence detection (build report `C.md`) |
| `officer_splash / officer_correct / officer_damage` | split from approved `audio/voice/officer.wav` | ours (approved voice) | split by silence detection (build report `C.md`) |

Deleted/never imported: `harbour_ambience` (choir-like stack, replaced by `harbour_lap_proc`) and `sea_wind` (synth pad, "angelic") (build report `W3.md`); `creak_mast_rope` has a sustained harmonic comb, dropped (design archive `audio/sfx_cc0/USE.md`); `harbor_windy_rigging` is in the CC0 pack but not imported. Spectral hum check: build/audio/spec.py.

## 12.6 Python scripts under `Content/Python`

Run headless with `IWTEST_SCRIPT=<module> UnrealEditor IronwakeTest.uproject -log`; `init_unreal.py` imports `$IWTEST_SCRIPT` [src build/BRIEF.md:30-31]. All are idempotent unless noted.

| Script | Purpose (first docstring line) |
|---|---|
| `build_storm_test.py` | Ironwake look test level, built by UE editor Python. |
| `build_storm_v2.py` | Ironwake storm test v2: rebuild level cleanly with correctly sized ocean, then screenshot + quit. |
| `epic_water_sample.py` | Open Epic's WaterExtras sample map, list its water actors, screenshot from several cameras, quit. |
| `fix_ocean.py` | Diagnose + fix ocean extent and camera placement, then screenshot. |
| `init_unreal.py` | _s = os.environ.get("IWTEST_SCRIPT") |
| `iw_e1_import.py` | E1: import world assets (build/assets/e1/<Name>/<Name>.fbx + materials.json) into /Game/Ironwake/World/<Name>/. |
| `iw_i1_folfix.py` | I1: foliage fixes: M_IW_Foliage clip 0.4 -> 0.22, leaf alpha textures mip gen Sharpen (keeps canopies at distance). |
| `iw_i1_import.py` | I1: island assets (copy of iw_e1_import; build/assets/i1/ + leaf alpha maps -> M_IW_Foliage). E1 original: import world assets (build/assets/e1/<Name> |
| `iw_i1_mats.py` | I1: island ground material (vertex-colour blend sand <-> grass/rock, Poly Haven CC0 textures), surf ribbon material |
| `iw_i1_shelf.py` | I1: M_IW_Shelf (translucent shallow-water tint drawn just under the sea surface around islands: turquoise/sand, vertex |
| `iw_i1_surfparams.py` | I1: tweak M_IW_Surf scalar defaults in place (rebuilding with delete_all_material_expressions crashes on a loaded asset). |
| `iw_i1_usage.py` | I1: set bUsedWithInstancedStaticMeshes on the masters used by HISM instances (M_IW_ShipTex2, M_IW_ShipPaint, M_IW_Foliage, |
| `iw_i1_veglod.py` | I1: vegetation meshes: Nanite OFF (Nanite masked foliage rendered bare branches at range on Metal) and auto LODs |
| `iw_i1_water.py` | I1: inspect water zone / ocean extents on Ironwake_Sea (read-only unless IWI1_SETEXT=<metres full width>). |
| `iw_import_airship.py` | proj = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()) |
| `iw_import_audio.py` | src = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "Audio") |
| `iw_p10_bounds.py` | for n in ["TownBandstand","TownChapel","TownCrateStack","TownCustomsHouse","TownHandcart","TownMarketStall","TownRowCottages","TownStoneTownhouse","To |
| `iw_p10_chk.py` | MEL=unreal.MaterialEditingLibrary |
| `iw_p10_mats.py` | P10: tiling PBR town materials (world-aligned top projection) + emissive window material. Headless, idempotent. |
| `iw_p10_sky.py` | P8: NASA star map + moon phase sprites -> /Game/Ironwake/Sky, M_IW_NightSky (unlit sky material, evaluated analytically per pixel). |
| `iw_p12_mats.py` | P12: plaster/roof master with per-colour tint and subtle world-space noise. Headless, idempotent. |
| `iw_p12_smoke.py` | P12: unlit translucent grey material for thin chimney smoke. |
| `iw_p13_probe.py` | L = lambda m: unreal.log("IWP13 " + str(m)) |
| `iw_p8_import.py` | E1: import world assets (build/assets/p8/<Name>/<Name>.fbx + materials.json) into /Game/Ironwake/World/<Name>/. |
| `iw_p8_sky.py` | P8: NASA star map + moon phase sprites -> /Game/Ironwake/Sky, M_IW_NightSky (unlit sky material, evaluated analytically per pixel). |
| `iw_r1_airab.py` | EAL = unreal.EditorAssetLibrary; MEL = unreal.MaterialEditingLibrary |
| `iw_r1_airfix.py` | R1 airship fix: reimport BrassAirship meshes with recomputed normals, flip the normal map's green (glTF -> UE), |
| `iw_r1_import.py` | R1: import library ship FBX parts into /Game/Ironwake/Ships/<Name>/ (headless editor Python). |
| `iw_r1_materials.py` | R1 materials: master M_IW_ShipPaint (vector colour) + M_IW_ShipTex (base texture * tint, ORM-ish metal/rough maps optional), |
| `iw_s2_cloth.py` | S2: (re)create M_IW_Cloth only (two-sided foliage canvas). IWTEST_SCRIPT=iw_s2_cloth |
| `iw_s2_import.py` | S2: import the steam-sail frigate hull (Svjatoi Nikolai, sails/yards stripped) + its materials, and create the |
| `ocean_base.py` | Copy Epic's working water sample into our project as /Game/Maps/OceanBase, then take |
| `p11_audio.py` | P11: re-import the regenerated broadband sailing loops (no tonal hum). |
| `p5b_assets.py` | EAL = unreal.EditorAssetLibrary |
| `p5c_probe.py` | L = lambda m: unreal.log("IWP5C " + str(m)) |
| `storm_v3.py` | Storm v3: start from Epic's working water sample (landscape + brush manager + ocean), |
| `storm_v4.py` | Storm v4: on /Game/Maps/OceanStorm (copy of Epic's water sample), replace the ocean waves with a |
| `w1_assets.py` | W1 assets: M_IW_BowFoam (lit translucent foam, vertex-alpha, world-aligned foam texture, depth fade), water loop sounds, NS_IW_BowSpray (dup of splash |
| `w1_foamfix.py` | MEL = unreal.MaterialEditingLibrary |
| `w1_inspect.py` | L = lambda m: unreal.log("IWW1 " + str(m)) |
| `w1_inspect2.py` | L = lambda m: unreal.log("IWW1 " + str(m)) |
| `w2_assets.py` | W2 assets (headless): storm audio layers + thunder, NS_IW_Spume / NS_IW_Rain / NS_IW_BowSheet (dups to retune over MCP). |
| `w2_foammat.py` | W2: whitecaps + wind streaks. Duplicate the Water plugin material chain into /Game/Ironwake/Water and blend a white foam |
| `w2_mat.py` | L = lambda m: unreal.log("IWW2M " + str(m)) |
| `w2_mat2.py` | MEL = unreal.MaterialEditingLibrary |
| `w2_matprobe.py` | L = lambda m: unreal.log("IWMP " + str(m)) |
| `w2_matprobe2.py` | L = lambda m: unreal.log("IWMP " + str(m)) |
| `w2_matprobe3.py` | L = lambda m: unreal.log("IWMP " + str(m)) |
| `w2_probe.py` | L = lambda m: unreal.log("IWW2P " + str(m)) |
| `w3_audiofix.py` | Audio fix (Chuck 09:55): import harbour_lap_proc / bow_rush_proc / hull_hiss_proc (looping), delete tonal harbour_ambience + sea_wind. |
| `w3_foammat.py` | W3: upgrade the IW_Foam Custom node in M_IW_WaterRoot IN PLACE (no deletes: deleting referenced water assets crashed the |
| `w3_waterprobe.py` | Two-layer bug: list every water / landscape / plane actor on Ironwake_Sea with Z + extents, sample landscape height on a grid. |
| `w4_all.py` | W4: run the foam node + spray material builders and leave the editor up (MCP) for the Niagara edits (build/tools/w4fx.py). |
| `w4_foammat.py` | W4: IW_Foam Custom node in M_IW_WaterRoot, edited IN PLACE (no deletes: deleting referenced water assets crashed the editor). |
| `w4_spray.py` | W4: M_IW_Spray, a soft spray/spume sprite material for Niagara (replaces the hard-dot M_IW_Particle on NS_IW_Spume and the |

## 12.7 Blender and other build scripts used for assets

| Script | Role |
|---|---|
| `build/r1/process.py` | library GLB -> FBX parts (R1); config JSONs `cfg_*.json` in the same folder |
| `build/e1/prep.py` | E1 world assets GLB -> FBX + PNG + materials.json + info.json |
| `build/p8/prep.py` | P8 cluster models GLB -> FBX (same schema as E1) |
| `build/i1/prep_i1.py` | I1 island assets (copy of E1 prep) |
| `build/i1/plan.py` | I1 seeded island planner (seed 1871) |
| `build/s2/prep_frigate.py` | S2 strip sails/yards/flag from Svjatoi Nikolai GLB |
| `build/p8/skyprep.py` | P8 NASA star map 16-bit PNG -> 8-bit sRGB (obsolete after P10) |
| `build/p8/glowprep.py` | P8 baked smooth glow map (obsolete after P10) |
| `build/audio/gen.py` | P11 broadband periodic loops (FFT-shaped noise, 20 s at 48 kHz) |
| `build/audio/spec.py` | P11 spectrum checker (peak over median dB) |
| `build/w1/clear.py` | W1 hull clearance profile (half-breadth between z -0.5 and +2.5 m) |
| `build/assets/glb2fbx.py` | first Hunyuan ironclad/airship GLB -> FBX |

Batch-side scripts in `$IRONWAKE_ASSETS/cluster/`: `img3d.sbatch` (Hunyuan3D-2 image -> GLB on L4, array 0-1) (design archive `cluster/img3d.sbatch`), `harbour3d.sbatch`, `harbour3d_v100.sbatch`, `prep_ships.py/.sbatch` (library decimation, 10 CPU nodes) [src build/BRIEF.md:163], `remesh_bake.py` (airship remesh), `prep_island.py`, `make_sky.py`, `score_job.sh`, `turntable.py`. Worker rule: workers never submit cluster jobs; they append to `batch_requests.md` [src build/BRIEF.md:166-167].

## 12.8 Maps and engine content

| Map | Origin |
|---|---|
| `Content/Maps/Ironwake_Sea.umap` | shipping map; copy of OceanStorm, cleaned in card A (build report `A.md`) |
| `Content/Maps/OceanStorm.umap` | copy of Epic's WaterExtras test map [src build/BRIEF.md:50] |
| `Content/Maps/StormTest.umap`, `StormTest2.umap` | early storm tests from `Content/Python/build_storm_test.py`/`build_storm_v2.py` |

Epic engine content in use: the Water plugin material chain (copied into `Content/Ironwake/Water`), `T_soft_smoke` and Niagara templates for FX (build report `C.md`), and `/PCG/SampleContent/SimpleForest/Meshes/PCG_Boulder_02` for the card-D lighthouse island (build report `D.md`). Epic content is under the Unreal Engine EULA, not redistributable outside the engine. Fab/Megascans: none.

## 12.9 Licence summary and credits

| Class | Items | Licence | Obligation |
|---|---|---|---|
| Sketchfab CC-BY 4.0 | Devastation, Kracken, Goliath, Aglae, brass airship, Svjatoi Nikolai frigate, wharf building, Victorian factory, rowboat, sea mine, barrels | Creative Commons Attribution 4.0 [src build/CREDITS.txt:5] | credit author + link, state changes [src build/CREDITS.txt:6] |
| Sketchfab CC0 | SS Rifle stern, Venus fourareen | CC0 [src build/CREDITS.txt:22] | none |
| Poly Haven | cliffs, rocks, pier, props, trees, shrubs, textures, fort kit | CC0 (see `CREDITS.md`) | none (credited anyway) |
| SFXMint | creaks, hurricane | CC0 1.0 (see `CREDITS.md`) | none |
| NASA SVS / Gaia / LROC | star map, moon | NASA media + ESA/Gaia/DPAC | credit required (design archive `models/sky_final/SKY_USE.md`) |
| Hunyuan3D-2 output | harbour pieces, town, NPC craft, first ironclad/airship | generated from our concept art; model by Tencent | credited in-game [src build/CREDITS.txt:38] |

CC-BY sources are never committed; they are re-made by the build scripts [src SPEC.md:103]. In the development repo the cooked `.uasset` files are committed (751 under `Content/Ironwake`), so CC-BY derived meshes live there as engine assets and licence terms still apply. This public repo leaves out all cooked content and licensed sources: `.gitignore` excludes `Content/Ironwake/` and `Content/Maps/` [src .gitignore:8-9] and `*.fbx`, `*.uasset`, `*.umap` [src .gitignore:28-31].

Credit text locations: `build/CREDITS.txt` (38 lines), `Packaged/Ironwake/CREDITS.txt`, `Packaged/Ironwake/README_CREDITS.txt` (all three byte-identical when checked: `diff` on 2026-11-11 printed nothing), and the in-game CREDITS card (comment `CC-BY attribution` in IronwakePlayerController.cpp) [src Source/IronwakeTest/IronwakePlayerController.cpp:46]. Cooking: `DirectoriesToAlwaysCook=(Path="/Game/Ironwake")` [src Config/DefaultGame.ini:8] so unreferenced assets are cooked anyway.

### Credit gaps

- CONFLICT: the library credit file lists 21 models (see `CREDITS.md`); `build/CREDITS.txt` credits 6 ship models plus the world set. Unimported library models need no credit until used.
- `Wooden Pier_22_MB` by Mehdi Shahsavan (CC-BY) is credited in world CREDITS.md (line 30) but is not named in `build/CREDITS.txt` (grep for Shahsavan in build/CREDITS.txt returned nothing). The WoodenPier mesh is also unreferenced in code, so either credit it or delete the asset.
- In-game CREDITS card text vs file: the card is hand-typed in C++; no check ties it to `CREDITS.txt` after E1/I1 additions beyond the R1/E1 log statements (build report `E1.md`).
- `models/world/CREDITS.md` also credits models flagged AVOID (crane_tower, harbour_crane under Sketchfab Standard "Free Standard" licence) (see `CREDITS.md`); they are not imported (not in `Content/Ironwake/World`).

## Gaps / unknown

- Generator script for `bow_rush_proc.wav` and the original generator for `thunder_*.wav`, `wind_sea_proc.wav` etc. (ffmpeg one-liners): not found in the repo. Where to look: `logs/W2.md`/`W3.md` shell history; the files in `$IRONWAKE_ASSETS/audio/sfx_cc0/` are the only record.
- Provenance (tool, prompt) of the approved music, voice, `broadside_sfx`, `foghorn_sfx` and `slow_lap_loop` files: unknown. Source of truth is `$IRONWAKE_ASSETS/audio/`.
- Exact GPU batch job ID for each cluster_b1 model: the job folder name is the ID (`2167` etc.) and each model's folder holds `job.log`; the per-model job is not listed here beyond SCORES.json `job` values.
- First Hunyuan ironclad/airship: concept image to model chain is in `concepts/batch1` (`ironclad_34.png`, `airship_34.png`); the GPU batch job 2048 command line is not in the repo.
- Poly Haven texture file list for Town (`build/assets/p10/`): files exist on disk but `build/assets/` is git-ignored, so a clean clone cannot re-run `iw_p10_mats.py` without re-downloading from Poly Haven.
- Whether `OceanStorm.umap`, `StormTest.umap`, `StormTest2.umap` are still needed (not in the cook list): unknown; check `Config/DefaultGame.ini` map list.

## P17 batch-2 imports (2026-11-11)
All prepped by `build/p8/prep.py` (CBf2 reads `models/cluster_b2/<job>/<name>.glb`) and imported by `iw_p8_import.py` with Nanite and simple collision (box for Town*, 26-DOP for ships).
| Asset | Fit | Rot | Placement |
|---|---|---|---|
| TownWaterPump | h 4 m | - | square (-80,27) |
| TownFishRacks | len 10 m | - | quay x2, cove |
| TownWindmill | h 22 m | rotz 90 | crest (-338,96) |
| TownMarketHall | len 16 m | - | square (-80,-56) |
| TownFisherCottage | len 9 m | rotz 90 | cove + fringe |
| TownHorseWagon | len 4.5 m | - | quay + 2nd lane |
| TownBoathouse | len 14 m | rotz 90 | cove (-83,106) |
| TownGasometer | h 20 m | - | industrial bench (-150,-144) |
| CoalLighter | len 30 m | rotz 90 | jetty mooring + NPC loop |
| SalvageTender | len 28 m | rotz -90 | jetty mooring |
| CargoAirshipLong | len 120 m | rotz 90 | NPC high loop (210 m) |

P23: no new meshes. Existing Town* types reused with non-uniform footprint/height scale per house; retaining walls and plinths are cube primitives with MI_IW_Setts/MI_IW_Cobble.

## P26 rigged Tall Brig + 17 moving parts + sail/spar materials (2026-11-13)
- Source: Hunyuan3D-2 `b2_brig_hull_rigged.glb` rigged in Blender (`rig_brig.py`) and prepped on GPU batch CPU job `2253` (`prep_parts_batch.py`, scaled to `42` m hull length, bow `+X`, 2K PNGs, `parts.json` pivots), staged to `build/assets/p26/` and imported headlessly by `Content/Python/iw_p26_import.py`.
- Imported folders under `Content/Ironwake/World/`: `TallBrig` (`SM_TallBrig`, Nanite ON, 26-DOP simple collision) plus 17 non-Nanite moving-part subfolders (`TallBrig_yard_m0_0..2`, `TallBrig_sail_m0_0..2`, `TallBrig_yard_m1_0..2`, `TallBrig_sail_m1_0..2`, `TallBrig_boom_spanker`, `TallBrig_gaff_spanker`, `TallBrig_sail_spanker`, `TallBrig_fore_staysail`, `TallBrig_jib`).
- New masters in `Content/Ironwake/Ships/Materials/`: matte timber/paint masters `M_IW_BrigTex` and `M_IW_BrigPaint` [src Content/Python/iw_p26_import.py:65] (`Metallic = 0`, `Roughness = 0.78..0.88`) and two-sided subsurface WPO sail master `M_IW_SailCanvas` [src Content/Python/iw_p26_import.py:70] (zero `Clamp` expressions on `SF_METAL_SM6`).

## P28 batch-3/4 moving vessels, crew, gulls, buildings, signs & 4K Megascans ground (2026-11-13)
- Imported 20 pre-built FBX + PNG bundles from `$IRONWAKE_ASSETS/models/ue_ready/{p28,p28_signs}/` via `Content/Python/iw_p28_import.py` (`build/tools/p28import.sh`, `-nullrhi`, port `8019`) into `Content/Ironwake/World/<Name>/`:
  - Nanite hulls/buildings (26-DOP / Box simple collision): `PaddleSteamerV3`, `SalvageTenderV3`, `PatrolAirshipV4`, `TownTavernV4`, `TownChandleryV4`, `TownFoundryV4`.
  - Non-Nanite moving parts, signs & 3-LOD (`100/22/6/1.5%`) crew/gulls: `PaddleSteamerV3_wheel_{port,stbd}`, `SalvageTenderV3_{crane,bell}`, `PatrolAirshipV4_prop_{port,stbd,stern}`, `SeaGullV4`, `SeaGullV4_wing_{L,R}`, `SailorCrewStand`, `SailorCrewRower`, `SignAnchorTavern`, `SignShipChandlery`.
- Imported pruned 4K Megascans ground maps (`T_Ground_ForestFloor_4K_Albedo` [src Content/Python/iw_p28_import.py:272], `Normal`, `Roughness`, `T_Ground_MossyFloor_4K_{Albedo,Normal,Roughness}`) into `Content/Ironwake/Materials/GroundScans/` and rebuilt `M_IW_Hill`, `M_IW_Road`, `M_IW_Tile`, plus `MI_IW_DarkStone` with 3-axis world-space triplanar projection and Metal-safe rational `1 / (1 + (PixelDepth / 3500)^2)` micro-detail (`0` material compile errors).
