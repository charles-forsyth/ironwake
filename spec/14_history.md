# Chapter 14: Card history

Scope: every Kanban card from A through P14 plus D1 (this spec task): goal, outcome, vision scores, commit SHA and packaged app. Commit SHAs are cited to `docs/spec/_gitlog.txt`, a verbatim `git log --format='%h %ad %s'` dump taken 2026-11-11 for this card (D1 itself had only its first commit `84c9efa` at that time). Packaged-app names are cited to `docs/spec/_apps.txt`, a dump of `ls -l ~/Games/Ironwake/*.app`. Outcomes and scores are copied from the card logs in `build/logs/` (which are git-ignored) and the review files. Scores are vision-model scores from the cards themselves and jitter about 1.5 points between identical frames [src build/logs/P5c.md:13]; they are not independent measurements unless the row says orchestrator or Opus.

Order of cards: first night (2026-11-09) A, B, C, D, E; pass 2 (BRIEF line 92) P1, S1, H1, R1, A1, W1, E1, S2; storm cards W2, W3, W4 and I1 on 2026-11-10; P3, P5, P6, P4, P5b, P5c, P7 on 2026-11-10; P8-P14 and D1 on 2026-11-10 night to 2026-11-11. The BRIEF order lines are [src build/BRIEF.md:145], [src build/BRIEF.md:151], [src build/BRIEF.md:154], [src build/BRIEF.md:157], [src build/BRIEF.md:170], [src build/BRIEF.md:181], [src build/BRIEF.md:187], [src build/BRIEF.md:190].

Packaged apps present in `~/Games/Ironwake/`: 23 bundles, listed in the index below. `Ironwake.app` last written Nov 10 10:01 [src docs/spec/_apps.txt:1], which is the W3 water-fix/audio-fix build per the W3 log [src build/logs/W3.md:4]. CONFLICT: SPEC.md line 3 calls it the "older promoted A1-era build" [src SPEC.md:3] and README.txt says "A1/W3-era" [src [REDACTED]/Games/Ironwake/README.txt:1]; the W3 log and file date agree with W3, so treat it as the W3 build. Latest build: Ironwake_P14.app [src SPEC.md:3].

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
| P6 | Opus review | `52796b0` [src docs/spec/_gitlog.txt:37] | none (editor build only) | game 4, look 5, feel 5 |
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

Apps with no card of their own: `Ironwake.app` (W3 water-fix build, Nov 10 10:01) [src docs/spec/_apps.txt:1]; `Ironwake_v1.app` is the D-card build kept as the original [src build/logs/H1.md:3]. P4, P6, P1, S1, W3 and D1 produced no new numbered app (P4 recorded from the editor build [src build/logs/P4.md:8]; P6 reviewed Ironwake_P5.app [src build/REVIEW_opus.md:3]).

## 14.1 Card A: storm ocean level

| Field | Value |
|---|---|
| Goal | Storm ocean level `Ironwake_Sea` from a copy of OceanStorm [src build/logs/A.md:1-3] |
| Outcome | Epic test content removed; ocean hole fixed (spline shrunk to ~6 m); 24-wave Gerstner generator, max wave `8.1 m` [src build/logs/A.md:9]; storm sky, fog, grade, `MI_Ocean_Storm`. No whitecaps: reads as "heavy swell", not "storm" [src build/logs/A.md:22] |
| Scores | A_E `~7/10` [src build/logs/A.md:18] (best), A_N `~6/10` [src build/logs/A.md:17], A_W `~6/10` [src build/logs/A.md:19] |
| Commit | `29730d6` [src docs/spec/_gitlog.txt:141] (final); WIP `683b5be` [src docs/spec/_gitlog.txt:142] |
| Packaged app | none |

Pitfalls recorded: MIs created through MCP rendered no clouds; deleting a referenced MI corrupts its package [src build/logs/A.md:27]. Ocean extents `4000 m` at this card [src build/logs/A.md:8]; later raised (I1 8.4 km, P5c 300 km).

## 14.2 Card B: playable ship pawn

| Field | Value |
|---|---|
| Goal | C++ `AIronwakeShipPawn` with heavy telegraph/rudder controls, HUD, wake [src build/logs/B.md:1-3] |
| Outcome | Works in PIE: `13.0 kn` at 34 s, `15.9 kn` at 90 s on FLANK [src build/logs/B.md:35]; hard starboard turns about 0.6-0.7 deg/s [src build/logs/B.md:36]. `12/12` pontoons wet [src build/logs/B.md:34]. Ironclad mesh `SM_Ironclad` from Hunyuan GLB, 140 m [src build/logs/B.md:29-31] |
| Scores | B_1 `~6/10` [src build/logs/B.md:41], B_2 `~4-5/10` [src build/logs/B.md:43], B_3 `~4/10` [src build/logs/B.md:45], overall visuals ~5/10 [src build/logs/B.md:41-47] |
| Commit | `da3a74c` [src docs/spec/_gitlog.txt:132] (done); module start `0030f0a` [src docs/spec/_gitlog.txt:140] |
| Packaged app | none |

## 14.3 Card C: gunnery and one NPC enemy

| Field | Value |
|---|---|
| Goal | Ballistic shells, turret traverse, damage/fire/sinking, NPC utility-AI ironclad, combat audio [src build/logs/C.md:1-5] |
| Outcome | Verified in PIE: enemy spawned 3956 m away, fled below 35% HP and sank at t about 60 s; `36` shells, `14` hits, `6` fires [src build/logs/C.md:54]. Muzzle velocity `600 m/s`, max range `10 km` [src build/logs/C.md:8]; player `160 HP`, NPC `120 HP`, `6` per shell [src build/logs/C.md:16] |
| Scores | C_1 ~7, C_2 8, C_3 6, C_4 8, C_5 8 [src build/logs/C.md:63-67]; overall ~6-7/10 [src build/logs/C.md:70] |
| Commit | `77db4b0` [src docs/spec/_gitlog.txt:130] (code), FX `3781a8b` [src docs/spec/_gitlog.txt:131], log `59ba5d8` [src docs/spec/_gitlog.txt:129] |
| Packaged app | none |

## 14.4 Card D: living sea, airships, menu, package, clip

| Field | Value |
|---|---|
| Goal | Living sea (3 merchants, airship, island + lighthouse), C++ UMG title/pause menu, package, 59 s clip [src build/logs/D.md:1-7] |
| Outcome | Packaged app verified end to end; `-iwtour` run: `IWCDEMO done t=70.6 shots=32 hits=21` [src build/logs/D.md:52]. Clip `slice_v1.mp4` 59 s 1280x720 [src build/logs/D.md:71]. Real package is `Saved/StagedBuilds/Mac/IronwakeTest.app` copied with ditto [src build/logs/D.md:88] |
| Scores | D_4 airship 10, D_3 merchant 8, D_1 title 5, D_2 island 4 [src build/logs/D.md:60-64]; clip honest score `6.5/10` [src build/logs/D.md:75] |
| Commit | `b0874af` [src docs/spec/_gitlog.txt:128] (living sea/menu), package prep `b18012f` [src docs/spec/_gitlog.txt:127], log `6514293` [src docs/spec/_gitlog.txt:125] |
| Packaged app | first package at `~/Games/Ironwake/Ironwake.app`, later kept as `Ironwake_v1.app` [src docs/spec/_apps.txt:23] |

Known issues carried: island invisible in fog (4/10), plain menu, merchants invulnerable, bundle id `com.YourCompany.IronwakeTest` [src build/logs/D.md:91].

## 14.5 Card E: independent review

| Field | Value |
|---|---|
| Goal | Independent play-through and morning report; changed nothing in the game [src build/logs/E.md:1-3] |
| Outcome | Report build/MORNING_REPORT.md. Top finding: an idle Captain-mode player sinks `121 s` after start [src build/MORNING_REPORT.md:32]. Top 5 next steps at [src build/MORNING_REPORT.md:58-63] became pass 2 |
| Scores | slice as a game `5/10` [src build/MORNING_REPORT.md:55], look `4.5/10` [src build/MORNING_REPORT.md:56] |
| Commit | `fc9c4a6` [src docs/spec/_gitlog.txt:124] |
| Packaged app | reviewed Ironwake.app (first build) |

## 14.6 Card P1: fair opening + game over/restart

| Field | Value |
|---|---|
| Goal | Enemy 7.5 km out in front, hold fire, SHIP LOST / VICTORY screens, camera over the bow, Peaceful default [src build/logs/P1.md:1-5] |
| Outcome | Spawn moved to (-3000, -2700) m heading `045` [src build/logs/P1.md:7]; enemy holds fire `60 s` or until under `5 km` [src build/logs/P1.md:12]; `MinAimRangeCm` `1.5 km` [src build/logs/P1.md:23]; PIE tests for idle, sink, restart, menu, victory all pass [src build/logs/P1.md:30-36] |
| Scores | SHIP LOST card `6/10` [src build/logs/P1.md:44], VICTORY `7.5/10` [src build/logs/P1.md:46] |
| Commit | `3590950` [src docs/spec/_gitlog.txt:118] |
| Packaged app | none (P5 re-packaged) [src build/logs/P1.md:56] |

## 14.7 Card S1: peaceful sailing feel (Voyager)

| Field | Value |
|---|---|
| Goal | Peaceful default, POIs, compass/minimap HUD, helmsman, anchor, fast sail, photo mode, weather cycle, gulls [src build/logs/S1.md:1-8] |
| Outcome | Standalone -game tour: arrived at Wreck t=`114.9` [src build/logs/S1.md:46], Needles `287.6`, lighthouse `463.3` [src build/logs/S1.md:47-48]; anchor 12.1 kn to 0.2 kn in 16 s [src build/logs/S1.md:28]; `22` gulls [src build/logs/S1.md:39] |
| Scores | sailing to places `7/10` [src build/logs/S1.md:69], look of new places `4-5/10`, weather/time `3-4/10` [src build/logs/S1.md:69-71]; HUD 8, wreck arrival 8, dusk 3 [src build/logs/S1.md:59-65] |
| Commit | `38c89f2` [src docs/spec/_gitlog.txt:115] |
| Packaged app | none (not packaged) [src build/logs/S1.md:3] |

## 14.8 Card H1: 60 FPS, tight handling, WoWS camera, music off

| Field | Value |
|---|---|
| Goal | Fix 6 FPS and drifty handling; World-of-Warships camera; music off [src build/logs/H1.md:1-3] |
| Outcome | Median FPS v1 `16.1` to H1 MEDIUM `60.0` [src build/logs/H1.md:11-13]; cloud deck 41 ms to 2-3 ms [src build/logs/H1.md:21]; STOP to HALF 90% `10.0 s` [src build/logs/H1.md:41-42]; bow starts swinging `0.84 s` [src build/logs/H1.md:43]; presets LOW/MEDIUM/HIGH [src build/logs/H1.md:29-32] |
| Scores | zoomed-out camera 8-10/10; default view `3.5-4/10` vs WoWS [src build/logs/H1.md:68-70] |
| Commit | `ee306ce` [src docs/spec/_gitlog.txt:111] |
| Packaged app | `Ironwake.app` [src docs/spec/_apps.txt:1] at that time; previous kept as `Ironwake_v1.app` [src docs/spec/_apps.txt:23]; later file Ironwake_H1.app `Ironwake_H1.app` [src docs/spec/_apps.txt:4] |

## 14.9 Card R1: hi-res library ships

| Field | Value |
|---|---|
| Goal | Swap Hunyuan hulls for library CC-BY hulls (Devastation, Kracken, Goliath, Aglae, brass airship) [src build/logs/R1.md:1-12] |
| Outcome | Waterline found `4.7 m` apparent offset (`RenderedSeaOffsetCm = -470`) [src build/logs/R1.md:21], later shown WRONG in W3; paired FPS H1 `24.2` vs R1 `32.0` under load [src build/logs/R1.md:27]; airship close-up speckles traced to decimation [src build/logs/R1.md:47]; in-game CREDITS panel added [src build/logs/R1.md:50] |
| Scores | player close 8.5/10 (blunt `3.5-4`) [src build/logs/R1.md:33]; airship wide 7.5, close-up 4 [src build/logs/R1.md:36-39] |
| Commit | `e132d9f` [src docs/spec/_gitlog.txt:105] (swap); assets `1141efd` [src docs/spec/_gitlog.txt:107]; log `01f670d` [src docs/spec/_gitlog.txt:103] |
| Packaged app | `Ironwake_R1.app` [src docs/spec/_apps.txt:18] |

