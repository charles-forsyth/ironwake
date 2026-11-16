# Chapter 1: Design

Scope: what Ironwake is meant to be and what the shipped slice actually is. Covers the concept, pillars and setting from CONCEPT.md, look A (ART_BIBLE.md palette and scale rules), the two play modes, the long-term world plan against the current slice, items Chuck parked and why, and a dated decisions log built from BRIEF.md, card logs, MORNING_REPORT*.md and REVIEW_opus.md. Where a design doc and the code disagree, the code value is given as the one in effect and the doc value is marked CONFLICT. Sources are design docs in `~/Projects/game-concepts/ironwake/` (cited by absolute path) and project files (cited relative to the project root).

## 1.1 Concept

- One-line pitch: single-player open-sandbox steampunk naval game, "World of Warships, but mine": sail, trade, fight, upgrade, explore, in a living world [src [REDACTED]/Projects/game-concepts/ironwake/CONCEPT.md:3].
- Engine/platform: Unreal Engine 5.8 on the Mac [src [REDACTED]/Projects/game-concepts/ironwake/CONCEPT.md:4]. The built project reports UE `5.8.4` [src build/BRIEF.md:19].
- Working title: Ironwake is a placeholder per DESIGN.md open questions [src [REDACTED]/Projects/game-concepts/ironwake/CONCEPT.md:110].
- Setting: an invented archipelago age, roughly `1890s` [src [REDACTED]/Projects/game-concepts/ironwake/CONCEPT.md:19] tech bent sideways: coal and steam, riveted iron, brass, rare aether/tesla oddities [src [REDACTED]/Projects/game-concepts/ironwake/CONCEPT.md:19].
- Region in the concept: the `Shattered Reach` [src [REDACTED]/Projects/game-concepts/ironwake/CONCEPT.md:21], `30-40` [src [REDACTED]/Projects/game-concepts/ironwake/CONCEPT.md:21] islands across `3-4` [src [REDACTED]/Projects/game-concepts/ironwake/CONCEPT.md:21] sea zones [src [REDACTED]/Projects/game-concepts/ironwake/CONCEPT.md:21-22].

### Pillars (CONCEPT.md)

| # | Pillar | Source |
|---|---|---|
| 1 | The sea is the star: real waves with buoyancy; ships pitch, roll and take green water; storms, fog, night actions | [src [REDACTED]/Projects/game-concepts/ironwake/CONCEPT.md:9-10] |
| 2 | Weight and craft: big slow ships, shells with flight time, armor angles, visible damage | [src [REDACTED]/Projects/game-concepts/ironwake/CONCEPT.md:11-12] |
| 3 | A world that doesn't wait: NPC traders, patrols, pirates and fleets run their own lives | [src [REDACTED]/Projects/game-concepts/ironwake/CONCEPT.md:13-14] |
| 4 | Simple systems, rich screen: few goods, few hulls, few slots | [src [REDACTED]/Projects/game-concepts/ironwake/CONCEPT.md:15-16] |

### Systems the concept asks for versus what exists

| Concept item | Concept value | State in the slice |
|---|---|---|
| Goods | `Goods: 8` [src [REDACTED]/Projects/game-concepts/ironwake/CONCEPT.md:57] | economy is a stub (SPEC.md known gap 6: "trade/cargo/economy are stubs") [src SPEC.md:98] |
| Hulls at launch | `6 hulls` [src [REDACTED]/Projects/game-concepts/ironwake/CONCEPT.md:59] | roster is 3 hulls: Albatross, Kittiwake, Devastation (ch. 14, S2) |
| Factions | `Factions (4)` [src [REDACTED]/Projects/game-concepts/ironwake/CONCEPT.md:23] | none implemented (no faction code; trim colours only appear in ART_BIBLE) |
| Combat | WoWS-style aim, shell travel time [src [REDACTED]/Projects/game-concepts/ironwake/CONCEPT.md:62] | exists, PARKED (SPEC.md:40) [src SPEC.md:40] |
| Airships | scouts/bombers/spotters [src [REDACTED]/Projects/game-concepts/ironwake/CONCEPT.md:72] | 2 ambient NPC airships on loops, no gameplay role [src SPEC.md:37] |

