# Chapter 14: Card history

Scope: every task card from A through P14 plus D1 (this spec task): goal, outcome, vision scores, commit SHA and packaged app. Commit SHAs are cited to `docs/spec/_gitlog.txt`, a verbatim `git log --format='%h %ad %s'` dump taken 2026-11-11 for this card (D1 itself had only its first commit `84c9efa` at that time). Packaged-app names are cited to `docs/spec/_apps.txt`, a dump of `ls -l Packaged/Ironwake/*.app`. Outcomes and scores are copied from the card logs in `logs/` (which are git-ignored) and the review files. Scores are vision-model scores from the cards themselves and jitter about 1.5 points between identical frames (build report `P5c.md`); they are not independent measurements unless the row says orchestrator or Phase-review.

Order of cards: first night (2026-11-09) A, B, C, D, E; pass 2 (BRIEF line 92) P1, S1, H1, R1, A1, W1, E1, S2; storm cards W2, W3, W4 and I1 on 2026-11-10; P3, P5, P6, P4, P5b, P5c, P7 on 2026-11-10; P8-P14 and D1 on 2026-11-10 night to 2026-11-11. The BRIEF order lines are [src build/BRIEF.md:145], [src build/BRIEF.md:151], [src build/BRIEF.md:154], [src build/BRIEF.md:157], [src build/BRIEF.md:170], [src build/BRIEF.md:181], [src build/BRIEF.md:187], [src build/BRIEF.md:190].

Packaged apps present in `Packaged/Ironwake/`: 23 bundles, listed in the index below. `Ironwake.app` last written Nov 10 10:01 [src docs/spec/_apps.txt:1], which is the W3 water-fix/audio-fix build per the W3 log (build report `W3.md`). CONFLICT: SPEC.md line 3 calls it the "older promoted A1-era build" [src SPEC.md:3] and README.txt says "A1/W3-era" (macOS container `GameUserSettings.ini`); the W3 log and file date agree with W3, so treat it as the W3 build. Latest build: Ironwake_P14.app [src SPEC.md:3].

## 14.0 Index

| Card | Topic | Commit (final/main) | Packaged app | Headline score |
|---|---|---|---|---|
| A | Storm ocean level | `fda3254` [src docs/spec/_gitlog.txt:143] | none (editor build only) | look ~5-6/10 |
| B | Playable ship pawn | `da3a74c` [src docs/spec/_gitlog.txt:132] | none (editor build only) | visuals ~5/10 |
| C | Gunnery + NPC enemy | `77db4b0` [src docs/spec/_gitlog.txt:130] | none (editor build only) | visuals ~6-7/10 |
| D | Living sea, menu, package, clip | `b0874af` [src docs/spec/_gitlog.txt:128] | `Ironwake_v1.app` [src docs/spec/_apps.txt:23] | clip 6.5/10 |
| E | Independent review | `fc9c4a6` [src docs/spec/_gitlog.txt:124] | none (editor build only) | game 5/10, look 4.5/10 |
| P1 | Fair opening + game over | `3590950` [src docs/spec/_gitlog.txt:118] | none (editor build only) | end card 6/10 |
| S1 | Peaceful sailing | `38c89f2` [src docs/spec/_gitlog.txt:115] | none (editor build only) | sailing to places 7/10 |
| H1 | 60 FPS + handling + camera | `ee306ce` [src docs/spec/_gitlog.txt:111] | `Ironwake_H1.app` [src docs/spec/_apps.txt:4] | 60.0 FPS MEDIUM |
| R1 | Hi-res library ships | `e132d9f` [src docs/spec/_gitlog.txt:105] | `Ironwake_R1.app` [src docs/spec/_apps.txt:18] | player close 8.5/10 |
| A1 | Autopilot | `350d307` [src docs/spec/_gitlog.txt:93] | `Ironwake_A1.app` [src docs/spec/_apps.txt:2] | waterline 10/10 |
| W1 | Heavy ship + bow wave | `83a9d2d` [src docs/spec/_gitlog.txt:100] | `Ironwake_W1.app` [src docs/spec/_apps.txt:20] | bow wave 7/10 |
| E1 | Real world assets | `c84ea33` [src docs/spec/_gitlog.txt:91] | `Ironwake_E1.app` [src docs/spec/_apps.txt:3] | harbour close 8.5 |
| S2 | Roster + sailing frigate | `3d72afb` [src docs/spec/_gitlog.txt:84] | `Ironwake_S2.app` [src docs/spec/_apps.txt:19] | frigate 9.5 |
| W2 | Seas + storm systems | `b7032b6` [src docs/spec/_gitlog.txt:77] | `Ironwake_W2.app` [src docs/spec/_apps.txt:21] | storm 3.5/10 |
| I1 | Archipelago | `aea70ca` [src docs/spec/_gitlog.txt:67] | `Ironwake_I1.app` [src docs/spec/_apps.txt:5] | chart 8.5 |
| W3 | Two-layer water fix + storm | `2d64892` [src docs/spec/_gitlog.txt:64] | none (editor build only) | storm 4-5.5/10 |
| W4 | Finish storm | `e8aea61` [src docs/spec/_gitlog.txt:55] | `Ironwake_W4.app` [src docs/spec/_apps.txt:22] | storm 4-6/10 |
| P3 | Handling + sea look | `f5bb53c` [src docs/spec/_gitlog.txt:44] | `Ironwake_P3.app` [src docs/spec/_apps.txt:11] | hull-side FX 0-2/10 |
| P5 | Package + gates | `b9f0201` [src docs/spec/_gitlog.txt:41] | `Ironwake_P5.app` [src docs/spec/_apps.txt:12] | not promoted |
| P6 | Phase-review review | `52796b0` [src docs/spec/_gitlog.txt:37] | none (editor build only) | game 4, look 5, feel 5 |
| P4 | Clip v2 | `2b51e78` [src docs/spec/_gitlog.txt:33] | none (editor build only) | ~7/10 |
| P5b | Speed-scaled FX + grey smoke | `a903a96` [src docs/spec/_gitlog.txt:35] | `Ironwake_P5b.app` [src docs/spec/_apps.txt:13] | pass rule not fully met |
| P5c | Ship water look | `6f75630` [src docs/spec/_gitlog.txt:30] | `Ironwake_P5c.app` [src docs/spec/_apps.txt:14] | ~8 at 13 kn (SPEC) |
| P7 | Frame-rate-independent turning | `c63b246` [src docs/spec/_gitlog.txt:28] | `Ironwake_P7.app` [src docs/spec/_apps.txt:15] | all 3 hulls within 5% |
| P8 | Cluster models + sky v1 | `71d6c10` [src docs/spec/_gitlog.txt:21] | `Ironwake_P8.app` [src docs/spec/_apps.txt:16] | town 1-2, sky 3.5 |
| P9 | Collision + grounding | `3c392c5` [src docs/spec/_gitlog.txt:18] | `Ironwake_P9.app` [src docs/spec/_apps.txt:17] | all PASS |
| P10 | HDR sky + town v1 | `7f8417b` [src docs/spec/_gitlog.txt:16] | `Ironwake_P10.app` [src docs/spec/_apps.txt:6] | sky 9.5, town 3.5-5.5 |
| P11 | Clock, weather, help | `c593db4` [src docs/spec/_gitlog.txt:13] | `Ironwake_P11.app` [src docs/spec/_apps.txt:7] | night 9.5, rain 10 |
| P12 | Terraced town | `b8a636a` [src docs/spec/_gitlog.txt:11] | `Ironwake_P12.app` [src docs/spec/_apps.txt:8] | day 3.5-4 |
| P13 | Light + sea polish | `e3bfcd3` [src docs/spec/_gitlog.txt:9] | `Ironwake_P13.app` [src docs/spec/_apps.txt:9] | dawn 6.5, dusk 7.5 |
| P14 | Noon, night sea, basin | `828ea94` [src docs/spec/_gitlog.txt:3] | `Ironwake_P14.app` [src docs/spec/_apps.txt:10] | noon 7.5-8.5 |
| D1 | Exhaustive spec | `84c9efa` [src docs/spec/_gitlog.txt:1] | none (editor build only) | in progress |
| P15 | Natural hillside town | `367724c` | `Ironwake_P15.app` | noon 4-5/10 (target 7 not met) |
| P16 | Real swell + CAMERA MOTION | `5e03270` | `Ironwake_P16.app` | measured sea; motion OFF/LOW/MED |
| P17 | Cove, slipway, batch-2 models | `e6ff846` | `Ironwake_P17.app` | noon 4-6.5/10; night 6.5 |
| P18 | Scaled menu + 8-class audio | `e5cf2d4` | `Ironwake_P18.app` | 72 layout checks PASS |
| P19 | Analytic hull wave field | `21095c9` | `Ironwake_P19.app` | side sheet 8-10; Kelvin/wash 2-4 |
| P20 | Living sea NPC traffic (1st pass) | `2530571` | none (editor build only) | ledger 24 ships, utility AI |
| P21 | Living sea NPC traffic (2nd pass) | `09fd959` | `Ironwake_P21.app` | token, anchor grid, new hulls |
| P22 | Moving parts + traffic presence | `d58fd07` | `Ironwake_P22.app` | gig oars, patrol prop discs |
| P23 | Town placement rewrite | `dc70ca6` | `Ironwake_P23.app` | contour lanes, noon 4.5 vs 3 |

Apps with no card of their own: `Ironwake.app` (W3 water-fix build, Nov 10 10:01) [src docs/spec/_apps.txt:1]; `Ironwake_v1.app` is the D-card build kept as the original (build report `H1.md`). P4, P6, P1, S1, W3 and D1 produced no new numbered app (P4 recorded from the editor build (build report `P4.md`); P6 reviewed Ironwake_P5.app (phase review)).

## 14.1 Card A: storm ocean level

| Field | Value |
|---|---|
| Goal | Storm ocean level `Ironwake_Sea` from a copy of OceanStorm (build report `A.md`) |
| Outcome | Epic test content removed; ocean hole fixed (spline shrunk to ~6 m); 24-wave Gerstner generator, max wave `8.1 m` (build report `A.md`); storm sky, fog, grade, `MI_Ocean_Storm`. No whitecaps: reads as "heavy swell", not "storm" (build report `A.md`) |
| Scores | A_E `~7/10` (build report `A.md`) (best), A_N `~6/10` (build report `A.md`), A_W `~6/10` (build report `A.md`) |
| Commit | `29730d6` [src docs/spec/_gitlog.txt:141] (final); WIP `683b5be` [src docs/spec/_gitlog.txt:142] |
| Packaged app | none |

Pitfalls recorded: MIs created through MCP rendered no clouds; deleting a referenced MI corrupts its package (build report `A.md`). Ocean extents `4000 m` at this card (build report `A.md`); later raised (I1 8.4 km, P5c 300 km).

## 14.2 Card B: playable ship pawn

| Field | Value |
|---|---|
| Goal | C++ `AIronwakeShipPawn` with heavy telegraph/rudder controls, HUD, wake (build report `B.md`) |
| Outcome | Works in PIE: `13.0 kn` at 34 s, `15.9 kn` at 90 s on FLANK (build report `B.md`); hard starboard turns about 0.6-0.7 deg/s (build report `B.md`). `12/12` pontoons wet (build report `B.md`). Ironclad mesh `SM_Ironclad` from Hunyuan GLB, 140 m (build report `B.md`) |
| Scores | B_1 `~6/10` (build report `B.md`), B_2 `~4-5/10` (build report `B.md`), B_3 `~4/10` (build report `B.md`), overall visuals ~5/10 (build report `B.md`) |
| Commit | `da3a74c` [src docs/spec/_gitlog.txt:132] (done); module start `0030f0a` [src docs/spec/_gitlog.txt:140] |
| Packaged app | none |

## 14.3 Card C: gunnery and one NPC enemy

| Field | Value |
|---|---|
| Goal | Ballistic shells, turret traverse, damage/fire/sinking, NPC utility-AI ironclad, combat audio (build report `C.md`) |
| Outcome | Verified in PIE: enemy spawned 3956 m away, fled below 35% HP and sank at t about 60 s; `36` shells, `14` hits, `6` fires (build report `C.md`). Muzzle velocity `600 m/s`, max range `10 km` (build report `C.md`); player `160 HP`, NPC `120 HP`, `6` per shell (build report `C.md`) |
| Scores | C_1 ~7, C_2 8, C_3 6, C_4 8, C_5 8 (build report `C.md`); overall ~6-7/10 (build report `C.md`) |
| Commit | `77db4b0` [src docs/spec/_gitlog.txt:130] (code), FX `3781a8b` [src docs/spec/_gitlog.txt:131], log `59ba5d8` [src docs/spec/_gitlog.txt:129] |
| Packaged app | none |

## 14.4 Card D: living sea, airships, menu, package, clip