## 14.10 Card A1: autopilot + repackage + "looks like it's sinking" fix

| Field | Value |
|---|---|
| Goal | P cycles OFF/HOLD/WAYPOINTS/TOUR, M chart, give way, under-fire disengage [src build/logs/A1.md:1-12] |
| Outcome | Hold course max error `0.20 deg`, rms `0.07` [src build/logs/A1.md:21]; 3-waypoint route done in `504 s` [src build/logs/A1.md:27]; TOUR done in `1647 s` [src build/logs/A1.md:28]; boot-top clearance `+0.66 m` calm [src build/logs/A1.md:40]; `DraftBelowBootTopCm` 50 to -60 [src build/logs/A1.md:34] |
| Scores | chart 7.5, autopilot HUD 10, waterline calm 10, storm 9.5 [src build/logs/A1.md:31-42] |
| Commit | `350d307` [src docs/spec/_gitlog.txt:93] (autopilot); log `6918aa0` [src docs/spec/_gitlog.txt:92] |
| Packaged app | `Ironwake_A1.app` [src docs/spec/_apps.txt:2]; W1 build kept as Ironwake_W1.app [src build/logs/A1.md:3] |

Known: orbit entry cuts inside hazard rings (~70 m inside the Needles ring) [src build/logs/A1.md:45].

## 14.11 Card W1: heavy ship physics, real bow wave, bigger sea

| Field | Value |
|---|---|
| Goal | Chuck 12:20: ship too buoyant, bow splash through the hull, sea too waveless [src build/logs/W1.md:3] |
| Outcome | Pontoons replaced by waterplane fit with heave 8 s, pitch 8.5 s, roll 13 s natural periods [src build/logs/W1.md:9]; hull/sea heave ratio `0.36` moderate stopped [src build/logs/W1.md:19]; settle `4.7 s` [src build/logs/W1.md:22]; new `UIronwakeBowWave`; sea presets overcast `1.0`, dusk `0.65`, foggy night `1.6` [src build/logs/W1.md:50-52] |
| Scores | bow wave 2-3 to `7/10` after foam fix; bow dig `0-1/10`; sea `3-4/10` [src build/logs/W1.md:58-60]; W1 `35.9` vs R1 `33.3` fps under load [src build/logs/W1.md:68] |
| Commit | `83a9d2d` [src docs/spec/_gitlog.txt:100] (physics + bow wave); log `a96130f` [src docs/spec/_gitlog.txt:94] |
| Packaged app | `Ironwake_W1.app` [src docs/spec/_apps.txt:20] |

## 14.12 Card E1: real world assets

| Field | Value |
|---|---|
| Goal | Replace greybox harbour, cliffs, wreck with scans and Hunyuan steampunk pieces [src build/logs/E1.md:1-5] |
| Outcome | `33` meshes imported with Nanite [src build/logs/E1.md:11]; Greyhaven, St Brannoc, Needles `95/74/55/36/22 m`, Wreck of the Valiant rebuilt [src build/logs/E1.md:28]; day fog density `0.020` to `0.014` [src build/logs/E1.md:34]; FPS `60.0` median harbour/island/tour on a quiet machine [src build/logs/E1.md:58-60] |
| Scores | harbour close 8.5, island 7.5, wreck close 9, needles 1 km 7.5, airship close 9 [src build/logs/E1.md:41-49]; orchestrator fixes: no grey boxes `9.5/10` [src build/logs/E1.md:65] |
| Commit | `c84ea33` [src docs/spec/_gitlog.txt:91] (assets); log `54855f3` [src docs/spec/_gitlog.txt:87]; orchestrator fixes `9d17cb5` [src docs/spec/_gitlog.txt:88] |
| Packaged app | `Ironwake_E1.app` [src docs/spec/_apps.txt:3] |

## 14.13 Card S2: roster, shipyard, sailing frigate

| Field | Value |
|---|---|
| Goal | Sailing frigate default, Kittiwake cutter, Devastation unlock, sail rig, points of sail [src build/logs/S2.md:1-13] |
| Outcome | Roster in `IronwakeFleet.{h,cpp}`: Albatross `88 m`, `2,200 t`, turn `R 210 m` (coded values change later: ch. 2) [src build/logs/S2.md:9]; points of sail e.g. beam reach `9.3-10.6 kn`, close-hauled `5.8-6.1 kn` [src build/logs/S2.md:26-28]; yard braces 38/27/18/1.5 deg [src build/logs/S2.md:17]; shipyard LAUNCH crash fixed (dangling static material ptr) [src build/logs/S2.md:46] |
| Scores | frigate waterline 9.5, cutter 10, ironclad 10, close zoom 9.5, shipyard 8.5 [src build/logs/S2.md:40-52] |
| Commit | `3d72afb` [src docs/spec/_gitlog.txt:84] (sail rig); `fe319ee` [src docs/spec/_gitlog.txt:85] (roster wip); `37f7069` [src docs/spec/_gitlog.txt:81] (waterline); log `2f1161f` [src docs/spec/_gitlog.txt:78] |
| Packaged app | `Ironwake_S2.app` [src docs/spec/_apps.txt:19] |

## 14.14 Card W2: big seas, weather, open-ocean storm

| Field | Value |
|---|---|
| Goal | Five sea states, storm director, deep-sea fade, STORM AT SEA start, evil-storm look [src build/logs/W2.md:1-3] |
| Outcome | Hs measured CALM `0.9 m`, MODERATE `2.1 m`, ROUGH `4.0 m`, STORM `10.5 m` [src build/logs/W2.md:12]; wind 8/16/24/36/48 kn [src build/logs/W2.md:13]; round 2 proved rendered waves match query (rendered minus query mean `+0.24 m`) [src build/logs/W2.md:85]; own ocean material chain `M_IW_WaterRoot` [src build/logs/W2.md:96]; storm bench `25.5` fps with editor open [src build/logs/W2.md:53] |
| Scores | storm look 0-3.5/10: calm 2, rough 0, storm chase `3.5`, round 2 best `3.5/10` [src build/logs/W2.md:36-41], [src build/logs/W2.md:69] |
| Commit | `b7032b6` [src docs/spec/_gitlog.txt:77] (systems); round 2 `8a86297` [src docs/spec/_gitlog.txt:74]; log `d344fc0` [src docs/spec/_gitlog.txt:75] |
| Packaged app | `Ironwake_W2.app` [src docs/spec/_apps.txt:21]; Ironwake.app stays S2 [src build/logs/W2.md:3] |

## 14.15 Card I1: archipelago

| Field | Value |
|---|---|
| Goal | Many islands to explore with surf and shelf integration [src build/logs/I1.md:1-9] |
| Outcome | `14` new seeded islands (seed `1871`), total `18` places [src build/logs/I1.md:6]; C++ scatter, `23` HISM components, ~`6,000` instances [src build/logs/I1.md:25]; ocean extents 8 to `8.4 km` [src build/logs/I1.md:32]; FPS NOT verified at 60: I1 `32.7` tour, `22.7-24.0` five-island bench under load [src build/logs/I1.md:64-66] |
| Scores | chart 8.5, far silhouettes 7.5-8, stacks/rocky 6.5-7.5, pines 6-7, fort `3-5` at range [src build/logs/I1.md:44-54] |
| Commit | `aea70ca` [src docs/spec/_gitlog.txt:67] (final); wip `686e233` [src docs/spec/_gitlog.txt:72]; surf/shelf `470ac87` [src docs/spec/_gitlog.txt:70] |
| Packaged app | `Ironwake_I1.app` [src docs/spec/_apps.txt:5]; Ironwake.app not promoted [src build/logs/I1.md:3] |

## 14.16 Card W3: two-layer water bug + readable storm + ship FX

| Field | Value |
|---|---|
| Goal | Chuck 09:20 two-layer water bug first; readable evil storm second [src build/logs/W3.md:1-3] |
| Outcome | ROOT CAUSE: R1 gauge reader mistook post bands seen through translucent water for the surface; fix `RenderedSeaOffsetCm` to 0 and drafts frigate `-40`, cutter `-25`, ironclad `-50` cm; rendered minus query `-0.08 m` sd `0.09` [src build/logs/W3.md:9-17]; audio: harbour_ambience removed [src build/logs/W3.md:19]; packaged FPS `33.8` median, GPU `29.4 ms` [src build/logs/W3.md:55] |
| Scores | storm `4-5.5/10`: chase 4, whitecaps 2.5, trough 5.5, bow rough 5.5, wake_high 0 [src build/logs/W3.md:39-45] |
| Commit | `2d64892` [src docs/spec/_gitlog.txt:64] (water fix); `28de2c7` [src docs/spec/_gitlog.txt:63] (audio); storm `c859910` [src docs/spec/_gitlog.txt:62] |
| Packaged app | water-fix build became Ironwake.app (`Ironwake.app` [src docs/spec/_apps.txt:1]); storm work not packaged [src build/logs/W3.md:4] |

## 14.17 Card W4: finish the readable storm

| Field | Value |
|---|---|
| Goal | Fix combed stripes, coverage runaway, invisible wake, spume, bow wave; promotion gate storm >= 6 [src build/logs/W4.md:1-4] |
| Outcome | Mip-filtered foam fixes stripes; coverage ROUGH `3.6-10 %` after calibration [src build/logs/W4.md:11]; wake from 200 m `9.5/10` [src build/logs/W4.md:12]; spume still 1-2/10; bow wave `4/10` [src build/logs/W4.md:14]; calm `8.5/10` [src build/logs/W4.md:21]. Structural verdict: shader foam on 16-wave Gerstner reads as flat decals [src build/logs/W4.md:25] |
| Scores | storm 4-6/10, gate not met, NOT promoted [src build/logs/W4.md:3] |
| Commit | `e8aea61` [src docs/spec/_gitlog.txt:55] (r2); `11cb827` [src docs/spec/_gitlog.txt:58] (r1); `add81de` [src docs/spec/_gitlog.txt:54] (r3); log `618557c` [src docs/spec/_gitlog.txt:53] |
| Packaged app | `Ironwake_W4.app` [src docs/spec/_apps.txt:22] (side-by-side only) |

## 14.18 Card P3: handling + sea look + voyage

| Field | Value |
|---|---|
| Goal | Tighter turning, 18-place voyage, ship FX fixes [src build/logs/P3.md:1-12] |
| Outcome | Circle times ironclad `240.8 s` to `201.4 s`, frigate `252.3 s` to `222.2 s` [src build/logs/P3.md:4-5]; voyage 18/18 waypoints, `30.4 km` in `1221 s` [src build/logs/P3.md:12]; BowSheet sprites cut from 170 to `60` [src build/logs/P3.md:16]; FPS `13.3` P3 vs `12.8` under load (no claim) [src build/logs/P3.md:25] |
| Scores | hull-side frames still 0-2/10 vision (flat foam band) [src build/logs/P3.md:18]; Ironwake.app NOT promoted [src build/logs/P3.md:33] |
| Commit | `f5bb53c` [src docs/spec/_gitlog.txt:44] (log); `691e385` [src docs/spec/_gitlog.txt:49] (tighter turning) |
| Packaged app | `Ironwake_P3.app` [src docs/spec/_apps.txt:11] |

## 14.19 Card P5: package + gates + MORNING_REPORT_2

| Field | Value |
|---|---|
| Goal | Package Ironwake_P5.app, run gates [src build/MORNING_REPORT_2.md:1-5] |
| Outcome | Bundle id `com.omstead.ironwake` [src build/MORNING_REPORT_2.md:3]; P5 `15.9` fps vs `14.6` current with editor open [src build/MORNING_REPORT_2.md:15]; waterline frigate `+0.39 m`, cutter `+0.23 m`, ironclad `+0.59 m` [src build/MORNING_REPORT_2.md:20]; promotion gate NOT met: Ironwake.app not replaced [src build/MORNING_REPORT_2.md:27] |
| Scores | no vision scoring; pause menu not verified by P5 (later verified by P6) |
| Commit | `b9f0201` [src docs/spec/_gitlog.txt:41] |
| Packaged app | `Ironwake_P5.app` [src docs/spec/_apps.txt:12] |

