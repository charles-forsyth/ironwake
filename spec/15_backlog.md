# Chapter 15: Known issues and backlog

Scope: every open problem recorded in the card logs' known-gaps sections, the Phase-review review (`phase_review.md`), MORNING_REPORT and MORNING_REPORT_2, SPEC.md section 9 and the BRIEF hard rules, merged, de-duplicated and ordered. Order key: (1) hard-rule violations and things Chuck complained about directly, (2) things a first-time player hits first, (3) look/readability misses against stated targets, (4) verification debts, (5) content and gameplay depth, (6) tooling and housekeeping. Items Chuck parked (storm look, combat) are listed last in their own section and must not be treated as blockers [src build/BRIEF.md:62]. Each item names its source, whether it is still open as of the D1 snapshot (HEAD `84c9efa`), and where to measure or fix it. "Open" means no later log or commit closes it; this chapter did not launch the game to re-check, so some may be fixed without a log entry.

## 15.1 Priority list

| # | Item | Class | Source | Status |
|---|---|---|---|---|
| 1 | Town hillside is a hard uniform wedge with flat blocky ground and no paths; plateau flat top visibly straight | look/world | [src SPEC.md:92]; (build report `P14.md`) | open (P14: heightfield rebuild "outside this card") |
| 2 | Ship FX still weakest on-screen: flat foam band parallel to the camera, hull-side frames scored 0-2/10 (P3), pass rule not met (P5b, P5c) | look (Chuck complaint) | (build report `P3.md`); (build report `P5b.md`); (build report `P5c.md`) | partly fixed in P5c (~8 at 13 kn per SPEC); frigate chase wake unstable 3-7 |
| 3 | Frigate (default hull) chase camera: aux-stack smoke hidden behind sails | look | (build report `P5c.md`) | P7 moved the stack (-23 m, 23 m) scale 0.9; vision after = soft pale column (build report `P7.md`) |
| 4 | Frigate has no visible boot-top band (black hull to waterline) | look | (phase review); [src build/MORNING_REPORT_2.md:20] | open; SPEC.md lists "frigate boot-top band" under gap 5 [src SPEC.md:97] |
| 5 | Cutter rides a little light | look | (phase review) | open (drafts re-set in W3 (build report `W3.md`)) |
| 6 | In-game audio never recorded at 13 kn; hum fix verified only offline (+3.6/+5.3/+1.1 dB over median) | verification | (build report `P11.md`); [src SPEC.md:96] | open: needs Chuck away from the Mac (BRIEF audio rule [src build/BRIEF.md:10-12]) |
| 7 | Impact SFX is a reused creak; no scrape/impact sample | audio | (build report `P9.md`); (build report `P9.md`) | open |
| 8 | Nothing to do on arrival: 18 places are waypoints with a banner; no docking, salvage, logbook; trade/cargo/economy are stubs | gameplay | (phase review); [src SPEC.md:98] | open; REVIEW card suggested one arrival verb per place type |
| 9 | Shoreline integration: 5 of 18 places lacked a surf line, none a lighter shelf ring (Phase-review); I1 says surf patchy | look | (phase review) | partly open; I1 later added shelf `M_IW_Shelf` (build report `I1.md`) |
| 10 | Keybind strip and dev-style status lines read as debug text to a first-time player | UX | (phase review) | partly addressed in P11 (hint line + help panel, dev lines only with -iwdebug (build report `P11.md`)) |
| 11 | Menus plain: default font, flat card; period serif and brass framing proposed | UX | (build report `D.md`); (build report `P1.md`) | open |
| 12 | Default camera view scores 3.5-4/10 against WoWS; funnel hides foredeck from dead astern | UX | (build report `H1.md`) | open (Chuck may want a higher default pitch) |
| 13 | Manual keyboard/mouse play never done by a human in logs: SHIFT scope, C free look, wheel, held A/D, R damage control, gamepad | verification | (build report `H1.md`); [src build/MORNING_REPORT.md:52] | open |
| 14 | Clear/fair noon: worker 7.5-8.5 vs orchestrator 5.5 (disagreement); town still reads flat from sea | look | [src SPEC.md:84]; (build report `P14.md`) | open (re-score by orchestrator needed) |
| 15 | Squall from high camera reads flat (4/10): no crest foam (parked look) | look (parked) | (build report `P14.md`) | parked |
| 16 | Harbour at 650 m reads partly as a flat block; Wreck at 300 m small and dark | look | (build report `E1.md`) | open |
| 17 | Fort walls read as grey blocks from 400 m; trees dark/sparse at range; surf ribbons look like stamped rings from high angles | look | (build report `I1.md`) | open |
| 18 | GPU batch batch 2: rebuild thin parts on the 9 `fix` models; generate chandlery + horse wagon; build fence and street lamp procedurally; fishing smack hull + new rig | assets | [src SPEC.md:93]; (design archive `models/cluster_b1/SCORES.json`) | open |
| 19 | Airship: baked-in prop under R1's separate spinning prop; three Blender cuts tore the duct ring | assets | (build report `E1.md`) | open (needs GPU batch remesh with prop masked) |
| 20 | Lighthouse still S1's primitive tower; signal tower Hunyuan 5/10 up close; gulls primitive | assets | (build report `E1.md`) | open |
| 21 | Player hull texture sharpness up close (detail normal layer proposed) | look | (build report `R1.md`) | open |
| 22 | FPS ~33 median on a loaded machine (GPU-bound), ~59 on a quiet machine (P7); never a clean 60 re-measure since P7 beyond E1 (60.0) | perf | [src SPEC.md:89]; (build report `P7.md`) | open: re-bench with no other app on the GPU (`build/tools/e1openbench.sh`) |
| 23 | Island view uses ~93% of the GPU budget (p90 `17.2 ms`); further look work near islands needs a profile first | perf | (build report `E1.md`) | open |
| 24 | Water bright V seam in zoomed-out views (tile edge) | look | (build report `H1.md`) | P5c raised extents to 300 km; seam reported gone by vision (build report `P5c.md`) |
| 25 | Ironclad circle `208.8 s` (R 330 m) outside the original 3-4 min brief range at 3.5 min; frigate `192` s, cutter `32.9` s outside 3-4 min | handling | (build report `P8.md`); (build report `P7.md`) | decision pending: raise MinTurnRadiusM or lower YawResponse if Chuck wants slower |
| 26 | Substepping affects every physics body; airship/NPC fleets not re-timed | handling | (build report `P7.md`) | open |
| 27 | Collision is outline-vs-bounds (ellipses/boxes): concave rocks can leave visible gaps of a few metres; side-view captures unreliable | collision | (build report `P9.md`) | open |
| 28 | AGROUND/SHOAL toast drawn by the HUD is not visible in photo-mode shots | test | (build report `P9.md`) | open (test limitation) |
| 29 | Autopilot orbit entry cuts inside hazard rings (~70 m inside Needles ring, 176 m from lighthouse centre) | navigation | (build report `A1.md`) | open; P9 widened island hazard circles to cover the shelf (build report `P9.md`) |
| 30 | Give-way is a fixed starboard swerve plus slow; no crossing-track prediction; chart labels crowd around Greyhaven | navigation | (build report `A1.md`) | open |
| 31 | Merchants still use the 4-point kinematic float; Goliath/Aglae hull profiles ready but not wired | ships | (build report `W1.md`) | open (W1 note) |
| 32 | Shipyard has no silhouette/preview image; waterline drafts empirical per hull | UX/ships | (build report `S2.md`) | open; W3 re-set drafts against the true sea (build report `W3.md`) |
| 33 | Packaged FPS not re-benchmarked after the frigate rig added ~20k vertices | perf | (build report `S2.md`) | open |
| 34 | Dawn/dusk zenith dark at the exact blue-hour start; dawn and dusk differ | look | (build report `P13.md`) | open |
| 35 | Weather drift never soaked for a full 48 min day (only first states run) | verification | (build report `P11.md`) | open |
| 36 | Combat-era leftovers: no armor zones/AP/HE, one hull box, NPC always knows player inside 9 km, enemy can sink an idle player; shells pass through merchants | combat (parked) | (build report `C.md`); (build report `D.md`) | parked |
| 37 | Hold-fire release by hit/range verified only in code, idle run covers the 60 s timer | verification (combat) | (build report `P1.md`) | parked |
| 38 | `build/tools/luma.sh` truncates YAVG (sed strips digits); earlier cards' luma lines unreliable | tooling | (phase review) | open per Phase-review; the P4 log says luma.sh still has the YAVG sed bug (build report `P4.md`) |
| 39 | Editor Python from the editor build cannot see the live editor via UDP remote; use MCP or headless script | tooling | (build report `A.md`) | known |
| 40 | Bundle id was `com.YourCompany.IronwakeTest`; P5 set `io.github.charles-forsyth.ironwake` so saves/settings reset for new builds | packaging | (build report `D.md`); [src build/MORNING_REPORT_2.md:3] | fixed in P5 |
| 41 | No promoted build: `Ironwake.app` is the W3 water-fix build (Nov 10 10:01), newer cards each ship `Ironwake_<card>.app`; promotion gate never passed since | release | [src SPEC.md:63]; [src docs/spec/_apps.txt:1] | open: decide whether P14 promotes |
| 42 | World-scale roadmap not started: World Partition + streamed ocean (W-STRUCT), port kit + town builder, trade, zones | roadmap | [src WORLD_PLAN.md:39-44] | open |
| 43 | Poly Haven town textures and library GLBs live outside the repo/git-ignored; a clean clone cannot regenerate assets without them | reproducibility | [src .gitignore:16-25] | open |
| 44 | Credit gap: `Wooden Pier_22_MB` (Mehdi Shahsavan, CC-BY) absent from `build/CREDITS.txt`; mesh `WoodenPier` unreferenced in code | licence | (see `CREDITS.md`) | open (see ch. 12) |