| Field | Value |
|---|---|
| Goal | Living sea (3 merchants, airship, island + lighthouse), C++ UMG title/pause menu, package, 59 s clip (build report `D.md`) |
| Outcome | Packaged app verified end to end; `-iwtour` run: `IWCDEMO done t=70.6 shots=32 hits=21` (build report `D.md`). Clip `slice_v1.mp4` 59 s 1280x720 (build report `D.md`). Real package is `Saved/StagedBuilds/Mac/IronwakeTest.app` copied with ditto (build report `D.md`) |
| Scores | D_4 airship 10, D_3 merchant 8, D_1 title 5, D_2 island 4 (build report `D.md`); clip honest score `6.5/10` (build report `D.md`) |
| Commit | `b0874af` [src docs/spec/_gitlog.txt:128] (living sea/menu), package prep `b18012f` [src docs/spec/_gitlog.txt:127], log `6514293` [src docs/spec/_gitlog.txt:125] |
| Packaged app | first package at `Packaged/Ironwake/Ironwake.app`, later kept as `Ironwake_v1.app` [src docs/spec/_apps.txt:23] |

Known issues carried: island invisible in fog (4/10), plain menu, merchants invulnerable, bundle id `com.YourCompany.IronwakeTest` (build report `D.md`).

## 14.5 Card E: independent review

| Field | Value |
|---|---|
| Goal | Independent play-through and morning report; changed nothing in the game (build report `E.md`) |
| Outcome | Report build/MORNING_REPORT.md. Top finding: an idle Captain-mode player sinks `121 s` after start [src build/MORNING_REPORT.md:32]. Top 5 next steps at [src build/MORNING_REPORT.md:58-63] became pass 2 |
| Scores | slice as a game `5/10` [src build/MORNING_REPORT.md:55], look `4.5/10` [src build/MORNING_REPORT.md:56] |
| Commit | `fc9c4a6` [src docs/spec/_gitlog.txt:124] |
| Packaged app | reviewed Ironwake.app (first build) |

## 14.6 Card P1: fair opening + game over/restart

| Field | Value |
|---|---|
| Goal | Enemy 7.5 km out in front, hold fire, SHIP LOST / VICTORY screens, camera over the bow, Peaceful default (build report `P1.md`) |
| Outcome | Spawn moved to (-3000, -2700) m heading `045` (build report `P1.md`); enemy holds fire `60 s` or until under `5 km` (build report `P1.md`); `MinAimRangeCm` `1.5 km` (build report `P1.md`); PIE tests for idle, sink, restart, menu, victory all pass (build report `P1.md`) |
| Scores | SHIP LOST card `6/10` (build report `P1.md`), VICTORY `7.5/10` (build report `P1.md`) |
| Commit | `3590950` [src docs/spec/_gitlog.txt:118] |
| Packaged app | none (P5 re-packaged) (build report `P1.md`) |

## 14.7 Card S1: peaceful sailing feel (Voyager)

| Field | Value |
|---|---|
| Goal | Peaceful default, POIs, compass/minimap HUD, helmsman, anchor, fast sail, photo mode, weather cycle, gulls (build report `S1.md`) |
| Outcome | Standalone -game tour: arrived at Wreck t=`114.9` (build report `S1.md`), Needles `287.6`, lighthouse `463.3` (build report `S1.md`); anchor 12.1 kn to 0.2 kn in 16 s (build report `S1.md`); `22` gulls (build report `S1.md`) |
| Scores | sailing to places `7/10` (build report `S1.md`), look of new places `4-5/10`, weather/time `3-4/10` (build report `S1.md`); HUD 8, wreck arrival 8, dusk 3 (build report `S1.md`) |
| Commit | `38c89f2` [src docs/spec/_gitlog.txt:115] |
| Packaged app | none (not packaged) (build report `S1.md`) |

## 14.8 Card H1: 60 FPS, tight handling, WoWS camera, music off

| Field | Value |
|---|---|
| Goal | Fix 6 FPS and drifty handling; World-of-Warships camera; music off (build report `H1.md`) |
| Outcome | Median FPS v1 `16.1` to H1 MEDIUM `60.0` (build report `H1.md`); cloud deck 41 ms to 2-3 ms (build report `H1.md`); STOP to HALF 90% `10.0 s` (build report `H1.md`); bow starts swinging `0.84 s` (build report `H1.md`); presets LOW/MEDIUM/HIGH (build report `H1.md`) |
| Scores | zoomed-out camera 8-10/10; default view `3.5-4/10` vs WoWS (build report `H1.md`) |
| Commit | `ee306ce` [src docs/spec/_gitlog.txt:111] |
| Packaged app | `Ironwake.app` [src docs/spec/_apps.txt:1] at that time; previous kept as `Ironwake_v1.app` [src docs/spec/_apps.txt:23]; later file Ironwake_H1.app `Ironwake_H1.app` [src docs/spec/_apps.txt:4] |

## 14.9 Card R1: hi-res library ships

| Field | Value |
|---|---|
| Goal | Swap Hunyuan hulls for library CC-BY hulls (Devastation, Kracken, Goliath, Aglae, brass airship) (build report `R1.md`) |
| Outcome | Waterline found `4.7 m` apparent offset (`RenderedSeaOffsetCm = -470`) (build report `R1.md`), later shown WRONG in W3; paired FPS H1 `24.2` vs R1 `32.0` under load (build report `R1.md`); airship close-up speckles traced to decimation (build report `R1.md`); in-game CREDITS panel added (build report `R1.md`) |
| Scores | player close 8.5/10 (blunt `3.5-4`) (build report `R1.md`); airship wide 7.5, close-up 4 (build report `R1.md`) |
| Commit | `e132d9f` [src docs/spec/_gitlog.txt:105] (swap); assets `1141efd` [src docs/spec/_gitlog.txt:107]; log `01f670d` [src docs/spec/_gitlog.txt:103] |
| Packaged app | `Ironwake_R1.app` [src docs/spec/_apps.txt:18] |

## 14.10 Card A1: autopilot + repackage + "looks like it's sinking" fix

| Field | Value |
|---|---|
| Goal | P cycles OFF/HOLD/WAYPOINTS/TOUR, M chart, give way, under-fire disengage (build report `A1.md`) |
| Outcome | Hold course max error `0.20 deg`, rms `0.07` (build report `A1.md`); 3-waypoint route done in `504 s` (build report `A1.md`); TOUR done in `1647 s` (build report `A1.md`); boot-top clearance `+0.66 m` calm (build report `A1.md`); `DraftBelowBootTopCm` 50 to -60 (build report `A1.md`) |
| Scores | chart 7.5, autopilot HUD 10, waterline calm 10, storm 9.5 (build report `A1.md`) |
| Commit | `350d307` [src docs/spec/_gitlog.txt:93] (autopilot); log `6918aa0` [src docs/spec/_gitlog.txt:92] |
| Packaged app | `Ironwake_A1.app` [src docs/spec/_apps.txt:2]; W1 build kept as Ironwake_W1.app (build report `A1.md`) |

Known: orbit entry cuts inside hazard rings (~70 m inside the Needles ring) (build report `A1.md`).

## 14.11 Card W1: heavy ship physics, real bow wave, bigger sea

| Field | Value |
|---|---|
| Goal | Chuck 12:20: ship too buoyant, bow splash through the hull, sea too waveless (build report `W1.md`) |
| Outcome | Pontoons replaced by waterplane fit with heave 8 s, pitch 8.5 s, roll 13 s natural periods (build report `W1.md`); hull/sea heave ratio `0.36` moderate stopped (build report `W1.md`); settle `4.7 s` (build report `W1.md`); new `UIronwakeBowWave`; sea presets overcast `1.0`, dusk `0.65`, foggy night `1.6` (build report `W1.md`) |
| Scores | bow wave 2-3 to `7/10` after foam fix; bow dig `0-1/10`; sea `3-4/10` (build report `W1.md`); W1 `35.9` vs R1 `33.3` fps under load (build report `W1.md`) |
| Commit | `83a9d2d` [src docs/spec/_gitlog.txt:100] (physics + bow wave); log `a96130f` [src docs/spec/_gitlog.txt:94] |
| Packaged app | `Ironwake_W1.app` [src docs/spec/_apps.txt:20] |

## 14.12 Card E1: real world assets

| Field | Value |
|---|---|
| Goal | Replace greybox harbour, cliffs, wreck with scans and Hunyuan steampunk pieces (build report `E1.md`) |
| Outcome | `33` meshes imported with Nanite (build report `E1.md`); Greyhaven, St Brannoc, Needles `95/74/55/36/22 m`, Wreck of the Valiant rebuilt (build report `E1.md`); day fog density `0.020` to `0.014` (build report `E1.md`); FPS `60.0` median harbour/island/tour on a quiet machine (build report `E1.md`) |
| Scores | harbour close 8.5, island 7.5, wreck close 9, needles 1 km 7.5, airship close 9 (build report `E1.md`); orchestrator fixes: no grey boxes `9.5/10` (build report `E1.md`) |
| Commit | `c84ea33` [src docs/spec/_gitlog.txt:91] (assets); log `54855f3` [src docs/spec/_gitlog.txt:87]; orchestrator fixes `9d17cb5` [src docs/spec/_gitlog.txt:88] |
| Packaged app | `Ironwake_E1.app` [src docs/spec/_apps.txt:3] |

## 14.13 Card S2: roster, shipyard, sailing frigate

| Field | Value |
|---|---|
| Goal | Sailing frigate default, Kittiwake cutter, Devastation unlock, sail rig, points of sail (build report `S2.md`) |
| Outcome | Roster in `IronwakeFleet.{h,cpp}`: Albatross `88 m`, `2,200 t`, turn `R 210 m` (coded values change later: ch. 2) (build report `S2.md`); points of sail e.g. beam reach `9.3-10.6 kn`, close-hauled `5.8-6.1 kn` (build report `S2.md`); yard braces 38/27/18/1.5 deg (build report `S2.md`); shipyard LAUNCH crash fixed (dangling static material ptr) (build report `S2.md`) |
| Scores | frigate waterline 9.5, cutter 10, ironclad 10, close zoom 9.5, shipyard 8.5 (build report `S2.md`) |
| Commit | `3d72afb` [src docs/spec/_gitlog.txt:84] (sail rig); `fe319ee` [src docs/spec/_gitlog.txt:85] (roster wip); `37f7069` [src docs/spec/_gitlog.txt:81] (waterline); log `2f1161f` [src docs/spec/_gitlog.txt:78] |
| Packaged app | `Ironwake_S2.app` [src docs/spec/_apps.txt:19] |

## 14.14 Card W2: big seas, weather, open-ocean storm

| Field | Value |
|---|---|
| Goal | Five sea states, storm director, deep-sea fade, STORM AT SEA start, evil-storm look (build report `W2.md`) |
| Outcome | Hs measured CALM `0.9 m`, MODERATE `2.1 m`, ROUGH `4.0 m`, STORM `10.5 m` (build report `W2.md`); wind 8/16/24/36/48 kn (build report `W2.md`); round 2 proved rendered waves match query (rendered minus query mean `+0.24 m`) (build report `W2.md`); own ocean material chain `M_IW_WaterRoot` (build report `W2.md`); storm bench `25.5` fps with editor open (build report `W2.md`) |
| Scores | storm look 0-3.5/10: calm 2, rough 0, storm chase `3.5`, round 2 best `3.5/10` (build report `W2.md`), (build report `W2.md`) |
| Commit | `b7032b6` [src docs/spec/_gitlog.txt:77] (systems); round 2 `8a86297` [src docs/spec/_gitlog.txt:74]; log `d344fc0` [src docs/spec/_gitlog.txt:75] |
| Packaged app | `Ironwake_W2.app` [src docs/spec/_apps.txt:21]; Ironwake.app stays S2 (build report `W2.md`) |

## 14.15 Card I1: archipelago

| Field | Value |
|---|---|
| Goal | Many islands to explore with surf and shelf integration (build report `I1.md`) |
| Outcome | `14` new seeded islands (seed `1871`), total `18` places (build report `I1.md`); C++ scatter, `23` HISM components, ~`6,000` instances (build report `I1.md`); ocean extents 8 to `8.4 km` (build report `I1.md`); FPS NOT verified at 60: I1 `32.7` tour, `22.7-24.0` five-island bench under load (build report `I1.md`) |
| Scores | chart 8.5, far silhouettes 7.5-8, stacks/rocky 6.5-7.5, pines 6-7, fort `3-5` at range (build report `I1.md`) |
| Commit | `aea70ca` [src docs/spec/_gitlog.txt:67] (final); wip `686e233` [src docs/spec/_gitlog.txt:72]; surf/shelf `470ac87` [src docs/spec/_gitlog.txt:70] |
| Packaged app | `Ironwake_I1.app` [src docs/spec/_apps.txt:5]; Ironwake.app not promoted (build report `I1.md`) |

## 14.16 Card W3: two-layer water bug + readable storm + ship FX

