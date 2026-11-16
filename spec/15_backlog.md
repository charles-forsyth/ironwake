# Chapter 15: Known issues and backlog

Scope: every open problem recorded in the card logs' known-gaps sections, the Opus review (`build/REVIEW_opus.md`), MORNING_REPORT and MORNING_REPORT_2, SPEC.md section 9 and the BRIEF hard rules, merged, de-duplicated and ordered. Order key: (1) hard-rule violations and things Chuck complained about directly, (2) things a first-time player hits first, (3) look/readability misses against stated targets, (4) verification debts, (5) content and gameplay depth, (6) tooling and housekeeping. Items Chuck parked (storm look, combat) are listed last in their own section and must not be treated as blockers [src build/BRIEF.md:62]. Each item names its source, whether it is still open as of the D1 snapshot (HEAD `84c9efa`), and where to measure or fix it. "Open" means no later log or commit closes it; this chapter did not launch the game to re-check, so some may be fixed without a log entry.

## 15.1 Priority list

| # | Item | Class | Source | Status |
|---|---|---|---|---|
| 1 | Town hillside is a hard uniform wedge with flat blocky ground and no paths; plateau flat top visibly straight | look/world | [src SPEC.md:92]; [src build/logs/P14.md:13] | open (P14: heightfield rebuild "outside this card") |
| 2 | Ship FX still weakest on-screen: flat foam band parallel to the camera, hull-side frames scored 0-2/10 (P3), pass rule not met (P5b, P5c) | look (Chuck complaint) | [src build/logs/P3.md:18]; [src build/logs/P5b.md:10]; [src build/logs/P5c.md:15] | partly fixed in P5c (~8 at 13 kn per SPEC); frigate chase wake unstable 3-7 |
| 3 | Frigate (default hull) chase camera: aux-stack smoke hidden behind sails | look | [src build/logs/P5c.md:18] | P7 moved the stack (-23 m, 23 m) scale 0.9; vision after = soft pale column [src build/logs/P7.md:27] |
| 4 | Frigate has no visible boot-top band (black hull to waterline) | look | [src build/REVIEW_opus.md:22]; [src build/MORNING_REPORT_2.md:20] | open; SPEC.md lists "frigate boot-top band" under gap 5 [src SPEC.md:97] |
| 5 | Cutter rides a little light | look | [src build/REVIEW_opus.md:46] | open (drafts re-set in W3 [src build/logs/W3.md:16]) |
| 6 | In-game audio never recorded at 13 kn; hum fix verified only offline (+3.6/+5.3/+1.1 dB over median) | verification | [src build/logs/P11.md:6]; [src SPEC.md:96] | open: needs Chuck away from the Mac (BRIEF audio rule [src build/BRIEF.md:10-12]) |
| 7 | Impact SFX is a reused creak; no scrape/impact sample | audio | [src build/logs/P9.md:6]; [src build/logs/P9.md:23] | open |
| 8 | Nothing to do on arrival: 18 places are waypoints with a banner; no docking, salvage, logbook; trade/cargo/economy are stubs | gameplay | [src build/REVIEW_opus.md:27]; [src SPEC.md:98] | open; REVIEW card suggested one arrival verb per place type |
| 9 | Shoreline integration: 5 of 18 places lacked a surf line, none a lighter shelf ring (Opus); I1 says surf patchy | look | [src build/REVIEW_opus.md:41] | partly open; I1 later added shelf `M_IW_Shelf` [src build/logs/I1.md:29] |
| 10 | Keybind strip and dev-style status lines read as debug text to a first-time player | UX | [src build/REVIEW_opus.md:48] | partly addressed in P11 (hint line + help panel, dev lines only with -iwdebug [src build/logs/P11.md:15]) |
| 11 | Menus plain: default font, flat card; period serif and brass framing proposed | UX | [src build/logs/D.md:83]; [src build/logs/P1.md:55] | open |
| 12 | Default camera view scores 3.5-4/10 against WoWS; funnel hides foredeck from dead astern | UX | [src build/logs/H1.md:70] | open (Chuck may want a higher default pitch) |
| 13 | Manual keyboard/mouse play never done by a human in logs: SHIFT scope, C free look, wheel, held A/D, R damage control, gamepad | verification | [src build/logs/H1.md:80]; [src build/MORNING_REPORT.md:52] | open |
| 14 | Clear/fair noon: worker 7.5-8.5 vs orchestrator 5.5 (disagreement); town still reads flat from sea | look | [src SPEC.md:84]; [src build/logs/P14.md:7] | open (re-score by orchestrator needed) |
| 15 | Squall from high camera reads flat (4/10): no crest foam (parked look) | look (parked) | [src build/logs/P14.md:9] | parked |
| 16 | Harbour at 650 m reads partly as a flat block; Wreck at 300 m small and dark | look | [src build/logs/E1.md:72-73] | open |
| 17 | Fort walls read as grey blocks from 400 m; trees dark/sparse at range; surf ribbons look like stamped rings from high angles | look | [src build/logs/I1.md:70-72] | open |
| 18 | Ursa batch 2: rebuild thin parts on the 9 `fix` models; generate chandlery + horse wagon; build fence and street lamp procedurally; fishing smack hull + new rig | assets | [src SPEC.md:93]; [src [REDACTED]/Projects/game-concepts/ironwake/models/cluster_b1/SCORES.json:2-6] | open |
| 19 | Airship: baked-in prop under R1's separate spinning prop; three Blender cuts tore the duct ring | assets | [src build/logs/E1.md:71] | open (needs Ursa remesh with prop masked) |
| 20 | Lighthouse still S1's primitive tower; signal tower Hunyuan 5/10 up close; gulls primitive | assets | [src build/logs/E1.md:74-76] | open |
| 21 | Player hull texture sharpness up close (detail normal layer proposed) | look | [src build/logs/R1.md:57] | open |
| 22 | FPS ~33 median on a loaded machine (GPU-bound), ~59 on a quiet machine (P7); never a clean 60 re-measure since P7 beyond E1 (60.0) | perf | [src SPEC.md:89]; [src build/logs/P7.md:11] | open: re-bench with no other app on the GPU (`build/tools/e1openbench.sh`) |
| 23 | Island view uses ~93% of the GPU budget (p90 `17.2 ms`); further look work near islands needs a profile first | perf | [src build/logs/E1.md:61] | open |
| 24 | Water bright V seam in zoomed-out views (tile edge) | look | [src build/logs/H1.md:84] | P5c raised extents to 300 km; seam reported gone by vision [src build/logs/P5c.md:10] |
| 25 | Ironclad circle `208.8 s` (R 330 m) outside the original 3-4 min brief range at 3.5 min; frigate `192` s, cutter `32.9` s outside 3-4 min | handling | [src build/logs/P8.md:22]; [src build/logs/P7.md:23] | decision pending: raise MinTurnRadiusM or lower YawResponse if Chuck wants slower |
| 26 | Substepping affects every physics body; airship/NPC fleets not re-timed | handling | [src build/logs/P7.md:31] | open |
| 27 | Collision is outline-vs-bounds (ellipses/boxes): concave rocks can leave visible gaps of a few metres; side-view captures unreliable | collision | [src build/logs/P9.md:25] | open |
| 28 | AGROUND/SHOAL toast drawn by the HUD is not visible in photo-mode shots | test | [src build/logs/P9.md:23] | open (test limitation) |
| 29 | Autopilot orbit entry cuts inside hazard rings (~70 m inside Needles ring, 176 m from lighthouse centre) | navigation | [src build/logs/A1.md:45] | open; P9 widened island hazard circles to cover the shelf [src build/logs/P9.md:7] |
| 30 | Give-way is a fixed starboard swerve plus slow; no crossing-track prediction; chart labels crowd around Greyhaven | navigation | [src build/logs/A1.md:47-48] | open |
| 31 | Merchants still use the 4-point kinematic float; Goliath/Aglae hull profiles ready but not wired | ships | [src build/logs/W1.md:82] | open (W1 note) |
| 32 | Shipyard has no silhouette/preview image; waterline drafts empirical per hull | UX/ships | [src build/logs/S2.md:57-58] | open; W3 re-set drafts against the true sea [src build/logs/W3.md:16] |
| 33 | Packaged FPS not re-benchmarked after the frigate rig added ~20k vertices | perf | [src build/logs/S2.md:60] | open |
| 34 | Dawn/dusk zenith dark at the exact blue-hour start; dawn and dusk differ | look | [src build/logs/P13.md:19] | open |
| 35 | Weather drift never soaked for a full 48 min day (only first states run) | verification | [src build/logs/P11.md:32] | open |
| 36 | Combat-era leftovers: no armor zones/AP/HE, one hull box, NPC always knows player inside 9 km, enemy can sink an idle player; shells pass through merchants | combat (parked) | [src build/logs/C.md:76-77]; [src build/logs/D.md:84] | parked |
| 37 | Hold-fire release by hit/range verified only in code, idle run covers the 60 s timer | verification (combat) | [src build/logs/P1.md:38] | parked |
| 38 | `build/tools/luma.sh` truncates YAVG (sed strips digits); earlier cards' luma lines unreliable | tooling | [src build/REVIEW_opus.md:49] | open per Opus; the P4 log says luma.sh still has the YAVG sed bug [src build/logs/P4.md:15] |
| 39 | Editor Python from the editor build cannot see the live editor via UDP remote; use MCP or headless script | tooling | [src build/logs/A.md:28] | known |
| 40 | Bundle id was `com.YourCompany.IronwakeTest`; P5 set `com.omstead.ironwake` so saves/settings reset for new builds | packaging | [src build/logs/D.md:91]; [src build/MORNING_REPORT_2.md:3] | fixed in P5 |
| 41 | No promoted build: `Ironwake.app` is the W3 water-fix build (Nov 10 10:01), newer cards each ship `Ironwake_<card>.app`; promotion gate never passed since | release | [src SPEC.md:63]; [src docs/spec/_apps.txt:1] | open: decide whether P14 promotes |
| 42 | World-scale roadmap not started: World Partition + streamed ocean (W-STRUCT), port kit + town builder, trade, zones | roadmap | [src [REDACTED]/Projects/game-concepts/ironwake/WORLD_PLAN.md:39-44] | open |
| 43 | Poly Haven town textures and library GLBs live outside the repo/git-ignored; a clean clone cannot regenerate assets without them | reproducibility | [src .gitignore:16-25] | open |
| 44 | Credit gap: `Wooden Pier_22_MB` (Mehdi Shahsavan, CC-BY) absent from `build/CREDITS.txt`; mesh `WoodenPier` unreferenced in code | licence | [src [REDACTED]/Projects/game-concepts/ironwake/models/world/CREDITS.md:30] | open (see ch. 12) |

