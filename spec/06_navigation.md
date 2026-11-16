# Chapter 6: Navigation

Scope: how the player finds and reaches places: the 18 places (4 core points of interest plus 14 archipelago islands; names, coordinates, sizes, what is there), the NEXT / point-of-interest logic (N key, arrival test, visited flags, progression), the helmsman (J), anchor (X), the compass tape and minimap HUD, the autopilot (`P`: OFF / HOLD COURSE / WAYPOINTS / TOUR) and its steering law, the full-screen chart (`M`) with waypoint editing, the obstacle planner (hazard circles, tangent detours), the give-way and under-fire rules, the tour route and its orbits, and the measured autopilot results (A1, P3, P9). Source files: `IronwakeVoyage.{h,cpp}` (POI list, HUD, helmsman, anchor, NextPoint), `IronwakeAutopilot.cpp` (modes, chart, planner), `IronwakeArchipelago.{h,cpp}` (islands), `IronwakeLivingSea.*` (St Brannoc and merchants), `IronwakePlayerController.cpp` (key bindings). All world coordinates are metres or cm in the world frame +X = north, +Y = east, yaw 0 = +X = bearing 000, clockwise positive. Every value is cited as `value` [src path:line]; conflicts are marked CONFLICT with the live value.

## 6.1 World frame, spawn and the home port

| Item | Value | Source |
|---|---|---|
| Player spawn (cm) | `(-300000, -270000, 150)`, yaw `45` | `FVector(-300000.f, -270000.f, 150.f)` [src Source/IronwakeTest/IronwakeGameMode.h:42] `float SeaSpawnYaw = 45.f` [src Source/IronwakeTest/IronwakeGameMode.h:46] |
| Greyhaven Harbour centre `HarbourC` (cm) | `(-345000, -335000)` = `(-3450, -3350)` m | `FVector HarbourC = FVector(-345000.f, -335000.f, 0.f)` [src Source/IronwakeTest/IronwakeVoyage.h:192] |
| Wreck centre `WreckC` (cm) | `(-92000, -150000)` = `(-920, -1500)` m | `FVector WreckC = FVector(-92000.f, -150000.f, 0.f)` [src Source/IronwakeTest/IronwakeVoyage.h:193] |
| Needles centre `StacksC` (cm) | `(110000, -90000)` = `(1100, -900)` m | `FVector StacksC = FVector(110000.f, -90000.f, 0.f)` [src Source/IronwakeTest/IronwakeVoyage.h:194] |
| St Brannoc island centre (cm) | `(-50000, 190000)` = `(-500, 1900)` m ("1.8 km off the spawn, 15 deg to starboard of its heading") | `FVector IslandCentre = FVector(-50000.f, 190000.f, 0.f)` [src Source/IronwakeTest/IronwakeLivingSea.h:121] |
| Harbour frame | yawed `45` deg, local +X seaward (NE), +Y along the quay | `FRotator(0.f, 45.f, 0.f)` [src Source/IronwakeTest/IronwakeAutopilot.cpp:71] |
| Anchorage `AnchorPt` | harbour-frame `(450, 0)` m = world `(-313180, -303180)` cm (logged) | `AnchorPt = L(450.f, 0.f)` [src Source/IronwakeTest/IronwakeAutopilot.cpp:114] `X=-313180.195` [src build/review_opus/circle2_ironclad.log:363] |
| Spawn to harbour | 7 km sea tile: ocean/zone extents raised to 8.4 km in I1 | `8.4 km` [src build/logs/I1.md:32] |

Distance spawn -> Greyhaven centre (derived arithmetic): `sqrt(450^2 + 650^2)` = about `790` m (the harbour is "just behind the spawn").

## 6.2 The 18 places (POIs)

The POI list is built in `AIronwakeVoyage::BeginPlay`: "POIs in sailing order from the spawn (just outside the harbour): wreck -> stacks -> lighthouse -> home." `POIs in sailing order from the spawn` [src Source/IronwakeTest/IronwakeVoyage.cpp:284] Index 0-3 are the core points; indices 4+ are the 14 islands appended in `IsleDefs` order `every archipelago island is a point of interest (indices 4+` [src Source/IronwakeTest/IronwakeVoyage.cpp:295]. Struct `FIronwakePOI`: `Name`, `Tag` (1-2 letters for the minimap), `Loc`, `ArriveRadiusCm` default `50000`, `HardRadiusCm` default `0`, `bVisited` `float ArriveRadiusCm = 50000.f` [src Source/IronwakeTest/IronwakeVoyage.h:30].

### 6.2.1 Core four (indices 0-3)

| Idx | Name (as shown) | Tag | Location (m) | Arrive radius (cm) | Hard radius (cm) | What is there |
|---|---|---|---|---|---|---|
| 0 | Wreck of the Valiant | `W` | `(-920, -1500)` | `60000.f` [src Source/IronwakeTest/IronwakeVoyage.cpp:289] | `9000` | SS Rifle stern scan x8 (60 m), heeled and cocked up; foam rings, broken mast, 20 floating debris pieces, salvage boat, green wreck buoy; hazard circle `17000` cm |
| 1 | The Needles (sea stacks) | `N` | `(1100, -900)` | `70000.f` [src Source/IronwakeTest/IronwakeVoyage.cpp:290] | `26000` | five boulder-mesh stacks `95`, `74`, `55`, `36`, `22` m high; hazard circle `36000` |
| 2 | St Brannoc Light | `L` | `(-500, 1900)` | `80000.f` [src Source/IronwakeTest/IronwakeVoyage.cpp:293] | `0` | lighthouse island (card D): boulder crag, lime/oxblood tower, hut, lamp light, harbour-type ambience; hazard circle `42000` |
| 3 | Greyhaven Harbour | `H` | `(-3450, -3350)` | `60000.f` [src Source/IronwakeTest/IronwakeVoyage.cpp:289] | `0` | home port: quay, breakwaters, pier, town, factory, moored SS Wren, mooring mast for the brass airship, signal tower (red) and beacon (green) at the mouth |

Source of the hard radii: `Add(name, tag, loc, Arrive, Hard)` calls `Add(TEXT("The Needles (sea stacks)"), TEXT("N"), StacksC, 70000.f, 26000.f)` [src Source/IronwakeTest/IronwakeVoyage.cpp:290] and the comment "card D already stops ships at 300 m" for St Brannoc `card D already stops ships at 300 m` [src Source/IronwakeTest/IronwakeVoyage.cpp:293]. `HardRadiusCm` has no remaining collision role (chapter 5.13); it is read only by the S2 test director `Pt.HardRadiusCm + 15000.f` [src Source/IronwakeTest/IronwakeS2Test.cpp:694].