Note on item 25: BRIEF/DESIGN asks big ships 30-60 s to full speed [src build/BRIEF.md:47]; H1 changed acceleration (`STOP -> HALF AHEAD 90% 10.0 s`) (build report `H1.md`). Chuck asked for tight handling at 09:58 [src build/BRIEF.md:145]. CONFLICT between original "30-60 s" design text and measured values: measured value takes effect.

## 15.2 Parked: not blockers

| Item | Parked by | State | Source |
|---|---|---|---|
| Open-sea storm look (waves, whitecaps, foam, spume, breaking crests) | Chuck 2026-11-10 12:55 | systems built; look 4-6/10; W4 not promoted | [src build/BRIEF.md:62], (build report `W4.md`) |
| What would move it (if un-parked): breaking-crest ribbons on Jacobian minima, wind-driven spume ribbon sheets, real reticulated foam texture | W4 log | structural, not tuning | (build report `W4.md`) |
| Combat polish (P2: enemy readability, kill landing) | Chuck 2026-11-09 08:45 | not run | [src build/BRIEF.md:138] |
| Storm bench (STORM FPS on idle GPU) | W2/W4 | skipped, another Unreal Editor instance open | (build report `W4.md`) |

## 15.3 Known gaps by origin

### SPEC.md section 9 (orchestrator, 2026-11-11 10:30)