| Field | Value |
|---|---|
| Goal | Chuck 09:20 two-layer water bug first; readable evil storm second (build report `W3.md`) |
| Outcome | ROOT CAUSE: R1 gauge reader mistook post bands seen through translucent water for the surface; fix `RenderedSeaOffsetCm` to 0 and drafts frigate `-40`, cutter `-25`, ironclad `-50` cm; rendered minus query `-0.08 m` sd `0.09` (build report `W3.md`); audio: harbour_ambience removed (build report `W3.md`); packaged FPS `33.8` median, GPU `29.4 ms` (build report `W3.md`) |
| Scores | storm `4-5.5/10`: chase 4, whitecaps 2.5, trough 5.5, bow rough 5.5, wake_high 0 (build report `W3.md`) |
| Commit | `2d64892` [src docs/spec/_gitlog.txt:64] (water fix); `28de2c7` [src docs/spec/_gitlog.txt:63] (audio); storm `c859910` [src docs/spec/_gitlog.txt:62] |
| Packaged app | water-fix build became Ironwake.app (`Ironwake.app` [src docs/spec/_apps.txt:1]); storm work not packaged (build report `W3.md`) |

## 14.17 Card W4: finish the readable storm

| Field | Value |
|---|---|
| Goal | Fix combed stripes, coverage runaway, invisible wake, spume, bow wave; promotion gate storm >= 6 (build report `W4.md`) |
| Outcome | Mip-filtered foam fixes stripes; coverage ROUGH `3.6-10 %` after calibration (build report `W4.md`); wake from 200 m `9.5/10` (build report `W4.md`); spume still 1-2/10; bow wave `4/10` (build report `W4.md`); calm `8.5/10` (build report `W4.md`). Structural verdict: shader foam on 16-wave Gerstner reads as flat decals (build report `W4.md`) |
| Scores | storm 4-6/10, gate not met, NOT promoted (build report `W4.md`) |
| Commit | `e8aea61` [src docs/spec/_gitlog.txt:55] (r2); `11cb827` [src docs/spec/_gitlog.txt:58] (r1); `add81de` [src docs/spec/_gitlog.txt:54] (r3); log `618557c` [src docs/spec/_gitlog.txt:53] |
| Packaged app | `Ironwake_W4.app` [src docs/spec/_apps.txt:22] (side-by-side only) |

## 14.18 Card P3: handling + sea look + voyage

| Field | Value |
|---|---|
| Goal | Tighter turning, 18-place voyage, ship FX fixes (build report `P3.md`) |
| Outcome | Circle times ironclad `240.8 s` to `201.4 s`, frigate `252.3 s` to `222.2 s` (build report `P3.md`); voyage 18/18 waypoints, `30.4 km` in `1221 s` (build report `P3.md`); BowSheet sprites cut from 170 to `60` (build report `P3.md`); FPS `13.3` P3 vs `12.8` under load (no claim) (build report `P3.md`) |
| Scores | hull-side frames still 0-2/10 vision (flat foam band) (build report `P3.md`); Ironwake.app NOT promoted (build report `P3.md`) |
| Commit | `f5bb53c` [src docs/spec/_gitlog.txt:44] (log); `691e385` [src docs/spec/_gitlog.txt:49] (tighter turning) |
| Packaged app | `Ironwake_P3.app` [src docs/spec/_apps.txt:11] |

## 14.19 Card P5: package + gates + MORNING_REPORT_2

| Field | Value |
|---|---|
| Goal | Package Ironwake_P5.app, run gates [src build/MORNING_REPORT_2.md:1-5] |
| Outcome | Bundle id `io.github.charles-forsyth.ironwake` [src build/MORNING_REPORT_2.md:3]; P5 `15.9` fps vs `14.6` current with editor open [src build/MORNING_REPORT_2.md:15]; waterline frigate `+0.39 m`, cutter `+0.23 m`, ironclad `+0.59 m` [src build/MORNING_REPORT_2.md:20]; promotion gate NOT met: Ironwake.app not replaced [src build/MORNING_REPORT_2.md:27] |
| Scores | no vision scoring; pause menu not verified by P5 (later verified by P6) |
| Commit | `b9f0201` [src docs/spec/_gitlog.txt:41] |
| Packaged app | `Ironwake_P5.app` [src docs/spec/_apps.txt:12] |

## 14.20 Card P6: Phase-review phase review (read-only)

| Field | Value |
|---|---|
| Goal | Judge the phase as an outside reviewer on Ironwake_P5.app (phase review) |
| Outcome | Verdict `ship with caveats` (phase review); measured FPS editor closed `43.6` spawn, `45.3` islands (phase review); found turning frame-rate dependent: ironclad `221 s` at 30 fps vs `288 s` at 19 fps (phase review); frigate has no boot-top band (phase review); water loops carry 393/407 Hz tone (phase review) |
| Scores | game `4/10` (phase review), look `5/10` (phase review), feel `5/10` (phase review) |
| Commit | `52796b0` [src docs/spec/_gitlog.txt:37] |
| Packaged app | reviewed `Ironwake_P5.app` [src docs/spec/_apps.txt:12] |

## 14.21 Card P4: gameplay clip v2 (peaceful)

| Field | Value |
|---|---|
| Goal | 49.5 s peaceful sailing clip `slice_v2.mp4` (build report `P4.md`) |
| Outcome | Nine scripted shots of 5.5 s each, recorded with `-benchmark -fps=30 -dumpmovie` from the editor build (not packaged) (build report `P4.md`); storm look not used (build report `P4.md`) |
| Scores | honest ~`7/10`; per shot 7.5-9; flat bright wake ribbon still the weak point (build report `P4.md`) |
| Commit | `2b51e78` [src docs/spec/_gitlog.txt:33] |
| Packaged app | none (editor build) (build report `P4.md`) |

## 14.22 Card P5b: speed-scaled ship FX + grey funnel smoke

| Field | Value |
|---|---|
| Goal | Verify f8b5bdc speed scaling with a ladder: 3 hulls x {4, 8, 13 kn} x {calm, moderate} (build report `P5b.md`) |
| Outcome | New `NS_IW_FunnelSmoke` grey 0.35 alpha 0.3 (build report `P5b.md`); strict slam gate never fired (`BowSubmergeM` max `-0.57 m`) (build report `P5b.md`) |
| Scores | cutter side 13 kn `3/10`, cutter 4 kn `7/10`, ironclad 8.5/8/5.5, funnel chase `8.5/10` (build report `P5b.md`); pass rule NOT fully met (build report `P5b.md`) |
| Commit | `a903a96` [src docs/spec/_gitlog.txt:35]; Chuck rule `f8b5bdc` [src docs/spec/_gitlog.txt:40]; grey smoke `0eee597` [src docs/spec/_gitlog.txt:38] |
| Packaged app | `Ironwake_P5b.app` [src docs/spec/_apps.txt:13] |

## 14.23 Card P5c: ship water look pass

| Field | Value |
|---|---|
| Goal | Soft wake, patchy hull foam, bow cushion, slam gate, funnel smoke, seam (build report `P5c.md`) |
| Outcome | Wake `6` verts with feathered edges, centre opacity x`0.55` (build report `P5c.md`); slam counts calm 0, moderate `8`, rough `35` per 60 s (build report `P5c.md`); funnel smoke alpha 0.75, 14/s (build report `P5c.md`); ocean/zone extents to `300 km`, seam gone (build report `P5c.md`) |
| Scores | cutter side 7, frigate chase wake 3-7 (unstable); pass rule NOT fully met (build report `P5c.md`); SPEC.md cites ship water at 13 kn ~8 [src SPEC.md:83] |
| Commit | `6f75630` [src docs/spec/_gitlog.txt:30] (final); WIP `98974b0` [src docs/spec/_gitlog.txt:31] |
| Packaged app | `Ironwake_P5c.app` [src docs/spec/_apps.txt:14] |

## 14.24 Card P7: frame-rate-independent handling

| Field | Value |
|---|---|
| Goal | Fix turning that slowed at low FPS (P6); bench P5 vs P5c; frigate smoke (build report `P7.md`) |
| Outcome | Physics substepping `bSubstepping=True`, max `1/60`, `6` substeps [src Config/DefaultEngine.ini:37-40]; circle times after fix ironclad `178.3 s` at 30 and 18 fps, frigate `192.1`, cutter `32.9` (build report `P7.md`); P7 `59.0` fps avg (build report `P7.md`); aux stack moved to (-23 m, 23 m) scale `0.9` (build report `P7.md`) |
| Scores | pass within 5% for all three hulls (build report `P7.md`) |
| Commit | `c63b246` [src docs/spec/_gitlog.txt:28] (substepping); `7bed173` [src docs/spec/_gitlog.txt:27] (logs) |
| Packaged app | `Ironwake_P7.app` [src docs/spec/_apps.txt:15] |

## 14.25 Card P8: cluster models, NASA sky v1, ironclad turn

| Field | Value |
|---|---|
| Goal | Import 17 GPU batch Hunyuan models, build town + NPC craft, NASA night sky (build report `P8.md`) |
| Outcome | `17` keep GLBs imported with Nanite + simple collision (build report `P8.md`); ironclad `MinTurnRadiusM` 280 to `330` (Chuck OK 21:25), circle `208.8 s` (build report `P8.md`); paired FPS 33.1/33.5 (build report `P8.md`) |
| Scores | BOTH TARGETS MISSED: town 1-2/10 (build report `P8.md`); sky `1-2.5/10` over five iterations, best Cygnus `3.5/10` (build report `P8.md`); targets missed (build report `P8.md`) |
| Commit | `71d6c10` [src docs/spec/_gitlog.txt:21] (final); import `039a79f` [src docs/spec/_gitlog.txt:25]; turn `b70985d` [src docs/spec/_gitlog.txt:24] |
| Packaged app | `Ironwake_P8.app` [src docs/spec/_apps.txt:16] |

## 14.26 Card P9: real collision + grounding

| Field | Value |
|---|---|
| Goal | Analytic collision for rocks, piers, buildings, hulls; depth grounding; AvoidLand (build report `P9.md`) |
| Outcome | `1108` solids (build report `P9.md`); all three hulls PASS beach, stack, breakwater, pier, ship-vs-ship, astern-frees, autopilot around island (`137 s`, `117 s`, `198 s`) (build report `P9.md`); open-water circle unchanged `208.8 s` (build report `P9.md`); bench P9 33.6/33.4 (build report `P9.md`) |
| Scores | pass by overhead frames + numeric pen=0; side views unreliable (build report `P9.md`) |
| Commit | `3c392c5` [src docs/spec/_gitlog.txt:18] |
| Packaged app | `Ironwake_P9.app` [src docs/spec/_apps.txt:17] |

## 14.27 Card P10: HDR night sky + town v1

| Field | Value |
|---|---|
| Goal | Fix P8 sky and town misses (build report `P10.md`) |
| Outcome | Real HDR `starmap_2020_8k.exr` as RGBA16F (build report `P10.md`); locked exposure at clear night; `54` houses, `27` gas lamps, `231` window quads (build report `P10.md`); bench P9 33.4 / P10 33.7 (build report `P10.md`) |
| Scores | sky PASS: Orion `9.5/10`, Cygnus/Milky Way 9-10 (build report `P10.md`); town PARTLY `3.5-5.5/10` target 7 NOT met (build report `P10.md`) |
| Commit | `7f8417b` [src docs/spec/_gitlog.txt:16] |
| Packaged app | `Ironwake_P10.app` [src docs/spec/_apps.txt:6] |

## 14.28 Card P11: living world, hum, help panel

| Field | Value |
|---|---|
| Goal | Clock + weather, sea by weather, regenerate hum-free loops, keyboard help (build report `P11.md`) |
| Outcome | Loops regenerated: bow `+3.6 dB`, wake `+5.3`, hiss `+1.1` over median (build report `P11.md`); clock start `09:30`, `48` real min/day (build report `P11.md`); sea per weather clear `0.55` to squall `2.7` (build report `P11.md`); harbour Hs basin `1.0 m` vs offshore `2.1 m` (build report `P11.md`); bench P11 33.7/33.3 (build report `P11.md`) |
| Scores | night 9.5, rain 10, dawn 4, dusk 3-5, night/rain/dawn/dusk (build report `P11.md`); harbour moderate `4/10` (build report `P11.md`) |
| Commit | `c593db4` [src docs/spec/_gitlog.txt:13] (final); wip `0543c7a` [src docs/spec/_gitlog.txt:14] |
| Packaged app | `Ironwake_P11.app` [src docs/spec/_apps.txt:7] |

## 14.29 Card P12: terraced harbour town

| Field | Value |
|---|---|
| Goal | Make the town read from the sea (second pass) (build report `P12.md`) |
| Outcome | `5` terraces in `5.5 m` steps, ~`74` houses, `7` tints, ~`105` chimneys, ~`39 m` spire (build report `P12.md`); fixed P11 lamp inversion (SmoothStep A < B) (build report `P12.md`); bench P12 32.7/32.9 (build report `P12.md`) |
| Scores | PARTLY: day `3.5-4/10` at 400 m, ~1-3 at 900 m, night 6.5; target 7 NOT met (build report `P12.md`) |
| Commit | `b8a636a` [src docs/spec/_gitlog.txt:11] |
| Packaged app | `Ironwake_P12.app` [src docs/spec/_apps.txt:8] |

## 14.30 Card P13: light and sea polish