Wreck stack heights from `BuildStacks`: heights `95`, `74`, `55`, `36`, `22` m at local offsets `(0,0)`, `(9000,6000)`, `(-7000,9500)`, `(4000,-9000)`, `(-11000,-4000)` cm `FVector2D(0, 0), 95.f, 1.3f, 10.f` [src Source/IronwakeTest/IronwakeVoyage.cpp:596] `FVector2D(-11000, -4000), 22.f` [src Source/IronwakeTest/IronwakeVoyage.cpp:600]. Wreck orientation: yaw `115` `FRotator(0.f, 115.f, 0.f)` [src Source/IronwakeTest/IronwakeVoyage.cpp:525]. Wreck debris: `20` pieces within `1500-4500` cm `i < 20` [src Source/IronwakeTest/IronwakeVoyage.cpp:560].

Channel buoys: 4 red/green pairs from `HarbourC + (30000, 30000)` toward the wreck, every `40000` cm, `16000` cm either side (8 buoys), port red / starboard green, lamps blink at night `40000.f * i` [src Source/IronwakeTest/IronwakeVoyage.cpp:638] `s * 16000.f` [src Source/IronwakeTest/IronwakeVoyage.cpp:638].

### 6.2.2 The 14 islands (indices 4-17)

Positions and nominal radii are the `IsleDefs` table (`build/i1/plan.py`, seed 1871) in metres `build/i1/plan.py (seed 1871)` [src Source/IronwakeTest/IronwakeArchipelago.cpp:46]; kind multipliers: Cay `R *= 0.95`, Rocky `R *= 1.1` `I.R *= 0.95f` [src Source/IronwakeTest/IronwakeArchipelago.cpp:247]. Logged hazard radius is from an actual run `IWI1 isle Kestrel Isle` [src build/review_opus/circle2_ironclad.log:334].

| Idx | Name | Tag | Kind | X, Y, Rm (m, code) | Peak (m) | Hazard radius (m, logged) | What is there |
|---|---|---|---|---|---|---|---|
| 4 | Kestrel Isle | `K` | Wooded | `620.f, -3210.f, 520.f` [src Source/IronwakeTest/IronwakeArchipelago.cpp:48] | `48` | `hazard=762 m` [src build/review_opus/circle2_ironclad.log:334] | wooded island ~1 km across, beach cove on the lee side, fishing village (scaled E1 wharf + warehouse, 4-section timber pier, moored rowboats/fourareen/pinnace, boats hauled up, gas lamps lit at night), rock ring and 6 outcrops |
| 5 | Fort Grimsby | `F` | Fort | `1250.f, 1950.f, 230.f` [src Source/IronwakeTest/IronwakeArchipelago.cpp:49] | `14` (plateau) | `hazard=370 m` [src build/review_opus/circle2_ironclad.log:341] | table island with a ruined square fort (Poly Haven modular fort kit, 44 m curtain, gate, round towers, bastions; ~20 % tumbled) |
| 6 | Morrow Head | `M` | Lighthouse | `20.f, -910.f, 260.f` [src Source/IronwakeTest/IronwakeArchipelago.cpp:50] | `9` (+ crag top 40 m recipe) | `hazard=411 m` [src build/review_opus/circle2_ironclad.log:343] | boulder crag with a lime/oxblood lighthouse; its lamp is a night lamp |
| 7 | Gannet Stacks | `G` | Stacks | `-1950.f, -1130.f, 200.f` [src Source/IronwakeTest/IronwakeArchipelago.cpp:51] | n/a (rocks only) | `hazard=330 m` [src build/review_opus/circle2_ironclad.log:344] | sea stacks: `5` stacks (R > 180 m) `20-80` m tall plus a natural arch |
| 8 | Sable Cay | `S` | Cay | `-2160.f, 140.f, 210.f` [src Source/IronwakeTest/IronwakeArchipelago.cpp:52] | `7` | `hazard=329 m` [src build/review_opus/circle2_ironclad.log:346] | low sandy cay with dunes, scrub, driftwood, pebble beaches, turquoise shelf |
| 9 | The Teeth (reef) | `T` | Reef | `3420.f, 3140.f, 260.f` [src Source/IronwakeTest/IronwakeArchipelago.cpp:53] | n/a | `hazard=414 m` [src build/review_opus/circle2_ironclad.log:347] | crescent reef of `26` low rocks awash with constant breaking surf |
| 10 | Drowned Men's Shoal | `D` | Graveyard | `-2950.f, 3030.f, 280.f` [src Source/IronwakeTest/IronwakeArchipelago.cpp:54] | n/a | `hazard=424 m` [src build/review_opus/circle2_ironclad.log:348] | wreck graveyard: `34` rocks on a reef, three wreck-stern scans, torpedo boat on her beam ends, floating barrels/crates |
| 11 | Fir Holm | `Fh` | Pines | `-3360.f, -1310.f, 200.f` [src Source/IronwakeTest/IronwakeArchipelago.cpp:55] | `12` | `hazard=330 m` [src build/review_opus/circle2_ironclad.log:350] | pine holm on rock-scan landforms with firs, olive-type trees, shrubs, dead trunks |
| 12 | Seal Rocks | `Sr` | Rocky | `2590.f, 90.f, 120.f` [src Source/IronwakeTest/IronwakeArchipelago.cpp:56] | `5` | `hazard=258 m` [src build/review_opus/circle2_ironclad.log:351] | rock platform with `20-60` m stacks and an arch |
| 13 | Little Sable | `Ls` | Cay | `-570.f, 3550.f, 120.f` [src Source/IronwakeTest/IronwakeArchipelago.cpp:57] | `7` | `hazard=214 m` [src build/review_opus/circle2_ironclad.log:353] | small sandy cay |
| 14 | Heron Holm | `Hh` | Pines | `1560.f, 890.f, 150.f` [src Source/IronwakeTest/IronwakeArchipelago.cpp:58] | `12` | `hazard=262 m` [src build/review_opus/circle2_ironclad.log:355] | pine holm |
| 15 | The Anvil | `A` | Rocky | `470.f, 670.f, 140.f` [src Source/IronwakeTest/IronwakeArchipelago.cpp:59] | `5` | `hazard=291 m` [src build/review_opus/circle2_ironclad.log:356] | rock platform, stacks, arch |
| 16 | Blackscar | `B` | Cliff | `-1240.f, -2650.f, 180.f` [src Source/IronwakeTest/IronwakeArchipelago.cpp:60] | `8` (+ crag 50 m) | `hazard=303 m` [src build/review_opus/circle2_ironclad.log:358] | cliff crag |
| 17 | The Sisters | `Si` | Stacks | `-1480.f, 3360.f, 150.f` [src Source/IronwakeTest/IronwakeArchipelago.cpp:61] | n/a | `hazard=262 m` [src build/review_opus/circle2_ironclad.log:359] | `3` sea stacks (R <= 180 m) and an arch |