| # | Gap | Source |
|---|---|---|
| 1 | Town hillside hard uniform wedge | [src SPEC.md:92] |
| 2 | GPU batch batch 2: rebuild thin parts on 9 fix models, chandlery + horse wagon, 3 rejects built procedurally | [src SPEC.md:93] |
| 3 | Squall from high camera reads flat without crest foam | [src SPEC.md:94] |
| 4 | In-game audio recording at 13 kn (needs Chuck away) | [src SPEC.md:95] |
| 5 | Impact SFX reused creak, frigate boot-top band, plateau flat top | [src SPEC.md:96] |
| 6 | Gameplay: trade/cargo/economy stubs, combat parked, world is one port | [src SPEC.md:97] |

### Phase-review review top 5 (P6) and what happened

| Phase-review item | Card suggested | Result |
|---|---|---|
| 1 ship foam broken, not speed-scaled | P5b ladder, wake side ribbon pinned to hull half-breadth, shadow receive off (phase review) | P5b/P5c done; "pinning" and "shadow receive off" listed NOT done in P5b (build report `P5b.md`) |
| 2 nothing to do on arrival | arrival verb per place type + logbook (phase review) | not started |
| 3 turning depends on frame rate | substepping 1/60, 6 substeps; test at `t.MaxFPS 20` vs 60 (phase review) | done in P7: (build report `P7.md`) |
| 4 shorelines on glass | I1 surf ribbon + shelf + wet band on every island (phase review) | partly; I1 had already added them, review says 5 of 18 lacked |
| 5 45 fps not 60, GPU-bound | frame-budget pass, 16 ms GPU target (phase review) | open; bench rule: no editor, `open -n`, 500 frames, interleaved |