Note on item 25: BRIEF/DESIGN asks big ships 30-60 s to full speed [src build/BRIEF.md:47]; H1 changed acceleration (`STOP -> HALF AHEAD 90% 10.0 s`) [src build/logs/H1.md:41-42]. Chuck asked for tight handling at 09:58 [src build/BRIEF.md:145]. CONFLICT between original "30-60 s" design text and measured values: measured value takes effect.

## 15.2 Parked: not blockers

| Item | Parked by | State | Source |
|---|---|---|---|
| Open-sea storm look (waves, whitecaps, foam, spume, breaking crests) | Chuck 2026-11-10 12:55 | systems built; look 4-6/10; W4 not promoted | [src build/BRIEF.md:62], [src build/logs/W4.md:3] |
| What would move it (if un-parked): breaking-crest ribbons on Jacobian minima, wind-driven spume ribbon sheets, real reticulated foam texture | W4 log | structural, not tuning | [src build/logs/W4.md:25-30] |
| Combat polish (P2: enemy readability, kill landing) | Chuck 2026-11-09 08:45 | not run | [src build/BRIEF.md:138] |
| Storm bench (STORM FPS on idle GPU) | W2/W4 | skipped, [REDACTED] open | [src build/logs/W4.md:22] |

## 15.3 Known gaps by origin