Notes: peaks per kind from `case EIsle::...: I.Hp = N * CM` (Wooded 48, Pines 12, Cay 7, Fort 14, Lighthouse 9, Cliff 8, Rocky 5) `case EIsle::Fort: I.Hp = 14.f * CM; I.bPlateau = true` [src Source/IronwakeTest/IronwakeArchipelago.cpp:248]; Stacks count `I.R > 18000 ? 5 : 3` `I.R > 18000.f ? 5 : 3` [src Source/IronwakeTest/IronwakeArchipelago.cpp:384]; reef rock count `26` `k < 26` [src Source/IronwakeTest/IronwakeArchipelago.cpp:409]; graveyard rocks `34` `k < 34` [src Source/IronwakeTest/IronwakeArchipelago.cpp:839]. I1 log lists islands by kind and notes "Every island is a voyage POI, an autopilot obstacle, a chart outline" `Every island is:` [src build/logs/I1.md:6].

Arrive radius for islands = `HazardR + 25000` cm (so arrival is reached while still outside the hazard circle) `I.HazardR + 25000.f` [src Source/IronwakeTest/IronwakeVoyage.cpp:305]. Derived: Kestrel 762 m + 250 m = `1012` m; Little Sable 214 m + 250 m = `464` m.

Place count: 4 core + 14 islands = 18 (SPEC.md: "18 places") `18 places` [src SPEC.md:46]. The Kestrel isle name differs from the card-D island name St Brannoc (core). Island names in the minimap/compass are the `Tag` strings; the NEXT line and toasts use the full name.

Island list, sea-state and chart fields live in `FIsle` (name, tag, kind, centre, radius, peak, `HazardR`, `bTerrain`, `bPlateau`) `struct FIsle` [src Source/IronwakeTest/IronwakeArchipelago.h:26].

## 6.3 NEXT / point-of-interest logic

State: `NextPOI` (index into `POIs`, starts `0`) `int32 NextPOI = 0;` [src Source/IronwakeTest/IronwakeVoyage.h:84].

| Behaviour | Rule | Source |
|---|---|---|
| N key | `NextPOI = (NextPOI + 1) % POIs.Num()`; toast "Course for <name>" | `NextPOI = (NextPOI + 1) % POIs.Num();` [src Source/IronwakeTest/IronwakeVoyage.cpp:875] ; key N is voyage key index 2 `case 2: Vy->NextPoint(); break;` [src Source/IronwakeTest/IronwakePlayerController.cpp:804] |
| Arrival | each tick, if `Dist2D(ship, POIs[NextPOI].Loc) < ArriveRadiusCm`: `bVisited = true`; `RecordArrival(NextPOI, POIs.Num())`; toast "Arrived: <name>" (6 s) or "...NEW SHIP UNLOCKED..." (10 s); `NextPOI` advances by one (wrapping) | `FVector::Dist2D(P->GetActorLocation(), Pt.Loc) < Pt.ArriveRadiusCm` [src Source/IronwakeTest/IronwakeVoyage.cpp:1078] |
| Only `NextPOI` is tested | other places can be sailed through without counting; sailing to a later one needs N presses | same block |
| Helmsman arrival | helmsman stops (`bHelm = false`), rudder midships, telegraph `3` (SLOW AHEAD) | `P->IWTelegraph(3)` [src Source/IronwakeTest/IronwakeVoyage.cpp:1086] |
| Progression | visited mask bit `1 << PoiIndex` (indices < 31) and `Arrivals + 1` saved in GameUserSettings `[Ironwake]` keys `PoiMask`, `Arrivals`; ironclad unlocks at `Arrivals >= 10` | `PoiIndex >= 0 && PoiIndex < 31` [src Source/IronwakeTest/IronwakeFleet.cpp:175] |
| Progress line | counts only bits 0-3: "Points of interest visited %d/4     Voyages %d/%d (ironclad)" | `for (int32 i = 0; i < 4; ++i)` [src Source/IronwakeTest/IronwakeFleet.cpp:194] |

**CONFLICT (progression text):** the S2 log and `IronwakeFleet.h` say "the cutter is the start ship; visiting all four points of interest unlocks the frigate; 10 arrivals unlock the ironclad" (header comment, itself stale: the header says cutter is the start ship, the code makes the frigate the start ship) `visiting all four points of` [src Source/IronwakeTest/IronwakeFleet.h:17], but `IsUnlocked` returns true for the frigate and cutter unconditionally ("Chuck (22:35): the sailing frigate is the start ship") `the sailing frigate is the start ship` [src Source/IronwakeTest/IronwakeFleet.cpp:165]. Code wins: frigate and cutter always, ironclad after 10 arrivals. Arrivals increment on every arrival including repeats (no de-duplication by place), so 10 arrivals can be earned on one place by looping N around.

HUD NEXT line text format: "NEXT  <name>   <km> km   brg <deg>  (<dead ahead | N deg to port/starboard>)" plus "   [HELMSMAN]" when `bHelm`; distance in km `Dist2D / 100000` `NEXT  %s   %.1f km   brg %03.0f  (%s)%s` [src Source/IronwakeTest/IronwakeVoyage.cpp:1290]. "dead ahead" when `|rel| < 3` deg `FMath::Abs(Rel) < 3.f` [src Source/IronwakeTest/IronwakeVoyage.cpp:1289].

## 6.4 Compass tape, minimap and key hints (HUD)

Compass tape (drawn by `DrawVoyageHUD`): width `640 * S`, height `34 * S`, top `18 * S`; shows `+-60` deg around the camera view yaw (`PxPerDeg = TapeW/120`); tick every `5` deg, major every `45` deg with N/NE/E... labels, numeric labels every `15` deg `const float TapeW = 640.f * S, TapeH = 34.f * S, TY = 18.f * S;` [src Source/IronwakeTest/IronwakeVoyage.cpp:1231] `d += 5` [src Source/IronwakeTest/IronwakeVoyage.cpp:1235]. POI diamonds: next = bright `Hi`, visited = dim, others grey; tag text unless off-tape (clamped at the edge, next always labelled) `const bool bEdge = FMath::Abs(Dl) > 60.f` [src Source/IronwakeTest/IronwakeVoyage.cpp:1254]. Ship heading caret and text "HDG %03.0f %s" with 8-point compass names `HDG %03.0f %s` [src Source/IronwakeTest/IronwakeVoyage.cpp:1278]. Autopilot set-course marker: hollow caret in colour `(0.45, 0.95, 0.75)` at the hold yaw (mode 1) or the planner's target yaw (modes 2-3) `ApMode == 1 ? HoldYaw : ApTargetYaw` [src Source/IronwakeTest/IronwakeVoyage.cpp:1266]. Chip "AUTOPILOT: <label>" below.