### MORNING_REPORT (card E) top 5 and what happened

| Item | Result |
|---|---|
| 1 fix opening and death loop | done in P1 (7.5 km opening, hold fire, SHIP LOST card) (build report `P1.md`) |
| 2 make the enemy readable | P2 parked; never run [src build/BRIEF.md:138] |
| 3 make hits and sink land | P2 parked |
| 4 handling and aim polish | H1 handling, camera over the bow in P1, funnel smoke P5b/P5c/P7 |
| 5 sea and look pass | W-series, P3, P5b-P5c; storm parked |

### Pitfalls recorded (re-hit risk)

| Pitfall | Source |
|---|---|
| UBT can leave `UnrealEditor.modules` pointing at an old dylib after a rebuild: delete `libUnrealEditor-IronwakeTest-*.dylib` and the modules file, then build; check `InternalLoadLibrary` in the log | (build report `B.md`); (build report `P1.md`) |
| Blueprint children keep serialized component values; C++ constructor changes do not reach `BP_PlayerShip`: set on `Default__BP_PlayerShip_C` | (build report `B.md`) |
| Content loaded only by path string is invisible to the cooker; use hard refs on the CDO or `DirectoriesToAlwaysCook` | (build report `D.md`) |
| `-archive` on Mac copied a bare binary `.app` that crashes on missing `libtbb.12.dylib`; the real package is `Saved/StagedBuilds/Mac/IronwakeTest.app` copied with `ditto` | (build report `D.md`) |
| Never delete a referenced asset (ensure in ObjectTools.cpp:4051 killed the editor); edit water nodes in place | [src build/BRIEF.md:88-89] |
| Stale autosaves trigger a "Restore Packages" modal that blocks MCP: `rm -rf Saved/Autosaves/*` first | [src build/BRIEF.md:85-87] |
| Function-local `static UMaterialInterface*` dangles after map reload (shipyard LAUNCH crash); use a weak pointer | (build report `S2.md`) |
| Launching the binary directly from a shell makes capped runs read ~18 FPS; use `open -n` | (build report `H1.md`) |
| Keyboard input reaches a focused test window and spoils unattended runs; test directors must clear mappings | (build report `A1.md`) |
| Niagara chimney smoke ignores component scale (dark pillars); use sphere plumes | (build report `P12.md`) |
| `SmoothStep(A, B, x)` needs A < B (the P11 lamp inversion) | (build report `P12.md`) |
| Wait until `lsof -iTCP:8001 -sTCP:LISTEN` is empty before relaunching the editor (HttpListener bind failure) | (build report `P1.md`) |
| `bUseUnity = false` needed after I1 (anonymous-namespace name clashes) | (build report `I1.md`) |

## 15.4 Verification debts