### SPEC.md section 9 (orchestrator, 2026-11-11 10:30)

| # | Gap | Source |
|---|---|---|
| 1 | Town hillside hard uniform wedge | [src SPEC.md:92] |
| 2 | Ursa batch 2: rebuild thin parts on 9 fix models, chandlery + horse wagon, 3 rejects built procedurally | [src SPEC.md:93] |
| 3 | Squall from high camera reads flat without crest foam | [src SPEC.md:94] |
| 4 | In-game audio recording at 13 kn (needs Chuck away) | [src SPEC.md:95] |
| 5 | Impact SFX reused creak, frigate boot-top band, plateau flat top | [src SPEC.md:96] |
| 6 | Gameplay: trade/cargo/economy stubs, combat parked, world is one port | [src SPEC.md:97] |

### Opus review top 5 (P6) and what happened

| Opus item | Card suggested | Result |
|---|---|---|
| 1 ship foam broken, not speed-scaled | P5b ladder, wake side ribbon pinned to hull half-breadth, shadow receive off [src build/REVIEW_opus.md:38] | P5b/P5c done; "pinning" and "shadow receive off" listed NOT done in P5b [src build/logs/P5b.md:9] |
| 2 nothing to do on arrival | arrival verb per place type + logbook [src build/REVIEW_opus.md:39] | not started |
| 3 turning depends on frame rate | substepping 1/60, 6 substeps; test at `t.MaxFPS 20` vs 60 [src build/REVIEW_opus.md:40] | done in P7: [src build/logs/P7.md:15] |
| 4 shorelines on glass | I1 surf ribbon + shelf + wet band on every island [src build/REVIEW_opus.md:41] | partly; I1 had already added them, review says 5 of 18 lacked |
| 5 45 fps not 60, GPU-bound | frame-budget pass, 16 ms GPU target [src build/REVIEW_opus.md:42] | open; bench rule: no editor, `open -n`, 500 frames, interleaved |