Minimap (bottom right): size `230 * S`, half-range `400000` cm (+-4 km), north up, east right; merchants as 4 px squares, airship as `~`, POIs as 7 px squares with tags, ship arrow, hold line `300000` cm long (mode 1), planned path in `(0.45, 0.95, 0.75)` (modes 2-3), and a faint line from the ship to the NEXT POI when the autopilot is off `const float MS = 230.f * S` [src Source/IronwakeTest/IronwakeVoyage.cpp:1347] `const float Half = 400000.f;` [src Source/IronwakeTest/IronwakeVoyage.cpp:1348] `FRotator(0.f, HoldYaw, 0.f).Vector() * 300000.f` [src Source/IronwakeTest/IronwakeVoyage.cpp:1389].

Hint line, bottom-left: "H: help   M: chart   P: autopilot   Esc: menu" `H: help   M: chart   P: autopilot   Esc: menu` [src Source/IronwakeTest/IronwakeVoyage.cpp:1408]. Chips: ANCHORED, FAST SAIL, weather (S1). Toasts: centred at `0.30` of screen height, scale `1.2` `Cv->ClipY * 0.30f` [src Source/IronwakeTest/IronwakeVoyage.cpp:1414].

## 6.5 Key bindings for navigation (IronwakePlayerController.cpp)

| Key | Action | Source |
|---|---|---|
| `N` | next point of interest | voyage key index 2 above |
| `J` | helmsman toggle | `case 7: Vy->ToggleHelm(); break;` [src Source/IronwakeTest/IronwakePlayerController.cpp:809] |
| `X` | anchor toggle | `case 3: Vy->ToggleAnchor(); break;` [src Source/IronwakeTest/IronwakePlayerController.cpp:805] |
| `G` | fast sail (x4 passage speed; peaceful only) | `case 4: Vy->ToggleFastSail(); break;` [src Source/IronwakeTest/IronwakePlayerController.cpp:806] |
| `P` | cycle autopilot (OFF -> HOLD -> WAYPOINTS -> TOUR -> OFF); in the chart with waypoints: start the route | `case 0: Vy->CycleAutopilot(); break;` [src Source/IronwakeTest/IronwakePlayerController.cpp:850] |
| `M` | open/close chart (not in photo or dock modal) | `case 1: if (!Vy->bPhoto && !Vy->bDockOpen) Vy->ToggleChart(); break;` [src Source/IronwakeTest/IronwakePlayerController.cpp:851] |
| `Enter` | start route (chart open), or Make Fast / Log Discovery underway | `if (Vy->bChart) Vy->ChartStart();` [src Source/IronwakeTest/IronwakePlayerController.cpp:853] |
| `Backspace` | clear waypoints (chart open) | `case 3: if (Vy->bChart) Vy->ClearWaypoints(); break;` [src Source/IronwakeTest/IronwakePlayerController.cpp:856] |
| `L` (chart open) | toggle route loop (else: weather) | `if (Vy->bChart && Which == 5)` [src Source/IronwakeTest/IronwakePlayerController.cpp:798] |
| Left / right click (chart) | add / remove-nearest waypoint | `OnChartClick, false` [src Source/IronwakeTest/IronwakePlayerController.cpp:776] |
| `W`/`S` | speed (the autopilot keeps steering) | chart sink context swallows A/D/look/zoom, W/S still pass `W/S still set the speed with the chart open` [src Source/IronwakeTest/IronwakePlayerController.cpp:745] |
| `A`/`D` | HOLD: nudge course +-5 deg; WAYPOINTS/TOUR: take the wheel (disengage) | `HoldYaw = FRotator::NormalizeAxis(HoldYaw + (RudIn > 0.f ? 5.f : -5.f))` [src Source/IronwakeTest/IronwakeAutopilot.cpp:460] |

Voyage keys exist only while `bStarted`, no menu/end screen and not paused `return bStarted && !IsMenuOpen()` [src Source/IronwakeTest/IronwakePlayerController.cpp:790]. In photo mode only F and L work `Which != 0 && Which != 5` [src Source/IronwakeTest/IronwakePlayerController.cpp:797], but `P` (autopilot) still works through its own context.

## 6.6 Helmsman (J) and anchor (X)

Helmsman (`bHelm`): each tick steers rudder steps for the NEXT POI: `Brg = BearingDeg(ship, POI)`; sailing ship: if the bearing is within `50` deg of the wind-from direction, bear away to `WindFrom +- 50` `if (FMath::Abs(Off) < 50.f)` [src Source/IronwakeTest/IronwakeVoyage.cpp:1066]; `Err = delta(heading, Brg)`, `E = Err - 6 * yawRate`, `Step = clamp(round(E / 9), -3, 3)`; applied via `IWRudder(Step)` only when the step changes `FMath::RoundToInt(E / 9.f), -3, 3` [src Source/IronwakeTest/IronwakeVoyage.cpp:1071]. Toggle: if the autopilot is engaged it is disengaged first ("Autopilot off: helmsman has the wheel"); on engage the anchor is weighed and a stopped ship is rung HALF AHEAD (`4`) `P->IWTelegraph(4);` [src Source/IronwakeTest/IronwakeVoyage.cpp:887]. The helmsman has no collision avoidance of its own (chapter 5.11).

Anchor (`bAnchored`): toggling on forces telegraph `2` (STOP), rudder midships, spawns a splash scale `0.25` at `6000` cm ahead of the bow, toast "Let go the anchor!" `SetWorldScale3D(FVector(0.25f))` [src Source/IronwakeTest/IronwakeVoyage.cpp:906]; while anchored, linear velocity decays with `exp(-Dt/5)` and yaw rate with `exp(-Dt/3)`; any telegraph other than STOP weighs the anchor ("Anchor aweigh") `FMath::Exp(-Dt / 5.f)` [src Source/IronwakeTest/IronwakeVoyage.cpp:1131] `Av.Z *= FMath::Exp(-Dt / 3.f)` [src Source/IronwakeTest/IronwakeVoyage.cpp:1134]. S1 measured: 12.1 kn -> 4.4 kn after 4 s, 1.5 kn after 8 s, 0.2 kn after 16 s `12.1 kn to 4.4 kn` [src build/logs/S1.md:28].

Fast sail (`G`): `SpeedCompression = 1.5 * 4` = `6` while on, `1.5` off; refused with an enemy in these waters in Captain mode `FastSailMulOn = 4.f` [src Source/IronwakeTest/IronwakeVoyage.h:315] `bFastSail ? 1.5f * FastSailMulOn : 1.5f` [src Source/IronwakeTest/IronwakeVoyage.cpp:921].

## 6.7 Autopilot modes (IronwakeAutopilot.cpp)

