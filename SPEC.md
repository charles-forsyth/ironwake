# IRONWAKE: SPEC (living document)

**Entry point.** This file is the summary; the exhaustive, source-cited chapters are in `spec/`.
Every value in the chapters is written as `value` [src path:line] and machine-checked by
`python3 build/tools/spec_check.py` (`--all` checks every tag; default is a random 30). Where code, comments and logs
disagree, the chapter states the value that takes effect and marks the other CONFLICT. "unknown" means not
derivable from files (the chapter says where to measure it).

| # | Chapter |
|---|---|
| 1 | [Design](spec/01_design.md) |
| 2 | [Ships](spec/02_ships.md) |
| 3 | [Water](spec/03_water.md) |
| 4 | [Ship water FX](spec/04_ship_fx.md) |
| 5 | [Collision and grounding](spec/05_collision.md) |
| 6 | [Navigation](spec/06_navigation.md) |
| 7 | [World](spec/07_world.md) |
| 8 | [Time, weather, sky, lighting](spec/08_time_weather_sky.md) |
| 9 | [Input, HUD, menus, settings](spec/09_input_hud.md) |
| 10 | [Audio](spec/10_audio.md) |
| 11 | [Performance](spec/11_performance.md) |
| 12 | [Assets and credits](spec/12_assets.md) |
| 13 | [Build, test, tooling](spec/13_tooling.md) |
| 14 | [Card history + self-check output](spec/14_history.md) |
| 15 | [Known issues and backlog](spec/15_backlog.md) |

Corrections found while writing the chapters (summary below is older; chapters win): ironclad is 125 m and frigate 88 m in
the fleet table (ART_BIBLE says 140/80); sailing no-go is 35 deg in the polar table (text below says 40); `Ironwake.app` is the
W3 water-fix build, not A1-era; frigate and cutter are always unlocked. See chapters 1, 2, 15 for the full conflict lists.

---