## 14.20 Card P6: Opus phase review (read-only)

| Field | Value |
|---|---|
| Goal | Judge the phase as an outside reviewer on Ironwake_P5.app [src build/REVIEW_opus.md:3-12] |
| Outcome | Verdict `ship with caveats` [src build/REVIEW_opus.md:12]; measured FPS editor closed `43.6` spawn, `45.3` islands [src build/REVIEW_opus.md:23]; found turning frame-rate dependent: ironclad `221 s` at 30 fps vs `288 s` at 19 fps [src build/REVIEW_opus.md:21]; frigate has no boot-top band [src build/REVIEW_opus.md:22]; water loops carry 393/407 Hz tone [src build/REVIEW_opus.md:24] |
| Scores | game `4/10` [src build/REVIEW_opus.md:27], look `5/10` [src build/REVIEW_opus.md:28], feel `5/10` [src build/REVIEW_opus.md:29] |
| Commit | `52796b0` [src docs/spec/_gitlog.txt:37] |
| Packaged app | reviewed `Ironwake_P5.app` [src docs/spec/_apps.txt:12] |

## 14.21 Card P4: gameplay clip v2 (peaceful)

| Field | Value |
|---|---|
| Goal | 49.5 s peaceful sailing clip `slice_v2.mp4` [src build/logs/P4.md:2] |
| Outcome | Nine scripted shots of 5.5 s each, recorded with `-benchmark -fps=30 -dumpmovie` from the editor build (not packaged) [src build/logs/P4.md:3-8]; storm look not used [src build/logs/P4.md:17] |
| Scores | honest ~`7/10`; per shot 7.5-9; flat bright wake ribbon still the weak point [src build/logs/P4.md:18-19] |
| Commit | `2b51e78` [src docs/spec/_gitlog.txt:33] |
| Packaged app | none (editor build) [src build/logs/P4.md:8] |

## 14.22 Card P5b: speed-scaled ship FX + grey funnel smoke

| Field | Value |
|---|---|
| Goal | Verify f8b5bdc speed scaling with a ladder: 3 hulls x {4, 8, 13 kn} x {calm, moderate} [src build/logs/P5b.md:4] |
| Outcome | New `NS_IW_FunnelSmoke` grey 0.35 alpha 0.3 [src build/logs/P5b.md:3]; strict slam gate never fired (`BowSubmergeM` max `-0.57 m`) [src build/logs/P5b.md:8] |
| Scores | cutter side 13 kn `3/10`, cutter 4 kn `7/10`, ironclad 8.5/8/5.5, funnel chase `8.5/10` [src build/logs/P5b.md:6-7]; pass rule NOT fully met [src build/logs/P5b.md:10] |
| Commit | `a903a96` [src docs/spec/_gitlog.txt:35]; Chuck rule `f8b5bdc` [src docs/spec/_gitlog.txt:40]; grey smoke `0eee597` [src docs/spec/_gitlog.txt:38] |
| Packaged app | `Ironwake_P5b.app` [src docs/spec/_apps.txt:13] |

## 14.23 Card P5c: ship water look pass

| Field | Value |
|---|---|
| Goal | Soft wake, patchy hull foam, bow cushion, slam gate, funnel smoke, seam [src build/logs/P5c.md:1-10] |
| Outcome | Wake `6` verts with feathered edges, centre opacity x`0.55` [src build/logs/P5c.md:5]; slam counts calm 0, moderate `8`, rough `35` per 60 s [src build/logs/P5c.md:8]; funnel smoke alpha 0.75, 14/s [src build/logs/P5c.md:9]; ocean/zone extents to `300 km`, seam gone [src build/logs/P5c.md:10] |
| Scores | cutter side 7, frigate chase wake 3-7 (unstable); pass rule NOT fully met [src build/logs/P5c.md:14-15]; SPEC.md cites ship water at 13 kn ~8 [src SPEC.md:83] |
| Commit | `6f75630` [src docs/spec/_gitlog.txt:30] (final); WIP `98974b0` [src docs/spec/_gitlog.txt:31] |
| Packaged app | `Ironwake_P5c.app` [src docs/spec/_apps.txt:14] |

## 14.24 Card P7: frame-rate-independent handling

| Field | Value |
|---|---|
| Goal | Fix turning that slowed at low FPS (P6); bench P5 vs P5c; frigate smoke [src build/logs/P7.md:1-14] |
| Outcome | Physics substepping `bSubstepping=True`, max `1/60`, `6` substeps [src Config/DefaultEngine.ini:37-40]; circle times after fix ironclad `178.3 s` at 30 and 18 fps, frigate `192.1`, cutter `32.9` [src build/logs/P7.md:19-21]; P7 `59.0` fps avg [src build/logs/P7.md:11]; aux stack moved to (-23 m, 23 m) scale `0.9` [src build/logs/P7.md:27] |
| Scores | pass within 5% for all three hulls [src build/logs/P7.md:22] |
| Commit | `c63b246` [src docs/spec/_gitlog.txt:28] (substepping); `7bed173` [src docs/spec/_gitlog.txt:27] (logs) |
| Packaged app | `Ironwake_P7.app` [src docs/spec/_apps.txt:15] |

## 14.25 Card P8: cluster models, NASA sky v1, ironclad turn

| Field | Value |
|---|---|
| Goal | Import 17 Ursa Hunyuan models, build town + NPC craft, NASA night sky [src build/logs/P8.md:1-8] |
| Outcome | `17` keep GLBs imported with Nanite + simple collision [src build/logs/P8.md:7]; ironclad `MinTurnRadiusM` 280 to `330` (Chuck OK 21:25), circle `208.8 s` [src build/logs/P8.md:22]; paired FPS 33.1/33.5 [src build/logs/P8.md:20] |
| Scores | BOTH TARGETS MISSED: town 1-2/10 [src build/logs/P8.md:11]; sky `1-2.5/10` over five iterations, best Cygnus `3.5/10` [src build/logs/P8.md:15]; targets missed [src build/logs/P8.md:4] |
| Commit | `71d6c10` [src docs/spec/_gitlog.txt:21] (final); import `039a79f` [src docs/spec/_gitlog.txt:25]; turn `b70985d` [src docs/spec/_gitlog.txt:24] |
| Packaged app | `Ironwake_P8.app` [src docs/spec/_apps.txt:16] |

## 14.26 Card P9: real collision + grounding

| Field | Value |
|---|---|
| Goal | Analytic collision for rocks, piers, buildings, hulls; depth grounding; AvoidLand [src build/logs/P9.md:1-8] |
| Outcome | `1108` solids [src build/logs/P9.md:5]; all three hulls PASS beach, stack, breakwater, pier, ship-vs-ship, astern-frees, autopilot around island (`137 s`, `117 s`, `198 s`) [src build/logs/P9.md:13-15]; open-water circle unchanged `208.8 s` [src build/logs/P9.md:17]; bench P9 33.6/33.4 [src build/logs/P9.md:19] |
| Scores | pass by overhead frames + numeric pen=0; side views unreliable [src build/logs/P9.md:24] |
| Commit | `3c392c5` [src docs/spec/_gitlog.txt:18] |
| Packaged app | `Ironwake_P9.app` [src docs/spec/_apps.txt:17] |

## 14.27 Card P10: HDR night sky + town v1

| Field | Value |
|---|---|
| Goal | Fix P8 sky and town misses [src build/logs/P10.md:1-3] |
| Outcome | Real HDR `starmap_2020_8k.exr` as RGBA16F [src build/logs/P10.md:5]; locked exposure at clear night; `54` houses, `27` gas lamps, `231` window quads [src build/logs/P10.md:13]; bench P9 33.4 / P10 33.7 [src build/logs/P10.md:19] |
| Scores | sky PASS: Orion `9.5/10`, Cygnus/Milky Way 9-10 [src build/logs/P10.md:4]; town PARTLY `3.5-5.5/10` target 7 NOT met [src build/logs/P10.md:12-14] |
| Commit | `7f8417b` [src docs/spec/_gitlog.txt:16] |
| Packaged app | `Ironwake_P10.app` [src docs/spec/_apps.txt:6] |

## 14.28 Card P11: living world, hum, help panel

| Field | Value |
|---|---|
| Goal | Clock + weather, sea by weather, regenerate hum-free loops, keyboard help [src build/logs/P11.md:1-3] |
| Outcome | Loops regenerated: bow `+3.6 dB`, wake `+5.3`, hiss `+1.1` over median [src build/logs/P11.md:6]; clock start `09:30`, `48` real min/day [src build/logs/P11.md:9]; sea per weather clear `0.55` to squall `2.7` [src build/logs/P11.md:11]; harbour Hs basin `1.0 m` vs offshore `2.1 m` [src build/logs/P11.md:17]; bench P11 33.7/33.3 [src build/logs/P11.md:27] |
| Scores | night 9.5, rain 10, dawn 4, dusk 3-5, night/rain/dawn/dusk [src build/logs/P11.md:24]; harbour moderate `4/10` [src build/logs/P11.md:17] |
| Commit | `c593db4` [src docs/spec/_gitlog.txt:13] (final); wip `0543c7a` [src docs/spec/_gitlog.txt:14] |
| Packaged app | `Ironwake_P11.app` [src docs/spec/_apps.txt:7] |

## 14.29 Card P12: terraced harbour town

| Field | Value |
|---|---|
| Goal | Make the town read from the sea (second pass) [src build/logs/P12.md:1-4] |
| Outcome | `5` terraces in `5.5 m` steps, ~`74` houses, `7` tints, ~`105` chimneys, ~`39 m` spire [src build/logs/P12.md:7-9]; fixed P11 lamp inversion (SmoothStep A < B) [src build/logs/P12.md:14]; bench P12 32.7/32.9 [src build/logs/P12.md:27] |
| Scores | PARTLY: day `3.5-4/10` at 400 m, ~1-3 at 900 m, night 6.5; target 7 NOT met [src build/logs/P12.md:4] |
| Commit | `b8a636a` [src docs/spec/_gitlog.txt:11] |
| Packaged app | `Ironwake_P12.app` [src docs/spec/_apps.txt:8] |

## 14.30 Card P13: light and sea polish

| Field | Value |
|---|---|
| Goal | Dawn/dusk ramps, moonlight, overcast, sea by weather, night exposure; paused by orchestrator 08:52 before all targets met [src build/logs/P13.md:1-3] |
| Outcome | Dawn ~`6.5`, dusk ~`7.5` [src build/logs/P13.md:7]; moon peak `2.5` [src build/logs/P13.md:9]; night exposure EV `1.0` to `0.4`, window emissive x0.25, lamps x0.3 [src build/logs/P13.md:27]; bench P13 31.3/32.1 [src build/logs/P13.md:21] |
| Scores | overcast 8-9; clear noon still `3.5-5`; night 3.5-5.5 then `7` after resume; squall 2-8 [src build/logs/P13.md:14-17] |
| Commit | `e3bfcd3` [src docs/spec/_gitlog.txt:9] (main); night pass `5e07bf8` [src docs/spec/_gitlog.txt:5] |
| Packaged app | `Ironwake_P13.app` [src docs/spec/_apps.txt:9] |

## 14.31 Card P14: clear noon, night sea, squall swell, harbour chop