`ApMode`: `0` OFF, `1` HOLD COURSE, `2` WAYPOINTS, `3` TOUR; labels `OFF`, `HOLD COURSE`, `WAYPOINTS`, `TOUR` `TEXT("OFF"), TEXT("HOLD COURSE"), TEXT("WAYPOINTS"), TEXT("TOUR")` [src Source/IronwakeTest/IronwakeAutopilot.cpp:29]]. Cycle: `P` -> 1 -> 2 -> 3 -> disengage (toast "Autopilot off: you have the wheel"); inside the chart with waypoints, `P` starts the route; with an empty chart `P` closes it then cycles `case 0: EngageAutopilot(1); break;` [src Source/IronwakeTest/IronwakeAutopilot.cpp:306].

Common engage rules (`EngageAutopilot`): weighs the anchor; clears the helmsman; sets `bAutoHelm` (the wheel then follows `SetAutoRudder` only; A/D input and auto-centre ignored); clears flags; **telegraph ownership**: the autopilot rings the telegraph only if the ship is stopped or astern (index <= `2`): then `CruiseTel = TelFull (5)` for TOUR or `TelHalf (4)` otherwise; else it keeps the captain's setting `bApOwnsTel = P->TelegraphIndex <= TelStop;` [src Source/IronwakeTest/IronwakeAutopilot.cpp:248] `Mode == 3 ? TelFull : TelHalf` [src Source/IronwakeTest/IronwakeAutopilot.cpp:249]. Constants: `TelStop = 2`, `TelSlow = 3`, `TelHalf = 4`, `TelFull = 5` `constexpr int32 TelStop = 2, TelSlow = 3, TelHalf = 4, TelFull = 5;` [src Source/IronwakeTest/IronwakeAutopilot.cpp:31]. A captain's W/S while engaged transfers speed ownership ("Autopilot steering - speed is yours") `Autopilot steering - speed is yours` [src Source/IronwakeTest/IronwakeAutopilot.cpp:450]. Disengage: `bAutoHelm = false`; wheel centred unless the captain is holding A/D; restores a give-way-slowed telegraph `P->IWRudder(0)` [src Source/IronwakeTest/IronwakeAutopilot.cpp:291].

| Mode | Behaviour |
|---|---|
| 1 HOLD COURSE | locks present heading, or the camera bearing if the camera is more than `15` deg off the bow ("camera bearing"); A/D nudge `+-5` deg per press; steering through the common law below |
| 2 WAYPOINTS | follows `Waypoints` (max `12`) through `PlanLeg`-planned legs; waypoint accepted when within `Accept = max(15000, speed*8)` cm; loops if `bApLoop`, else last waypoint = slow to SLOW inside `FinalSlowCm` and stop; with no waypoints it holds the heading and opens the chart ("planning") |
| 3 TOUR | wreck -> Needles -> lighthouse, one clockwise orbit around each at SLOW, then home to the anchorage and drop the anchor |

Constants: `WpArriveCm = 15000` (150 m), `FinalSlowCm = 60000` (600 m, multiplied by `max(1, SpeedCompression/1.5 * 0.5)`), `FinalStopCm = 15000` (150 m) `constexpr float WpArriveCm = 15000.f;` [src Source/IronwakeTest/IronwakeAutopilot.cpp:32] `constexpr float FinalSlowCm = 60000.f;` [src Source/IronwakeTest/IronwakeAutopilot.cpp:33] `constexpr float FinalStopCm = 15000.f;` [src Source/IronwakeTest/IronwakeAutopilot.cpp:34]. The last-waypoint logic: telegraph SLOW inside `FinalSlowCm`; STOP when `DistWp < FinalStopCm`, or within `50000` cm and the waypoint is behind; "ROUTE COMPLETE" when stopped (`|speed| < 0.6` kn) within `50000` cm, then disengage ("Route complete: all stop") `FMath::Abs(P->GetSpeedKnots()) < 0.6f` [src Source/IronwakeTest/IronwakeAutopilot.cpp:502].

Label strings: `HOLD COURSE %03.0f`; `WAYPOINTS %d/%d` (+ ` LOOP`); `WAYPOINTS (planning: M chart)`; `TOUR %d/4: making for|circling <place>`; `TOUR 4/4: home to Greyhaven` `TOUR %d/4: %s %s` [src Source/IronwakeTest/IronwakeAutopilot.cpp:202].

### 6.7.1 Steering law (SteerAuto)

1. Sailing ship (S2): never steers into irons. If the wanted course is within `Beat = 50` deg of the wind-from direction, beat close-hauled on the tack nearer the course, with `12` deg hysteresis (`Off > 12`) before tacking (toast "Autopilot: ready about - tacking") and feed-forward zeroed `constexpr float Beat = 50.f;` [src Source/IronwakeTest/IronwakeAutopilot.cpp:391].
2. P9 safety net: `Safe = AvoidLand(Desired, clamp(speedKn * 51.4444 * 1.5 * 40, 60000, 220000))`; if it changes the heading, feed-forward is zeroed `220000.f` [src Source/IronwakeTest/IronwakeAutopilot.cpp:404].
3. Invert the hull yaw model (yaw rate = v / MinTurnRadiusM * rudder fraction): `V = max(forward speed m/s, 1)`; `RMax = deg(V / MinTurnRadiusM)`; `Err = delta(yaw, Desired)`; `Rd = clamp(0.22 * Err + (|Err| < 25 ? FF : 0), -0.85 RMax, +0.85 RMax)`; measured yaw rate `R`; `Cmd = Rd + 1.2 (Rd - R)`; `Frac = clamp(Cmd / max(RMax, 0.05), -MaxRud, MaxRud)`; rudder = `Frac * MaxRudderDeg` via `SetAutoRudder` `0.22f * Err` [src Source/IronwakeTest/IronwakeAutopilot.cpp:415] `Rd + 1.2f * (Rd - R)` [src Source/IronwakeTest/IronwakeAutopilot.cpp:417]. "Normalised by speed so it behaves the same at SLOW and at fast sail." (A1 log text).

### 6.7.2 Look-ahead safety layer and give-way