| Debt | Where it was owed | Measure with |
|---|---|---|
| Clean-machine 60 FPS check | W1, A1, I1, P3, P5 each carried it; E1 measured 60.0 on a quiet machine (build report `E1.md`); later cards read ~33 loaded (build report `P8.md`) | packaged, `open -n`, editor closed, interleaved A/B (`build/tools/e1openbench.sh`) |
| Human play of packaged build | H1, R1, P5, P6 | Chuck at keyboard and mouse; the Mac screen was locked during P5/P6 so screencaptures were black (phase review) |
| Pause menu in packaged build | closed by P6 (MUSIC: OFF seen) (phase review) | n/a |
| Default mode on a cold start | P6 could not confirm (tour spawns the enemy) (phase review) | fresh container launch without `-iwtour`; code default is Peaceful [src Source/IronwakeTest/IronwakePlayerController.h:108] |
| Gamepad and R damage control | card E and H1 | hardware pad + packaged build |
| Wreck waterline foam ring at 300 m | E1 | needs a no-depth-fade foam material (build report `E1.md`) |

## 15.5 Key numbers behind the open items (machine-checked)

| Item | Value | Source |
|---|---|---|
| 25 | ironclad circle after P8 | `208.8` (build report `P8.md`) |
| 25 | frigate circle after P7 | `192.1` (build report `P7.md`) |
| 25 | cutter circle after P7 | `32.9` (build report `P7.md`) |
| 22 | quiet-machine FPS P7 | `59.0` (build report `P7.md`) |
| 22 | loaded-machine median P8 | `33.1` (build report `P8.md`) |
| 23 | island view GPU p90 (ms) | `17.2` (build report `E1.md`) |
| 6 | bow loop peak over median (dB) | `+3.6` (build report `P11.md`) |
| 14 | clear noon, worker vs orchestrator | `5.5 (orchestrator)` [src SPEC.md:119] |
| 12 | default view vision score | `3.5-4/10` (build report `H1.md`) |
| 15 | squall high score | `4/10` (build report `P14.md`) |
| 2 | hull-side FX vision (P3) | `0-2/10` (build report `P3.md`) |
| 27 | collision solids | `1108` (build report `P9.md`) |
| 5 | Phase-review frigate/cutter boot-top clearance cutter (m) | `+0.23 m` [src build/MORNING_REPORT_2.md:20] |
| 41 | Ironwake.app timestamp | `Nov 10 10:01` [src docs/spec/_apps.txt:1] |

## 15.6 Per-log gap quotes with the numbers they cite

Each row is a number a future card must beat or re-measure, copied from the log that recorded the gap.

| Log | Gap | Number |
|---|---|---|
| A | storm reads as swell not storm; look score | `6/10` (build report `A.md`) |
| B | heel in turns small (deg) | `2-3 deg` (build report `B.md`) |
| C | enemy HP loss on player in a 60 s duel | `160 → 80 HP` (build report `C.md`) |
| D | island through fog score | `4/10` (build report `D.md`) |
| S1 | weather/time score | `3-4/10` (build report `S1.md`) |
| S2 | extra vertices from procedural rig | `~20k` (build report `S2.md`) |
| H1 | HIGH preset FPS by design | `34 FPS` (build report `H1.md`) |
| R1 | decimated airship close-up score | `4/10` (build report `R1.md`) |
| W1 | bow dig vision score | `0-1/10` (build report `W1.md`) |
| W2 | storm best chase frame | `3.5/10` (build report `W2.md`) |
| W3 | whitecap coverage vs target | `65 %` (build report `W3.md`) |
| W4 | storm promotion gate | `>= 6` (build report `W4.md`) |
| I1 | 5-island view fps (loaded) | `22.7-24.0` (build report `I1.md`) |
| P3 | near-white share old voyage_03 | `10.5 %` (build report `P3.md`) |
| P5c | frigate chase wake score range | `3-7` (build report `P5c.md`) |
| P8 | town vision score | `1-2/10` (build report `P8.md`) |
| P9 | collision gap note | `few metres` (build report `P9.md`) |
| P10 | town day chase | `3.5/10` (build report `P10.md`) |
| P11 | dawn/dusk sky colour | `4-5/10` (build report `P11.md`) |
| P12 | day town at 400 m | `3.5-4/10` (build report `P12.md`) |
| P13 | night before resume | `3.5-5.5` (build report `P13.md`) |
| P14 | squall high | `4/10` (build report `P14.md`) |