## 1.2 Look A: palette and scale

Look A = stormy grey ironclads plus airships; reference `looks/look_a.png` [src [REDACTED]/Projects/game-concepts/ironwake/ART_BIBLE.md:3]. Mood: cold, heavy, maritime, North-Atlantic weather on an invented archipelago [src [REDACTED]/Projects/game-concepts/ironwake/ART_BIBLE.md:6].

### Palette (ART_BIBLE.md)

| Role | Colour | Hex | Source |
|---|---|---|---|
| World slate grey | | `#3E464D` [src [REDACTED]/Projects/game-concepts/ironwake/ART_BIBLE.md:11] | |
| Sea green-grey | | `#2F3D3A` [src [REDACTED]/Projects/game-concepts/ironwake/ART_BIBLE.md:11] | |
| Foam | | `#D8DEDF` [src [REDACTED]/Projects/game-concepts/ironwake/ART_BIBLE.md:11] | |
| Cliff basalt | | `#2A2A2C` [src [REDACTED]/Projects/game-concepts/ironwake/ART_BIBLE.md:11] | |
| Wet moss | | `#4B5A3A` [src [REDACTED]/Projects/game-concepts/ironwake/ART_BIBLE.md:11] | |
| Ship riveted iron | | `#4A4E52` [src [REDACTED]/Projects/game-concepts/ironwake/ART_BIBLE.md:12] | |
| Ship gunmetal | | `#2E3236` [src [REDACTED]/Projects/game-concepts/ironwake/ART_BIBLE.md:12] | |
| Ship soot black | | `#1A1A1B` [src [REDACTED]/Projects/game-concepts/ironwake/ART_BIBLE.md:12] | |
| Ship brass (sparingly) | | `#B08A3E` [src [REDACTED]/Projects/game-concepts/ironwake/ART_BIBLE.md:12] | |
| Red-oxide boot topping | | `#6B2A22` [src [REDACTED]/Projects/game-concepts/ironwake/ART_BIBLE.md:13] | |
| Weathered white superstructure | | `#C9C6BC` [src [REDACTED]/Projects/game-concepts/ironwake/ART_BIBLE.md:13] | |
| Accent furnace/aether orange (<3% of frame) | | `#FF7A1A` [src [REDACTED]/Projects/game-concepts/ironwake/ART_BIBLE.md:14] | |
| Faction trim: League gold | | `#C9A24A` [src [REDACTED]/Projects/game-concepts/ironwake/ART_BIBLE.md:16] | |
| Faction trim: Iron Navy blue-grey | | `#5B7088` [src [REDACTED]/Projects/game-concepts/ironwake/ART_BIBLE.md:16] | |
| Faction trim: Free Captains oxblood | | `#7A2430` [src [REDACTED]/Projects/game-concepts/ironwake/ART_BIBLE.md:16] | |
| Faction trim: Tech Order verdigris | | `#4F8C7E` [src [REDACTED]/Projects/game-concepts/ironwake/ART_BIBLE.md:16] | |

Factions are distinguished by trim colour only (stripe, flag, funnel band), never hull colour [src [REDACTED]/Projects/game-concepts/ironwake/ART_BIBLE.md:15].

### Scale rules versus the ships actually in the game

ART_BIBLE sizes are targets; the game uses the library hulls scaled per class. The values in the game column are what the fleet table sets, and they take effect over the ART_BIBLE value.