| Field | Value |
|---|---|
| Goal | Dawn/dusk ramps, moonlight, overcast, sea by weather, night exposure; paused by orchestrator 08:52 before all targets met (build report `P13.md`) |
| Outcome | Dawn ~`6.5`, dusk ~`7.5` (build report `P13.md`); moon peak `2.5` (build report `P13.md`); night exposure EV `1.0` to `0.4`, window emissive x0.25, lamps x0.3 (build report `P13.md`); bench P13 31.3/32.1 (build report `P13.md`) |
| Scores | overcast 8-9; clear noon still `3.5-5`; night 3.5-5.5 then `7` after resume; squall 2-8 (build report `P13.md`) |
| Commit | `e3bfcd3` [src docs/spec/_gitlog.txt:9] (main); night pass `5e07bf8` [src docs/spec/_gitlog.txt:5] |
| Packaged app | `Ironwake_P13.app` [src docs/spec/_apps.txt:9] |

## 14.31 Card P14: clear noon, night sea, squall swell, harbour chop

| Field | Value |
|---|---|
| Goal | P13 leftovers (build report `P14.md`) |
| Outcome | Clear noon 400 m `7.5` (was 3.5-4.5), 900 m `8.5` (build report `P14.md`); night chase 8.5, high 7.5 (build report `P14.md`); squall chase `6.5` (build report `P14.md`); harbour high `9` via flat water-material slab (build report `P14.md`); bench P14 32.4/32.6 (build report `P14.md`) |
| Scores | NOT met: squall high `4/10`; plateau flat top (build report `P14.md`). Orchestrator re-score of clear noon `5.5` vs worker 7.5-8.5 [src SPEC.md:84] |
| Commit | `828ea94` [src docs/spec/_gitlog.txt:3] (final); card `68ba0d7` [src docs/spec/_gitlog.txt:4] |
| Packaged app | `Ironwake_P14.app` [src docs/spec/_apps.txt:10] (latest) |

## 14.32 Card D1: exhaustive spec (this task, in progress)

| Field | Value |
|---|---|
| Goal | Replace overview SPEC.md with chapters in `docs/spec/`, every number cited and machine-checked [src build/cards2/D1_full_spec.md:7-11] |
| Outcome | In progress. 15 chapters planned [src build/cards2/D1_full_spec.md:13]; chapters 1, 12, 14, 15 written by this subtask; checker `build/tools/spec_check.py`; no editor or game launched [src build/cards2/D1_full_spec.md:3] |
| Scores | none |
| Commit | `84c9efa` [src docs/spec/_gitlog.txt:1] (card + SPEC refresh `3d7d0d4` precede)  |
| Packaged app | none |

## 14.33 Score trajectory of key views

| View | First score | Latest | Path |
|---|---|---|---|
| Open-sea storm (parked) | 5-6 (A) | 4-6 (W4) | A, W2 3.5, W3 4-5.5, W4 4-6; parked (build report `W4.md`) |
| Night sky | 1-2.5 (P8) | 9.5 (P10) | (build report `P8.md`), (build report `P10.md`) |
| Town from sea, day | 1-2 (P8) | 7.5 (P14, worker); 5.5 (orchestrator) | P10 3.5-5.5, P12 3.5-4, P14 7.5 [src SPEC.md:84] |
| Ship water at 13 kn | 0-2 (P3 hull-side) | ~8 (P5c, SPEC) | (build report `P3.md`), [src SPEC.md:83] |
| Waterline | wrong (R1 -470 cm) | one surface (W3) | (build report `W3.md`) |
| FPS | `16.1` v1 | `60.0` H1/E1 quiet machine; ~33 loaded | (build report `H1.md`), [src SPEC.md:89] |

## Gaps / unknown

- `docs/spec/_gitlog.txt` and `_apps.txt` are snapshots; commits made after this card (including the D1 commit that lands the chapters) are not listed. Re-run `git log` to refresh.
- Commits for P5b/P5c/P7/P12/P14 listed are the main code commits; several cards have multiple commits (see git log). A card-to-commit mapping is by subject prefix only.
- Vision scores are single-model and noisy; the only independent scores are Phase-review P6 and orchestrator re-scores quoted from SPEC.md. P14 orchestrator 5.5 for noon is the only recorded gap against a worker score.
- No packaged app exists for W3, S1, P1, P4, P6 or D1. W3's water-fix is in `Ironwake.app` per its log, which this chapter could not verify against the binary (no launch allowed).
- P2 (enemy visibility) was never run (parked); card file exists at build/cards2/P2_visibility.md.
- The self-check output is appended to this file by the orchestrator.
# Chapter 14: Card history

Scope: every task card from A through P14 plus D1 (this spec task): goal, outcome, vision scores, commit SHA and packaged app. Commit SHAs are cited to `docs/spec/_gitlog.txt`, a verbatim `git log --format='%h %ad %s'` dump taken 2026-11-11 for this card (D1 itself had only its first commit `84c9efa` at that time). Packaged-app names are cited to `docs/spec/_apps.txt`, a dump of `ls -l Packaged/Ironwake/*.app`. Outcomes and scores are copied from the card logs in `logs/` (which are git-ignored) and the review files. Scores are vision-model scores from the cards themselves and jitter about 1.5 points between identical frames (build report `P5c.md`); they are not independent measurements unless the row says orchestrator or Phase-review.

Order of cards: first night (2026-11-09) A, B, C, D, E; pass 2 (BRIEF line 92) P1, S1, H1, R1, A1, W1, E1, S2; storm cards W2, W3, W4 and I1 on 2026-11-10; P3, P5, P6, P4, P5b, P5c, P7 on 2026-11-10; P8-P14 and D1 on 2026-11-10 night to 2026-11-11. The BRIEF order lines are [src build/BRIEF.md:145], [src build/BRIEF.md:151], [src build/BRIEF.md:154], [src build/BRIEF.md:157], [src build/BRIEF.md:170], [src build/BRIEF.md:181], [src build/BRIEF.md:187], [src build/BRIEF.md:190].

Packaged apps present in `Packaged/Ironwake/`: 23 bundles, listed in the index below. `Ironwake.app` last written Nov 10 10:01 [src docs/spec/_apps.txt:1], which is the W3 water-fix/audio-fix build per the W3 log (build report `W3.md`). CONFLICT: SPEC.md line 3 calls it the "older promoted A1-era build" [src SPEC.md:3] and README.txt says "A1/W3-era" (macOS container `GameUserSettings.ini`); the W3 log and file date agree with W3, so treat it as the W3 build. Latest build: Ironwake_P14.app [src SPEC.md:3].

## 14.0 Index

| Card | Topic | Commit (final/main) | Packaged app | Headline score |
|---|---|---|---|---|
| A | Storm ocean level | `fda3254` [src docs/spec/_gitlog.txt:143] | none (editor build only) | look ~5-6/10 |
| B | Playable ship pawn | `da3a74c` [src docs/spec/_gitlog.txt:132] | none (editor build only) | visuals ~5/10 |
| C | Gunnery + NPC enemy | `77db4b0` [src docs/spec/_gitlog.txt:130] | none (editor build only) | visuals ~6-7/10 |
| D | Living sea, menu, package, clip | `b0874af` [src docs/spec/_gitlog.txt:128] | `Ironwake_v1.app` [src docs/spec/_apps.txt:23] | clip 6.5/10 |
| E | Independent review | `fc9c4a6` [src docs/spec/_gitlog.txt:124] | none (editor build only) | game 5/10, look 4.5/10 |
| P1 | Fair opening + game over | `3590950` [src docs/spec/_gitlog.txt:118] | none (editor build only) | end card 6/10 |
| S1 | Peaceful sailing | `38c89f2` [src docs/spec/_gitlog.txt:115] | none (editor build only) | sailing to places 7/10 |
| H1 | 60 FPS + handling + camera | `ee306ce` [src docs/spec/_gitlog.txt:111] | `Ironwake_H1.app` [src docs/spec/_apps.txt:4] | 60.0 FPS MEDIUM |
| R1 | Hi-res library ships | `e132d9f` [src docs/spec/_gitlog.txt:105] | `Ironwake_R1.app` [src docs/spec/_apps.txt:18] | player close 8.5/10 |
| A1 | Autopilot | `350d307` [src docs/spec/_gitlog.txt:93] | `Ironwake_A1.app` [src docs/spec/_apps.txt:2] | waterline 10/10 |
| W1 | Heavy ship + bow wave | `83a9d2d` [src docs/spec/_gitlog.txt:100] | `Ironwake_W1.app` [src docs/spec/_apps.txt:20] | bow wave 7/10 |
| E1 | Real world assets | `c84ea33` [src docs/spec/_gitlog.txt:91] | `Ironwake_E1.app` [src docs/spec/_apps.txt:3] | harbour close 8.5 |
| S2 | Roster + sailing frigate | `3d72afb` [src docs/spec/_gitlog.txt:84] | `Ironwake_S2.app` [src docs/spec/_apps.txt:19] | frigate 9.5 |
| W2 | Seas + storm systems | `b7032b6` [src docs/spec/_gitlog.txt:77] | `Ironwake_W2.app` [src docs/spec/_apps.txt:21] | storm 3.5/10 |
| I1 | Archipelago | `aea70ca` [src docs/spec/_gitlog.txt:67] | `Ironwake_I1.app` [src docs/spec/_apps.txt:5] | chart 8.5 |
| W3 | Two-layer water fix + storm | `2d64892` [src docs/spec/_gitlog.txt:64] | none (editor build only) | storm 4-5.5/10 |
| W4 | Finish storm | `e8aea61` [src docs/spec/_gitlog.txt:55] | `Ironwake_W4.app` [src docs/spec/_apps.txt:22] | storm 4-6/10 |
| P3 | Handling + sea look | `f5bb53c` [src docs/spec/_gitlog.txt:44] | `Ironwake_P3.app` [src docs/spec/_apps.txt:11] | hull-side FX 0-2/10 |
| P5 | Package + gates | `b9f0201` [src docs/spec/_gitlog.txt:41] | `Ironwake_P5.app` [src docs/spec/_apps.txt:12] | not promoted |
| P6 | Phase-review review | `52796b0` [src docs/spec/_gitlog.txt:37] | none (editor build only) | game 4, look 5, feel 5 |
| P4 | Clip v2 | `2b51e78` [src docs/spec/_gitlog.txt:33] | none (editor build only) | ~7/10 |
| P5b | Speed-scaled FX + grey smoke | `a903a96` [src docs/spec/_gitlog.txt:35] | `Ironwake_P5b.app` [src docs/spec/_apps.txt:13] | pass rule not fully met |
| P5c | Ship water look | `6f75630` [src docs/spec/_gitlog.txt:30] | `Ironwake_P5c.app` [src docs/spec/_apps.txt:14] | ~8 at 13 kn (SPEC) |
| P7 | Frame-rate-independent turning | `c63b246` [src docs/spec/_gitlog.txt:28] | `Ironwake_P7.app` [src docs/spec/_apps.txt:15] | all 3 hulls within 5% |
| P8 | Cluster models + sky v1 | `71d6c10` [src docs/spec/_gitlog.txt:21] | `Ironwake_P8.app` [src docs/spec/_apps.txt:16] | town 1-2, sky 3.5 |
| P9 | Collision + grounding | `3c392c5` [src docs/spec/_gitlog.txt:18] | `Ironwake_P9.app` [src docs/spec/_apps.txt:17] | all PASS |
| P10 | HDR sky + town v1 | `7f8417b` [src docs/spec/_gitlog.txt:16] | `Ironwake_P10.app` [src docs/spec/_apps.txt:6] | sky 9.5, town 3.5-5.5 |
| P11 | Clock, weather, help | `c593db4` [src docs/spec/_gitlog.txt:13] | `Ironwake_P11.app` [src docs/spec/_apps.txt:7] | night 9.5, rain 10 |
| P12 | Terraced town | `b8a636a` [src docs/spec/_gitlog.txt:11] | `Ironwake_P12.app` [src docs/spec/_apps.txt:8] | day 3.5-4 |
| P13 | Light + sea polish | `e3bfcd3` [src docs/spec/_gitlog.txt:9] | `Ironwake_P13.app` [src docs/spec/_apps.txt:9] | dawn 6.5, dusk 7.5 |
| P14 | Noon, night sea, basin | `828ea94` [src docs/spec/_gitlog.txt:3] | `Ironwake_P14.app` [src docs/spec/_apps.txt:10] | noon 7.5-8.5 |
| D1 | Exhaustive spec | `84c9efa` [src docs/spec/_gitlog.txt:1] | none (editor build only) | in progress |

Apps with no card of their own: `Ironwake.app` (W3 water-fix build, Nov 10 10:01) [src docs/spec/_apps.txt:1]; `Ironwake_v1.app` is the D-card build kept as the original (build report `H1.md`). P4, P6, P1, S1, W3 and D1 produced no new numbered app (P4 recorded from the editor build (build report `P4.md`); P6 reviewed Ironwake_P5.app (phase review)).

## 14.1 Card A: storm ocean level