Each tick after the mode logic:
- *Give way*: any merchant with `0 < Along < 40000 + LengthCm` and `|Lat| < 15000` cm (ahead and abeam) starts a give-way (counted) for `8` s: desired heading = ship yaw at the moment seen `+ 35` deg (to starboard), telegraph `max(SLOW, min(want, Cruise-1))`, toast "Autopilot: giving way to <name>"; not during an orbit `Along < 40000.f + M->LengthCm` [src Source/IronwakeTest/IronwakeAutopilot.cpp:607] `GiveWayUntil = Now + 8.f` [src Source/IronwakeTest/IronwakeAutopilot.cpp:617] `GiveWayYaw = P->GetActorRotation().Yaw + 35.f` [src Source/IronwakeTest/IronwakeAutopilot.cpp:612]. A1 log: "turns 35 deg to starboard and slows one step for 8 s" (mentions 400 m ahead and 150 m abeam).
- *Hazard look-ahead* (skipped while giving way): ray length `Look = 60000 + V*20` cm (limited to the target distance when following a leg); inside a circle (`D < 0.98 R`): head straight out at SLOW; ray crossing a circle: steer to the nearer tangent `Bc +- asin(1.15 R / D)`; during an orbit only hazards within `R + 5000` are considered `const float Look = 60000.f + VCm * 20.f;` [src Source/IronwakeTest/IronwakeAutopilot.cpp:631].
- *Under fire*: if the enemy's `ShellsFiredCount` rises while engaged: disengage with "Autopilot off: under fire" (A1 measured 0.9 s after the first salvo) `Autopilot off: under fire` [src Source/IronwakeTest/IronwakeAutopilot.cpp:437]. Also disengages when the player ship is sinking.
- *Replan*: once per second (`Fmod(Now, 1) < Dt`) if any hazard with `R >= 5000` now lies across the line to the target (`SegDist < 0.9 R`), `StartLeg` replans; buoys (`R < 5000`) are excluded because "replanning on them churns" `if (O.R < 5000.f) continue;` [src Source/IronwakeTest/IronwakeAutopilot.cpp:525].

### 6.7.3 Planner (`PlanLeg`)

For a straight segment A->B find the first hazard circle crossed (both ends outside `1.02 R`), then insert a detour at radius `Rm = 1.15 R` on the side the segment already passes: tangent points `acos(Rm/d)` off each end, an arc split into `ceil(sweep/40)` chords (chord points pushed out to `Rd = Rm / cos(half chord)`), or a single outside point at `1.05 Rm` when tangents overlap (sweep > `300` deg); recurse on every piece to depth `7` `Sw > 300.f` [src Source/IronwakeTest/IronwakeAutopilot.cpp:151] `FMath::CeilToInt(Sw / 40.f)` [src Source/IronwakeTest/IronwakeAutopilot.cpp:158]. `StartLeg` logs "IWA1 leg from ... pts, detours: ..." and toasts "Autopilot: detour around <name>". `BuildPreview` draws the whole route (from the current waypoint when mode 2).

Hazard circles list: chapter 5.12 (43 at runtime) `IWA1 obstacles=43` [src build/review_opus/circle2_ironclad.log:363].

## 6.8 Waypoint route UI (chart, `M`)

Opening: `ToggleChart` is blocked in photo mode; opening sets a chart input context (priority 30) that swallows mouse look, zoom, shift and A/D and routes left/right clicks to the chart `C->SetChartInput(bChart)` [src Source/IronwakeTest/IronwakeAutopilot.cpp:319]. A full-screen dark overlay (alpha `0.88`) with a square map: half-size in pixels `ChartHalfPx = min(ClipX * 0.36, ClipY * 0.45)`, centre at `(0.40 ClipX, 0.5 ClipY)`; world half-extent `ChartHalfCm = 1450000` (14.5 km half-width, 29 km x 29 km chart across the 34 km archipelago) `float ChartHalfCm = 1450000.f;` [src Source/IronwakeTest/IronwakeVoyage.h:249] `Cv->ClipX * 0.36f, Cv->ClipY * 0.45f` [src Source/IronwakeTest/IronwakeAutopilot.cpp:674]. Mapping: screen X = `CX + Y_world / half * halfPx`, screen Y = `CY - X_world / half * halfPx` (north up) `ChartCX + W.Y / ChartHalfCm * ChartHalfPx` [src Source/IronwakeTest/IronwakeAutopilot.cpp:374]. Clicks outside `|X|,|Y| <= ChartHalfCm` are ignored `FMath::Abs(W.X) > ChartHalfCm` [src Source/IronwakeTest/IronwakeAutopilot.cpp:327].

**Maritime grid & scale bar (P32):** when `ChartHalfCm > 600000` cm, the grid step scales automatically to `500000` cm (5 km grid) `GridStepCm = ChartHalfCm > 600000.f ? 500000.f : 100000.f` [src Source/IronwakeTest/IronwakeAutopilot.cpp:681] with a matching `5 km` brass scale bar `5 km` [src Source/IronwakeTest/IronwakeAutopilot.cpp:698].

Drawn layers in order: land (archipelago `DrawChartLand`: 48-segment filled polygon per terrain island using `RTheta`, dotted symbols for rock/reef/wreck islands, graveyard cross), hazards (dashed circle; filled brown disc at `0.62 R`, walls `0.55 R`, for non-island hazards), buoys (red/green 6 px squares), merchants (8 px squares with names), POIs (10 px squares, visited dim, name plates), anchorage cross, planned route (gold polyline), waypoints (circle radius `9000` cm with number), the ship (triangle + "YOU"), legend panel (CHART, "Greyhaven waters", autopilot label, route length km and ETA, waypoint count and LOOP state, detours, key list) `Circle(Waypoints[i], 9000.f` [src Source/IronwakeTest/IronwakeAutopilot.cpp:798]. ETA: `LenCm / 100 / max(Mps, 0.1) / 60` min with `Mps = max(speedKn, 5) * 0.5144 * SpeedCompression` (and " (fast sail)" when `bFastSail`) `FMath::Max(Ship->GetSpeedKnots(), 5.f)` [src Source/IronwakeTest/IronwakeAutopilot.cpp:846].

Editing: left click adds a waypoint: a point inside any hazard is pushed out to `R + 5000` cm; maximum `12` ("Chart full: 12 waypoints max") `O.R + 5000.f` [src Source/IronwakeTest/IronwakeAutopilot.cpp:346] `Waypoints.Num() >= 12` [src Source/IronwakeTest/IronwakeAutopilot.cpp:348]. Right click removes the nearest waypoint within `40000` cm (decrementing `CurWp` when needed) `BestD < 40000.f` [src Source/IronwakeTest/IronwakeAutopilot.cpp:332]. Backspace clears (and disengages mode 2). `L` toggles loop ("Route LOOP on" / "Route LOOP off: stop at the last waypoint"). Start: Enter or P (needs waypoints; else toast "No waypoints: left-click the chart to add some"); starting closes the chart. With waypoints already planned while engaged, waypoint progression: reached -> toast "Waypoint N reached", `CurWp = (CurWp + 1) % N`, `StartLeg` `Waypoint %d reached` [src Source/IronwakeTest/IronwakeAutopilot.cpp:513]. Legend text lines: "Left-click add waypoint", "Right-click remove nearest", "Enter / P start the route", "L loop on / off", "Backspace clear route", "M close chart", "W / S speed (kept by the autopilot)" `Right-click   remove nearest` [src Source/IronwakeTest/IronwakeAutopilot.cpp:867].

Mode 2 entry with no waypoints: holds present heading, opens the chart, toast "AUTOPILOT: WAYPOINTS - click the chart to add points, Enter or P to start" (6 s) `click the chart to add points, Enter or P to start` [src Source/IronwakeTest/IronwakeAutopilot.cpp:232].