## 15.7 Open items after cards P15..P23

The following items are carried forward as open after cards P15..P23:
- NPC soak not clean: collisions 28-48 / stuck 16-33 on 3 seeds at x8 3000 s (target 0/0).
- Gig oars still clip thwarts in some frames and rowers are placeholders.
- Patrol airship prop strut has leftover black faces.
- Harbour reads empty from aerial views.
- Town 400 m noon scored 4.5 (worker) / 6 (orchestrator) vs target 7 (silhouette dense low band; roads 1-2 px at 400 m; anchor changes, lighthouse, quay-front variety, wind-bent trees not done; street-view window planes floating).
- Wake/foam look: prop wash remains a solid band (2-4/10), Kelvin wedge lines 3/10 against real photos.
- Tall brig model exists in `$IRONWAKE_ASSETS/models/parts/b2_brig_hull_rigged.glb` (not imported).
- Paddle steamer wheels and salvage tender crane do not move.
- New hull models (tug, coastal steamer, pilot cutter) imported in P21.
- Moving-part models in `models/parts/` (cargo airship props, windmill sails, gig oars, patrol props).
- Combat and storm look remain parked.

## 15.8 Updates after cards P24..P30 (2026-11-13)
- **CLOSED (NPC soak):** P25 harbour-mouth one-way channel + quay-lane & waiting-slot arbitrator achieved literal `IWNPC collisions=0 stuck=0 groundings=0` (build report `P25.md`) across all 3 seeds (`3000` game-s at x8, `30` ledger ships, `14-16` moving), preserved in P26 with 4 rigged Tall Brigs (`SEED 1: IWNPC collisions=0 stuck=0 groundings=0` (build report `P26.md`), `moving_parts=119` (build report `P26.md`)); P30 6-port multi-port + gale 3-seed soak recorded `IWNPC collisions=0 stuck=1 groundings=0` (build report `P30.md`), `IWNPC collisions=1 stuck=0 groundings=0` (build report `P30.md`), `IWNPC collisions=2 stuck=3 groundings=0` (build report `P30.md`) (`0` groundings across all seeds).
- **CLOSED (TallBrig 17-part rig + player Shipyard):** P26 imported `SM_TallBrig` + 17 moving spar/sail parts incl. gaff spanker, wired playable `Curlew` (42 m, 480 t) into the 4-ship Shipyard and upgraded all 4 NPC brigs with standing shrouds, WPO wind sails (`M_IW_SailCanvas`), and harbour reduced canvas (`Vision 8.0–10/10` (build report `P26.md`), `+2.05 FPS` (build report `P26.md`)).
- **CLOSED (street floating windows & headland lighthouse):** P24 added SW headland lighthouse (`lighthouse` collision + night lamp) and line-traced facade windows (`street 7.5` (build report `P24.md`)); upper town bands 3-5 (`noon_400 4` (build report `P24.md`)), rotating lighthouse beam, and stilt huts remain open.
- **CLOSED (prop-wash & Kelvin-wedge LUT + hull-climbing foam + camera-centred NPC wakes):** P27 replaced P19 analytic ship-wave trig loops and solid `M_IW_Surf2` wake band with 7-ship GPU batch Havelock `T_IW_KelvinWaveLut` + `T_IW_PropWashFoamLut` + seamless 3-octave Voronoi `T_IW_CellularFoamTile` + turquoise SSS + lit `M_IW_WakeLut` (`+6` cm lift, `d_outboard >= 0` hull exclusion) and measured `FxRadiusCm` / 7-ship wake distance from `PlayerCameraManager` (`8.5 / 10` (build report `P27.md`) prop wash & Kelvin wake, `9.5 / 10` (build report `P27.md`) sail-vs-steam / hull waterline / speed & NPC6, `+1.75 FPS` (build report `P27.md`)).
- **CLOSED (moving paddle/crane/bell/airship parts, 3D crew/rowers/gulls, b4 buildings+signs, 400 m town & triplanar 4K ground):** P28 wired `PaddleSteamerV3` spinning wheels (`8.5 / 10` (build report `P28.md`)), `SalvageTenderV3` slewing derrick + pendulum bell (`9.5 / 10` (build report `P28.md`)), clean `PatrolAirshipV4` 3 props (`7.5 / 10` (build report `P28.md`)), 3D `SailorCrewRower` 4-phase gig stroke (`9.5 / 10` (build report `P28.md`)), `SeaGullV4` flapping wings, `SailorCrewStand` helmsmen & HISM dockhands, `TownTavernV4`/`TownChandleryV4`/`TownFoundryV4` + painted signs (`9.5 / 10` (build report `P28.md`)), 400 m noon town (`8.5 / 10` (build report `P28.md`)), live occupied-only mooring hawsers, and 3-axis triplanar 4K Megascans ground (`+1.85 FPS` (build report `P28.md`) vs P27).
- **CLOSED (multi-port world `world_ports.json`, arrival Harbourmaster diorama, 8-good trade market & Captain's Logbook — Phase-review item #8):** P29 added data-driven secondary working ports **Ironcrag** (`7.8 / 10` (build report `P29.md`) noon 400 m, `8.5 / 10` (build report `P29.md`) night 400 m) and **Stigsby** (`7.8 / 10` (build report `P29.md`) noon 400 m, `10 / 10` (build report `P29.md`) 140 m cove), 3-port M-chart (`9.5 / 10` (build report `P29.md`)), `< 2.5` kn `Enter` mooring & wreck-salvage verb, live Harbourmaster diorama + 8-good cargo market (`10 / 10` (build report `P29.md`)), persisted `SilverCrowns` + Captain's Logbook (`9.8 / 10` (build report `P29.md`)), `SEED 1: IWNPC collisions=0 stuck=0 groundings=0` (build report `P29.md`), and `-0.20 FPS` (build report `P29.md`) vs P28.
- **CLOSED (promotion gate & clean-machine bench — item #22 & #41):** P30 verified `20 / 20` (build report `P30.md`) clean packaged launches, `53.10` (build report `P30.md`) FPS MEDIUM (`+1.15 FPS` (build report `P30.md`) vs P19, `+18.15 FPS` (build report `P30.md`) vs W3), backed up `Ironwake_W3_prev.app`, promoted `Ironwake_P29.app` -> `Packaged/Ironwake/Ironwake.app` (`Promoted Ironwake.app smoke rc=0` (build report `P30.md`)), and delivered `build/MORNING_REPORT_3.md`.
- **OPEN (minor P30 polish notes & parked items):** Stigsby night tree-crown shading (`3.5 / 10` (build report `P30.md`) isolated vs `7.5 / 10` (build report `P30.md`) sheet) + reversed lateral beacons; Ironcrag `145` m slope cones (`5.5 / 10` (build report `P30.md`) isolated); POM cobbles at `1.5` m eye height (`4.0/10` (build report `P30.md`)); `TallBrig` quarterdeck `HelmsmanPivot` Z offset; `SM_Frigate_Hull` unpainted black boot-top; combat (`P2`) and open-sea storm look (`W4`) remain parked.

## Gaps / unknown

- This chapter did not re-run any test; "open" means no later log closes the item. Any item could have been fixed in a commit with no log text. Check with `git log --since` on the owning file (chapter 13 lists owners).
- Priorities are the author's ordering from the key in the scope paragraph, not Chuck's. Chuck's explicit priority statements are in chapter 1 section 1.8.
- Frame-time budget per feature is not recorded anywhere except H1's GPU table (build report `H1.md`) and P5's top-cost list [src build/MORNING_REPORT_2.md:18]; a current GPU profile (`stat gpu` / CSV profiler) is unknown.
- Number of Chuck-visible regressions between W3 (`Ironwake.app`) and P14 is unknown: no side-by-side play log exists.