### MORNING_REPORT (card E) top 5 and what happened

| Item | Result |
|---|---|
| 1 fix opening and death loop | done in P1 (7.5 km opening, hold fire, SHIP LOST card) [src build/logs/P1.md:5-14] |
| 2 make the enemy readable | P2 parked; never run [src build/BRIEF.md:138] |
| 3 make hits and sink land | P2 parked |
| 4 handling and aim polish | H1 handling, camera over the bow in P1, funnel smoke P5b/P5c/P7 |
| 5 sea and look pass | W-series, P3, P5b-P5c; storm parked |

### Pitfalls recorded (re-hit risk)

| Pitfall | Source |
|---|---|
| UBT can leave `UnrealEditor.modules` pointing at an old dylib after a rebuild: delete `libUnrealEditor-IronwakeTest-*.dylib` and the modules file, then build; check `InternalLoadLibrary` in the log | [src build/logs/B.md:57-59]; [src build/logs/P1.md:49] |
| Blueprint children keep serialized component values; C++ constructor changes do not reach `BP_PlayerShip`: set on `Default__BP_PlayerShip_C` | [src build/logs/B.md:60-62] |
| Content loaded only by path string is invisible to the cooker; use hard refs on the CDO or `DirectoriesToAlwaysCook` | [src build/logs/D.md:89] |
| `-archive` on Mac copied a bare binary `.app` that crashes on missing `libtbb.12.dylib`; the real package is `Saved/StagedBuilds/Mac/IronwakeTest.app` copied with `ditto` | [src build/logs/D.md:88] |
| Never delete a referenced asset (ensure in ObjectTools.cpp:4051 killed the editor); edit water nodes in place | [src build/BRIEF.md:88-89] |
| Stale autosaves trigger a "Restore Packages" modal that blocks MCP: `rm -rf Saved/Autosaves/*` first | [src build/BRIEF.md:85-87] |
| Function-local `static UMaterialInterface*` dangles after map reload (shipyard LAUNCH crash); use a weak pointer | [src build/logs/S2.md:46] |
| Launching the binary directly from a shell makes capped runs read ~18 FPS; use `open -n` | [src build/logs/H1.md:27] |
| Keyboard input reaches a focused test window and spoils unattended runs; test directors must clear mappings | [src build/logs/A1.md:46] |
| Niagara chimney smoke ignores component scale (dark pillars); use sphere plumes | [src build/logs/P12.md:15] |
| `SmoothStep(A, B, x)` needs A < B (the P11 lamp inversion) | [src build/logs/P12.md:14] |
| Wait until `lsof -iTCP:8001 -sTCP:LISTEN` is empty before relaunching the editor (HttpListener bind failure) | [src build/logs/P1.md:50] |
| `bUseUnity = false` needed after I1 (anonymous-namespace name clashes) | [src build/logs/I1.md:37] |

## 15.4 Verification debts