| Field | Value |
|---|---|
| Goal | P13 leftovers [src build/logs/P14.md:1-4] |
| Outcome | Clear noon 400 m `7.5` (was 3.5-4.5), 900 m `8.5` [src build/logs/P14.md:7]; night chase 8.5, high 7.5 [src build/logs/P14.md:8]; squall chase `6.5` [src build/logs/P14.md:9]; harbour high `9` via flat water-material slab [src build/logs/P14.md:10]; bench P14 32.4/32.6 [src build/logs/P14.md:17] |
| Scores | NOT met: squall high `4/10`; plateau flat top [src build/logs/P14.md:13-14]. Orchestrator re-score of clear noon `5.5` vs worker 7.5-8.5 [src SPEC.md:84] |
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
| Open-sea storm (parked) | 5-6 (A) | 4-6 (W4) | A, W2 3.5, W3 4-5.5, W4 4-6; parked [src build/logs/W4.md:3] |
| Night sky | 1-2.5 (P8) | 9.5 (P10) | [src build/logs/P8.md:15], [src build/logs/P10.md:4] |
| Town from sea, day | 1-2 (P8) | 7.5 (P14, worker); 5.5 (orchestrator) | P10 3.5-5.5, P12 3.5-4, P14 7.5 [src SPEC.md:84] |
| Ship water at 13 kn | 0-2 (P3 hull-side) | ~8 (P5c, SPEC) | [src build/logs/P3.md:18], [src SPEC.md:83] |
| Waterline | wrong (R1 -470 cm) | one surface (W3) | [src build/logs/W3.md:9-17] |
| FPS | `16.1` v1 | `60.0` H1/E1 quiet machine; ~33 loaded | [src build/logs/H1.md:11], [src SPEC.md:89] |

## Gaps / unknown

- `docs/spec/_gitlog.txt` and `_apps.txt` are snapshots; commits made after this card (including the D1 commit that lands the chapters) are not listed. Re-run `git log` to refresh.
- Commits for P5b/P5c/P7/P12/P14 listed are the main code commits; several cards have multiple commits (see git log). A card-to-commit mapping is by subject prefix only.
- Vision scores are single-model and noisy; the only independent scores are Opus P6 and orchestrator re-scores quoted from SPEC.md. P14 orchestrator 5.5 for noon is the only recorded gap against a worker score.
- No packaged app exists for W3, S1, P1, P4, P6 or D1. W3's water-fix is in `Ironwake.app` per its log, which this chapter could not verify against the binary (no launch allowed).
- P2 (enemy visibility) was never run (parked); card file exists at build/cards2/P2_visibility.md.
- The self-check output is appended to this file by the orchestrator.
# Chapter 14: Card history

Scope: every Kanban card from A through P14 plus D1 (this spec task): goal, outcome, vision scores, commit SHA and packaged app. Commit SHAs are cited to `docs/spec/_gitlog.txt`, a verbatim `git log --format='%h %ad %s'` dump taken 2026-11-11 for this card (D1 itself had only its first commit `84c9efa` at that time). Packaged-app names are cited to `docs/spec/_apps.txt`, a dump of `ls -l ~/Games/Ironwake/*.app`. Outcomes and scores are copied from the card logs in `build/logs/` (which are git-ignored) and the review files. Scores are vision-model scores from the cards themselves and jitter about 1.5 points between identical frames [src build/logs/P5c.md:13]; they are not independent measurements unless the row says orchestrator or Opus.

Order of cards: first night (2026-11-09) A, B, C, D, E; pass 2 (BRIEF line 92) P1, S1, H1, R1, A1, W1, E1, S2; storm cards W2, W3, W4 and I1 on 2026-11-10; P3, P5, P6, P4, P5b, P5c, P7 on 2026-11-10; P8-P14 and D1 on 2026-11-10 night to 2026-11-11. The BRIEF order lines are [src build/BRIEF.md:145], [src build/BRIEF.md:151], [src build/BRIEF.md:154], [src build/BRIEF.md:157], [src build/BRIEF.md:170], [src build/BRIEF.md:181], [src build/BRIEF.md:187], [src build/BRIEF.md:190].

Packaged apps present in `~/Games/Ironwake/`: 23 bundles, listed in the index below. `Ironwake.app` last written Nov 10 10:01 [src docs/spec/_apps.txt:1], which is the W3 water-fix/audio-fix build per the W3 log [src build/logs/W3.md:4]. CONFLICT: SPEC.md line 3 calls it the "older promoted A1-era build" [src SPEC.md:3] and README.txt says "A1/W3-era" [src [REDACTED]/Games/Ironwake/README.txt:1]; the W3 log and file date agree with W3, so treat it as the W3 build. Latest build: Ironwake_P14.app [src SPEC.md:3].

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
| P6 | Opus review | `52796b0` [src docs/spec/_gitlog.txt:37] | none (editor build only) | game 4, look 5, feel 5 |
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

Apps with no card of their own: `Ironwake.app` (W3 water-fix build, Nov 10 10:01) [src docs/spec/_apps.txt:1]; `Ironwake_v1.app` is the D-card build kept as the original [src build/logs/H1.md:3]. P4, P6, P1, S1, W3 and D1 produced no new numbered app (P4 recorded from the editor build [src build/logs/P4.md:8]; P6 reviewed Ironwake_P5.app [src build/REVIEW_opus.md:3]).

## 14.1 Card A: storm ocean level

| Field | Value |
|---|---|
| Goal | Storm ocean level `Ironwake_Sea` from a copy of OceanStorm [src build/logs/A.md:1-3] |
| Outcome | Epic test content removed; ocean hole fixed (spline shrunk to ~6 m); 24-wave Gerstner generator, max wave `8.1 m` [src build/logs/A.md:9]; storm sky, fog, grade, `MI_Ocean_Storm`. No whitecaps: reads as "heavy swell", not "storm" [src build/logs/A.md:22] |
| Scores | A_E `~7/10` [src build/logs/A.md:18] (best), A_N `~6/10` [src build/logs/A.md:17], A_W `~6/10` [src build/logs/A.md:19] |
| Commit | `29730d6` [src docs/spec/_gitlog.txt:141] (final); WIP `683b5be` [src docs/spec/_gitlog.txt:142] |
| Packaged app | none |

Pitfalls recorded: MIs created through MCP rendered no clouds; deleting a referenced MI corrupts its package [src build/logs/A.md:27]. Ocean extents `4000 m` at this card [src build/logs/A.md:8]; later raised (I1 8.4 km, P5c 300 km).

## 14.2 Card B: playable ship pawn

| Field | Value |
|---|---|
| Goal | C++ `AIronwakeShipPawn` with heavy telegraph/rudder controls, HUD, wake [src build/logs/B.md:1-3] |
| Outcome | Works in PIE: `13.0 kn` at 34 s, `15.9 kn` at 90 s on FLANK [src build/logs/B.md:35]; hard starboard turns about 0.6-0.7 deg/s [src build/logs/B.md:36]. `12/12` pontoons wet [src build/logs/B.md:34]. Ironclad mesh `SM_Ironclad` from Hunyuan GLB, 140 m [src build/logs/B.md:29-31] |
| Scores | B_1 `~6/10` [src build/logs/B.md:41], B_2 `~4-5/10` [src build/logs/B.md:43], B_3 `~4/10` [src build/logs/B.md:45], overall visuals ~5/10 [src build/logs/B.md:41-47] |
| Commit | `da3a74c` [src docs/spec/_gitlog.txt:132] (done); module start `0030f0a` [src docs/spec/_gitlog.txt:140] |
| Packaged app | none |

## 14.3 Card C: gunnery and one NPC enemy

| Field | Value |
|---|---|
| Goal | Ballistic shells, turret traverse, damage/fire/sinking, NPC utility-AI ironclad, combat audio [src build/logs/C.md:1-5] |
| Outcome | Verified in PIE: enemy spawned 3956 m away, fled below 35% HP and sank at t about 60 s; `36` shells, `14` hits, `6` fires [src build/logs/C.md:54]. Muzzle velocity `600 m/s`, max range `10 km` [src build/logs/C.md:8]; player `160 HP`, NPC `120 HP`, `6` per shell [src build/logs/C.md:16] |
| Scores | C_1 ~7, C_2 8, C_3 6, C_4 8, C_5 8 [src build/logs/C.md:63-67]; overall ~6-7/10 [src build/logs/C.md:70] |
| Commit | `77db4b0` [src docs/spec/_gitlog.txt:130] (code), FX `3781a8b` [src docs/spec/_gitlog.txt:131], log `59ba5d8` [src docs/spec/_gitlog.txt:129] |
| Packaged app | none |

## 14.4 Card D: living sea, airships, menu, package, clip

| Field | Value |
|---|---|
| Goal | Living sea (3 merchants, airship, island + lighthouse), C++ UMG title/pause menu, package, 59 s clip [src build/logs/D.md:1-7] |
| Outcome | Packaged app verified end to end; `-iwtour` run: `IWCDEMO done t=70.6 shots=32 hits=21` [src build/logs/D.md:52]. Clip `slice_v1.mp4` 59 s 1280x720 [src build/logs/D.md:71]. Real package is `Saved/StagedBuilds/Mac/IronwakeTest.app` copied with ditto [src build/logs/D.md:88] |
| Scores | D_4 airship 10, D_3 merchant 8, D_1 title 5, D_2 island 4 [src build/logs/D.md:60-64]; clip honest score `6.5/10` [src build/logs/D.md:75] |
| Commit | `b0874af` [src docs/spec/_gitlog.txt:128] (living sea/menu), package prep `b18012f` [src docs/spec/_gitlog.txt:127], log `6514293` [src docs/spec/_gitlog.txt:125] |
| Packaged app | first package at `~/Games/Ironwake/Ironwake.app`, later kept as `Ironwake_v1.app` [src docs/spec/_apps.txt:23] |

Known issues carried: island invisible in fog (4/10), plain menu, merchants invulnerable, bundle id `com.YourCompany.IronwakeTest` [src build/logs/D.md:91].

## 14.5 Card E: independent review

| Field | Value |
|---|---|
| Goal | Independent play-through and morning report; changed nothing in the game [src build/logs/E.md:1-3] |
| Outcome | Report build/MORNING_REPORT.md. Top finding: an idle Captain-mode player sinks `121 s` after start [src build/MORNING_REPORT.md:32]. Top 5 next steps at [src build/MORNING_REPORT.md:58-63] became pass 2 |
| Scores | slice as a game `5/10` [src build/MORNING_REPORT.md:55], look `4.5/10` [src build/MORNING_REPORT.md:56] |
| Commit | `fc9c4a6` [src docs/spec/_gitlog.txt:124] |
| Packaged app | reviewed Ironwake.app (first build) |

## 14.6 Card P1: fair opening + game over/restart

| Field | Value |
|---|---|
| Goal | Enemy 7.5 km out in front, hold fire, SHIP LOST / VICTORY screens, camera over the bow, Peaceful default [src build/logs/P1.md:1-5] |
| Outcome | Spawn moved to (-3000, -2700) m heading `045` [src build/logs/P1.md:7]; enemy holds fire `60 s` or until under `5 km` [src build/logs/P1.md:12]; `MinAimRangeCm` `1.5 km` [src build/logs/P1.md:23]; PIE tests for idle, sink, restart, menu, victory all pass [src build/logs/P1.md:30-36] |
| Scores | SHIP LOST card `6/10` [src build/logs/P1.md:44], VICTORY `7.5/10` [src build/logs/P1.md:46] |
| Commit | `3590950` [src docs/spec/_gitlog.txt:118] |
| Packaged app | none (P5 re-packaged) [src build/logs/P1.md:56] |

## 14.7 Card S1: peaceful sailing feel (Voyager)

| Field | Value |
|---|---|
| Goal | Peaceful default, POIs, compass/minimap HUD, helmsman, anchor, fast sail, photo mode, weather cycle, gulls [src build/logs/S1.md:1-8] |
| Outcome | Standalone -game tour: arrived at Wreck t=`114.9` [src build/logs/S1.md:46], Needles `287.6`, lighthouse `463.3` [src build/logs/S1.md:47-48]; anchor 12.1 kn to 0.2 kn in 16 s [src build/logs/S1.md:28]; `22` gulls [src build/logs/S1.md:39] |
| Scores | sailing to places `7/10` [src build/logs/S1.md:69], look of new places `4-5/10`, weather/time `3-4/10` [src build/logs/S1.md:69-71]; HUD 8, wreck arrival 8, dusk 3 [src build/logs/S1.md:59-65] |
| Commit | `38c89f2` [src docs/spec/_gitlog.txt:115] |
| Packaged app | none (not packaged) [src build/logs/S1.md:3] |