Status as of 2026-11-13 13:00 EST. **Promoted build: `Packaged/Ironwake/Ironwake.app`** (promoted in **P30** from verified `Ironwake_P29.app`: data-driven 3-port world via `world_ports.json` — Greyhaven + Ironcrag + Stigsby, 4-ship Shipyard incl. 42 m rigged Tall Brig, 7-ship Havelock LUT wakes, 120+ moving parts & 3D crew, Harbourmaster 8-good trade market & Captain's Logbook; `53.10 FPS` MEDIUM `+1.15` vs P19 & `+18.15` vs W3, `33.25 FPS` HIGH `+1.40` vs P19).
Showcase (2026-11-13, after P30): public repo `charles-forsyth/ironwake`, site https://charles-forsyth.github.io/ironwake/ (18 web models, 25 sounds, wake lab, 4-cut reel). Pipeline, cluster runs and release gate: chapter 13.12. No game code changed.
Previous Nov 10 W3 `Ironwake.app` backed up to `Packaged/Ironwake/Ironwake_W3_prev.app`; per-card archives remain at `Packaged/Ironwake/Ironwake_<card>.app`.

New to the project? Read `HANDOFF.md` first.

Owner: Chuck Forsyth. Built by Hermes (orchestrator) and the automated build pipeline
(automated multi-pass build & verification workflow).

## 1. What the game is
A steampunk naval sailing game: sail ironclads, steam frigates and cutters around an archipelago
in changing weather, day and night, with airships overhead. Peaceful sailing comes first;
combat (P2) and the open-sea storm look are PARKED until Chuck un-parks them.
Long-term target (WORLD_PLAN.md in game-concepts): ~120 x 120 km, 30-40 islands, 4 sea zones,
many ports and towns, NPC ships and airships, trade. The current slice (`P29`) spans ~8.4 km with
18 places, **3 full working ports** driven from `Content/IronwakeData/world_ports.json` (**Greyhaven**, **Ironcrag**, **Stigsby**), a 30-ship multi-port NPC sea (`0/0/0` soak), an 8-good Harbourmaster cargo market (`Silver Crowns`), and a Captain's Logbook.

## 2. Engine, project, sources
- Unreal Engine 5.8.4, Apple Silicon Mac (Metal), project `./` (git, `main`).
- C++ module `Source/IronwakeTest/` (systems below), map `/Game/Maps/Ironwake_Sea`.
- Design docs and asset sources: `$IRONWAKE_ASSETS/` (CONCEPT, DESIGN, ART_BIBLE,
  WORLD_PLAN, concepts/, models/, audio/, cluster/ scripts for the GPU batch GPU/CPU jobs).
- Worker rules: `build/BRIEF.md` (read first). Per-card specs: `build/cards2/*.md`. Logs: `logs/` (git-ignored).

## 3. Systems (where they live)
| System | Files | State |
|---|---|---|
| Ship pawn, handling, telegraph, rudder | IronwakeShipPawn.*, IronwakeShipHydro.cpp, IronwakeFleet.* | Heavy ships; physics substepping (1/60, 6 substeps) so turning is frame-rate independent (P7). Full circle at flank: ironclad 209 s (R 330 m), frigate 192 s, cutter 33 s. |
| Water and buoyancy | IronwakeHydro.*, IronwakeP16.cpp, Water plugin ocean (Gerstner) | Sea state CALM..GALE tables; measured crest-to-trough (both analytic and renderer path agree): CALM 0.97 m, MODERATE 2.34 m, ROUGH 4.82 m, GALE 8.78 m (P16). Ocean extent 300 km (no horizon seam, P5c). |
| Ship water FX | IronwakeShipPawn.cpp (UIronwakeWake, UIronwakeBowWave), IronwakeShipWave.*, M_IW_WaterRoot | P27: 7-ship GPU batch Havelock Kelvin LUT (`T_IW_KelvinWaveLut` + CPU `kelvin_lut_256.bin`) + turbulent twin-vortex Voronoi prop-wash LUT (`T_IW_PropWashFoamLut` + `T_IW_CellularFoamTile`) + turquoise SSS + lit `M_IW_WakeLut` (`+6 cm`, zero hull climb; vision 8.5-9.5/10, `+1.75` FPS vs P26). |
| Funnel smoke | NS_IW_FunnelSmoke | Pale grey, speed-scaled; NS_IW_Smoke only for burning/sinking. |
| Collision + grounding | IronwakeCollision.*, IronwakeShipCollision.cpp | 1108 analytic solids (rocks, cliffs, piers, quays, buildings, buoys, lighthouse, wreck) + ship hulls; depth grounding vs per-hull draught (SHOAL WATER / AGROUND); astern frees; autopilot and NPCs avoid land (P9). `-iwnocoll` disables. |
| Autopilot + chart | IronwakeAutopilot.cpp, IronwakeVoyage.* | OFF / HOLD COURSE / WAYPOINTS / TOUR; 3-port trade triangle (`Greyhaven • Ironcrag • Stigsby`), breakwater throat gates, routes bend round hazards. |
| Archipelago + multi-port world | IronwakeArchipelago.*, IronwakePorts.*, Content/IronwakeData/world_ports.json | 18 places + 3 data-driven ports (Greyhaven `(-3450,-3350)`, Ironcrag `(2590,90)` on isle 8, Stigsby `(-570,3550)` on isle 9), 8-good trade market, live Harbourmaster diorama (`Enter`), Captain's Logbook (`O`). |
| Clock + weather + sea by weather | IronwakeP11.*, IronwakeVoyage.*, IronwakeStorm.* | Continuous clock (start 09:30, 48 real min per day), sun for lat 50 N; weather clear/fair/overcast/mist/rain/squall drifting every 10-30 game min with 2-4 min blends; sea state follows weather; harbour basin calm slab + shore shelter (P11-P14). |
| Night sky | IronwakeP8.* (AIronwakeNightSky), /Game/Ironwake/Sky | NASA SVS Deep Star Maps 2020 HDR (RGBA16F), dome rotates with clock at 50 N, moon disc with real daily phase (moon_2027 table). Locked exposure on clear nights. Vision: Orion 9.5, Cygnus 9-10 (P10). |
| Harbour town | IronwakeP12.cpp, IronwakeP15.cpp (hillside heightfield, roads), IronwakeP23.cpp (placement planner), IronwakeP8.cpp, /Game/Ironwake/World, /Game/Ironwake/Town | P15/P23/P24/P28: analytic hillside, 74 procedural houses + `TownTavernV4` (`SignAnchorTavern`), row-nestled `TownChandleryV4` (`SignShipChandlery`), `TownFoundryV4`, `MI_IW_DarkStone` triplanar kerbs/retaining walls, 280 wind-bent broadleaf trees, 3-axis triplanar 4K Megascans ground micro-detail (`<45 m`), busy quay cargo & 24 HISM dockhands (400 m noon 8.5, street/tavern/ground 9.0-9.5). |
| NPC craft + living sea | IronwakeP8.cpp, IronwakeLivingSea.*, IronwakeNpc*.cpp | P20-P28: 30 ledger ships (`MaxLive = 26`) over 4 ports, harbour-mouth channel lock + quay reservations (`0/0/0` soak), 4 rigged `SM_TallBrig`, `PaddleSteamerV3` spinning sponson wheels + churn, `SalvageTenderV3` slewing A-frame crane + pendulum diving bell, `PatrolAirshipV4` 3 props, 3D `SailorCrewRower` gig crew + `SailorCrewStand` helmsmen, `SeaGullV4` flapping wings, live occupied-only mooring hawsers (`moving_parts=122`). |
| HUD, help, menus | IronwakeHUD.*, IronwakePlayerController.*, IronwakeP11.cpp, IronwakeSettings.* | Hint line bottom-left; H/F1 help panel (3 columns, DPI-scaled, HUD hidden while open); debug lines only with `-iwdebug`. P18/P26 menu: scale-box fitted to any window (clamp(min(w/1920,h/1080),0.5,1.5)) + scroll safety net; pages MAIN / SETTINGS (GAMEPLAY, GRAPHICS, VIEW, AUDIO) / CREDITS / 4-ship SHIPYARD (Frigate, Brig, Cutter, Ironclad), Esc/B back, full keyboard + gamepad navigation, same pages when paused.; P16 VIEW tab CAMERA MOTION OFF / LOW (default) / MEDIUM (camera follows <=15 % of ship heave, pitch, roll; roll <= 1.5 deg). |
| Audio | Audio/*.wav -> /Game/Ironwake/Audio, Classes/SC_* | P18: 8 sound classes (Master + Music, Sea, Wind, Ship, Guns, Voices, UI) with a slider each in SETTINGS > AUDIO (5 % steps, TEST, RESET, saved as `Vol_<Class>`, `-iwvol=Class:pct`); MUSIC slider 0 = off (default). Music OFF by default. Sailing loops (bow_wash, hull_wake, hull_hiss) regenerated broadband in P11 (no tone: worst peak +2..6 dB over median). In-game audio recording not yet done. |
| Ship response | IronwakeShipHydro.cpp, IronwakeShipPawn.cpp | P16: livelier heave/pitch/roll periods (ironclad about 6.5 / 6.8 / 10 s), rough sea roll ~5 deg, pitch ~4 deg; turning circle unchanged (208.8 s at 18 and 30 fps). |
| Combat | IronwakeCombat.cpp, IronwakeShell.* | Exists from the first slice; PARKED. |

## 4. Controls (from the help panel, IronwakeP11.cpp)
- Helm: W/S telegraph (one step per press; sailing ship = sail plan), A/D hold to turn the wheel,
  Q/E rudder midships, J helmsman steers to next point, X anchor, G fast sail x4.
- Engine & sails: B auxiliary steam (sailing ship; can't sail within 40 deg of wind), R damage control,
  left click fire (when guns READY and ON).
- Navigation & arrival: M chart (left-click add waypoint, right-click remove, Enter start, Backspace clear, L loop),
  P autopilot (off / hold / waypoints / tour; A/D nudge 5 deg), N next point of interest, Enter (< 2.5 kn in port / at POI) make fast at Harbourmaster or salvage/log discovery, O Captain's Logbook.
- Camera: menu CAMERA MOTION OFF / LOW / MEDIUM (default LOW); mouse orbit/aim, wheel zoom, Shift binoculars, C or middle mouse free look, F photo mode.
- View & world: T clock speed x1 / x10 / pause, Shift+T or ] jump +3 h, L next weather, K weather drift,
  H/F1 help, Tab/Esc pause menu.
- Gamepad: D-pad telegraph/rudder, R trigger fire, R stick look, L trigger exit binoculars.

## 5. Hard rules (Chuck)
- Collision: ships never pass through land, rocks, piers, buildings or other ships.
- Ship FX scale with speed; small at low speed.
- Music off by default; no tonal/"angelic" ambience.
- Storm crest/whitecap/spume look PARKED; normal wave height by sea state is wanted.
- Combat PARKED.
- Never touch another Unreal Editor instance (`(concurrent editor project)`).
- Test runs: `-RenderOffScreen -nosound -unattended -nosplash`, end with `pkill -x CrashReportClient`
  (trap). No windows or sound while Chuck uses the Mac.
- Never replace `Ironwake.app` without a promotion gate; each card packages `Ironwake_<card>.app`.
- FPS: compare builds in back-to-back pairs (A, B, A, B); absolute numbers depend on machine load.

## 6. Verification method
Each card ends with offscreen captures scored by a vision model against a target (usually >= 7/10),
a paired bench (600 frames, 1920x1080, packaged), and a log in `logs/<card>.md`.
The orchestrator re-scores key frames itself and writes follow-up cards for misses.

## 7. Card history (2026-11-09 .. 11)
First night: S1 sailing, H1 perf/handling, R1 hi-res ships, W1 heavy bow wave, A1 autopilot, E1 world assets,
S2 ships, I1 islands, W2-W4 storm (parked at 4-6/10).
Day 2-3: P3 handling + sea, P4 clip (media/slice_v2.mp4), P5 package, P6 Phase-review review (phase_review.md),
P5b speed FX + grey smoke, P5c ship water look, P7 frame-rate-independent turning, P8 cluster models + sky v1,
P9 collision, P10 HDR night sky + town v1, P11 clock/weather/hum/help, P12 terraced town, P13 dawn/dusk/lamps,
P14 clear noon / night sea / harbour basin.
Day 3-5 (2026-11-11 .. 13): P15 natural hillside, P16 measured swell + livelier ship + CAMERA MOTION, P17 cove/jetties + batch-2 models,
P18 scaled menu + 8 audio sliders, P19 ship-wave field, P20-P22 living sea, moving parts, wake hysteresis,
P23-P24 stepped-bench town + headland lighthouse + trace windows (67 houses, street 7.5, noon 400 m 4), P25 NPC quay-lane & mouth arbitrator (`0/0/0` soak across seeds 1-3), P26 rigged 42 m Tall Brig (17 moving parts + gaff spanker + WPO wind sails + 4-ship Shipyard, vision 8.0-10/10, `+2.05` FPS), P27 7-ship Havelock Kelvin & Voronoi prop-wash LUT + zero hull climb (vision 8.5-9.5/10, `+1.75` FPS), P28 moving paddle/crane/bell/airship parts + 3D crew/rowers/gulls + b4 tavern/chandlery/foundry + shop signs + triplanar 4K ground (vision 7.5-10/10, `+1.85` FPS), P29 data-driven 3-port world (`world_ports.json`: Greyhaven + Ironcrag + Stigsby) + Harbourmaster diorama + 8-good trade market + Captain's Logbook (vision 7.8-10/10, `0/0/0` soak, `-0.20` FPS vs P28), P30 independent verification, promotion of `Ironwake_P29.app` -> `Packaged/Ironwake/Ironwake.app`, full spec check (`0 fail`), and `build/MORNING_REPORT_3.md`.

## 8. Current scores (latest frames)
| View | Score | Card |
|---|---|---|
| Ship water at 13 kn | ~8 | P5c |
| Night sky (Orion / Milky Way) | 9.5 / 9-10 | P10 |
| Clear noon town from sea | 7.5-8.5 (worker), 5.5 (orchestrator) | P14 |
| Night chase / high | 8.5 / 7.5 | P14 |
| Dawn / dusk / overcast | 6.5 / 7.5 / 8-9 | P13 |
| Squall chase / high | 6.5 / 4 | P14 |
| Harbour basin vs open sea | 9 | P14 |
| Real swell (surface moves vertically) | measured, see water row | P16 |
| Town 400 m noon / street / night | 8.5 noon 400 m / 9.0-9.5 street & 4K ground / 7.5 night | P28 / P24 |
| Wake continuity / NPC soak | 7 wake; **0/0/0** collisions/stuck/groundings (quay north 7.0-10) | P22 / P25 / P28 |
| Tall Brig (waterline / broad reach / reefed-furled / NPC harbour) | 8.5 / 8.0-8.5 / 10 / 10 | P26 |
| Prop wash / Kelvin wedge / hull waterline vs real photos | 8.5 / 8.5 / 9.5 (sail vs steam 9.5, speed/NPC6 9.5) | P27 |
| Moving parts & crew (gig 4-phase / tender crane+bell / paddle / airship) | 9.5 / 9.5 / 8.5 / 7.5 | P28 |
| Multi-port Ironcrag & Stigsby (noon 400 / night 400 / close cove / chart / market & logbook UI) | 7.8 / 8.5 / 8.5-10 / 9.5 / 9.8-10 | P29 |
| FPS (paired, editor closed, P30) | **53.10 MEDIUM** (`+1.15` vs P19 `51.95`, `+18.15` vs W3 `33.55`); **33.25 HIGH** (`+1.40` vs P19 `31.85`) | P30 |

## 9. Known gaps / next
1. Town: **CLOSED in P28/P30** (`74` procedural houses + 3 V4 landmarks, `280` broadleaf trees, `7.5-8.5/10` at 400 m noon & `7.8-8.5/10` at 400 m night); optional polish: POM street cobbles at 1.5 m eye height, rotating headland lighthouse beam, Stigsby night tree-crown shading & reversed lateral beacons, Ironcrag upper-slope cone artifacts.
2. NPC harbour soak: `0/0/0` in Greyhaven-only P25; `0/1/0`, `1/0/0`, `2/3/0` across 3 seeds in P30's 6-port multi-port + gale soak (`0` groundings across all seeds, `101-120` moving parts).
3. Moving parts & crew: **CLOSED in P28/P30** (`PaddleSteamerV3` spinning wheels `8.5-9.0/10`, `SalvageTenderV3` slewing crane + diving bell `8.5-9.5/10`, `PatrolAirshipV4` 3 props `7.5-8.0/10`, 3D `SailorCrewRower` gig stroke `9.5/10`, `SeaGullV4` wings, `SailorCrewStand` helmsmen & dockhands; minor polish: lower `TallBrig` quarterdeck `HelmsmanPivot` Z).
4. Ship wake look: **CLOSED in P27/P30** (`9.8/10` speed-scaling sheet in P30, `8.5-9.5/10` vs real photos, zero hull climb `insideHullFoamMax=0.0000`).
5. Multi-port arrival, trade & logbook (Phase-review item #8): **CLOSED in P29/P30** (3 data-driven ports in `world_ports.json`: Greyhaven, Ironcrag, Stigsby; `< 2.5 kn` `Enter` mooring & salvage verb; live Harbourmaster diorama; 8-good cargo market; persisted Silver Crowns & Captain's Logbook).
6. Squall from high camera reads flat without crest foam (parked look).
7. In-game audio recording at 13 kn to confirm no hum (needs Chuck away from the Mac); `25/25` sounds classed (`0` unclassed, `bad=0` verified in P30).
8. Combat remains parked; **`Ironwake_P29.app` promoted to `Packaged/Ironwake/Ironwake.app` in P30** (`build/MORNING_REPORT_3.md`).
9. Chapters under `spec/` are the detailed record; run build/tools/spec_check.py --all after code changes.
10. Public showcase: re-run `prep_web_models.py` / `render_rig_reel.py` on Ursa and `tools/verify_repo.py` in the public repo whenever ships or parts change (chapter 13.12).

## 10. Credits
Star map: NASA/Goddard Space Flight Center Scientific Visualization Studio. Gaia DR2: ESA/Gaia/DPAC.
Moon: NASA SVS CGI Moon Kit (LROC). Library ship and world assets: see build/CREDITS.txt and
`Packaged/Ironwake/CREDITS.txt` (CC-BY sources are never committed; re-made by build scripts).