| Field | Value |
|---|---|
| Goal | Storm ocean level `Ironwake_Sea` from a copy of OceanStorm (build report `A.md`) |
| Outcome | Epic test content removed; ocean hole fixed (spline shrunk to ~6 m); 24-wave Gerstner generator, max wave `8.1 m` (build report `A.md`); storm sky, fog, grade, `MI_Ocean_Storm`. No whitecaps: reads as "heavy swell", not "storm" (build report `A.md`) |
| Scores | A_E `~7/10` (build report `A.md`) (best), A_N `~6/10` (build report `A.md`), A_W `~6/10` (build report `A.md`) |
| Commit | `29730d6` [src docs/spec/_gitlog.txt:141] (final); WIP `683b5be` [src docs/spec/_gitlog.txt:142] |
| Packaged app | none |

Pitfalls recorded: MIs created through MCP rendered no clouds; deleting a referenced MI corrupts its package (build report `A.md`). Ocean extents `4000 m` at this card (build report `A.md`); later raised (I1 8.4 km, P5c 300 km).

## 14.2 Card B: playable ship pawn

| Field | Value |
|---|---|
| Goal | C++ `AIronwakeShipPawn` with heavy telegraph/rudder controls, HUD, wake (build report `B.md`) |
| Outcome | Works in PIE: `13.0 kn` at 34 s, `15.9 kn` at 90 s on FLANK (build report `B.md`); hard starboard turns about 0.6-0.7 deg/s (build report `B.md`). `12/12` pontoons wet (build report `B.md`). Ironclad mesh `SM_Ironclad` from Hunyuan GLB, 140 m (build report `B.md`) |
| Scores | B_1 `~6/10` (build report `B.md`), B_2 `~4-5/10` (build report `B.md`), B_3 `~4/10` (build report `B.md`), overall visuals ~5/10 (build report `B.md`) |
| Commit | `da3a74c` [src docs/spec/_gitlog.txt:132] (done); module start `0030f0a` [src docs/spec/_gitlog.txt:140] |
| Packaged app | none |

## 14.3 Card C: gunnery and one NPC enemy

| Field | Value |
|---|---|
| Goal | Ballistic shells, turret traverse, damage/fire/sinking, NPC utility-AI ironclad, combat audio (build report `C.md`) |
| Outcome | Verified in PIE: enemy spawned 3956 m away, fled below 35% HP and sank at t about 60 s; `36` shells, `14` hits, `6` fires (build report `C.md`). Muzzle velocity `600 m/s`, max range `10 km` (build report `C.md`); player `160 HP`, NPC `120 HP`, `6` per shell (build report `C.md`) |
| Scores | C_1 ~7, C_2 8, C_3 6, C_4 8, C_5 8 (build report `C.md`); overall ~6-7/10 (build report `C.md`) |
| Commit | `77db4b0` [src docs/spec/_gitlog.txt:130] (code), FX `3781a8b` [src docs/spec/_gitlog.txt:131], log `59ba5d8` [src docs/spec/_gitlog.txt:129] |
| Packaged app | none |

## 14.4 Card D: living sea, airships, menu, package, clip

| Field | Value |
|---|---|
| Goal | Living sea (3 merchants, airship, island + lighthouse), C++ UMG title/pause menu, package, 59 s clip (build report `D.md`) |
| Outcome | Packaged app verified end to end; `-iwtour` run: `IWCDEMO done t=70.6 shots=32 hits=21` (build report `D.md`). Clip `slice_v1.mp4` 59 s 1280x720 (build report `D.md`). Real package is `Saved/StagedBuilds/Mac/IronwakeTest.app` copied with ditto (build report `D.md`) |
| Scores | D_4 airship 10, D_3 merchant 8, D_1 title 5, D_2 island 4 (build report `D.md`); clip honest score `6.5/10` (build report `D.md`) |
| Commit | `b0874af` [src docs/spec/_gitlog.txt:128] (living sea/menu), package prep `b18012f` [src docs/spec/_gitlog.txt:127], log `6514293` [src docs/spec/_gitlog.txt:125] |
| Packaged app | first package at `Packaged/Ironwake/Ironwake.app`, later kept as `Ironwake_v1.app` [src docs/spec/_apps.txt:23] |

Known issues carried: island invisible in fog (4/10), plain menu, merchants invulnerable, bundle id `com.YourCompany.IronwakeTest` (build report `D.md`).

## 14.5 Card E: independent review

| Field | Value |
|---|---|
| Goal | Independent play-through and morning report; changed nothing in the game (build report `E.md`) |
| Outcome | Report build/MORNING_REPORT.md. Top finding: an idle Captain-mode player sinks `121 s` after start [src build/MORNING_REPORT.md:32]. Top 5 next steps at [src build/MORNING_REPORT.md:58-63] became pass 2 |
| Scores | slice as a game `5/10` [src build/MORNING_REPORT.md:55], look `4.5/10` [src build/MORNING_REPORT.md:56] |
| Commit | `fc9c4a6` [src docs/spec/_gitlog.txt:124] |
| Packaged app | reviewed Ironwake.app (first build) |

## 14.6 Card P1: fair opening + game over/restart

| Field | Value |
|---|---|
| Goal | Enemy 7.5 km out in front, hold fire, SHIP LOST / VICTORY screens, camera over the bow, Peaceful default (build report `P1.md`) |
| Outcome | Spawn moved to (-3000, -2700) m heading `045` (build report `P1.md`); enemy holds fire `60 s` or until under `5 km` (build report `P1.md`); `MinAimRangeCm` `1.5 km` (build report `P1.md`); PIE tests for idle, sink, restart, menu, victory all pass (build report `P1.md`) |
| Scores | SHIP LOST card `6/10` (build report `P1.md`), VICTORY `7.5/10` (build report `P1.md`) |
| Commit | `3590950` [src docs/spec/_gitlog.txt:118] |
| Packaged app | none (P5 re-packaged) (build report `P1.md`) |

## 14.7 Card S1: peaceful sailing feel (Voyager)

| Field | Value |
|---|---|
| Goal | Peaceful default, POIs, compass/minimap HUD, helmsman, anchor, fast sail, photo mode, weather cycle, gulls (build report `S1.md`) |
| Outcome | Standalone -game tour: arrived at Wreck t=`114.9` (build report `S1.md`), Needles `287.6`, lighthouse `463.3` (build report `S1.md`); anchor 12.1 kn to 0.2 kn in 16 s (build report `S1.md`); `22` gulls (build report `S1.md`) |
| Scores | sailing to places `7/10` (build report `S1.md`), look of new places `4-5/10`, weather/time `3-4/10` (build report `S1.md`); HUD 8, wreck arrival 8, dusk 3 (build report `S1.md`) |
| Commit | `38c89f2` [src docs/spec/_gitlog.txt:115] |
| Packaged app | none (not packaged) (build report `S1.md`) |

## 14.8 Card H1: 60 FPS, tight handling, WoWS camera, music off

| Field | Value |
|---|---|
| Goal | Fix 6 FPS and drifty handling; World-of-Warships camera; music off (build report `H1.md`) |
| Outcome | Median FPS v1 `16.1` to H1 MEDIUM `60.0` (build report `H1.md`); cloud deck 41 ms to 2-3 ms (build report `H1.md`); STOP to HALF 90% `10.0 s` (build report `H1.md`); bow starts swinging `0.84 s` (build report `H1.md`); presets LOW/MEDIUM/HIGH (build report `H1.md`) |
| Scores | zoomed-out camera 8-10/10; default view `3.5-4/10` vs WoWS (build report `H1.md`) |
| Commit | `ee306ce` [src docs/spec/_gitlog.txt:111] |
| Packaged app | `Ironwake.app` [src docs/spec/_apps.txt:1] at that time; previous kept as `Ironwake_v1.app` [src docs/spec/_apps.txt:23]; later file Ironwake_H1.app `Ironwake_H1.app` [src docs/spec/_apps.txt:4] |

## 14.9 Card R1: hi-res library ships

| Field | Value |
|---|---|
| Goal | Swap Hunyuan hulls for library CC-BY hulls (Devastation, Kracken, Goliath, Aglae, brass airship) (build report `R1.md`) |
| Outcome | Waterline found `4.7 m` apparent offset (`RenderedSeaOffsetCm = -470`) (build report `R1.md`), later shown WRONG in W3; paired FPS H1 `24.2` vs R1 `32.0` under load (build report `R1.md`); airship close-up speckles traced to decimation (build report `R1.md`); in-game CREDITS panel added (build report `R1.md`) |
| Scores | player close 8.5/10 (blunt `3.5-4`) (build report `R1.md`); airship wide 7.5, close-up 4 (build report `R1.md`) |
| Commit | `e132d9f` [src docs/spec/_gitlog.txt:105] (swap); assets `1141efd` [src docs/spec/_gitlog.txt:107]; log `01f670d` [src docs/spec/_gitlog.txt:103] |
| Packaged app | `Ironwake_R1.app` [src docs/spec/_apps.txt:18] |

## 14.10 Card A1: autopilot + repackage + "looks like it's sinking" fix

| Field | Value |
|---|---|
| Goal | P cycles OFF/HOLD/WAYPOINTS/TOUR, M chart, give way, under-fire disengage (build report `A1.md`) |
| Outcome | Hold course max error `0.20 deg`, rms `0.07` (build report `A1.md`); 3-waypoint route done in `504 s` (build report `A1.md`); TOUR done in `1647 s` (build report `A1.md`); boot-top clearance `+0.66 m` calm (build report `A1.md`); `DraftBelowBootTopCm` 50 to -60 (build report `A1.md`) |
| Scores | chart 7.5, autopilot HUD 10, waterline calm 10, storm 9.5 (build report `A1.md`) |
| Commit | `350d307` [src docs/spec/_gitlog.txt:93] (autopilot); log `6918aa0` [src docs/spec/_gitlog.txt:92] |
| Packaged app | `Ironwake_A1.app` [src docs/spec/_apps.txt:2]; W1 build kept as Ironwake_W1.app (build report `A1.md`) |

Known: orbit entry cuts inside hazard rings (~70 m inside the Needles ring) (build report `A1.md`).

## 14.11 Card W1: heavy ship physics, real bow wave, bigger sea

| Field | Value |
|---|---|
| Goal | Chuck 12:20: ship too buoyant, bow splash through the hull, sea too waveless (build report `W1.md`) |
| Outcome | Pontoons replaced by waterplane fit with heave 8 s, pitch 8.5 s, roll 13 s natural periods (build report `W1.md`); hull/sea heave ratio `0.36` moderate stopped (build report `W1.md`); settle `4.7 s` (build report `W1.md`); new `UIronwakeBowWave`; sea presets overcast `1.0`, dusk `0.65`, foggy night `1.6` (build report `W1.md`) |
| Scores | bow wave 2-3 to `7/10` after foam fix; bow dig `0-1/10`; sea `3-4/10` (build report `W1.md`); W1 `35.9` vs R1 `33.3` fps under load (build report `W1.md`) |
| Commit | `83a9d2d` [src docs/spec/_gitlog.txt:100] (physics + bow wave); log `a96130f` [src docs/spec/_gitlog.txt:94] |
| Packaged app | `Ironwake_W1.app` [src docs/spec/_apps.txt:20] |

## 14.12 Card E1: real world assets

| Field | Value |
|---|---|
| Goal | Replace greybox harbour, cliffs, wreck with scans and Hunyuan steampunk pieces (build report `E1.md`) |
| Outcome | `33` meshes imported with Nanite (build report `E1.md`); Greyhaven, St Brannoc, Needles `95/74/55/36/22 m`, Wreck of the Valiant rebuilt (build report `E1.md`); day fog density `0.020` to `0.014` (build report `E1.md`); FPS `60.0` median harbour/island/tour on a quiet machine (build report `E1.md`) |
| Scores | harbour close 8.5, island 7.5, wreck close 9, needles 1 km 7.5, airship close 9 (build report `E1.md`); orchestrator fixes: no grey boxes `9.5/10` (build report `E1.md`) |
| Commit | `c84ea33` [src docs/spec/_gitlog.txt:91] (assets); log `54855f3` [src docs/spec/_gitlog.txt:87]; orchestrator fixes `9d17cb5` [src docs/spec/_gitlog.txt:88] |
| Packaged app | `Ironwake_E1.app` [src docs/spec/_apps.txt:3] |

## 14.13 Card S2: roster, shipyard, sailing frigate

| Field | Value |
|---|---|
| Goal | Sailing frigate default, Kittiwake cutter, Devastation unlock, sail rig, points of sail (build report `S2.md`) |
| Outcome | Roster in `IronwakeFleet.{h,cpp}`: Albatross `88 m`, `2,200 t`, turn `R 210 m` (coded values change later: ch. 2) (build report `S2.md`); points of sail e.g. beam reach `9.3-10.6 kn`, close-hauled `5.8-6.1 kn` (build report `S2.md`); yard braces 38/27/18/1.5 deg (build report `S2.md`); shipyard LAUNCH crash fixed (dangling static material ptr) (build report `S2.md`) |
| Scores | frigate waterline 9.5, cutter 10, ironclad 10, close zoom 9.5, shipyard 8.5 (build report `S2.md`) |
| Commit | `3d72afb` [src docs/spec/_gitlog.txt:84] (sail rig); `fe319ee` [src docs/spec/_gitlog.txt:85] (roster wip); `37f7069` [src docs/spec/_gitlog.txt:81] (waterline); log `2f1161f` [src docs/spec/_gitlog.txt:78] |
| Packaged app | `Ironwake_S2.app` [src docs/spec/_apps.txt:19] |