## 14.8 Card H1: 60 FPS, tight handling, WoWS camera, music off

| Field | Value |
|---|---|
| Goal | Fix 6 FPS and drifty handling; World-of-Warships camera; music off [src build/logs/H1.md:1-3] |
| Outcome | Median FPS v1 `16.1` to H1 MEDIUM `60.0` [src build/logs/H1.md:11-13]; cloud deck 41 ms to 2-3 ms [src build/logs/H1.md:21]; STOP to HALF 90% `10.0 s` [src build/logs/H1.md:41-42]; bow starts swinging `0.84 s` [src build/logs/H1.md:43]; presets LOW/MEDIUM/HIGH [src build/logs/H1.md:29-32] |
| Scores | zoomed-out camera 8-10/10; default view `3.5-4/10` vs WoWS [src build/logs/H1.md:68-70] |
| Commit | `ee306ce` [src docs/spec/_gitlog.txt:111] |
| Packaged app | `Ironwake.app` [src docs/spec/_apps.txt:1] at that time; previous kept as `Ironwake_v1.app` [src docs/spec/_apps.txt:23]; later file Ironwake_H1.app `Ironwake_H1.app` [src docs/spec/_apps.txt:4] |

## 14.9 Card R1: hi-res library ships

| Field | Value |
|---|---|
| Goal | Swap Hunyuan hulls for library CC-BY hulls (Devastation, Kracken, Goliath, Aglae, brass airship) [src build/logs/R1.md:1-12] |
| Outcome | Waterline found `4.7 m` apparent offset (`RenderedSeaOffsetCm = -470`) [src build/logs/R1.md:21], later shown WRONG in W3; paired FPS H1 `24.2` vs R1 `32.0` under load [src build/logs/R1.md:27]; airship close-up speckles traced to decimation [src build/logs/R1.md:47]; in-game CREDITS panel added [src build/logs/R1.md:50] |
| Scores | player close 8.5/10 (blunt `3.5-4`) [src build/logs/R1.md:33]; airship wide 7.5, close-up 4 [src build/logs/R1.md:36-39] |
| Commit | `e132d9f` [src docs/spec/_gitlog.txt:105] (swap); assets `1141efd` [src docs/spec/_gitlog.txt:107]; log `01f670d` [src docs/spec/_gitlog.txt:103] |
| Packaged app | `Ironwake_R1.app` [src docs/spec/_apps.txt:18] |

## 14.10 Card A1: autopilot + repackage + "looks like it's sinking" fix

| Field | Value |
|---|---|
| Goal | P cycles OFF/HOLD/WAYPOINTS/TOUR, M chart, give way, under-fire disengage [src build/logs/A1.md:1-12] |
| Outcome | Hold course max error `0.20 deg`, rms `0.07` [src build/logs/A1.md:21]; 3-waypoint route done in `504 s` [src build/logs/A1.md:27]; TOUR done in `1647 s` [src build/logs/A1.md:28]; boot-top clearance `+0.66 m` calm [src build/logs/A1.md:40]; `DraftBelowBootTopCm` 50 to -60 [src build/logs/A1.md:34] |
| Scores | chart 7.5, autopilot HUD 10, waterline calm 10, storm 9.5 [src build/logs/A1.md:31-42] |
| Commit | `350d307` [src docs/spec/_gitlog.txt:93] (autopilot); log `6918aa0` [src docs/spec/_gitlog.txt:92] |
| Packaged app | `Ironwake_A1.app` [src docs/spec/_apps.txt:2]; W1 build kept as Ironwake_W1.app [src build/logs/A1.md:3] |

Known: orbit entry cuts inside hazard rings (~70 m inside the Needles ring) [src build/logs/A1.md:45].

## 14.11 Card W1: heavy ship physics, real bow wave, bigger sea

| Field | Value |
|---|---|
| Goal | Chuck 12:20: ship too buoyant, bow splash through the hull, sea too waveless [src build/logs/W1.md:3] |
| Outcome | Pontoons replaced by waterplane fit with heave 8 s, pitch 8.5 s, roll 13 s natural periods [src build/logs/W1.md:9]; hull/sea heave ratio `0.36` moderate stopped [src build/logs/W1.md:19]; settle `4.7 s` [src build/logs/W1.md:22]; new `UIronwakeBowWave`; sea presets overcast `1.0`, dusk `0.65`, foggy night `1.6` [src build/logs/W1.md:50-52] |
| Scores | bow wave 2-3 to `7/10` after foam fix; bow dig `0-1/10`; sea `3-4/10` [src build/logs/W1.md:58-60]; W1 `35.9` vs R1 `33.3` fps under load [src build/logs/W1.md:68] |
| Commit | `83a9d2d` [src docs/spec/_gitlog.txt:100] (physics + bow wave); log `a96130f` [src docs/spec/_gitlog.txt:94] |
| Packaged app | `Ironwake_W1.app` [src docs/spec/_apps.txt:20] |

## 14.12 Card E1: real world assets

| Field | Value |
|---|---|
| Goal | Replace greybox harbour, cliffs, wreck with scans and Hunyuan steampunk pieces [src build/logs/E1.md:1-5] |
| Outcome | `33` meshes imported with Nanite [src build/logs/E1.md:11]; Greyhaven, St Brannoc, Needles `95/74/55/36/22 m`, Wreck of the Valiant rebuilt [src build/logs/E1.md:28]; day fog density `0.020` to `0.014` [src build/logs/E1.md:34]; FPS `60.0` median harbour/island/tour on a quiet machine [src build/logs/E1.md:58-60] |
| Scores | harbour close 8.5, island 7.5, wreck close 9, needles 1 km 7.5, airship close 9 [src build/logs/E1.md:41-49]; orchestrator fixes: no grey boxes `9.5/10` [src build/logs/E1.md:65] |
| Commit | `c84ea33` [src docs/spec/_gitlog.txt:91] (assets); log `54855f3` [src docs/spec/_gitlog.txt:87]; orchestrator fixes `9d17cb5` [src docs/spec/_gitlog.txt:88] |
| Packaged app | `Ironwake_E1.app` [src docs/spec/_apps.txt:3] |

## 14.13 Card S2: roster, shipyard, sailing frigate

| Field | Value |
|---|---|
| Goal | Sailing frigate default, Kittiwake cutter, Devastation unlock, sail rig, points of sail [src build/logs/S2.md:1-13] |
| Outcome | Roster in `IronwakeFleet.{h,cpp}`: Albatross `88 m`, `2,200 t`, turn `R 210 m` (coded values change later: ch. 2) [src build/logs/S2.md:9]; points of sail e.g. beam reach `9.3-10.6 kn`, close-hauled `5.8-6.1 kn` [src build/logs/S2.md:26-28]; yard braces 38/27/18/1.5 deg [src build/logs/S2.md:17]; shipyard LAUNCH crash fixed (dangling static material ptr) [src build/logs/S2.md:46] |
| Scores | frigate waterline 9.5, cutter 10, ironclad 10, close zoom 9.5, shipyard 8.5 [src build/logs/S2.md:40-52] |
| Commit | `3d72afb` [src docs/spec/_gitlog.txt:84] (sail rig); `fe319ee` [src docs/spec/_gitlog.txt:85] (roster wip); `37f7069` [src docs/spec/_gitlog.txt:81] (waterline); log `2f1161f` [src docs/spec/_gitlog.txt:78] |
| Packaged app | `Ironwake_S2.app` [src docs/spec/_apps.txt:19] |

## 14.14 Card W2: big seas, weather, open-ocean storm

| Field | Value |
|---|---|
| Goal | Five sea states, storm director, deep-sea fade, STORM AT SEA start, evil-storm look [src build/logs/W2.md:1-3] |
| Outcome | Hs measured CALM `0.9 m`, MODERATE `2.1 m`, ROUGH `4.0 m`, STORM `10.5 m` [src build/logs/W2.md:12]; wind 8/16/24/36/48 kn [src build/logs/W2.md:13]; round 2 proved rendered waves match query (rendered minus query mean `+0.24 m`) [src build/logs/W2.md:85]; own ocean material chain `M_IW_WaterRoot` [src build/logs/W2.md:96]; storm bench `25.5` fps with editor open [src build/logs/W2.md:53] |
| Scores | storm look 0-3.5/10: calm 2, rough 0, storm chase `3.5`, round 2 best `3.5/10` [src build/logs/W2.md:36-41], [src build/logs/W2.md:69] |
| Commit | `b7032b6` [src docs/spec/_gitlog.txt:77] (systems); round 2 `8a86297` [src docs/spec/_gitlog.txt:74]; log `d344fc0` [src docs/spec/_gitlog.txt:75] |
| Packaged app | `Ironwake_W2.app` [src docs/spec/_apps.txt:21]; Ironwake.app stays S2 [src build/logs/W2.md:3] |

## 14.15 Card I1: archipelago

| Field | Value |
|---|---|
| Goal | Many islands to explore with surf and shelf integration [src build/logs/I1.md:1-9] |
| Outcome | `14` new seeded islands (seed `1871`), total `18` places [src build/logs/I1.md:6]; C++ scatter, `23` HISM components, ~`6,000` instances [src build/logs/I1.md:25]; ocean extents 8 to `8.4 km` [src build/logs/I1.md:32]; FPS NOT verified at 60: I1 `32.7` tour, `22.7-24.0` five-island bench under load [src build/logs/I1.md:64-66] |
| Scores | chart 8.5, far silhouettes 7.5-8, stacks/rocky 6.5-7.5, pines 6-7, fort `3-5` at range [src build/logs/I1.md:44-54] |
| Commit | `aea70ca` [src docs/spec/_gitlog.txt:67] (final); wip `686e233` [src docs/spec/_gitlog.txt:72]; surf/shelf `470ac87` [src docs/spec/_gitlog.txt:70] |
| Packaged app | `Ironwake_I1.app` [src docs/spec/_apps.txt:5]; Ironwake.app not promoted [src build/logs/I1.md:3] |

## 14.16 Card W3: two-layer water bug + readable storm + ship FX

| Field | Value |
|---|---|
| Goal | Chuck 09:20 two-layer water bug first; readable evil storm second [src build/logs/W3.md:1-3] |
| Outcome | ROOT CAUSE: R1 gauge reader mistook post bands seen through translucent water for the surface; fix `RenderedSeaOffsetCm` to 0 and drafts frigate `-40`, cutter `-25`, ironclad `-50` cm; rendered minus query `-0.08 m` sd `0.09` [src build/logs/W3.md:9-17]; audio: harbour_ambience removed [src build/logs/W3.md:19]; packaged FPS `33.8` median, GPU `29.4 ms` [src build/logs/W3.md:55] |
| Scores | storm `4-5.5/10`: chase 4, whitecaps 2.5, trough 5.5, bow rough 5.5, wake_high 0 [src build/logs/W3.md:39-45] |
| Commit | `2d64892` [src docs/spec/_gitlog.txt:64] (water fix); `28de2c7` [src docs/spec/_gitlog.txt:63] (audio); storm `c859910` [src docs/spec/_gitlog.txt:62] |
| Packaged app | water-fix build became Ironwake.app (`Ironwake.app` [src docs/spec/_apps.txt:1]); storm work not packaged [src build/logs/W3.md:4] |

## 14.17 Card W4: finish the readable storm

| Field | Value |
|---|---|
| Goal | Fix combed stripes, coverage runaway, invisible wake, spume, bow wave; promotion gate storm >= 6 [src build/logs/W4.md:1-4] |
| Outcome | Mip-filtered foam fixes stripes; coverage ROUGH `3.6-10 %` after calibration [src build/logs/W4.md:11]; wake from 200 m `9.5/10` [src build/logs/W4.md:12]; spume still 1-2/10; bow wave `4/10` [src build/logs/W4.md:14]; calm `8.5/10` [src build/logs/W4.md:21]. Structural verdict: shader foam on 16-wave Gerstner reads as flat decals [src build/logs/W4.md:25] |
| Scores | storm 4-6/10, gate not met, NOT promoted [src build/logs/W4.md:3] |
| Commit | `e8aea61` [src docs/spec/_gitlog.txt:55] (r2); `11cb827` [src docs/spec/_gitlog.txt:58] (r1); `add81de` [src docs/spec/_gitlog.txt:54] (r3); log `618557c` [src docs/spec/_gitlog.txt:53] |
| Packaged app | `Ironwake_W4.app` [src docs/spec/_apps.txt:22] (side-by-side only) |