| Class | ART_BIBLE length | In game | Note |
|---|---|---|---|
| Cutter | `cutter 25 m` [src [REDACTED]/Projects/game-concepts/ironwake/ART_BIBLE.md:24] | `30.f` [src Source/IronwakeTest/IronwakeFleet.cpp:64] | Kittiwake |
| Steam frigate | `steam frigate 80 m` [src [REDACTED]/Projects/game-concepts/ironwake/ART_BIBLE.md:24] | `88.f` [src Source/IronwakeTest/IronwakeFleet.cpp:30] | Albatross; CONFLICT 80 vs 88: fleet value (88) takes effect |
| Ironclad | `ironclad 140 m` [src [REDACTED]/Projects/game-concepts/ironwake/ART_BIBLE.md:24] | `125.f` [src Source/IronwakeTest/IronwakeFleet.cpp:115] | Devastation; CONFLICT 140 vs 125: fleet value (125) takes effect (R1 log says real model is 87 m, scaled to class) |
| Gunboat 45 m / cruiser 120 m / dreadnought 180 m | `dreadnought 180 m` [src [REDACTED]/Projects/game-concepts/ironwake/ART_BIBLE.md:24] | not built | no roster entry |
| Airship scout/gunship/carrier | `gunship 110 m` [src [REDACTED]/Projects/game-concepts/ironwake/ART_BIBLE.md:27] | brass airship 110 m [src build/logs/R1.md:11] | brass airship = gunship-class size |

Other art-bible rules: PBR ranges roughness iron `0.5-0.7` [src [REDACTED]/Projects/game-concepts/ironwake/ART_BIBLE.md:21], brass `0.3-0.45` [src [REDACTED]/Projects/game-concepts/ironwake/ART_BIBLE.md:21]; hero ship budget `150-300k` [src [REDACTED]/Projects/game-concepts/ironwake/ART_BIBLE.md:37] tris with Nanite [src [REDACTED]/Projects/game-concepts/ironwake/ART_BIBLE.md:37]; no stylized props, world from scans only [src [REDACTED]/Projects/game-concepts/ironwake/ART_BIBLE.md:34]. CONFLICT: Hunyuan-generated town buildings and harbour pieces are used (ch. 12), which the bible's "scans only" rule does not cover; Chuck's decision log (1.8) has no explicit waiver, so treat as undocumented deviation.

## 1.3 Modes: Peaceful and Captain

| Aspect | Design (CONCEPT.md) | In effect (code/logs) |
|---|---|---|
| Default | Captain is the default [src [REDACTED]/Projects/game-concepts/ironwake/CONCEPT.md:28] | PEACEFUL: `bPeaceful = true` [src Source/IronwakeTest/IronwakePlayerController.h:164]. CONFLICT: code wins |
| Peaceful | no combat, nobody attacks, guns become cargo/survey gear [src [REDACTED]/Projects/game-concepts/ironwake/CONCEPT.md:29-30] | no enemy spawns; HUD drops HULL/GUNS lines and crosshair [src build/logs/S1.md:8] |
| Captain | trade, fight, pirates, faction wars [src [REDACTED]/Projects/game-concepts/ironwake/CONCEPT.md:28] | one enemy ironclad, spawns `7.5 km` [src build/logs/P1.md:8], holds fire `60 s` [src build/logs/P1.md:12] |
| Switching | per save at the start, not mid-save [src [REDACTED]/Projects/game-concepts/ironwake/CONCEPT.md:33] | title-menu MODE toggle; the pause menu does not switch it. Restart keeps the mode [src build/logs/P1.md:17] |
| Priority | n/a | Chuck 08:45 on 2026-11-09: peaceful sailing first, fighting later [src build/BRIEF.md:136] |

Peaceful goals in the concept (trade routes, charting, wreck salvage, photo mode) are only partly present: charting is the M chart and autopilot, photo mode is F, salvage/trade do not exist. REVIEW_opus scored the game `4/10` [src build/REVIEW_opus.md:27]: "arriving at a place gives you a banner and the next waypoint, and that is all" [src build/REVIEW_opus.md:27].

## 1.4 Controls: design versus built