## 14.14 Card W2: big seas, weather, open-ocean storm

| Field | Value |
|---|---|
| Goal | Five sea states, storm director, deep-sea fade, STORM AT SEA start, evil-storm look (build report `W2.md`) |
| Outcome | Hs measured CALM `0.9 m`, MODERATE `2.1 m`, ROUGH `4.0 m`, STORM `10.5 m` (build report `W2.md`); wind 8/16/24/36/48 kn (build report `W2.md`); round 2 proved rendered waves match query (rendered minus query mean `+0.24 m`) (build report `W2.md`); own ocean material chain `M_IW_WaterRoot` (build report `W2.md`); storm bench `25.5` fps with editor open (build report `W2.md`) |
| Scores | storm look 0-3.5/10: calm 2, rough 0, storm chase `3.5`, round 2 best `3.5/10` (build report `W2.md`), (build report `W2.md`) |
| Commit | `b7032b6` [src docs/spec/_gitlog.txt:77] (systems); round 2 `8a86297` [src docs/spec/_gitlog.txt:74]; log `d344fc0` [src docs/spec/_gitlog.txt:75] |
| Packaged app | `Ironwake_W2.app` [src docs/spec/_apps.txt:21]; Ironwake.app stays S2 (build report `W2.md`) |

## 14.15 Card I1: archipelago

| Field | Value |
|---|---|
| Goal | Many islands to explore with surf and shelf integration (build report `I1.md`) |
| Outcome | `14` new seeded islands (seed `1871`), total `18` places (build report `I1.md`); C++ scatter, `23` HISM components, ~`6,000` instances (build report `I1.md`); ocean extents 8 to `8.4 km` (build report `I1.md`); FPS NOT verified at 60: I1 `32.7` tour, `22.7-24.0` five-island bench under load (build report `I1.md`) |
| Scores | chart 8.5, far silhouettes 7.5-8, stacks/rocky 6.5-7.5, pines 6-7, fort `3-5` at range (build report `I1.md`) |
| Commit | `aea70ca` [src docs/spec/_gitlog.txt:67] (final); wip `686e233` [src docs/spec/_gitlog.txt:72]; surf/shelf `470ac87` [src docs/spec/_gitlog.txt:70] |
| Packaged app | `Ironwake_I1.app` [src docs/spec/_apps.txt:5]; Ironwake.app not promoted (build report `I1.md`) |

## 14.16 Card W3: two-layer water bug + readable storm + ship FX

| Field | Value |
|---|---|
| Goal | Chuck 09:20 two-layer water bug first; readable evil storm second (build report `W3.md`) |
| Outcome | ROOT CAUSE: R1 gauge reader mistook post bands seen through translucent water for the surface; fix `RenderedSeaOffsetCm` to 0 and drafts frigate `-40`, cutter `-25`, ironclad `-50` cm; rendered minus query `-0.08 m` sd `0.09` (build report `W3.md`); audio: harbour_ambience removed (build report `W3.md`); packaged FPS `33.8` median, GPU `29.4 ms` (build report `W3.md`) |
| Scores | storm `4-5.5/10`: chase 4, whitecaps 2.5, trough 5.5, bow rough 5.5, wake_high 0 (build report `W3.md`) |
| Commit | `2d64892` [src docs/spec/_gitlog.txt:64] (water fix); `28de2c7` [src docs/spec/_gitlog.txt:63] (audio); storm `c859910` [src docs/spec/_gitlog.txt:62] |
| Packaged app | water-fix build became Ironwake.app (`Ironwake.app` [src docs/spec/_apps.txt:1]); storm work not packaged (build report `W3.md`) |

## 14.17 Card W4: finish the readable storm

| Field | Value |
|---|---|
| Goal | Fix combed stripes, coverage runaway, invisible wake, spume, bow wave; promotion gate storm >= 6 (build report `W4.md`) |
| Outcome | Mip-filtered foam fixes stripes; coverage ROUGH `3.6-10 %` after calibration (build report `W4.md`); wake from 200 m `9.5/10` (build report `W4.md`); spume still 1-2/10; bow wave `4/10` (build report `W4.md`); calm `8.5/10` (build report `W4.md`). Structural verdict: shader foam on 16-wave Gerstner reads as flat decals (build report `W4.md`) |
| Scores | storm 4-6/10, gate not met, NOT promoted (build report `W4.md`) |
| Commit | `e8aea61` [src docs/spec/_gitlog.txt:55] (r2); `11cb827` [src docs/spec/_gitlog.txt:58] (r1); `add81de` [src docs/spec/_gitlog.txt:54] (r3); log `618557c` [src docs/spec/_gitlog.txt:53] |
| Packaged app | `Ironwake_W4.app` [src docs/spec/_apps.txt:22] (side-by-side only) |

## 14.18 Card P3: handling + sea look + voyage

| Field | Value |
|---|---|
| Goal | Tighter turning, 18-place voyage, ship FX fixes (build report `P3.md`) |
| Outcome | Circle times ironclad `240.8 s` to `201.4 s`, frigate `252.3 s` to `222.2 s` (build report `P3.md`); voyage 18/18 waypoints, `30.4 km` in `1221 s` (build report `P3.md`); BowSheet sprites cut from 170 to `60` (build report `P3.md`); FPS `13.3` P3 vs `12.8` under load (no claim) (build report `P3.md`) |
| Scores | hull-side frames still 0-2/10 vision (flat foam band) (build report `P3.md`); Ironwake.app NOT promoted (build report `P3.md`) |
| Commit | `f5bb53c` [src docs/spec/_gitlog.txt:44] (log); `691e385` [src docs/spec/_gitlog.txt:49] (tighter turning) |
| Packaged app | `Ironwake_P3.app` [src docs/spec/_apps.txt:11] |

## 14.19 Card P5: package + gates + MORNING_REPORT_2

| Field | Value |
|---|---|
| Goal | Package Ironwake_P5.app, run gates [src build/MORNING_REPORT_2.md:1-5] |
| Outcome | Bundle id `io.github.charles-forsyth.ironwake` [src build/MORNING_REPORT_2.md:3]; P5 `15.9` fps vs `14.6` current with editor open [src build/MORNING_REPORT_2.md:15]; waterline frigate `+0.39 m`, cutter `+0.23 m`, ironclad `+0.59 m` [src build/MORNING_REPORT_2.md:20]; promotion gate NOT met: Ironwake.app not replaced [src build/MORNING_REPORT_2.md:27] |
| Scores | no vision scoring; pause menu not verified by P5 (later verified by P6) |
| Commit | `b9f0201` [src docs/spec/_gitlog.txt:41] |
| Packaged app | `Ironwake_P5.app` [src docs/spec/_apps.txt:12] |

## 14.20 Card P6: Phase-review phase review (read-only)

| Field | Value |
|---|---|
| Goal | Judge the phase as an outside reviewer on Ironwake_P5.app (phase review) |
| Outcome | Verdict `ship with caveats` (phase review); measured FPS editor closed `43.6` spawn, `45.3` islands (phase review); found turning frame-rate dependent: ironclad `221 s` at 30 fps vs `288 s` at 19 fps (phase review); frigate has no boot-top band (phase review); water loops carry 393/407 Hz tone (phase review) |
| Scores | game `4/10` (phase review), look `5/10` (phase review), feel `5/10` (phase review) |
| Commit | `52796b0` [src docs/spec/_gitlog.txt:37] |
| Packaged app | reviewed `Ironwake_P5.app` [src docs/spec/_apps.txt:12] |

## 14.21 Card P4: gameplay clip v2 (peaceful)

| Field | Value |
|---|---|
| Goal | 49.5 s peaceful sailing clip `slice_v2.mp4` (build report `P4.md`) |
| Outcome | Nine scripted shots of 5.5 s each, recorded with `-benchmark -fps=30 -dumpmovie` from the editor build (not packaged) (build report `P4.md`); storm look not used (build report `P4.md`) |
| Scores | honest ~`7/10`; per shot 7.5-9; flat bright wake ribbon still the weak point (build report `P4.md`) |
| Commit | `2b51e78` [src docs/spec/_gitlog.txt:33] |
| Packaged app | none (editor build) (build report `P4.md`) |

## 14.22 Card P5b: speed-scaled ship FX + grey funnel smoke

| Field | Value |
|---|---|
| Goal | Verify f8b5bdc speed scaling with a ladder: 3 hulls x {4, 8, 13 kn} x {calm, moderate} (build report `P5b.md`) |
| Outcome | New `NS_IW_FunnelSmoke` grey 0.35 alpha 0.3 (build report `P5b.md`); strict slam gate never fired (`BowSubmergeM` max `-0.57 m`) (build report `P5b.md`) |
| Scores | cutter side 13 kn `3/10`, cutter 4 kn `7/10`, ironclad 8.5/8/5.5, funnel chase `8.5/10` (build report `P5b.md`); pass rule NOT fully met (build report `P5b.md`) |
| Commit | `a903a96` [src docs/spec/_gitlog.txt:35]; Chuck rule `f8b5bdc` [src docs/spec/_gitlog.txt:40]; grey smoke `0eee597` [src docs/spec/_gitlog.txt:38] |
| Packaged app | `Ironwake_P5b.app` [src docs/spec/_apps.txt:13] |

## 14.23 Card P5c: ship water look pass

| Field | Value |
|---|---|
| Goal | Soft wake, patchy hull foam, bow cushion, slam gate, funnel smoke, seam (build report `P5c.md`) |
| Outcome | Wake `6` verts with feathered edges, centre opacity x`0.55` (build report `P5c.md`); slam counts calm 0, moderate `8`, rough `35` per 60 s (build report `P5c.md`); funnel smoke alpha 0.75, 14/s (build report `P5c.md`); ocean/zone extents to `300 km`, seam gone (build report `P5c.md`) |
| Scores | cutter side 7, frigate chase wake 3-7 (unstable); pass rule NOT fully met (build report `P5c.md`); SPEC.md cites ship water at 13 kn ~8 [src SPEC.md:83] |
| Commit | `6f75630` [src docs/spec/_gitlog.txt:30] (final); WIP `98974b0` [src docs/spec/_gitlog.txt:31] |
| Packaged app | `Ironwake_P5c.app` [src docs/spec/_apps.txt:14] |

## 14.24 Card P7: frame-rate-independent handling

| Field | Value |
|---|---|
| Goal | Fix turning that slowed at low FPS (P6); bench P5 vs P5c; frigate smoke (build report `P7.md`) |
| Outcome | Physics substepping `bSubstepping=True`, max `1/60`, `6` substeps [src Config/DefaultEngine.ini:37-40]; circle times after fix ironclad `178.3 s` at 30 and 18 fps, frigate `192.1`, cutter `32.9` (build report `P7.md`); P7 `59.0` fps avg (build report `P7.md`); aux stack moved to (-23 m, 23 m) scale `0.9` (build report `P7.md`) |
| Scores | pass within 5% for all three hulls (build report `P7.md`) |
| Commit | `c63b246` [src docs/spec/_gitlog.txt:28] (substepping); `7bed173` [src docs/spec/_gitlog.txt:27] (logs) |
| Packaged app | `Ironwake_P7.app` [src docs/spec/_apps.txt:15] |

## 14.25 Card P8: cluster models, NASA sky v1, ironclad turn

| Field | Value |
|---|---|
| Goal | Import 17 GPU batch Hunyuan models, build town + NPC craft, NASA night sky (build report `P8.md`) |
| Outcome | `17` keep GLBs imported with Nanite + simple collision (build report `P8.md`); ironclad `MinTurnRadiusM` 280 to `330` (Chuck OK 21:25), circle `208.8 s` (build report `P8.md`); paired FPS 33.1/33.5 (build report `P8.md`) |
| Scores | BOTH TARGETS MISSED: town 1-2/10 (build report `P8.md`); sky `1-2.5/10` over five iterations, best Cygnus `3.5/10` (build report `P8.md`); targets missed (build report `P8.md`) |
| Commit | `71d6c10` [src docs/spec/_gitlog.txt:21] (final); import `039a79f` [src docs/spec/_gitlog.txt:25]; turn `b70985d` [src docs/spec/_gitlog.txt:24] |
| Packaged app | `Ironwake_P8.app` [src docs/spec/_apps.txt:16] |

## 14.26 Card P9: real collision + grounding