## 14.18 Card P3: handling + sea look + voyage

| Field | Value |
|---|---|
| Goal | Tighter turning, 18-place voyage, ship FX fixes [src build/logs/P3.md:1-12] |
| Outcome | Circle times ironclad `240.8 s` to `201.4 s`, frigate `252.3 s` to `222.2 s` [src build/logs/P3.md:4-5]; voyage 18/18 waypoints, `30.4 km` in `1221 s` [src build/logs/P3.md:12]; BowSheet sprites cut from 170 to `60` [src build/logs/P3.md:16]; FPS `13.3` P3 vs `12.8` under load (no claim) [src build/logs/P3.md:25] |
| Scores | hull-side frames still 0-2/10 vision (flat foam band) [src build/logs/P3.md:18]; Ironwake.app NOT promoted [src build/logs/P3.md:33] |
| Commit | `f5bb53c` [src docs/spec/_gitlog.txt:44] (log); `691e385` [src docs/spec/_gitlog.txt:49] (tighter turning) |
| Packaged app | `Ironwake_P3.app` [src docs/spec/_apps.txt:11] |

## 14.19 Card P5: package + gates + MORNING_REPORT_2

| Field | Value |
|---|---|
| Goal | Package Ironwake_P5.app, run gates [src build/MORNING_REPORT_2.md:1-5] |
| Outcome | Bundle id `com.omstead.ironwake` [src build/MORNING_REPORT_2.md:3]; P5 `15.9` fps vs `14.6` current with editor open [src build/MORNING_REPORT_2.md:15]; waterline frigate `+0.39 m`, cutter `+0.23 m`, ironclad `+0.59 m` [src build/MORNING_REPORT_2.md:20]; promotion gate NOT met: Ironwake.app not replaced [src build/MORNING_REPORT_2.md:27] |
| Scores | no vision scoring; pause menu not verified by P5 (later verified by P6) |
| Commit | `b9f0201` [src docs/spec/_gitlog.txt:41] |
| Packaged app | `Ironwake_P5.app` [src docs/spec/_apps.txt:12] |

## 14.20 Card P6: Opus phase review (read-only)

| Field | Value |
|---|---|
| Goal | Judge the phase as an outside reviewer on Ironwake_P5.app [src build/REVIEW_opus.md:3-12] |
| Outcome | Verdict `ship with caveats` [src build/REVIEW_opus.md:12]; measured FPS editor closed `43.6` spawn, `45.3` islands [src build/REVIEW_opus.md:23]; found turning frame-rate dependent: ironclad `221 s` at 30 fps vs `288 s` at 19 fps [src build/REVIEW_opus.md:21]; frigate has no boot-top band [src build/REVIEW_opus.md:22]; water loops carry 393/407 Hz tone [src build/REVIEW_opus.md:24] |
| Scores | game `4/10` [src build/REVIEW_opus.md:27], look `5/10` [src build/REVIEW_opus.md:28], feel `5/10` [src build/REVIEW_opus.md:29] |
| Commit | `52796b0` [src docs/spec/_gitlog.txt:37] |
| Packaged app | reviewed `Ironwake_P5.app` [src docs/spec/_apps.txt:12] |

## 14.21 Card P4: gameplay clip v2 (peaceful)

| Field | Value |
|---|---|
| Goal | 49.5 s peaceful sailing clip `slice_v2.mp4` [src build/logs/P4.md:2] |
| Outcome | Nine scripted shots of 5.5 s each, recorded with `-benchmark -fps=30 -dumpmovie` from the editor build (not packaged) [src build/logs/P4.md:3-8]; storm look not used [src build/logs/P4.md:17] |
| Scores | honest ~`7/10`; per shot 7.5-9; flat bright wake ribbon still the weak point [src build/logs/P4.md:18-19] |
| Commit | `2b51e78` [src docs/spec/_gitlog.txt:33] |
| Packaged app | none (editor build) [src build/logs/P4.md:8] |

## 14.22 Card P5b: speed-scaled ship FX + grey funnel smoke

| Field | Value |
|---|---|
| Goal | Verify f8b5bdc speed scaling with a ladder: 3 hulls x {4, 8, 13 kn} x {calm, moderate} [src build/logs/P5b.md:4] |
| Outcome | New `NS_IW_FunnelSmoke` grey 0.35 alpha 0.3 [src build/logs/P5b.md:3]; strict slam gate never fired (`BowSubmergeM` max `-0.57 m`) [src build/logs/P5b.md:8] |
| Scores | cutter side 13 kn `3/10`, cutter 4 kn `7/10`, ironclad 8.5/8/5.5, funnel chase `8.5/10` [src build/logs/P5b.md:6-7]; pass rule NOT fully met [src build/logs/P5b.md:10] |
| Commit | `a903a96` [src docs/spec/_gitlog.txt:35]; Chuck rule `f8b5bdc` [src docs/spec/_gitlog.txt:40]; grey smoke `0eee597` [src docs/spec/_gitlog.txt:38] |
| Packaged app | `Ironwake_P5b.app` [src docs/spec/_apps.txt:13] |

## 14.23 Card P5c: ship water look pass

| Field | Value |
|---|---|
| Goal | Soft wake, patchy hull foam, bow cushion, slam gate, funnel smoke, seam [src build/logs/P5c.md:1-10] |
| Outcome | Wake `6` verts with feathered edges, centre opacity x`0.55` [src build/logs/P5c.md:5]; slam counts calm 0, moderate `8`, rough `35` per 60 s [src build/logs/P5c.md:8]; funnel smoke alpha 0.75, 14/s [src build/logs/P5c.md:9]; ocean/zone extents to `300 km`, seam gone [src build/logs/P5c.md:10] |
| Scores | cutter side 7, frigate chase wake 3-7 (unstable); pass rule NOT fully met [src build/logs/P5c.md:14-15]; SPEC.md cites ship water at 13 kn ~8 [src SPEC.md:83] |
| Commit | `6f75630` [src docs/spec/_gitlog.txt:30] (final); WIP `98974b0` [src docs/spec/_gitlog.txt:31] |
| Packaged app | `Ironwake_P5c.app` [src docs/spec/_apps.txt:14] |

## 14.24 Card P7: frame-rate-independent handling

| Field | Value |
|---|---|
| Goal | Fix turning that slowed at low FPS (P6); bench P5 vs P5c; frigate smoke [src build/logs/P7.md:1-14] |
| Outcome | Physics substepping `bSubstepping=True`, max `1/60`, `6` substeps [src Config/DefaultEngine.ini:37-40]; circle times after fix ironclad `178.3 s` at 30 and 18 fps, frigate `192.1`, cutter `32.9` [src build/logs/P7.md:19-21]; P7 `59.0` fps avg [src build/logs/P7.md:11]; aux stack moved to (-23 m, 23 m) scale `0.9` [src build/logs/P7.md:27] |
| Scores | pass within 5% for all three hulls [src build/logs/P7.md:22] |
| Commit | `c63b246` [src docs/spec/_gitlog.txt:28] (substepping); `7bed173` [src docs/spec/_gitlog.txt:27] (logs) |
| Packaged app | `Ironwake_P7.app` [src docs/spec/_apps.txt:15] |

## 14.25 Card P8: cluster models, NASA sky v1, ironclad turn

| Field | Value |
|---|---|
| Goal | Import 17 Ursa Hunyuan models, build town + NPC craft, NASA night sky [src build/logs/P8.md:1-8] |
| Outcome | `17` keep GLBs imported with Nanite + simple collision [src build/logs/P8.md:7]; ironclad `MinTurnRadiusM` 280 to `330` (Chuck OK 21:25), circle `208.8 s` [src build/logs/P8.md:22]; paired FPS 33.1/33.5 [src build/logs/P8.md:20] |
| Scores | BOTH TARGETS MISSED: town 1-2/10 [src build/logs/P8.md:11]; sky `1-2.5/10` over five iterations, best Cygnus `3.5/10` [src build/logs/P8.md:15]; targets missed [src build/logs/P8.md:4] |
| Commit | `71d6c10` [src docs/spec/_gitlog.txt:21] (final); import `039a79f` [src docs/spec/_gitlog.txt:25]; turn `b70985d` [src docs/spec/_gitlog.txt:24] |
| Packaged app | `Ironwake_P8.app` [src docs/spec/_apps.txt:16] |

## 14.26 Card P9: real collision + grounding

| Field | Value |
|---|---|
| Goal | Analytic collision for rocks, piers, buildings, hulls; depth grounding; AvoidLand [src build/logs/P9.md:1-8] |
| Outcome | `1108` solids [src build/logs/P9.md:5]; all three hulls PASS beach, stack, breakwater, pier, ship-vs-ship, astern-frees, autopilot around island (`137 s`, `117 s`, `198 s`) [src build/logs/P9.md:13-15]; open-water circle unchanged `208.8 s` [src build/logs/P9.md:17]; bench P9 33.6/33.4 [src build/logs/P9.md:19] |
| Scores | pass by overhead frames + numeric pen=0; side views unreliable [src build/logs/P9.md:24] |
| Commit | `3c392c5` [src docs/spec/_gitlog.txt:18] |
| Packaged app | `Ironwake_P9.app` [src docs/spec/_apps.txt:17] |

## 14.27 Card P10: HDR night sky + town v1

| Field | Value |
|---|---|
| Goal | Fix P8 sky and town misses [src build/logs/P10.md:1-3] |
| Outcome | Real HDR `starmap_2020_8k.exr` as RGBA16F [src build/logs/P10.md:5]; locked exposure at clear night; `54` houses, `27` gas lamps, `231` window quads [src build/logs/P10.md:13]; bench P9 33.4 / P10 33.7 [src build/logs/P10.md:19] |
| Scores | sky PASS: Orion `9.5/10`, Cygnus/Milky Way 9-10 [src build/logs/P10.md:4]; town PARTLY `3.5-5.5/10` target 7 NOT met [src build/logs/P10.md:12-14] |
| Commit | `7f8417b` [src docs/spec/_gitlog.txt:16] |
| Packaged app | `Ironwake_P10.app` [src docs/spec/_apps.txt:6] |

## 14.28 Card P11: living world, hum, help panel

| Field | Value |
|---|---|
| Goal | Clock + weather, sea by weather, regenerate hum-free loops, keyboard help [src build/logs/P11.md:1-3] |
| Outcome | Loops regenerated: bow `+3.6 dB`, wake `+5.3`, hiss `+1.1` over median [src build/logs/P11.md:6]; clock start `09:30`, `48` real min/day [src build/logs/P11.md:9]; sea per weather clear `0.55` to squall `2.7` [src build/logs/P11.md:11]; harbour Hs basin `1.0 m` vs offshore `2.1 m` [src build/logs/P11.md:17]; bench P11 33.7/33.3 [src build/logs/P11.md:27] |
| Scores | night 9.5, rain 10, dawn 4, dusk 3-5, night/rain/dawn/dusk [src build/logs/P11.md:24]; harbour moderate `4/10` [src build/logs/P11.md:17] |
| Commit | `c593db4` [src docs/spec/_gitlog.txt:13] (final); wip `0543c7a` [src docs/spec/_gitlog.txt:14] |
| Packaged app | `Ironwake_P11.app` [src docs/spec/_apps.txt:7] |

## 14.29 Card P12: terraced harbour town

| Field | Value |
|---|---|
| Goal | Make the town read from the sea (second pass) [src build/logs/P12.md:1-4] |
| Outcome | `5` terraces in `5.5 m` steps, ~`74` houses, `7` tints, ~`105` chimneys, ~`39 m` spire [src build/logs/P12.md:7-9]; fixed P11 lamp inversion (SmoothStep A < B) [src build/logs/P12.md:14]; bench P12 32.7/32.9 [src build/logs/P12.md:27] |
| Scores | PARTLY: day `3.5-4/10` at 400 m, ~1-3 at 900 m, night 6.5; target 7 NOT met [src build/logs/P12.md:4] |
| Commit | `b8a636a` [src docs/spec/_gitlog.txt:11] |
| Packaged app | `Ironwake_P12.app` [src docs/spec/_apps.txt:8] |