| Control | DESIGN.md | Built (SPEC.md section 4 from IronwakeP11.cpp) | Status |
|---|---|---|---|
| W/S | engine telegraph steps, holds [src [REDACTED]/Projects/game-concepts/ironwake/DESIGN.md:38] | one step per press, sailing ship = sail plan [src SPEC.md:43] | matches |
| A/D | rudder steps [src [REDACTED]/Projects/game-concepts/ironwake/DESIGN.md:39] | hold to turn the wheel [src SPEC.md:45]; H1: held A/D turns wheel at 30 deg/s [src build/logs/H1.md:51] | CHANGED in H1 |
| RMB | zoom [src [REDACTED]/Projects/game-concepts/ironwake/DESIGN.md:41] | removed in H1: "Removed: right-click zoom" [src build/logs/H1.md:64] | CONFLICT: code wins |
| Shift | n/a | binoculars/scope [src SPEC.md:49] | added |
| M | map, click waypoint [src [REDACTED]/Projects/game-concepts/ironwake/DESIGN.md:43] | chart with waypoints [src SPEC.md:47] | matches |
| Tab | fleet orders [src [REDACTED]/Projects/game-concepts/ironwake/DESIGN.md:44] | pause menu [src SPEC.md:51] | CONFLICT: code wins |
| F | searchlight [src [REDACTED]/Projects/game-concepts/ironwake/DESIGN.md:45] | photo mode [src SPEC.md:49] | CONFLICT: code wins |
| R | damage control [src [REDACTED]/Projects/game-concepts/ironwake/DESIGN.md:45] | damage control, also in help panel [src SPEC.md:45] | matches |
| Camera | third-person orbit, WoWS style [src [REDACTED]/Projects/game-concepts/ironwake/DESIGN.md:26] | smoothed pivot orbit, 6 eased zoom steps [src build/logs/H1.md:60] | matches |

Full binding table lives in chapter 9. Chapter 1 only records the design deltas.

## 1.5 World plan: target versus current slice

| Aspect | Target (WORLD_PLAN.md / DESIGN.md) | Current slice | Source |
|---|---|---|---|
| Extent | `120 x 120 km` [src [REDACTED]/Projects/game-concepts/ironwake/WORLD_PLAN.md:3] | `34 km diagonal` [src SPEC.md:47] | slice spans 6 working ports across the archipelago [src [REDACTED]/Projects/game-concepts/ironwake/WORLD_PLAN.md:3] |
| Islands | `30-40` [src [REDACTED]/Projects/game-concepts/ironwake/WORLD_PLAN.md:21] | `18 places` [src build/logs/I1.md:6] places incl. 14 seeded islands [src build/logs/I1.md:6] | WORLD_PLAN counts 14 built [src [REDACTED]/Projects/game-concepts/ironwake/WORLD_PLAN.md:21] |
| Ports | `8-12` [src [REDACTED]/Projects/game-concepts/ironwake/WORLD_PLAN.md:21] | `6 full working ports` [src SPEC.md:48] | Greyhaven, Ironcrag, Stigsby, Kestrel Cove, North Grimscar, Sable Haven |
| Sea zones | `Sea zones 4` [src [REDACTED]/Projects/game-concepts/ironwake/WORLD_PLAN.md:20] | calm trade + outer straits | stormy straits, pirate reef, outer ocean expanding |
| Streaming | World Partition + Large World Coordinates [src [REDACTED]/Projects/game-concepts/ironwake/WORLD_PLAN.md:8] | single map /Game/Maps/Ironwake_Sea, ocean extents `300 km` [src build/logs/P5c.md:10] | 48 km WaterZone in P32 |
| Ship roster | `player roster 6 hulls` [src [REDACTED]/Projects/game-concepts/ironwake/WORLD_PLAN.md:23] | `6 playable hulls` [src SPEC.md:48] | matches WORLD_PLAN target |
| NPC ship types | `NPC types ~15` [src [REDACTED]/Projects/game-concepts/ironwake/WORLD_PLAN.md:23] | 30 NPC ships across 11 hull classes + 2 airships | see ch. 7 |
| Goods | `8 goods` [src [REDACTED]/Projects/game-concepts/ironwake/WORLD_PLAN.md:26] | `8-good` [src SPEC.md:48] | Pareto-solved markets in `world_ports.json` |
| Visibility | `15-20 km` [src [REDACTED]/Projects/game-concepts/ironwake/DESIGN.md:20] in clear weather [src [REDACTED]/Projects/game-concepts/ironwake/DESIGN.md:20] | fog-limited; see ch. 8 | |
| Speed compression | `~3x` [src [REDACTED]/Projects/game-concepts/ironwake/DESIGN.md:10] open water [src [REDACTED]/Projects/game-concepts/ironwake/DESIGN.md:11] | `1.5f` [src Source/IronwakeTest/IronwakeShipPawn.h:95] (SpeedCompression default) [src Source/IronwakeTest/IronwakeShipPawn.h:95] | CONFLICT: design says 3x, code is 1.5x. Code takes effect |