| Debt | Where it was owed | Measure with |
|---|---|---|
| Clean-machine 60 FPS check | W1, A1, I1, P3, P5 each carried it; E1 measured 60.0 on a quiet machine [src build/logs/E1.md:58]; later cards read ~33 loaded [src build/logs/P8.md:20] | packaged, `open -n`, editor closed, interleaved A/B (`build/tools/e1openbench.sh`) |
| Human play of packaged build | H1, R1, P5, P6 | Chuck at keyboard and mouse; the Mac screen was locked during P5/P6 so screencaptures were black [src build/REVIEW_opus.md:5] |
| Pause menu in packaged build | closed by P6 (MUSIC: OFF seen) [src build/REVIEW_opus.md:24] | n/a |
| Default mode on a cold start | P6 could not confirm (tour spawns the enemy) [src build/REVIEW_opus.md:58] | fresh container launch without `-iwtour`; code default is Peaceful [src Source/IronwakeTest/IronwakePlayerController.h:108] |
| Gamepad and R damage control | card E and H1 | hardware pad + packaged build |
| Wreck waterline foam ring at 300 m | E1 | needs a no-depth-fade foam material [src build/logs/E1.md:70] |

## 15.5 Key numbers behind the open items (machine-checked)

| Item | Value | Source |
|---|---|---|
| 25 | ironclad circle after P8 | `208.8` [src build/logs/P8.md:22] |
| 25 | frigate circle after P7 | `192.1` [src build/logs/P7.md:20] |
| 25 | cutter circle after P7 | `32.9` [src build/logs/P7.md:21] |
| 22 | quiet-machine FPS P7 | `59.0` [src build/logs/P7.md:11] |
| 22 | loaded-machine median P8 | `33.1` [src build/logs/P8.md:20] |
| 23 | island view GPU p90 (ms) | `17.2` [src build/logs/E1.md:59] |
| 6 | bow loop peak over median (dB) | `+3.6` [src build/logs/P11.md:6] |
| 14 | clear noon, worker vs orchestrator | `5.5 (orchestrator)` [src SPEC.md:119] |
| 12 | default view vision score | `3.5-4/10` [src build/logs/H1.md:70] |
| 15 | squall high score | `4/10` [src build/logs/P14.md:13] |
| 2 | hull-side FX vision (P3) | `0-2/10` [src build/logs/P3.md:18] |
| 27 | collision solids | `1108` [src build/logs/P9.md:5] |
| 5 | Opus frigate/cutter boot-top clearance cutter (m) | `+0.23 m` [src build/MORNING_REPORT_2.md:20] |
| 41 | Ironwake.app timestamp | `Nov 10 10:01` [src docs/spec/_apps.txt:1] |

## 15.6 Per-log gap quotes with the numbers they cite

Each row is a number a future card must beat or re-measure, copied from the log that recorded the gap.

| Log | Gap | Number |
|---|---|---|
| A | storm reads as swell not storm; look score | `6/10` [src build/logs/A.md:17] |
| B | heel in turns small (deg) | `2-3 deg` [src build/logs/B.md:53] |
| C | enemy HP loss on player in a 60 s duel | `160 → 80 HP` [src build/logs/C.md:75] |
| D | island through fog score | `4/10` [src build/logs/D.md:82] |
| S1 | weather/time score | `3-4/10` [src build/logs/S1.md:71] |
| S2 | extra vertices from procedural rig | `~20k` [src build/logs/S2.md:60] |
| H1 | HIGH preset FPS by design | `34 FPS` [src build/logs/H1.md:83] |
| R1 | decimated airship close-up score | `4/10` [src build/logs/R1.md:40] |
| W1 | bow dig vision score | `0-1/10` [src build/logs/W1.md:59] |
| W2 | storm best chase frame | `3.5/10` [src build/logs/W2.md:69] |
| W3 | whitecap coverage vs target | `65 %` [src build/logs/W3.md:50] |
| W4 | storm promotion gate | `>= 6` [src build/logs/W4.md:4] |
| I1 | 5-island view fps (loaded) | `22.7-24.0` [src build/logs/I1.md:66] |
| P3 | near-white share old voyage_03 | `10.5 %` [src build/logs/P3.md:17] |
| P5c | frigate chase wake score range | `3-7` [src build/logs/P5c.md:14] |
| P8 | town vision score | `1-2/10` [src build/logs/P8.md:11] |
| P9 | collision gap note | `few metres` [src build/logs/P9.md:25] |
| P10 | town day chase | `3.5/10` [src build/logs/P10.md:14] |
| P11 | dawn/dusk sky colour | `4-5/10` [src build/logs/P11.md:30] |
| P12 | day town at 400 m | `3.5-4/10` [src build/logs/P12.md:4] |
| P13 | night before resume | `3.5-5.5` [src build/logs/P13.md:16] |
| P14 | squall high | `4/10` [src build/logs/P14.md:13] |