## 14.30 Card P13: light and sea polish

| Field | Value |
|---|---|
| Goal | Dawn/dusk ramps, moonlight, overcast, sea by weather, night exposure; paused by orchestrator 08:52 before all targets met [src build/logs/P13.md:1-3] |
| Outcome | Dawn ~`6.5`, dusk ~`7.5` [src build/logs/P13.md:7]; moon peak `2.5` [src build/logs/P13.md:9]; night exposure EV `1.0` to `0.4`, window emissive x0.25, lamps x0.3 [src build/logs/P13.md:27]; bench P13 31.3/32.1 [src build/logs/P13.md:21] |
| Scores | overcast 8-9; clear noon still `3.5-5`; night 3.5-5.5 then `7` after resume; squall 2-8 [src build/logs/P13.md:14-17] |
| Commit | `e3bfcd3` [src docs/spec/_gitlog.txt:9] (main); night pass `5e07bf8` [src docs/spec/_gitlog.txt:5] |
| Packaged app | `Ironwake_P13.app` [src docs/spec/_apps.txt:9] |

## 14.31 Card P14: clear noon, night sea, squall swell, harbour chop

| Field | Value |
|---|---|
| Goal | P13 leftovers [src build/logs/P14.md:1-4] |
| Outcome | Clear noon 400 m `7.5` (was 3.5-4.5), 900 m `8.5` [src build/logs/P14.md:7]; night chase 8.5, high 7.5 [src build/logs/P14.md:8]; squall chase `6.5` [src build/logs/P14.md:9]; harbour high `9` via flat water-material slab [src build/logs/P14.md:10]; bench P14 32.4/32.6 [src build/logs/P14.md:17] |
| Scores | NOT met: squall high `4/10`; plateau flat top [src build/logs/P14.md:13-14]. Orchestrator re-score of clear noon `5.5` vs worker 7.5-8.5 [src SPEC.md:84] |
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
| Open-sea storm (parked) | 5-6 (A) | 4-6 (W4) | A, W2 3.5, W3 4-5.5, W4 4-6; parked [src build/logs/W4.md:3] |
| Night sky | 1-2.5 (P8) | 9.5 (P10) | [src build/logs/P8.md:15], [src build/logs/P10.md:4] |
| Town from sea, day | 1-2 (P8) | 7.5 (P14, worker); 5.5 (orchestrator) | P10 3.5-5.5, P12 3.5-4, P14 7.5 [src SPEC.md:84] |
| Ship water at 13 kn | 0-2 (P3 hull-side) | ~8 (P5c, SPEC) | [src build/logs/P3.md:18], [src SPEC.md:83] |
| Waterline | wrong (R1 -470 cm) | one surface (W3) | [src build/logs/W3.md:9-17] |
| FPS | `16.1` v1 | `60.0` H1/E1 quiet machine; ~33 loaded | [src build/logs/H1.md:11], [src SPEC.md:89] |

## Gaps / unknown

- `docs/spec/_gitlog.txt` and `_apps.txt` are snapshots; commits made after this card (including the D1 commit that lands the chapters) are not listed. Re-run `git log` to refresh.
- Commits for P5b/P5c/P7/P12/P14 listed are the main code commits; several cards have multiple commits (see git log). A card-to-commit mapping is by subject prefix only.
- Vision scores are single-model and noisy; the only independent scores are Opus P6 and orchestrator re-scores quoted from SPEC.md. P14 orchestrator 5.5 for noon is the only recorded gap against a worker score.
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
PASS 08_time_weather_sky.md:222 `AutoExposureMin = Max = 1.0` build/logs/P10.md:7 
PASS 07_world.md:99 `FRandRange(2.2f, 4.5f)` Source/IronwakeTest/IronwakeArchipelago.cpp:352 
PASS 12_assets.md:173 `namaqualand_cliff_02` build/i1/prep_i1.py:21 
PASS 11_performance.md:25 `12` Source/IronwakeTest/IronwakeSettings.cpp:83 
PASS 02_ships.md:291 `Frigate 3.2 min` build/logs/P7.md:23 
PASS 04_ship_fx.md:107 `22` Source/IronwakeTest/IronwakeHydro.cpp:356 
PASS 14_history.md:117 `114.9` build/logs/S1.md:46 
PASS 02_ships.md:166 `1.4f` Source/IronwakeTest/IronwakeLivingSea.h:42 
PASS 02_ships.md:65 `IronwakeFleet.cpp:63 set too, no effect alone` build/logs/P8.md:22 
PASS 08_time_weather_sky.md:270 `MoonRadiusDeg = 1.0f` Source/IronwakeTest/IronwakeP8.h:39 
PASS 08_time_weather_sky.md:288 `-1.2f : 40.f` Source/IronwakeTest/IronwakeP8.cpp:323 
PASS 06_navigation.md:87 `d += 5` Source/IronwakeTest/IronwakeVoyage.cpp:1109 
PASS 03_water.md:105 `IW_WaterTime` Source/IronwakeTest/IronwakeHydro.cpp:290-291 
PASS 07_world.md:42 `3420` Source/IronwakeTest/IronwakeArchipelago.cpp:53 
PASS 07_world.md:219 `FLinearColor(1.f, 0.15f, 0.1f), 60000.f, 60000.f` Source/IronwakeTest/IronwakeVoyage.cpp:375 
PASS 03_water.md:21 `11` Source/IronwakeTest/IronwakeHydro.cpp:62 
PASS 13_tooling.md:116 `list_properties` build/tools/props.py:6 
PASS 11_performance.md:199 `32.7` build/logs/I1.md:60-66 
PASS 06_navigation.md:202 `in 1647 s with fast sail` build/logs/A1.md:28 
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
| Outcome | Procedural heightfield hillside (`IronwakeP15.cpp`) replaces the P12 wedge; layered ground material, road ribbons, scatter, 60 land collision OBBs [src build/logs/P15.md] |
| Scores | NOT MET: vision 4-5/10 at noon 400 m, 4 at 900 m, night 5, dusk 2 (lighting); target 7 [src build/logs/P15.md] |
| Bench | P14 33.2/32.1, P15 31.3/30.8 fps [src build/logs/P15bench] |
| Collision | frigate runs stop 50-90 m short of land; P9 `ship all` partly run [src build/logs/P15.md] |
| Packaged app | `Ironwake_P15.app` |

## P16: real swell measured, livelier ship, CAMERA MOTION option (2026-11-11)

| Field | Value |
|---|---|
| Outcome | Measured sea: analytic = query = rendered (calm 0.97 / moderate 2.34 / rough 4.82 / gale 8.78 m p2p); hull made livelier (periods, follow gains, fit span, caps 8/12); CAMERA MOTION OFF/LOW/MEDIUM, default LOW [src build/logs/P16.md] |
| Scores | Vision on single frames 0-2/10 for visible swell; target 7 NOT demonstrated; pixel measurement shows hull 25-65 px motion and post waterline motion matching the log [src build/logs/P16.md] |
| Circle | ironclad 208.8 s at 30 fps, 208.7 s at 18 fps [src build/logs/P16.md] |
| Packaged app | `Ironwake_P16.app` |

## P17: town seaward look + batch-2 models (2026-11-11)

| Field | Value |
|---|---|
| Outcome | Quay split into wall + cove/beach/slipway + revetment flanks, 2 jetties, varied terrace walls, wider pale main road with kerbs and white steps, desaturated ground with patches, shrub belt, fringe houses, 8 batch-2 town models + 3 ships [src build/logs/P17.md] |
| Scores | NOT MET: vision 4-6.5/10 at noon 400 m (target 7), 8 at 900 m (one pass), night 6.5 [src build/logs/P17.md] |
| Bench | P16 33.5/33.0, P17 31.8/30.0 fps [src build/logs/P17bench] |
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
| Outcome | Analytic per-ship wave field (bow crest, midbody trough, quarter crest, Kelvin wedge) evaluated identically on CPU and in the water material (vertex displacement, normal tilt, foam); hull-class data in the ship defs; prop wash for steamers only (twin screws on the ironclad); NPC hulls are sources [src build/logs/P19.md] |
| Numbers | Ironclad 13 kn: bow crest 44 % freeboard, trough 8.6 % draft at 36 % LWL, quarter crest at 64 %, wedge 17.8 deg vs 18.3 formula; 16 kn trough 10 % [src build/logs/P19/p19all_ironclad.txt] |
| Scores | Side sheet (real cruiser vs ours, ironclad 16 kn): bow 8, trough 9, quarter 7, overall 8; sail vs steam sheet 10. NOT MET: Kelvin wedge 3 and prop wash 2-4 against real photos (single-frame top-down view alone scored wedge 8) [src build/logs/P19.md] |
| Bench | P17 33.2/33.5, P19 31.7/32.3 fps [src build/logs/P19bench] |
| Packaged app | `Ironwake_P19.app` |

## P20: living sea NPC traffic, first pass (2026-11-12)

| Field | Value |
|---|---|
| Outcome | AIronwakeNpcShip embodied NPC ship pawn; AIronwakeNpcDirector with ledger of 24 ships across 4 ports, utility AI (12 states), COLREGS-lite avoidance, land look-ahead, timetable. Smack, RevenueCutter, PatrolAirship split props imported. |
| NOT MET | Soak collisions=0 stuck=0 NOT met (collisions=41, stuck=75 at t=3014 s soak13); binocular tooltip, minimap contacts, paired bench not done. |
| Packaged app | none (editor build only) |

| P21 (2026-11-12) | Living sea, second pass: PARTIAL |
|---|---|
| Outcome | Traffic token plus offshore waiting points at the harbour mouth; anchorage slot grid; blocked ships back off; becalmed sailers use their auxiliary; legacy scripted merchants only with `-iwoldnpc`. New hulls (tug, coastal steamer, pilot cutter) and parts (cargo-airship props, gig oars, windmill sails) imported; oar stroke and windmill-by-wind animated; minimap contacts and binocular name tooltip. [src build/logs/P21.md] |
| NOT MET | Soak `collisions=0 stuck=0` (best 30 collisions / 22 stuck at x8 for 3000 s, seed 1); vision >= 7 (aerial 2, gig 2, cargo props 2, windmill 9); paired bench vs P19 not run [src build/logs/P21.md] |
| Packaged app | `Ironwake_P21.app` |

| P22 (2026-11-12) | Moving parts and traffic presence |
|---|---|
| Outcome | Gig oars fixed in engine (port/stbd pivot swap, dip axis, draft) with seated rowers; patrol props rebuilt procedurally with blade/disc swap and per-prop rate log; wake FX hysteresis (no eviction); aerial test camera follows traffic; roster 30 ships. [src build/logs/P22.md] |

| P23 (2026-11-12) | Town placement rewrite: planner in IronwakeP23.cpp (switchback road, contour lanes, clustered houses on benches, retaining walls). Vision 400 m noon 4.5 vs 3 (P17); target 7 NOT met. Bench -2.7..-2.9 fps vs P22. Soak seed 1 not worse. |

| P24 (2026-11-13) | Stepped-bench town, SW headland lighthouse, kerbs & line-traced windows (`Ironwake_P24.app`, commit `e63831a`): `67 houses placed` [src build/logs/P24.md:6], `190 wind-bent broadleaf trees` [src build/logs/P24.md:6], line-traced windows (`111..146`), street `7.5`, night `7.5` (orchestrator), noon 400 m `4` (PARTIAL). |