| Field | Value |
|---|---|
| Goal | Analytic collision for rocks, piers, buildings, hulls; depth grounding; AvoidLand (build report `P9.md`) |
| Outcome | `1108` solids (build report `P9.md`); all three hulls PASS beach, stack, breakwater, pier, ship-vs-ship, astern-frees, autopilot around island (`137 s`, `117 s`, `198 s`) (build report `P9.md`); open-water circle unchanged `208.8 s` (build report `P9.md`); bench P9 33.6/33.4 (build report `P9.md`) |
| Scores | pass by overhead frames + numeric pen=0; side views unreliable (build report `P9.md`) |
| Commit | `3c392c5` [src docs/spec/_gitlog.txt:18] |
| Packaged app | `Ironwake_P9.app` [src docs/spec/_apps.txt:17] |

## 14.27 Card P10: HDR night sky + town v1

| Field | Value |
|---|---|
| Goal | Fix P8 sky and town misses (build report `P10.md`) |
| Outcome | Real HDR `starmap_2020_8k.exr` as RGBA16F (build report `P10.md`); locked exposure at clear night; `54` houses, `27` gas lamps, `231` window quads (build report `P10.md`); bench P9 33.4 / P10 33.7 (build report `P10.md`) |
| Scores | sky PASS: Orion `9.5/10`, Cygnus/Milky Way 9-10 (build report `P10.md`); town PARTLY `3.5-5.5/10` target 7 NOT met (build report `P10.md`) |
| Commit | `7f8417b` [src docs/spec/_gitlog.txt:16] |
| Packaged app | `Ironwake_P10.app` [src docs/spec/_apps.txt:6] |

## 14.28 Card P11: living world, hum, help panel

| Field | Value |
|---|---|
| Goal | Clock + weather, sea by weather, regenerate hum-free loops, keyboard help (build report `P11.md`) |
| Outcome | Loops regenerated: bow `+3.6 dB`, wake `+5.3`, hiss `+1.1` over median (build report `P11.md`); clock start `09:30`, `48` real min/day (build report `P11.md`); sea per weather clear `0.55` to squall `2.7` (build report `P11.md`); harbour Hs basin `1.0 m` vs offshore `2.1 m` (build report `P11.md`); bench P11 33.7/33.3 (build report `P11.md`) |
| Scores | night 9.5, rain 10, dawn 4, dusk 3-5, night/rain/dawn/dusk (build report `P11.md`); harbour moderate `4/10` (build report `P11.md`) |
| Commit | `c593db4` [src docs/spec/_gitlog.txt:13] (final); wip `0543c7a` [src docs/spec/_gitlog.txt:14] |
| Packaged app | `Ironwake_P11.app` [src docs/spec/_apps.txt:7] |

## 14.29 Card P12: terraced harbour town

| Field | Value |
|---|---|
| Goal | Make the town read from the sea (second pass) (build report `P12.md`) |
| Outcome | `5` terraces in `5.5 m` steps, ~`74` houses, `7` tints, ~`105` chimneys, ~`39 m` spire (build report `P12.md`); fixed P11 lamp inversion (SmoothStep A < B) (build report `P12.md`); bench P12 32.7/32.9 (build report `P12.md`) |
| Scores | PARTLY: day `3.5-4/10` at 400 m, ~1-3 at 900 m, night 6.5; target 7 NOT met (build report `P12.md`) |
| Commit | `b8a636a` [src docs/spec/_gitlog.txt:11] |
| Packaged app | `Ironwake_P12.app` [src docs/spec/_apps.txt:8] |

## 14.30 Card P13: light and sea polish

| Field | Value |
|---|---|
| Goal | Dawn/dusk ramps, moonlight, overcast, sea by weather, night exposure; paused by orchestrator 08:52 before all targets met (build report `P13.md`) |
| Outcome | Dawn ~`6.5`, dusk ~`7.5` (build report `P13.md`); moon peak `2.5` (build report `P13.md`); night exposure EV `1.0` to `0.4`, window emissive x0.25, lamps x0.3 (build report `P13.md`); bench P13 31.3/32.1 (build report `P13.md`) |
| Scores | overcast 8-9; clear noon still `3.5-5`; night 3.5-5.5 then `7` after resume; squall 2-8 (build report `P13.md`) |
| Commit | `e3bfcd3` [src docs/spec/_gitlog.txt:9] (main); night pass `5e07bf8` [src docs/spec/_gitlog.txt:5] |
| Packaged app | `Ironwake_P13.app` [src docs/spec/_apps.txt:9] |

## 14.31 Card P14: clear noon, night sea, squall swell, harbour chop

| Field | Value |
|---|---|
| Goal | P13 leftovers (build report `P14.md`) |
| Outcome | Clear noon 400 m `7.5` (was 3.5-4.5), 900 m `8.5` (build report `P14.md`); night chase 8.5, high 7.5 (build report `P14.md`); squall chase `6.5` (build report `P14.md`); harbour high `9` via flat water-material slab (build report `P14.md`); bench P14 32.4/32.6 (build report `P14.md`) |
| Scores | NOT met: squall high `4/10`; plateau flat top (build report `P14.md`). Orchestrator re-score of clear noon `5.5` vs worker 7.5-8.5 [src SPEC.md:84] |
| Commit | `828ea94` [src docs/spec/_gitlog.txt:3] (final); card `68ba0d7` [src docs/spec/_gitlog.txt:4] |
| Packaged app | `Ironwake_P14.app` [src docs/spec/_apps.txt:10] (latest) |

## 14.32 Card D1: exhaustive spec (this task, in progress)

| Field | Value |
|---|---|
| Goal | Replace overview SPEC.md with chapters in `docs/spec/`, every number cited and machine-checked [src build/cards2/D1_full_spec.md:7-11] |
| Outcome | In progress. 15 chapters planned [src build/cards2/D1_full_spec.md:13]; chapters 1, 12, 14, 15 written by this subtask; checker `build/tools/spec_check.py`; no editor or game launched [src build/cards2/D1_full_spec.md:3] |
| Scores | none |
| Commit | `84c9efa` [src docs/spec/_gitlog.txt:1] (card + SPEC refresh `3d7d0d4` precede)  |
| Packaged app | none |

## 14.33 Score trajectory of key views

| View | First score | Latest | Path |
|---|---|---|---|
| Open-sea storm (parked) | 5-6 (A) | 4-6 (W4) | A, W2 3.5, W3 4-5.5, W4 4-6; parked (build report `W4.md`) |
| Night sky | 1-2.5 (P8) | 9.5 (P10) | (build report `P8.md`), (build report `P10.md`) |
| Town from sea, day | 1-2 (P8) | 7.5 (P14, worker); 5.5 (orchestrator) | P10 3.5-5.5, P12 3.5-4, P14 7.5 [src SPEC.md:84] |
| Ship water at 13 kn | 0-2 (P3 hull-side) | ~8 (P5c, SPEC) | (build report `P3.md`), [src SPEC.md:83] |
| Waterline | wrong (R1 -470 cm) | one surface (W3) | (build report `W3.md`) |
| FPS | `16.1` v1 | `60.0` H1/E1 quiet machine; ~33 loaded | (build report `H1.md`), [src SPEC.md:89] |

## Gaps / unknown

- `docs/spec/_gitlog.txt` and `_apps.txt` are snapshots; commits made after this card (including the D1 commit that lands the chapters) are not listed. Re-run `git log` to refresh.
- Commits for P5b/P5c/P7/P12/P14 listed are the main code commits; several cards have multiple commits (see git log). A card-to-commit mapping is by subject prefix only.
- Vision scores are single-model and noisy; the only independent scores are Phase-review P6 and orchestrator re-scores quoted from SPEC.md. P14 orchestrator 5.5 for noon is the only recorded gap against a worker score.
- No packaged app exists for W3, S1, P1, P4, P6 or D1. W3's water-fix is in `Ironwake.app` per its log, which this chapter could not verify against the binary (no launch allowed).
- P2 (enemy visibility) was never run (parked); card file exists at build/cards2/P2_visibility.md.
- The self-check output is appended to this file by the orchestrator.


## Self-check (build/tools/spec_check.py)
Run 2026-11-11 for D1. No editor or game was launched.