## 15.7 Open items after cards P15..P23

The following items are carried forward as open after cards P15..P23:
- NPC soak not clean: collisions 28-48 / stuck 16-33 on 3 seeds at x8 3000 s (target 0/0).
- Gig oars still clip thwarts in some frames and rowers are placeholders.
- Patrol airship prop strut has leftover black faces.
- Harbour reads empty from aerial views.
- Town 400 m noon scored 4.5 (worker) / 6 (orchestrator) vs target 7 (silhouette dense low band; roads 1-2 px at 400 m; anchor changes, lighthouse, quay-front variety, wind-bent trees not done; street-view window planes floating).
- Wake/foam look: prop wash remains a solid band (2-4/10), Kelvin wedge lines 3/10 against real photos.
- Tall brig model exists in `~/Projects/game-concepts/ironwake/models/parts/b2_brig_hull_rigged.glb` (not imported).
- Paddle steamer wheels and salvage tender crane do not move.
- New hull models (tug, coastal steamer, pilot cutter) imported in P21.
- Moving-part models in `models/parts/` (cargo airship props, windmill sails, gig oars, patrol props).
- Combat and storm look remain parked.

## 15.8 Updates after cards P24..P30 (2026-11-13)
- **CLOSED (NPC soak):** P25 harbour-mouth one-way channel + quay-lane & waiting-slot arbitrator achieved literal `IWNPC collisions=0 stuck=0 groundings=0` [src build/logs/P25.md:22] across all 3 seeds (`3000` game-s at x8, `30` ledger ships, `14-16` moving), preserved in P26 with 4 rigged Tall Brigs (`SEED 1: IWNPC collisions=0 stuck=0 groundings=0` [src build/logs/P26.md:36], `moving_parts=119` [src build/logs/P26.md:36]); P30 6-port multi-port + gale 3-seed soak recorded `IWNPC collisions=0 stuck=1 groundings=0` [src build/logs/P30.md:27], `IWNPC collisions=1 stuck=0 groundings=0` [src build/logs/P30.md:28], `IWNPC collisions=2 stuck=3 groundings=0` [src build/logs/P30.md:29] (`0` groundings across all seeds).
- **CLOSED (TallBrig 17-part rig + player Shipyard):** P26 imported `SM_TallBrig` + 17 moving spar/sail parts incl. gaff spanker, wired playable `Curlew` (42 m, 480 t) into the 4-ship Shipyard and upgraded all 4 NPC brigs with standing shrouds, WPO wind sails (`M_IW_SailCanvas`), and harbour reduced canvas (`Vision 8.0–10/10` [src build/logs/P26.md:7], `+2.05 FPS` [src build/logs/P26.md:42]).
- **CLOSED (street floating windows & headland lighthouse):** P24 added SW headland lighthouse (`lighthouse` collision + night lamp) and line-traced facade windows (`street 7.5` [src build/logs/P24.md:10]); upper town bands 3-5 (`noon_400 4` [src build/logs/P24.md:10]), rotating lighthouse beam, and stilt huts remain open.
- **CLOSED (prop-wash & Kelvin-wedge LUT + hull-climbing foam + camera-centred NPC wakes):** P27 replaced P19 analytic ship-wave trig loops and solid `M_IW_Surf2` wake band with 7-ship Ursa Havelock `T_IW_KelvinWaveLut` + `T_IW_PropWashFoamLut` + seamless 3-octave Voronoi `T_IW_CellularFoamTile` + turquoise SSS + lit `M_IW_WakeLut` (`+6` cm lift, `d_outboard >= 0` hull exclusion) and measured `FxRadiusCm` / 7-ship wake distance from `PlayerCameraManager` (`8.5 / 10` [src build/logs/P27.md:6] prop wash & Kelvin wake, `9.5 / 10` [src build/logs/P27.md:8-10] sail-vs-steam / hull waterline / speed & NPC6, `+1.75 FPS` [src build/logs/P27.md:13]).
- **CLOSED (moving paddle/crane/bell/airship parts, 3D crew/rowers/gulls, b4 buildings+signs, 400 m town & triplanar 4K ground):** P28 wired `PaddleSteamerV3` spinning wheels (`8.5 / 10` [src build/logs/P28.md:13]), `SalvageTenderV3` slewing derrick + pendulum bell (`9.5 / 10` [src build/logs/P28.md:7]), clean `PatrolAirshipV4` 3 props (`7.5 / 10` [src build/logs/P28.md:15]), 3D `SailorCrewRower` 4-phase gig stroke (`9.5 / 10` [src build/logs/P28.md:6]), `SeaGullV4` flapping wings, `SailorCrewStand` helmsmen & HISM dockhands, `TownTavernV4`/`TownChandleryV4`/`TownFoundryV4` + painted signs (`9.5 / 10` [src build/logs/P28.md:8]), 400 m noon town (`8.5 / 10` [src build/logs/P28.md:14]), live occupied-only mooring hawsers, and 3-axis triplanar 4K Megascans ground (`+1.85 FPS` [src build/logs/P28.md:18] vs P27).
- **CLOSED (multi-port world `world_ports.json`, arrival Harbourmaster diorama, 8-good trade market & Captain's Logbook — Opus item #8):** P29 added data-driven secondary working ports **Ironcrag** (`7.8 / 10` [src build/logs/P29.md:12] noon 400 m, `8.5 / 10` [src build/logs/P29.md:10] night 400 m) and **Stigsby** (`7.8 / 10` [src build/logs/P29.md:13] noon 400 m, `10 / 10` [src build/logs/P29.md:9] 140 m cove), 3-port M-chart (`9.5 / 10` [src build/logs/P29.md:8]), `< 2.5` kn `Enter` mooring & wreck-salvage verb, live Harbourmaster diorama + 8-good cargo market (`10 / 10` [src build/logs/P29.md:6]), persisted `SilverCrowns` + Captain's Logbook (`9.8 / 10` [src build/logs/P29.md:7]), `SEED 1: IWNPC collisions=0 stuck=0 groundings=0` [src build/logs/P29.md:25], and `-0.20 FPS` [src build/logs/P29.md:29] vs P28.
- **CLOSED (promotion gate & clean-machine bench — item #22 & #41):** P30 verified `20 / 20` [src build/logs/P30.md:5] clean packaged launches, `53.10` [src build/logs/P30.md:54] FPS MEDIUM (`+1.15 FPS` [src build/logs/P30.md:54] vs P19, `+18.15 FPS` [src build/logs/P30.md:56] vs W3), backed up `Ironwake_W3_prev.app`, promoted `Ironwake_P29.app` -> `~/Games/Ironwake/Ironwake.app` (`Promoted Ironwake.app smoke rc=0` [src build/logs/P30.md:4]), and delivered `build/MORNING_REPORT_3.md`.
- **OPEN (minor P30 polish notes & parked items):** **CLOSED in P31–P33:** Stigsby night tree-crown shading & lateral beacons fixed; Ironcrag basalt crags (`BasaltCrag_1..4`) placed; 4K POM street cobbles (`9.5 / 10` [src build/logs/P33.md:24]); `TallBrig` quarterdeck `HelmsmanPivot` lowered to `Z=750 cm` (`10 / 10` [src build/logs/P33.md:29]); `SM_Frigate_Hull` copper-red boot-top added (`9.5 / 10` [src build/logs/P33.md:28]); archipelago expanded to 6 working ports & 6 playable hulls (`ChapmanSloop`, `AmphionSchooner`). Combat (`P2`) and open-sea storm look (`W4`) remain parked by Chuck's direction.

## Gaps / unknown

- This chapter did not re-run any test; "open" means no later log closes the item. Any item could have been fixed in a commit with no log text. Check with `git log --since` on the owning file (chapter 13 lists owners).
- Priorities are the author's ordering from the key in the scope paragraph, not Chuck's. Chuck's explicit priority statements are in chapter 1 section 1.8.
- Frame-time budget per feature is not recorded anywhere except H1's GPU table [src build/logs/H1.md:11-16] and P5's top-cost list [src build/MORNING_REPORT_2.md:18]; a current GPU profile (`stat gpu` / CSV profiler) is unknown.
- Number of Chuck-visible regressions between W3 (`Ironwake.app`) and P14 is unknown: no side-by-side play log exists.