WORLD_PLAN order of work [src [REDACTED]/Projects/game-concepts/ironwake/WORLD_PLAN.md:36-44]: P6 review, P5b speed FX and grey smoke, P4 clip (done); cluster batches (NPC ships, town buildings, sky, economy sims); W-STRUCT; PORTS; TRADE; SKY (done as P8/P10/P11); grow outward. W-STRUCT, PORTS (as a kit), TRADE are not started.

## 1.6 Parked items and why

| Item | Parked by | Why / state | Source |
|---|---|---|---|
| Open-sea storm look (storm waves, whitecaps, open-sea foam, spume, breaking crests) | Chuck 2026-11-10 12:55 | W2-W4 reached `4-6/10` [src build/logs/W4.md:3] on camera; foam reads as flat decals, no breaking-crest geometry. Left at W4 state. Reviews must not treat it as a blocker | [src build/BRIEF.md:62], [src build/logs/W4.md:3] |
| Combat tuning (card P2: enemy readability, kill polish) | Chuck 08:45 on 2026-11-09 | peaceful sailing first | [src build/BRIEF.md:138] |
| Squall from high camera | consequence of storm-look park | P14 squall high `~4` [src build/logs/P14.md:9], needs crest foam | [src build/logs/P14.md:9] |
| Ship water FX | NOT parked | still wanted: bow wave, spray, churn, contact foam, wake | [src build/BRIEF.md:62] |

W4 promotion gate was storm >= 6 with no shot below it; it was not met, so Ironwake_W4.app is side-by-side only [src build/logs/W4.md:3-4].

## 1.7 Hard rules in force (BRIEF.md)

| Rule | Date/author | Source |
|---|---|---|
| Ship water FX scale with speed and wave; nothing at a crawl; never tune up to be visible at the cost of this | 2026-11-10 15:45 Chuck | [src build/BRIEF.md:61] |
| Clean up after every editor/-game run; kill CrashReportClient | 2026-11-10 Chuck | [src build/BRIEF.md:63] |
| Every launching script ends with `pkill -x CrashReportClient` | repeated misses 11-10 and 11-11 | [src build/BRIEF.md:4] |
| Use `-RenderOffScreen` for capture runs, one launch per ~20 min max | 2026-11-11 08:52 Chuck | [src build/BRIEF.md:3-9] |
| Always pass `-nosound` (plus `-unattended -nosplash`) | same | [src build/BRIEF.md:10] |
| Ships never pass through land, beaches, rocks, piers, buildings or other ships | 2026-11-10 22:40 Chuck | [src build/BRIEF.md:192-193] |
| Music OFF by default, menu toggle | 2026-11-09 10:20 Chuck | [src build/BRIEF.md:148] |
| Only water and environment sounds; remove sea_wind synth pad | 2026-11-09 23:50 Chuck | [src build/BRIEF.md:184] |
| Never let a hull sit below its paint line in calm water | 2026-11-09 23:50 Chuck | [src build/BRIEF.md:186] |
| No spending, no paid/Fab assets, no cluster jobs by workers | BRIEF hard rules 2 | [src build/BRIEF.md:66] |
| Never touch [REDACTED] or its editor on port 8000 | BRIEF | [src build/BRIEF.md:40] |
| Honest status: done means run and captured | BRIEF hard rules 3 | [src build/BRIEF.md:68] |

## 1.8 Decisions log (dated)

Times are EST as written in the sources. Ordered by time.