| P25 (2026-11-13) | NPC quay-lane reservations, one-way harbour channel & 3-seed soak (`Ironwake_P25.app`, commits `fc7b018`, `651fc75`): `MouthOwner` throat lock + split inbound/outbound lanes (`Y = -34` / `+44..46` m), `9` outer waiting slots (`R = 385..505` m), exclusive berth `(SpanIdx, YInterval)` & corridor locks (`180` s timeout), brig `1.55`x wide turn + `450` m mouth auxiliary rule, `30` ledger ships (`14-16` moving). 3-seed 3000 s x8 soak: `IWNPC collisions=0 stuck=0 groundings=0` [src build/logs/P25.md:22] across seeds 1-3; paired bench `33.0 / 33.5` vs P24 `33.3 / 33.0` (`+0.10 FPS` [src build/logs/P25.md:35]); aerial harbour vision `5.5 - 6.5 / 10` [src build/logs/P25.md:26] (PARTIAL on aerial wake look). |
| P26 (2026-11-13) | Rigged 42 m Tall Brig + 17 moving parts + gaff spanker + WPO wind sails + 4-ship Shipyard (`Ironwake_P26.app`, commits `8b3e3af`, `2de6537`): imported `SM_TallBrig` + 17 `SM_TallBrig_*` spar/sail parts, matte PBR hull/wood (`M_IW_BrigTex`), zero-Clamp WPO `M_IW_SailCanvas`, playable `Curlew` brig (`42` m, `480` t) in 4-ship Shipyard, NPC brigs upgraded with harbour reduced-canvas rule (`moving_parts=119` [src build/logs/P26.md:36]). Vision `8.0–10/10` [src build/logs/P26.md:26]; 1-seed soak `SEED 1: IWNPC collisions=0 stuck=0 groundings=0` [src build/logs/P26.md:36]; paired bench `35.8 / 34.1` vs P25 `32.8 / 33.0` (`+2.05 FPS` [src build/logs/P26.md:42]). PASS. |
| P27 (2026-11-13) | Hydrodynamic Kelvin wake & turbulent prop-wash LUT + hull-foam fix (`Ironwake_P27.app`): imported Ursa `1024x1024` Havelock `T_IW_KelvinWaveLut`, `T_IW_PropWashFoamLut`, seamless 3-octave Voronoi `T_IW_CellularFoamTile`, and staged CPU `kelvin_lut_256.bin`; upgraded `M_IW_WaterRoot` + `FIronwakeShipWave` to 7 simultaneous ships (camera-centred NPC selection), twin-screw vs single-screw vs pure-sail Voronoi boil + turquoise SSS, lit `M_IW_WakeLut` (`+6` cm lift), and zero hull climb (`0` Metal SM6 material compile errors [src build/logs/P27.md:4]). Vision: prop wash `8.5 / 10` [src build/logs/P27.md:6], Kelvin wake `8.5 / 10` [src build/logs/P27.md:7], sail vs steam `9.5 / 10` [src build/logs/P27.md:8], hull waterline `9.5 / 10` [src build/logs/P27.md:9], speed/NPC6 `9.5 / 10` [src build/logs/P27.md:10]; paired bench `36.5`, `33.1` vs P26 `32.7`, `33.4` (`+1.75 FPS` [src build/logs/P27.md:13]). PASS. |
| P28 (2026-11-13) | Moving parts, rigged crew/rowers/gulls, batch-4 buildings+signs & triplanar 4K ground (`Ironwake_P28.app`): imported 20 p28/sign FBX bundles (3-LOD crew/gulls) + 4K Megascans `ForestFloor`/`MossyFloor` scans; wired `PaddleSteamerV3` spinning wheels + churn (`8.5 / 10` [src build/logs/P28.md:13]), `SalvageTenderV3` slewing crane + pendulum diving bell (`9.5 / 10` [src build/logs/P28.md:7]), `PatrolAirshipV4` 3 props (`7.5 / 10` [src build/logs/P28.md:15]), 3D `SailorCrewRower` 4-phase gig stroke (`9.5 / 10` [src build/logs/P28.md:6]), `SeaGullV4` wings, `SailorCrewStand` helmsmen & HISM dockhands (`moving_parts=122` [src build/logs/P28.md:4]), `TownTavernV4` + `TownChandleryV4` + `TownFoundryV4` + painted signs (`9.5 / 10` [src build/logs/P28.md:8]), 400 m town (`8.5 / 10` [src build/logs/P28.md:14]), and 3-axis triplanar 4K ground (`0` Metal SM6 errors); paired bench `35.9`, `34.5` vs P27 `33.0`, `33.7` (`+1.85 FPS` [src build/logs/P28.md:18]). PASS. |
| P29 (2026-11-13) | Data-driven multi-port world (`world_ports.json`), arrival Harbourmaster diorama & 8-good trade logbook (`Ironwake_P29.app`): extracted seeded port-kit generator (`IwPorts::BuildSecondaryPort`) spawning **Ironcrag** (`HostIsleIdx = 8`, basalt coaling/foundry haven) and **Stigsby** (`HostIsleIdx = 9`, wooded fishing/timber outpost) alongside **Greyhaven**; wired full quay/breakwater collision, 5-circle breakwater throat gates, 3-port M-chart (`9.5 / 10` [src build/logs/P29.md:8]), 6-port NPC trade routes (`SEED 1: IWNPC collisions=0 stuck=0 groundings=0` [src build/logs/P29.md:25], `moving_parts=120` [src build/logs/P29.md:4]), `< 2.5` kn `Enter` mooring & salvage verb, live Harbourmaster diorama + `UIronwakeDockWidget` (`10 / 10` [src build/logs/P29.md:6], `outside=0 overlaps=0 clipped=0 PASS` [src build/logs/P29.md:15-19]), 8-good cargo market (`final_crowns=1278 net_profit=963` [src build/logs/P29.md:21]), and Captain's Logbook (`9.8 / 10` [src build/logs/P29.md:7]); port look `7.8 / 10` [src build/logs/P29.md:12-13] noon 400 m, `8.5 / 10` [src build/logs/P29.md:10] night 400 m, `10 / 10` [src build/logs/P29.md:9] Stigsby 140 m cove; paired bench `33.2`, `33.3` vs P28 `34.0`, `32.9` (`-0.20 FPS` [src build/logs/P29.md:29]). PASS. |
| P30 (2026-11-13) | Independent verification, promotion gate (`~/Games/Ironwake/Ironwake.app`), spec check & `build/MORNING_REPORT_3.md`: `20 / 20` [src build/logs/P30.md:5] clean offscreen packaged launches (`0` crashes, `0` Metal SM6 errors); Greyhaven 400 m noon `7.5 / 10` [src build/logs/P30.md:13] & night `8.5 / 10` [src build/logs/P30.md:14] (`7.8 / 10` [src build/logs/P30.md:14] isolated); wake speed-scaling sheet `9.8 / 10` [src build/logs/P30.md:44] (`0.0000` [src build/logs/P30.md:44] hull climb); UI & cold-start defaults (`MUSIC = 0% OFF` [src build/logs/P30.md:59], `CAMERA MOTION = LOW` [src build/logs/P30.md:59], `IWAUDIO routetest bad=0` [src build/logs/P30.md:60]) `10 / 10` [src build/logs/P30.md:61]; 3-seed 6-port soak `IWNPC collisions=0 stuck=1 groundings=0` [src build/logs/P30.md:27], `IWNPC collisions=1 stuck=0 groundings=0` [src build/logs/P30.md:28], `IWNPC collisions=2 stuck=3 groundings=0` [src build/logs/P30.md:29]; paired bench editor-closed `53.10` [src build/logs/P30.md:54] MEDIUM (`+1.15 FPS` [src build/logs/P30.md:54] vs P19, `+18.15 FPS` [src build/logs/P30.md:56] vs W3) and `33.25` [src build/logs/P30.md:55] HIGH (`+1.40 FPS` [src build/logs/P30.md:55] vs P19); backed up `Ironwake_W3_prev.app` and promoted `Ironwake_P29.app` -> `~/Games/Ironwake/Ironwake.app` (`Promoted Ironwake.app smoke rc=0` [src build/logs/P30.md:4]). PASS. |
| SC1 (2026-11-13) | Public showcase: repo `charles-forsyth/ironwake` + GitHub Pages site (3D hangar of 18 web GLBs, 25-sound mixer, wake lab, 4-cut reel); web models re-cut on Ursa with seam welding + two-sided materials (fixes torn surfaces); "Moving parts" studio reel rendered as an 8-task Ursa array; `verify_repo.py` + CI gate. No game code changed; promoted app unchanged. See 13.12. |
| P31–P33 (2026-11-15) | Parallel Ursa TPU/GPU/CPU visual polish, 34 km 6-port archipelago & 6-ship playable fleet (`Ironwake_P32.app` -> promoted to `~/Games/Ironwake/Ironwake.app`): Ursa `tpu6spot` solved continuous Kelvin/prop-wash LUTs & 8-good 6-port Pareto trade markets; Ursa `gpu1v1` + `computehigh` built `CustomsHouseV2` (`8.5/10`), `CoastalLightship`, basalt crags (`BasaltCrag_1..4`), and prepped `ChapmanSloop` (`26 m`, `10 / 10` [src build/logs/P33.md:30]) & `AmphionSchooner` (`38 m`, `9.5 / 10` [src build/logs/P33.md:31]); upgraded `SteamCutter` *Kittiwake* V2 (`9.5 / 10` [src build/logs/P33.md:27]), `Frigate` copper boot-top (`9.5 / 10` [src build/logs/P33.md:28]), woven flax two-sided SSS `M_IW_SailCanvas`/`M_IW_Cloth2`, 4K POM street cobbles (`9.5 / 10` [src build/logs/P33.md:24]), world-Z tidal wetness, and UE 5.8 Nanite Quixel Megaplants (`UInstancedSkinnedMeshComponent` with static-mesh fallback in `IronwakeMegaplants.h`); expanded `world_ports.json` to 6 working ports (**Kestrel Cove**, **North Grimscar**, **Sable Haven** `9.5 / 10` [src build/logs/P33.md:16-18]) over a `48 km` WaterZone (`29x29 km` chart `9.0 / 10` [src build/logs/P33.md:20]); 3-seed 6-port soak mean `0.33` collisions / `1.00` stuck / `0.00` groundings [src build/logs/P33.md:40]; paired bench `49.80 FPS` MEDIUM (`-0.65 FPS` vs P29) and `33.70 FPS` HIGH (`+0.30 FPS` vs P29) [src build/logs/P33.md:43-44]; public showcase expanded to 20 web 3D models (`VERIFY OK`). PASS. |
| P34–P35 (2026-11-16) | Harbour flicker fix, clear coastal air (`V`), progressive dispersive Kelvin wake, hull waterline polish & zero-slide GPU LWC camera sync (`Ironwake_P34.app` -> promoted to `~/Games/Ironwake/Ironwake.app`, launcher `~/Games/Ironwake/play`): retired coplanar `GBasinSlab = nullptr` [src Source/IronwakeTest/IronwakeP12.cpp:247], deepened submerged cove terrain, and expanded `WaterZone` to `36 x 36 km` (`1600 cm` LOD0 tiles, `MaxDimensionInTiles=2048` [src Source/IronwakeTest/IronwakeSettings.cpp:90], sequential look-back delta `0.038%` [src build/logs/P34.md:12]); thinned `ExponentialHeightFog` ~6x (`FogDensity = 0.0028` [src build/logs/P34.md:16], `StartDistance = 180000.0` [src build/logs/P34.md:16], `FogMaxOpacity = 0.42` [src build/logs/P34.md:16], `V` -> `ToggleClearHaze()` [src build/logs/P34.md:16]); replaced instant `6 * L` snapshot triangle with distance-integrated `DevelopedDistM` (`14.5 * L` dispersive continuation + `1.15x`/`1.25x` ship-to-ship wake rocking [src build/logs/P34.md:19-21]); clamped physical slope `IW_ShipNormalK = 1.35` [src build/logs/P34.md:20] and integrated sea-level bow/waterline lace into `M_IW_WaterRoot`; switched water & ribbon shaders to GPU-native LWC `WSHackToFloat(GetWorldPosition(Parameters))` + `TG_PostUpdateWork` [src build/logs/P34.md:25] (`360 deg/sec` camera whip 100% hull-locked [src build/logs/P34.md:26]; `Promoted Ironwake.app smoke rc=0` [src build/logs/P34.md:4]). PASS. |