### Full run: `spec_check.py --all --fails`
```
total tags: 3463 in 15 files
FAIL 01_design.md:105 `8.4 km` SPEC.md:14 value not in window
FAIL 02_ships.md:135 `can't sail within 40 deg of wind` SPEC.md:45 value not in window
FAIL 06_navigation.md:64 `18 places` SPEC.md:15 value not in window
FAIL 15_backlog.md:138 `5.5 (orchestrator)` SPEC.md:84 value not in window
checked 3463: 3459 pass, 4 fail
```

### Random 30 (`--seed 20261111`)
```
total tags: 3250 in 15 files
PASS 08_time_weather_sky.md:126 `FMath::SmoothStep(0.f, 1.f, LiveBlendT)` Source/IronwakeTest/IronwakeP11.cpp:156 
PASS 06_navigation.md:170 `for (FIronwakePOI& Pt : POIs) Pt.bVisited = false;` Source/IronwakeTest/IronwakeAutopilot.cpp:247 
PASS 07_world.md:173 `FVector(6.2f, 6.2f, 5.f)` Source/IronwakeTest/IronwakeLivingSea.cpp:580 
PASS 03_water.md:234 `13.f` Source/IronwakeTest/IronwakeShipPawn.h:476-478 
PASS 08_time_weather_sky.md:222 `AutoExposureMin = Max = 1.0` logs/P10.md:7 
PASS 07_world.md:99 `FRandRange(2.2f, 4.5f)` Source/IronwakeTest/IronwakeArchipelago.cpp:352 
PASS 12_assets.md:173 `namaqualand_cliff_02` build/i1/prep_i1.py:21 
PASS 11_performance.md:25 `12` Source/IronwakeTest/IronwakeSettings.cpp:83 
PASS 02_ships.md:291 `Frigate 3.2 min` logs/P7.md:23 
PASS 04_ship_fx.md:107 `22` Source/IronwakeTest/IronwakeHydro.cpp:356 
PASS 14_history.md:117 `114.9` logs/S1.md:46 
PASS 02_ships.md:166 `1.4f` Source/IronwakeTest/IronwakeLivingSea.h:42 
PASS 02_ships.md:65 `IronwakeFleet.cpp:63 set too, no effect alone` logs/P8.md:22 
PASS 08_time_weather_sky.md:270 `MoonRadiusDeg = 1.0f` Source/IronwakeTest/IronwakeP8.h:39 
PASS 08_time_weather_sky.md:288 `-1.2f : 40.f` Source/IronwakeTest/IronwakeP8.cpp:323 
PASS 06_navigation.md:87 `d += 5` Source/IronwakeTest/IronwakeVoyage.cpp:1109 
PASS 03_water.md:105 `IW_WaterTime` Source/IronwakeTest/IronwakeHydro.cpp:290-291 
PASS 07_world.md:42 `3420` Source/IronwakeTest/IronwakeArchipelago.cpp:53 
PASS 07_world.md:219 `FLinearColor(1.f, 0.15f, 0.1f), 60000.f, 60000.f` Source/IronwakeTest/IronwakeVoyage.cpp:375 
PASS 03_water.md:21 `11` Source/IronwakeTest/IronwakeHydro.cpp:62 
PASS 13_tooling.md:116 `list_properties` build/tools/props.py:6 
PASS 11_performance.md:199 `32.7` logs/I1.md:60-66 
PASS 06_navigation.md:202 `in 1647 s with fast sail` logs/A1.md:28 
PASS 10_audio.md:40 `48000,1,40.000000` docs/spec/audio_probe.txt:10 
PASS 05_collision.md:21 `O.R = Half.Size()` Source/IronwakeTest/IronwakeCollision.cpp:100 
PASS 02_ships.md:172 `bAglae ? 7.f : 12.f` Source/IronwakeTest/IronwakeLivingSea.cpp:210 
PASS 12_assets.md:108 `BollardCapstan` build/e1/prep.py:42 
PASS 05_collision.md:43 `Co->AddMeshBounds(H->GetStaticMesh(), FTransform(R, W, S), N, true, 250.f)` Source/IronwakeTest/IronwakeArchipelago.cpp:177 
PASS 14_history.md:17 `fc9c4a6` docs/spec/_gitlog.txt:124 
PASS 02_ships.md:188 `-130.f` Source/IronwakeTest/IronwakeP8.cpp:243 
checked 30: 30 pass, 0 fail
```

## P15: natural hillside for the harbour town (2026-11-11)

| Field | Value |
|---|---|
| Outcome | Procedural heightfield hillside (`IronwakeP15.cpp`) replaces the P12 wedge; layered ground material, road ribbons, scatter, 60 land collision OBBs [src logs/P15.md] |
| Scores | NOT MET: vision 4-5/10 at noon 400 m, 4 at 900 m, night 5, dusk 2 (lighting); target 7 [src logs/P15.md] |
| Bench | P14 33.2/32.1, P15 31.3/30.8 fps [src logs/P15bench] |
| Collision | frigate runs stop 50-90 m short of land; P9 `ship all` partly run [src logs/P15.md] |
| Packaged app | `Ironwake_P15.app` |

## P16: real swell measured, livelier ship, CAMERA MOTION option (2026-11-11)

| Field | Value |
|---|---|
| Outcome | Measured sea: analytic = query = rendered (calm 0.97 / moderate 2.34 / rough 4.82 / gale 8.78 m p2p); hull made livelier (periods, follow gains, fit span, caps 8/12); CAMERA MOTION OFF/LOW/MEDIUM, default LOW [src logs/P16.md] |
| Scores | Vision on single frames 0-2/10 for visible swell; target 7 NOT demonstrated; pixel measurement shows hull 25-65 px motion and post waterline motion matching the log [src logs/P16.md] |
| Circle | ironclad 208.8 s at 30 fps, 208.7 s at 18 fps [src logs/P16.md] |
| Packaged app | `Ironwake_P16.app` |

## P17: town seaward look + batch-2 models (2026-11-11)

| Field | Value |
|---|---|
| Outcome | Quay split into wall + cove/beach/slipway + revetment flanks, 2 jetties, varied terrace walls, wider pale main road with kerbs and white steps, desaturated ground with patches, shrub belt, fringe houses, 8 batch-2 town models + 3 ships [src logs/P17.md] |
| Scores | NOT MET: vision 4-6.5/10 at noon 400 m (target 7), 8 at 900 m (one pass), night 6.5 [src logs/P17.md] |
| Bench | P16 33.5/33.0, P17 31.8/30.0 fps [src logs/P17bench] |
| Packaged app | `Ironwake_P17.app` |

## P18: menu that fits every screen + AUDIO tab (2026-11-11)

| Field | Value |
|---|---|
| Outcome | Menu rebuilt with ScaleBox (0.5..1.5 scale) and card + ScrollBox; 8 sound classes (Master + 7 children), 25 cooked sounds assigned; AUDIO tab with 8 sliders (5 % steps), live mix, route test, and persistence. |
| Verification | 72 layout checks PASS across 8 resolutions; route test 25 sounds, 0 unclassed; bench within 2 fps of P17. |
| Packaged app | `Ironwake_P18.app` |

## P19: water that reacts to hulls (2026-11-11)

| Field | Value |
|---|---|
| Outcome | Analytic per-ship wave field (bow crest, midbody trough, quarter crest, Kelvin wedge) evaluated identically on CPU and in the water material (vertex displacement, normal tilt, foam); hull-class data in the ship defs; prop wash for steamers only (twin screws on the ironclad); NPC hulls are sources [src logs/P19.md] |
| Numbers | Ironclad 13 kn: bow crest 44 % freeboard, trough 8.6 % draft at 36 % LWL, quarter crest at 64 %, wedge 17.8 deg vs 18.3 formula; 16 kn trough 10 % [src logs/P19/p19all_ironclad.txt] |
| Scores | Side sheet (real cruiser vs ours, ironclad 16 kn): bow 8, trough 9, quarter 7, overall 8; sail vs steam sheet 10. NOT MET: Kelvin wedge 3 and prop wash 2-4 against real photos (single-frame top-down view alone scored wedge 8) [src logs/P19.md] |
| Bench | P17 33.2/33.5, P19 31.7/32.3 fps [src logs/P19bench] |
| Packaged app | `Ironwake_P19.app` |

## P20: living sea NPC traffic, first pass (2026-11-12)

| Field | Value |
|---|---|
| Outcome | AIronwakeNpcShip embodied NPC ship pawn; AIronwakeNpcDirector with ledger of 24 ships across 4 ports, utility AI (12 states), COLREGS-lite avoidance, land look-ahead, timetable. Smack, RevenueCutter, PatrolAirship split props imported. |
| NOT MET | Soak collisions=0 stuck=0 NOT met (collisions=41, stuck=75 at t=3014 s soak13); binocular tooltip, minimap contacts, paired bench not done. |
| Packaged app | none (editor build only) |

| P21 (2026-11-12) | Living sea, second pass: PARTIAL |
|---|---|
| Outcome | Traffic token plus offshore waiting points at the harbour mouth; anchorage slot grid; blocked ships back off; becalmed sailers use their auxiliary; legacy scripted merchants only with `-iwoldnpc`. New hulls (tug, coastal steamer, pilot cutter) and parts (cargo-airship props, gig oars, windmill sails) imported; oar stroke and windmill-by-wind animated; minimap contacts and binocular name tooltip. [src logs/P21.md] |
| NOT MET | Soak `collisions=0 stuck=0` (best 30 collisions / 22 stuck at x8 for 3000 s, seed 1); vision >= 7 (aerial 2, gig 2, cargo props 2, windmill 9); paired bench vs P19 not run [src logs/P21.md] |
| Packaged app | `Ironwake_P21.app` |

| P22 (2026-11-12) | Moving parts and traffic presence |
|---|---|
| Outcome | Gig oars fixed in engine (port/stbd pivot swap, dip axis, draft) with seated rowers; patrol props rebuilt procedurally with blade/disc swap and per-prop rate log; wake FX hysteresis (no eviction); aerial test camera follows traffic; roster 30 ships. [src logs/P22.md] |

| P23 (2026-11-12) | Town placement rewrite: planner in IronwakeP23.cpp (switchback road, contour lanes, clustered houses on benches, retaining walls). Vision 400 m noon 4.5 vs 3 (P17); target 7 NOT met. Bench -2.7..-2.9 fps vs P22. Soak seed 1 not worse. |

| P24 (2026-11-13) | Stepped-bench town, SW headland lighthouse, kerbs & line-traced windows (`Ironwake_P24.app`, commit `e63831a`): `67 houses placed` (build report `P24.md`), `190 wind-bent broadleaf trees` (build report `P24.md`), line-traced windows (`111..146`), street `7.5`, night `7.5` (orchestrator), noon 400 m `4` (PARTIAL). |

| P25 (2026-11-13) | NPC quay-lane reservations, one-way harbour channel & 3-seed soak (`Ironwake_P25.app`, commits `fc7b018`, `651fc75`): `MouthOwner` throat lock + split inbound/outbound lanes (`Y = -34` / `+44..46` m), `9` outer waiting slots (`R = 385..505` m), exclusive berth `(SpanIdx, YInterval)` & corridor locks (`180` s timeout), brig `1.55`x wide turn + `450` m mouth auxiliary rule, `30` ledger ships (`14-16` moving). 3-seed 3000 s x8 soak: `IWNPC collisions=0 stuck=0 groundings=0` (build report `P25.md`) across seeds 1-3; paired bench `33.0 / 33.5` vs P24 `33.3 / 33.0` (`+0.10 FPS` (build report `P25.md`)); aerial harbour vision `5.5 - 6.5 / 10` (build report `P25.md`) (PARTIAL on aerial wake look). |
| P26 (2026-11-13) | Rigged 42 m Tall Brig + 17 moving parts + gaff spanker + WPO wind sails + 4-ship Shipyard (`Ironwake_P26.app`, commits `8b3e3af`, `2de6537`): imported `SM_TallBrig` + 17 `SM_TallBrig_*` spar/sail parts, matte PBR hull/wood (`M_IW_BrigTex`), zero-Clamp WPO `M_IW_SailCanvas`, playable `Curlew` brig (`42` m, `480` t) in 4-ship Shipyard, NPC brigs upgraded with harbour reduced-canvas rule (`moving_parts=119` (build report `P26.md`)). Vision `8.0–10/10` (build report `P26.md`); 1-seed soak `SEED 1: IWNPC collisions=0 stuck=0 groundings=0` (build report `P26.md`); paired bench `35.8 / 34.1` vs P25 `32.8 / 33.0` (`+2.05 FPS` (build report `P26.md`)). PASS. |
| P27 (2026-11-13) | Hydrodynamic Kelvin wake & turbulent prop-wash LUT + hull-foam fix (`Ironwake_P27.app`): imported GPU batch `1024x1024` Havelock `T_IW_KelvinWaveLut`, `T_IW_PropWashFoamLut`, seamless 3-octave Voronoi `T_IW_CellularFoamTile`, and staged CPU `kelvin_lut_256.bin`; upgraded `M_IW_WaterRoot` + `FIronwakeShipWave` to 7 simultaneous ships (camera-centred NPC selection), twin-screw vs single-screw vs pure-sail Voronoi boil + turquoise SSS, lit `M_IW_WakeLut` (`+6` cm lift), and zero hull climb (`0` Metal SM6 material compile errors (build report `P27.md`)). Vision: prop wash `8.5 / 10` (build report `P27.md`), Kelvin wake `8.5 / 10` (build report `P27.md`), sail vs steam `9.5 / 10` (build report `P27.md`), hull waterline `9.5 / 10` (build report `P27.md`), speed/NPC6 `9.5 / 10` (build report `P27.md`); paired bench `36.5`, `33.1` vs P26 `32.7`, `33.4` (`+1.75 FPS` (build report `P27.md`)). PASS. |
| P28 (2026-11-13) | Moving parts, rigged crew/rowers/gulls, batch-4 buildings+signs & triplanar 4K ground (`Ironwake_P28.app`): imported 20 p28/sign FBX bundles (3-LOD crew/gulls) + 4K Megascans `ForestFloor`/`MossyFloor` scans; wired `PaddleSteamerV3` spinning wheels + churn (`8.5 / 10` (build report `P28.md`)), `SalvageTenderV3` slewing crane + pendulum diving bell (`9.5 / 10` (build report `P28.md`)), `PatrolAirshipV4` 3 props (`7.5 / 10` (build report `P28.md`)), 3D `SailorCrewRower` 4-phase gig stroke (`9.5 / 10` (build report `P28.md`)), `SeaGullV4` wings, `SailorCrewStand` helmsmen & HISM dockhands (`moving_parts=122` (build report `P28.md`)), `TownTavernV4` + `TownChandleryV4` + `TownFoundryV4` + painted signs (`9.5 / 10` (build report `P28.md`)), 400 m town (`8.5 / 10` (build report `P28.md`)), and 3-axis triplanar 4K ground (`0` Metal SM6 errors); paired bench `35.9`, `34.5` vs P27 `33.0`, `33.7` (`+1.85 FPS` (build report `P28.md`)). PASS. |
| P29 (2026-11-13) | Data-driven multi-port world (`world_ports.json`), arrival Harbourmaster diorama & 8-good trade logbook (`Ironwake_P29.app`): extracted seeded port-kit generator (`IwPorts::BuildSecondaryPort`) spawning **Ironcrag** (`HostIsleIdx = 8`, basalt coaling/foundry haven) and **Stigsby** (`HostIsleIdx = 9`, wooded fishing/timber outpost) alongside **Greyhaven**; wired full quay/breakwater collision, 5-circle breakwater throat gates, 3-port M-chart (`9.5 / 10` (build report `P29.md`)), 6-port NPC trade routes (`SEED 1: IWNPC collisions=0 stuck=0 groundings=0` (build report `P29.md`), `moving_parts=120` (build report `P29.md`)), `< 2.5` kn `Enter` mooring & salvage verb, live Harbourmaster diorama + `UIronwakeDockWidget` (`10 / 10` (build report `P29.md`), `outside=0 overlaps=0 clipped=0 PASS` (build report `P29.md`)), 8-good cargo market (`final_crowns=1278 net_profit=963` (build report `P29.md`)), and Captain's Logbook (`9.8 / 10` (build report `P29.md`)); port look `7.8 / 10` (build report `P29.md`) noon 400 m, `8.5 / 10` (build report `P29.md`) night 400 m, `10 / 10` (build report `P29.md`) Stigsby 140 m cove; paired bench `33.2`, `33.3` vs P28 `34.0`, `32.9` (`-0.20 FPS` (build report `P29.md`)). PASS. |
| P30 (2026-11-13) | Independent verification, promotion gate (`Packaged/Ironwake/Ironwake.app`), spec check & `build/MORNING_REPORT_3.md`: `20 / 20` (build report `P30.md`) clean offscreen packaged launches (`0` crashes, `0` Metal SM6 errors); Greyhaven 400 m noon `7.5 / 10` (build report `P30.md`) & night `8.5 / 10` (build report `P30.md`) (`7.8 / 10` (build report `P30.md`) isolated); wake speed-scaling sheet `9.8 / 10` (build report `P30.md`) (`0.0000` (build report `P30.md`) hull climb); UI & cold-start defaults (`MUSIC = 0% OFF` (build report `P30.md`), `CAMERA MOTION = LOW` (build report `P30.md`), `IWAUDIO routetest bad=0` (build report `P30.md`)) `10 / 10` (build report `P30.md`); 3-seed 6-port soak `IWNPC collisions=0 stuck=1 groundings=0` (build report `P30.md`), `IWNPC collisions=1 stuck=0 groundings=0` (build report `P30.md`), `IWNPC collisions=2 stuck=3 groundings=0` (build report `P30.md`); paired bench editor-closed `53.10` (build report `P30.md`) MEDIUM (`+1.15 FPS` (build report `P30.md`) vs P19, `+18.15 FPS` (build report `P30.md`) vs W3) and `33.25` (build report `P30.md`) HIGH (`+1.40 FPS` (build report `P30.md`) vs P19); backed up `Ironwake_W3_prev.app` and promoted `Ironwake_P29.app` -> `Packaged/Ironwake/Ironwake.app` (`Promoted Ironwake.app smoke rc=0` (build report `P30.md`)). PASS. |