| Date/time | Decision | Source |
|---|---|---|
| 2026-11-08 | Look A (stormy grey ironclads + airships) chosen; concept status CONCEPT | [src [REDACTED]/Projects/game-concepts/ironwake/CONCEPT.md:6] |
| 2026-11-08 | DESIGN decisions: speed compression yes, lead indicator ON by default, ship loss chosen per save | [src [REDACTED]/Projects/game-concepts/ironwake/DESIGN.md:99] |
| 2026-11-09 00:50 | Chuck to bed: "continue all night long without approval from me", end state a game | [src build/BRIEF.md:14] |
| 2026-11-09 08:15 | Pass 2 started: MORNING_REPORT top-5 fixes approved | [src build/BRIEF.md:92] |
| 2026-11-09 08:35 | Airship props must spin; water noises by speed; white water at the bow (P3 owns) | [src build/BRIEF.md:101] |
| 2026-11-09 08:40 | Moving parts everywhere (flags, recoil, funnel smoke, lighthouse beam) | [src build/BRIEF.md:119] |
| 2026-11-09 08:45 | PEACEFUL SAILING FIRST; P2 parked; game opens in Peaceful; ships swappable | [src build/BRIEF.md:136] |
| 2026-11-09 09:58 | H1 inserted: 'drifty and laggy', ~6 FPS measured | [src build/BRIEF.md:145] |
| 2026-11-09 10:20 | Music OFF by default in every build | [src build/BRIEF.md:148] |
| 2026-11-09 11:35 | R1 inserted: hi-res library ships after H1 | [src build/BRIEF.md:151] |
| 2026-11-09 11:56 | A1 inserted: autopilot after R1 | [src build/BRIEF.md:154] |
| 2026-11-09 12:21 | W1 inserted: ship too buoyant, bow splash clips through hull, sea too flat | [src build/BRIEF.md:157] |
| 2026-11-09 12:30 | Use Ursa (CPU + 2 GPU partitions) to offload batch work; workers write requests in URSA_REQUESTS.md | [src build/BRIEF.md:160] |
| 2026-11-09 13:20 | E1 inserted: real world assets from Ursa prep | [src build/BRIEF.md:170] |
| 2026-11-09 22:35-22:40 | Direction change: game centres on SAILING ships; sailing frigate default; close zoom to ~20 m; W2 evil storm requested | [src build/BRIEF.md:173] |
| 2026-11-09 23:50-00:00 | Audio only water + environment; I1 'make lots more islands'; never sink below paint line | [src build/BRIEF.md:183] |
| 2026-11-10 05:16 | W2 closed (storm 3.5/10, systems built); W3 readable storm after I1 | [src build/BRIEF.md:190] |
| 2026-11-10 09:20 | Chuck: two-layer water bug top priority (fixed in W3, commits 2d64892, 28de2c7) | [src build/logs/W3.md:6] |
| 2026-11-10 09:55 | Audio: harbour_ambience removed ('angelic'), bow_rush/hull_hiss added | [src build/logs/W3.md:19] |
| 2026-11-10 12:55 | Storm look PARKED; ship water FX stay in scope (corrected) | [src build/BRIEF.md:62] |
| 2026-11-10 15:45 | Ship water FX scale with speed and wave | [src build/BRIEF.md:61] |
| 2026-11-10 21:25 | Ironclad turn radius 280 -> 330 m approved | [src build/logs/P8.md:22] |
| 2026-11-10 22:40 | Collision is a hard rule | [src build/BRIEF.md:192] |
| 2026-11-10 22:55 | P11 card: living world, sailing hum, keyboard help (Chuck) | git log: commit 5be67b2 |
| 2026-11-11 08:52 | Test windows steal focus: -RenderOffScreen, then -nosound | [src build/BRIEF.md:3] |

Review verdicts folded into decisions:

- Card E morning report 2026-11-09: slice as a game `5/10` [src build/MORNING_REPORT.md:55], look `4.5/10` [src build/MORNING_REPORT.md:56].
- P6 Opus review 2026-11-10: game `4/10` [src build/REVIEW_opus.md:27], look `5/10` [src build/REVIEW_opus.md:28], feel `5/10` [src build/REVIEW_opus.md:29]; verdict "ship with caveats" [src build/REVIEW_opus.md:12].
- MORNING_REPORT_2 verdict: Ironwake_P5.app is the new build; Ironwake.app not replaced (promotion gate not met) [src build/MORNING_REPORT_2.md:3].

## 1.9 Design-versus-implementation conflicts (summary)

| # | Design says | Takes effect | Flag |
|---|---|---|---|
| 1 | Captain default (CONCEPT) | Peaceful default (code) | CONFLICT |
| 2 | Ironclad 140 m (ART_BIBLE), BRIEF says scale to ~140 m [src build/BRIEF.md:80] | 125 m fleet def | CONFLICT |
| 3 | Speed compression 3x open water (DESIGN) | 1.5 (code) | CONFLICT |
| 4 | RMB zoom, Tab fleet orders, F searchlight | removed / pause / photo mode | CONFLICT |
| 5 | Peaceful chosen per save at start | menu toggle each start | minor |
| 6 | Scans-only world (ART_BIBLE) | Hunyuan-generated town and harbour pieces | CONFLICT (undocumented waiver) |
| 7 | 3 sea zones, 4 factions | single calm sea, no factions | not built |

## Gaps / unknown

- Which of CONCEPT's emergent-battle AI features (utility AI with faction relations) exist: only the single NPC ironclad utility AI is documented (build/logs/C.md); faction layer is absent. Measure: grep IronwakeCombat.cpp for faction.
- Whether Chuck formally waived the scans-only art rule for Hunyuan assets: no log records a decision; ask Chuck.
- Whether the in-game MODE toggle persists across launches: unknown. Check IronwakeSettings.cpp keys (ch. 9).
- Open questions in CONCEPT.md (camera POV, fleet command, name) [src [REDACTED]/Projects/game-concepts/ironwake/CONCEPT.md:107-110]: name still a placeholder; fleet command not built; camera is third-person orbit with scope and photo views, no bridge view found in the controls list.

### P21 NPC traffic rules (2026-11-12)
- One vessel at a time between the harbour waiting points (H x 258-286 m) and the berths: the basin token (`BasinFree/Take/Drop`). Others hold at their waiting point, give up after 420 s, anchor and retry later.
- Work boats leave to the north roads only; anchorage arrivals use a slot grid per port. A ship blocked by a moored or lower-numbered neighbour for 5 s backs off for 10-14 s. Merchants and ferries skip the moored-yield rule.
- Status: not clean. Soak best 30 collisions / 22 stuck in 50 game-minutes (x8). Known: merchants and the lower-numbered brigs meet at the roads (H ~ 300-350 m, 60-110 m north), see build/logs/P21.md.

### P22 traffic state (2026-11-12)
See build/logs/P22.md for the soak numbers and what is still open. Rules added: tug station off the roads waypoint, moored steamer is an obstacle.

### P29 data-driven multi-port world, arrival Harbourmaster & 8-good trade loop (2026-11-13)
- World ports & commodities load at runtime from `Content/IronwakeData/world_ports.json` (`3 ports, 8 goods` [src build/logs/P29.md:4]): **Greyhaven Harbour** (`(-3450, -3350)` m), **Ironcrag** (`(2590, 90)` m on `HostIsleIdx = 8`, basalt coaling & foundry haven), and **Stigsby** (`(-570, 3550)` m on `HostIsleIdx = 9`, wooded fishing & timber outpost).
- Resolves Opus review gap #8: slowing `< 2.5` kn in port or at a landmark prompts `Enter` to Make Fast over a live Harbourside diorama (`10 / 10` [src build/logs/P29.md:6]), trade the 8 goods (`Coal`, `Pig Iron`, `Salt Fish`, `Naval Timber`, `Brasswork`, `Sailcloth`, `Whale Oil`, `Mail Packet`) for persisted `Silver Crowns`, repair/swap ships, and record career voyages in the Captain's Logbook (`9.8 / 10` [src build/logs/P29.md:7]).