## 6.9 Tour route (mode 3)

Order: `TourStop 0..2` = POIs[0] Wreck, POIs[1] Needles, POIs[2] St Brannoc Light; then home. Reset on engage: all `bVisited = false`, `TourStop = 0`, `TourPhase = 0` (phase 0 = making for, 1 = circling) `for (FIronwakePOI& Pt : POIs) Pt.bVisited = false;` [src Source/IronwakeTest/IronwakeAutopilot.cpp:272]. Approach point = POI location plus the unit vector from the POI toward the ship times `Ro`.

| Quantity | Value | Source |
|---|---|---|
| Orbit radius `Ro` in the tick | Wreck `50000` cm (500 m), Needles `60000`, others (lighthouse) `65000` | `Pt.Tag == TEXT("W") ? 50000.f : (Pt.Tag == TEXT("N") ? 60000.f : 65000.f)` [src Source/IronwakeTest/IronwakeAutopilot.cpp:535] |
| Orbit radius used for the FIRST leg at engage | wreck `50000`, otherwise `60000` | `Pt.Tag == TEXT("W") ? 50000.f : 60000.f` [src Source/IronwakeTest/IronwakeAutopilot.cpp:274] |
| Orbit start | when the leg ends or `abs(dist - Ro) < 8000` cm | `FMath::Abs(FVector::Dist2D(Loc, Pt.Loc) - Ro) < 8000.f` [src Source/IronwakeTest/IronwakeAutopilot.cpp:538] |
| Orbit direction | clockwise (sight stays on the starboard side) | `Clockwise orbit` [src Source/IronwakeTest/IronwakeAutopilot.cpp:549] |
| Orbit steering | `Desired = bearing(POI->ship) + 90 + clamp((r - Ro)/100 * 0.15, -35, +35)`, feed-forward `deg(v / Ro)` | `(Rm - Ro) / 100.f * 0.15f, -35.f, 35.f` [src Source/IronwakeTest/IronwakeAutopilot.cpp:555] |
| Orbit speed | SLOW (index `3`) when the autopilot owns the telegraph, else `min(Cruise, SLOW)` | `WantTel = bApOwnsTel ? TelSlow` [src Source/IronwakeTest/IronwakeAutopilot.cpp:557] |
| Orbit complete | swept angle >= `360` deg -> `bVisited`, toast "Tour: <name> visited", next leg | `OrbitSwept >= 360.f` [src Source/IronwakeTest/IronwakeAutopilot.cpp:559] |
| Home | leg to `AnchorPt`; SLOW inside `FinalSlowCm`; STOP inside `FinalStopCm` or `50000` cm and passing; stopped (`< 0.6` kn) within `50000` cm -> "TOUR COMPLETE", `POIs[3].bVisited = true`, autopilot off, anchor dropped, toast "Tour complete: anchored off Greyhaven" (6 s) | `Tour complete: anchored off Greyhaven` [src Source/IronwakeTest/IronwakeAutopilot.cpp:592] |

**CONFLICT (orbit radius):** the engage-time leg uses `60000` for non-wreck POIs, but the orbit loop uses `60000` for the Needles and `65000` for the lighthouse, so for St Brannoc the first approach point is at 600 m and the orbit then settles to 650 m (the steering term pushes outward). The live orbit radius is the tick values. The A1 log records the 3-4 orbits ran inside the hazard rings at entry ("Orbit entry cuts inside hazard rings") `Orbit entry cuts inside hazard rings` [src build/logs/A1.md:45] and the fix was not re-run in that card.

The tour does not visit the 14 islands: only the first 3 POIs and home (TOUR x/4). Marks `bVisited` only on the first three and home; it does NOT call `RecordArrival`, so tour completion adds no voyage progress (arrival counting is only in `TickSailing`'s NEXT test) `RecordArrival(NextPOI, POIs.Num())` [src Source/IronwakeTest/IronwakeVoyage.cpp:1082].

## 6.10 Tour camera clip (`-iwtour`, card D; not the autopilot tour)

The scripted gameplay clip in `AIronwakeLivingSea::TickTour` is separate: 0-3 s title menu over the sea, `StartGame(peaceful)` at `3.0` s, telegraph `4` at `3.2` s, crane shot 3-10 s, merchant pass 10-17 s, airship pass 17-24 s, at `24` s the enemy spawns and a scripted gunner (`StartCombatDemo`) fights; fight director from `30` s with a `14` s cycle; pause menu after the fight or `140` s; quit `3` s later `IPC->StartGame(false, true)` [src Source/IronwakeTest/IronwakeLivingSea.cpp:741] `TourT > 140.f` [src Source/IronwakeTest/IronwakeLivingSea.cpp:817]. FOV `50` `SetFieldOfView(50.f)` [src Source/IronwakeTest/IronwakeLivingSea.cpp:727]. Not part of free sailing.

## 6.11 Sailing ship routing considerations (S2)

The autopilot beating logic (6.7.1) and the helmsman's `50` deg bear-away are the only wind-aware routing; the planner and chart ignore wind (a route leg straight into the wind makes the frigate zig-zag). ETA on the chart uses current speed (min 5 kn) and compression, not polar speed, so sailing-ship ETAs are approximate. Anchor/hold on a sailing ship uses the same decay as steam.

## 6.12 Measured results (A1, P3, P9)

| Test | Result | Source |
|---|---|---|
| Hold course 120 s, storm sea x1.6, HALF | max error `0.20` deg, rms `0.07`; pass < 3 deg; peak wave yaw `0.78` deg/s | `max error 0.20 deg, rms 0.07` [src build/logs/A1.md:21] |
| Nudge D in HOLD | course 045 -> 050, settled < 1 deg in `26` s | `settled < 1 deg in 26 s` [src build/logs/A1.md:22] |
| 3-waypoint route (fast sail) | completed in `504` s; leg 2 detoured around The Needles; stopped `109` m from WP3; closest past hazard ring: Needles `24` m, wreck `268` m | `completed in 504 s` [src build/logs/A1.md:27] |
| TOUR (fast sail) | all 4 POIs visited, anchored `115` m from the anchorage (`564` m off Greyhaven) in `1647` s | `in 1647 s with fast sail` [src build/logs/A1.md:28] |
| Under fire | disengaged `0.9` s after first salvo | `disengaged 0.9 s after the first enemy salvo` [src build/logs/A1.md:29] |
| P3 voyage over all 18 places (nearest-neighbour order, fast sail x4, steam+sail frigate, moderate sea) | 18/18 waypoints; `30.4` km in `1221` s and `30.1` km in `1497` s | `30.4 km in 1221 s and 30.1 km in 1497 s` [src build/logs/P3.md:12] |
| Review log voyage run | "voyage complete: 18 waypoints, 30.4 km sailed, t=1185 s" | `18 waypoints, 30.4 km sailed` [src build/review_opus/voyage.log:1874] |
| P9 autopilot round island | ironclad `137` s, cutter `117` s, frigate `198` s, 0 groundings | see chapter 5.15 |
| Frigate beat test | held min TWA `50` deg with `0` s in irons | `0 s in irons` [src build/logs/S2.md:34] |

A1 known issues (still open in the logs): orbit entry cutting inside hazard rings; give-way is a fixed starboard swerve (no crossing prediction); chart labels crowd around Greyhaven `Chart labels still crowd around Greyhaven` [src build/logs/A1.md:48].

Voyage test hook: `-iwa1=hold|route|tour|fire|voyage|waterline` (`build/tools/a1run.sh`), `-iwa1quit`; the director clears all input mappings so stray keys cannot steer `ClearAllMappings` [src Source/IronwakeTest/IronwakeAutopilot.cpp:907]. Log key prefix `IWA1`; P3 voyage lines `IWP3 reached wp`.

## 6.13 Gaps / unknown

- Exact world coordinates of individual town features inside Greyhaven (terraces, houses) are chapter 7; only the harbour frame and centre are given here.
- Island peaks for Stacks/Reef/Graveyard (no terrain) and the Needles-style stack tops of islands: not constants (random ranges `20-80` m for Gannet Stacks per `FRandRange`); measure by logging the `Stack(...)` heights.
- The `+-4 km` minimap and `3.8 km` chart do not cover the whole tile (spawn box half-size `370000` cm): places beyond `3.8 km` of the origin (none of the 18 are, largest `|coordinate|` is `3550` m for Little Sable) are within range, but the enemy spawn box can be outside the chart.
- Tour route does not include the 14 islands; whether an island tour is wanted is a design question (SPEC.md does not list it).
- Whether `RecordArrival` should run for tour visits: code says no; behaviour unverified in play.
- Effective hazard radii of Kestrel and Fort Grimsby come from the I1-era logged run; later code changes to the `HazardR` formula (P9) were applied before the logs read here (the logs in `build/review_opus` post-date P9).
- Chart visual constants (colour values) are in the source but not tabulated here; see `DrawChart`.

## 6.14 P25 harbour-mouth one-way channel, waiting slots & quay-lane arbitrator (2026-11-13)

`AIronwakeNpcDirector` arbitrates Greyhaven harbour entrance and berth access via deterministic mutexes (`IronwakeNpc.h`, `IronwakeNpcBrain.cpp`):
- **Harbour Mouth Channel Lock & Split Fairways**: single-vessel throat token `MouthOwner` (`R < 260` m = `26000` cm around `H(155, 0)`) `FVector::Dist2D(O.Pos, MouthW) < 26000.f` [src Source/IronwakeTest/IronwakeNpcBrain.cpp:428]; inbound vessels enter on the starboard fairway `Y = -34` m `H2W(280.f, -34.f)` [src Source/IronwakeTest/IronwakeNpcBrain.cpp:456] while outbound vessels exit on the port-chart fairway `Y = +44..46` m `H2W(145.f, 46.f)` [src Source/IronwakeTest/IronwakeNpcBrain.cpp:772] (`>= 68` m lateral separation, clearing `ModularPier` and both stone jetties).
- **Outer Anchorage Waiting Ring (`WaitSlots`)**: `9` spaced slots on two arcs at `R = 385` m (`-50..+50` deg in `25` deg steps) and `R = 505` m (`-37.5..+37.5` deg in `25` deg steps) `385.f * FMath::Cos` [src Source/IronwakeTest/IronwakeNpcBrain.cpp:242] `505.f * FMath::Cos` [src Source/IronwakeTest/IronwakeNpcBrain.cpp:243]; holding ships set `bHold = true` and reset `StuckT = 0` when granted entry.
- **Exclusive Berth & Corridor Locks**: `ReserveBerthAndCorridor` checks `BowSternClearM = 0.35f * R.Def->LengthM + 10.f` [src Source/IronwakeTest/IronwakeNpcBrain.cpp:334], non-overlapping `Occ` Y-intervals (`HalfY = 0.5f * R.Def->BeamM + 4.5f` [src Source/IronwakeTest/IronwakeNpcBrain.cpp:335]), and adjacent-corridor exclusion (`|dY| < 24` m `< 24.f` [src Source/IronwakeTest/IronwakeNpcBrain.cpp:358]); stale reservations expire after `180` game-s `> 180.0` [src Source/IronwakeTest/IronwakeNpcBrain.cpp:280].
- **Measured 3-seed soak (`p22soak3.sh 1 2 3`, 3000 game-s at x8)**: Seed 1 `IWNPC collisions=0 stuck=0 groundings=0` [src build/logs/P25.md:22], Seed 2 `IWNPC collisions=0 stuck=0 groundings=0` [src build/logs/P25.md:23], Seed 3 `IWNPC collisions=0 stuck=0 groundings=0` [src build/logs/P25.md:24] (`live=30`, `moving=14-15`).
- **P30 6-port multi-port 3-seed soak (`3000` game-s at x8 across Greyhaven, Ironcrag & Stigsby)**: Seed 1 `IWNPC collisions=0 stuck=1 groundings=0` [src build/logs/P30.md:27], Seed 2 `IWNPC collisions=1 stuck=0 groundings=0` [src build/logs/P30.md:28], Seed 3 `IWNPC collisions=2 stuck=3 groundings=0` [src build/logs/P30.md:29] (`0` groundings across all 3 seeds).

## 6.15 P29 multi-port trade triangle, breakwater gates & arrival mooring (2026-11-13)
- **Hosted Port Obstacle Carve-outs:** islands listed in `Ar->HostedPortIsles.Contains(i)` [src Source/IronwakeTest/IronwakeAutopilot.cpp:93] skip monolithic island hazard discs and replace them with 3 back-quay + 2 breakwater-tip hazard circles, leaving the `130` m (`[-65, +65]` m) mouth throat open for autopilot and NPC route planning across all 3 ports (`Greyhaven • Ironcrag • Stigsby` [src Source/IronwakeTest/IronwakeAutopilot.cpp:842], chart vision `9.5 / 10` [src build/logs/P29.md:8]).
- **Arrival Mooring & Salvage Verb (`TryArrivalAction`):** slowing `< 2.5` kn inside any port's `MoorRadiusM` or near a landmark/wreck prompts `Enter` to call `Vy->TryArrivalAction()` [src Source/IronwakeTest/IronwakePlayerController.cpp:854], opening `UIronwakeDockWidget` or logging discovery/salvage; 1-seed 6-port NPC soak remains `SEED 1: IWNPC collisions=0 stuck=0 groundings=0` [src build/logs/P29.md:25].
