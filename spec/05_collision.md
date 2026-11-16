# Chapter 5: Collision and grounding

Scope: how ships interact with solid things. Source of truth is `IronwakeCollision.{h,cpp}` (the `UIronwakeColl` world subsystem: a registry of static 2D solids, moving hulls and the island seabed), `IronwakeShipCollision.cpp` (the per-tick response of the player/enemy pawn), plus the hooks in `IronwakeWorld.cpp` (`Place`), `IronwakeArchipelago.cpp`, `IronwakeP12.cpp`, `IronwakeLivingSea.cpp`, `IronwakeP8.cpp` (merchants and NPC craft) and the camera shake in `IronwakeShipPawn.cpp`. Covered: the solid types and how each is built (OBB / ellipse, margins, bow flare 2.5 %, x1.18 ellipse growth, the tall-rock rule), the z-range test against the draught, the spatial grid, the 2-tick prediction, the impact response formula, camera shake and pitch/roll kick, shoal/aground thresholds per hull, astern release, NPC and autopilot avoidance (`AvoidLand`, `PathClear`, hazard circles), and the `-iwnocoll` switch. Every value is cited as `value` [src path:line]; conflicts between comments and code are marked CONFLICT with the live value stated.

## 5.1 Design in one paragraph

Imported scans carry no simple collision and the hull is a velocity-driven physics box, so engine contact is not used. Instead the header states: "we keep a per-world registry of 2D obstacles (oriented boxes / ellipses taken from mesh bounds, z-range checked against the hull's draught), moving hulls (player, enemy, merchants, NPC craft: outline polygons) and the island seabed (FIsle::Height). Ships test their outline at the predicted pose every tick (sub-stepped, so nothing tunnels) and respond with speed loss / sliding / grounding." `per-world registry of 2D obstacles` [src Source/IronwakeTest/IronwakeCollision.h:3] The subsystem is created only in game worlds `W->IsGameWorld()` [src Source/IronwakeTest/IronwakeCollision.cpp:14].

Frame facts: all 2D coordinates are world cm in X/Y; an outline is in hull frame (cm, bow +X, relative to the actor origin) `bow +X, relative to the actor origin` [src Source/IronwakeTest/IronwakeCollision.h:45]. Yaw rotation helper: `Rot2` rotates by yaw degrees.

## 5.2 Data types

| Type | Fields | Source |
|---|---|---|
| `FIwHit` | `bHit`, `bLand`, `N` (unit, from obstacle toward ship), `Pen` (cm), `Tag`, `Other` (set for ship-vs-ship) | `Pen = 0.f;` [src Source/IronwakeTest/IronwakeCollision.h:23] |
| `FIwShape` | `Poly` (outline), `Dense` (edge sampling), `R` (bounding radius), `DraughtCm` default `500` | `float DraughtCm = 500.f` [src Source/IronwakeTest/IronwakeCollision.h:33] |
| `FObb` (a static solid) | centre `C`, unit axis `U`, half-extents `Half`, `ZMin`, `ZMax`, bounding radius `R`, `bEllipse`, `Tag` | `struct FObb` [src Source/IronwakeTest/IronwakeCollision.h:82] |
| Mask bits | `MaskSolids = 1`, `MaskPawns = 2`, `MaskOthers = 4`, `MaskAll = 7` | `MaskSolids = 1, MaskPawns = 2, MaskOthers = 4, MaskAll = 7` [src Source/IronwakeTest/IronwakeCollision.h:69] |
| Spatial grid cell | `CellCm = 25000` (250 m) | `CellCm = 25000.f` [src Source/IronwakeTest/IronwakeCollision.h:98] |

`AddObb` stores `U = Rot2((1,0), yaw)` and `R = Half.Size()` (the bounding radius used by the grid and the quick reject) `O.R = Half.Size()` [src Source/IronwakeTest/IronwakeCollision.cpp:100].

## 5.3 How each solid is built

### 5.3.1 From a mesh (AddMeshBounds)

Signature default `MinHalfCm = 80` (the header comment on registration says "anything >= 0.8 m half-extent") `float MinHalfCm = 80.f` [src Source/IronwakeTest/IronwakeCollision.h:55] `>= 0.8 m half-extent` [src Source/IronwakeTest/IronwakeCollision.h:7]. Steps in order `AddMeshBounds` [src Source/IronwakeTest/IronwakeCollision.cpp:104]:

1. Half-extents = mesh `BoxExtent.X/Y` x absolute transform scale (X/Y); centre = transform of the mesh bounds origin; yaw = transform yaw.
2. If `|Pitch| > 20` or `|Roll| > 20` degrees ("heavily tilted") the world AABB of the oriented box is used instead and yaw is forced to 0 `FMath::Abs(R.Pitch) > 20.f` [src Source/IronwakeTest/IronwakeCollision.cpp:114]. Else z range = world AABB min/max Z.
3. Reject if `max(Half.X, Half.Y) < MinHalfCm` `if (FMath::Max(Half.X, Half.Y) < MinHalfCm) return;` [src Source/IronwakeTest/IronwakeCollision.cpp:126].
4. Ellipse growth: `if (bEllipse) Half *= 1.18` ("inscribed ellipse of a rock's bounds under-covers its lumps: grow it toward the box") `Half *= 1.18f` [src Source/IronwakeTest/IronwakeCollision.cpp:127].
5. Tall-rock rule: if `ZMax - ZMin > 1500` and `ZMax > SeaZ + 1500` (a tall rock/stack) it becomes a plain box (`bEllipse = false`) and `Half += (350, 350)` cm margin, because "the hull's flared bow and rails overhang the waterline outline" `ZMax - ZMin > 1500.f` [src Source/IronwakeTest/IronwakeCollision.cpp:128] `Half += FVector2D(350.f, 350.f)` [src Source/IronwakeTest/IronwakeCollision.cpp:131]. (P9 log calls this "tall rocks use the full bounds box + 3.5 m margin".) `tall rocks use the full bounds box + 3.5 m margin` [src build/logs/P9.md:17]
6. `AddObb(centre, yaw, Half, ZMin, ZMax, bEllipse, Tag)`.

`AddComponent(C, Tag, bEllipse, MinHalfCm)` does not build immediately: it queues a pending entry and `Bake()` runs on the first collision query (so component transforms are final); the bake logs "IWP9 collision baked %d components -> %d solids total" `IWP9 collision baked %d components` [src Source/IronwakeTest/IronwakeCollision.cpp:151].

### 5.3.2 Who registers solids and with what parameters

| Registrant | Call | Shape | MinHalf (cm) | Source |
|---|---|---|---|---|
| Any `IronwakeWorld::Place` with `bSolid` (default `true`) | `Co->AddComponent(C, Name, bRound \|\| SteamBuoy, MinHalf)` | ellipse if the mesh name starts `Boulder`, `CoastLandRocks`, `CoastRocks`, `Nama`, `RockMoss`, `RockFace`, or `SteamBuoy`; else box | `250` (buoys `50`) | `N.StartsWith(TEXT("Boulder"))` [src Source/IronwakeTest/IronwakeWorld.cpp:37] `N.StartsWith(TEXT("SteamBuoy")) ? 50.f : 250.f` [src Source/IronwakeTest/IronwakeWorld.cpp:38] |
| Archipelago `Inst` (HISM instances of rocks and cliffs) | `AddMeshBounds(mesh, FTransform(R,W,S), N, true, 250)` for names starting Boulder, CoastRocks, CoastLandRocks, Nama, RockMoss, RockFace, CoastalCliff (vegetation and pebbles do not) | ellipse | `250` | `N.StartsWith(TEXT("CoastalCliff"))` [src Source/IronwakeTest/IronwakeArchipelago.cpp:186] `Co->AddMeshBounds(H->GetStaticMesh(), FTransform(R, W, S), N, true, 250.f)` [src Source/IronwakeTest/IronwakeArchipelago.cpp:187] |
| Archipelago lighthouse tower (primitive cylinder) | `AddComponent(Tw, "lighthouse", true, 100)` | ellipse | `100` | `TEXT("lighthouse"), true, 100.f` [src Source/IronwakeTest/IronwakeArchipelago.cpp:813] |
| Harbour quay apron slab (P12/P15) | `AddObb` for quay/land rectangles | box | 100 | `false, TEXT("quay")` [src Source/IronwakeTest/IronwakeP12.cpp:208] |
| P12 town props | `Place(..., bBlock=true)` for houses/rocks (default `bSolid=true`) | box / ellipse by name | `250` | `FVector(S), true);` [src Source/IronwakeTest/IronwakeP12.cpp:337] |
| P12 lamps and props | `Place(..., false, false)` (no collision) | none | | `FVector(1.f), false, false` [src Source/IronwakeTest/IronwakeP12.cpp:698] |
| P8 NPC craft hull | own `FIwShape` via `OutlineRect` (see 2.8) | polygon (moving) | | `OutlineRect(Bx.Min.X - 40.f` [src Source/IronwakeTest/IronwakeP8.cpp:160] |

**CONFLICT (comments vs code, code wins):** the `Place` header says "bSolid registers the mesh bounds (>= 2.5 m half-extent)" `>= 2.5 m half-extent` [src Source/IronwakeTest/IronwakeWorld.h:16] and the collision header says ">= 0.8 m half-extent" `>= 0.8 m half-extent` [src Source/IronwakeTest/IronwakeCollision.h:7]; the live value for `Place` is `MinHalf = 250` cm = 2.5 m (and `50` cm for buoys). `0.8 m` is only `AddMeshBounds`'s own default (80 cm), which `Place` overrides.

Everything with Nanite rock scans in the archipelago also generates a solid (so a large island has many ellipses), while the island body itself is NOT an obstacle solid: it is the seabed heightfield (5.6). Stacks, Reef and Graveyard islands have no terrain (`bTerrain = false`) and are therefore only rock solids `default: I.bTerrain = false; break; // Stacks / Reef / Graveyard: rocks only` [src Source/IronwakeTest/IronwakeArchipelago.cpp:252].

Solid count measured by P9: `1108` solids (rocks, cliffs, piers, quay slabs, buildings, buoys, lighthouse, wreck) `1108 solids` [src build/logs/P9.md:5]. The count at any later card is **unknown** (add more `Place` calls and re-read the "IWP9 collision baked" log line).

Solid sources by name seen in code: wreck stern (`WreckStern` via `Place`), the Needles stacks (`Boulder01`, scaled), St Brannoc island crag and rocks (`Boulder01` + footing rocks with `bBlock=true`), `SteamBuoy` channel buoys and the wreck buoy, harbour buildings, `MooringMast`, crates and barrels (props at `Place` default). Example calls: `TEXT("SteamBuoy"), FVector(-9000.f` [src Source/IronwakeTest/IronwakeVoyage.cpp:561] `TEXT("Boulder01"), FVector(0, 0, SeaR - 1500.f)` [src Source/IronwakeTest/IronwakeLivingSea.cpp:558].

### 5.3.3 Ship hulls as outlines

`OutlineFromProfile(Profile, Poly, PadCm)` builds the waterline polygon: starboard side stern to stem (each station `HalfB*100 + Pad`), a pointed stem at `X1*100 + Pad + 0.025 * Length*100` (the **2.5 % bow flare / forecastle overhang**), back along port, then the stern point at `X0*100 - Pad` `0.025f * P.Length() * 100.f` [src Source/IronwakeTest/IronwakeCollision.cpp:43]. With fewer than 2 stations it falls back to `OutlineRect(X0, X1, 1500)` `1500.f` [src Source/IronwakeTest/IronwakeCollision.cpp:37]. `OutlineRect(X0, X1, HalfB)` is a pointed rectangle: shoulders at `X1 - min(2.5*HalfB, 0.3*(X1-X0))`, stern corners at `0.7 * HalfB` `FMath::Min(HalfB * 2.5f, (X1 - X0) * 0.3f)` [src Source/IronwakeTest/IronwakeCollision.cpp:55] `HalfB * 0.7f` [src Source/IronwakeTest/IronwakeCollision.cpp:56].

`MakeShape(poly, draught)`: `Dense` = polygon edges sampled at `<= 300` cm `Densify(LocalPoly, 300.f, S.Dense)` [src Source/IronwakeTest/IronwakeCollision.cpp:157], `R` = max vertex distance from the origin, `DraughtCm` = draught.

Pad per pawn: `PadCm = clamp(0.012 * HullLengthM * 100, 60, 160)` cm `0.012f * HullLengthM * 100.f, 60.f, 160.f` [src Source/IronwakeTest/IronwakeShipCollision.cpp:26]. Derived by arithmetic: ironclad 125 m -> `150` cm; frigate 88 m -> `105.6`; cutter 30 m -> clamped up to `60`. Kracken 120 m -> `144`.

| Hull | Outline source | Pad (cm) | Draught (cm) |
|---|---|---|---|
| Player ironclad | `Devastation` profile | derived 150 | `700` `bIsNPC ? 600.f : 700.f` [src Source/IronwakeTest/IronwakeShipCollision.cpp:23] |
| Enemy | `Kracken` profile | derived 144 | `600` |
| Frigate | `Frigate` profile | derived 105.6 | `500` `DraughtCm = 500.f` [src Source/IronwakeTest/IronwakeShipCollision.cpp:22] |
| Cutter | `Aglae` profile | clamp -> 60 | `200` `DraughtCm = 200.f` [src Source/IronwakeTest/IronwakeShipCollision.cpp:21] |
| Goliath tug (merchant) | `Goliath` | `50` `OutlineFromProfile(Pr, Poly, 50.f)` [src Source/IronwakeTest/IronwakeLivingSea.cpp:138] | `250` |
| Aglae launch (merchant) | `Aglae` | `50` | `200` |
| P8 boats | bounds rectangle +40 cm each side | n/a | `clamp(-Bounds.Min.Z*0.5, 150, 400)` |

The keel check polygon (`CollKeel`): points every `6` m along the keel line (`y = 0`), plus every dense outline point with `y` scaled by `0.55` ("the keel is narrower than the waterline") `X += 6.f` [src Source/IronwakeTest/IronwakeShipCollision.cpp:32] `P.Y * 0.55f` [src Source/IronwakeTest/IronwakeShipCollision.cpp:33].

The pawn registers with `bPawn = true` for both player and NPC (`!bIsNPC || true`, always true) `RegisterHull(this, CollShape, !bIsNPC` [src Source/IronwakeTest/IronwakeShipCollision.cpp:34]. Merchants and P8 craft register `bPawn = false` (`RegisterHull(this, CollShape, false)` [src Source/IronwakeTest/IronwakeLivingSea.cpp:140]), so they are "others" (MaskOthers). `RegisterHull` replaces any old entry for the same actor; dead weak pointers are culled during `TestHull`.

Registration happens at the end of pawn `BeginPlay` via `RegisterCollisionHull()` `RegisterCollisionHull();` [src Source/IronwakeTest/IronwakeShipPawn.cpp:290], which also reads `-iwnocoll`.

## 5.4 Spatial grid and quick reject

Uniform grid over the solids, cell `25000` cm, key `(Cx << 32) ^ Cy`; built lazily and rebuilt when the solid count changed (`GridCount != Obbs.Num()`) `if (GridCount != Obbs.Num()) BuildGrid();` [src Source/IronwakeTest/IronwakeCollision.h:114]. Each solid is inserted into every cell its bounding circle (`C +- R`) touches `FMath::FloorToInt((O.C.X - O.R) / CellCm)` [src Source/IronwakeTest/IronwakeCollision.cpp:240]. `ForEachNear(P, Rad, Fn)` visits each solid once per query using a stamp array.

In `TestHull` a solid is skipped unless `dist(Centre, O.C) <= O.R + RadiusCm` `Dx * Dx + Dy * Dy > FMath::Square(O.R + RadiusCm)` [src Source/IronwakeTest/IronwakeCollision.cpp:260] and unless its z range overlaps the hull's wet band.

## 5.5 z-range versus draught

Static solid kept only if NOT (`ZMax < Sea - Draught` or `ZMin > Sea + 1500`): i.e. solids entirely below the keel are ignored, and solids that start more than `15` m above the sea (aerial structures: masts, bridge decks) are ignored `O.ZMax < Sea - DraughtCm` [src Source/IronwakeTest/IronwakeCollision.cpp:261]. `Sea` = `SeaZ()` = `WaterZAt(world, origin)` cached once `CachedSea = AIronwakeShell::WaterZAt` [src Source/IronwakeTest/IronwakeCollision.cpp:194]. The same filter is in `PathClear` (5.12).

Consequence for each hull: a rock whose top is below `Sea - Draught` is not a collision, i.e. the cutter (`200` cm) clears rocks that stop `2` m under the sea, the ironclad (`700` cm) does not clear anything shallower than `7` m below the sea reference (derived by subtraction). The wave height is not included: `Sea` is the still plane at the origin.

## 5.6 Seabed depth (islands as heightfields)

`SeabedAt(P, OutH)`: for each island with `bTerrain`, skip if `dist^2 > (R * 2.2)^2`; compute `Rs = RTheta(theta)`, `Rho = dist / Rs`; skip if `Rho >= 1.25` (beyond the shelf the sea is open and deep); `H = Height(dx, dy)`; the maximum `H` over islands wins `Rho >= 1.25f` [src Source/IronwakeTest/IronwakeCollision.cpp:218] `I.R * 2.2f` [src Source/IronwakeTest/IronwakeCollision.cpp:214]. `DepthAt(P) = -H`, or `1e7` (open sea) when no island covers `P`; negative on land `return 1e7f;` [src Source/IronwakeTest/IronwakeCollision.cpp:229].

`FIsle::Height(X, Y)` (local cm over the sea reference): `Under = -900` (seabed shelf under the shore); `Rho >= 1.25` returns `-900`; `1.0 <= Rho < 1.25` is `lerp(0, -900, smoothstep(1.0, 1.25, Rho))`; inside the shore a beach band `220 * Smoothx(1.0, 0.80, Rho)` (rises to ~2.2 m) and a body `Hp * smoothx` ridge (or plateau) plus hummocks; result `max(Beach, Body + N + 0.4 Beach)` `const float Under = -900.f` [src Source/IronwakeTest/IronwakeArchipelago.cpp:99] `220.f * Smoothx(1.f, 0.80f, Rho)` [src Source/IronwakeTest/IronwakeArchipelago.cpp:103]. So the water depth in the shelf ring grows from `0` at the shoreline to just under `900` cm at `Rho = 1.25`.

Derived (arithmetic on the formula above; not a source constant): inverting the smoothstep, depth `200`, `500`, `700` cm occur at `Rho` about `1.076`, `1.134`, `1.174`. Hence a cutter grounds inside about `Rho 1.08` of the local shoreline radius, a frigate inside `1.13`, the ironclad inside `1.17`; and the ironclad's "shoal" band (depth < 14 m) covers the whole shelf ring because the maximum shelf depth is under 9 m.

Peak heights per island kind (cm): Wooded `48 m`, Pines `12`, Cay `7` (and `R *= 0.95`), Fort `14` (plateau), Lighthouse `9`, Cliff `8`, Rocky `5` (and `R *= 1.1`) `case EIsle::Wooded: I.Hp = 48.f * CM` [src Source/IronwakeTest/IronwakeArchipelago.cpp:245] `case EIsle::Rocky: I.Hp = 5.f * CM; I.R *= 1.1f` [src Source/IronwakeTest/IronwakeArchipelago.cpp:251]. The 14 islands with positions are in chapter 6.

The hull's keel depth test samples `DepthAt` at every point of `CollKeel` (rotated to the current yaw) and takes the minimum `MinD = FMath::Min(MinD, Co->DepthAt(K))` [src Source/IronwakeTest/IronwakeShipCollision.cpp:102].

## 5.7 Per-tick algorithm (TickCollision)

Called from `ApplyShipForces` with the planned world velocity and yaw rate `TickCollision(Dt, PlanVel, YR)` [src Source/IronwakeTest/IronwakeShipPawn.cpp:744]. Early return when `-iwnocoll`, hull not registered, or sinking `bSinking` [src Source/IronwakeTest/IronwakeShipCollision.cpp:41].

**Prediction (2 ticks).** `Look = max(Dt, 0.02) * 2`; predicted position `P + V*Look`, predicted yaw `Yaw + YawRate*Look` `FMath::Max(Dt, 0.02f) * 2.f` [src Source/IronwakeTest/IronwakeShipCollision.cpp:48]. The hit used is the deeper of `Now` (current pose) and `Next` (predicted pose) by penetration `Next.Pen > Now.Pen ? Next : Now` [src Source/IronwakeTest/IronwakeShipCollision.cpp:58]. Note `Dt` is already capped at `0.1` s in `Tick`, and physics runs sub-stepped at 1/60 (chapter 2), so the look-ahead is at most `0.2` s and `0.04` s at 60 fps (derived).

**Outline test (TestHull)** returns the deepest penetration found in the three checks below:

1. *Hull outline points inside a solid*: each `Dense` point (world) is tested in the solid's local frame. Ellipse: `Q = (Lx/Hx)^2 + (Ly/Hy)^2`, hit if `Q < 1`; `Pen = |(Lx,Ly)| * (1/sqrt(Q) - 1)`; normal = gradient `(Lx/Hx^2, Ly/Hy^2)` normalised, rotated to world. Box: hit if `|Lx| < Hx` and `|Ly| < Hy`; `Pen = min(Hx - |Lx|, Hy - |Ly|)` along the axis of smaller penetration, normal `U * sign(Lx)` or `V * sign(Ly)` `FMath::Square(Lx / O.Half.X) + FMath::Square(Ly / O.Half.Y)` [src Source/IronwakeTest/IronwakeCollision.cpp:270] `const float Px = O.Half.X - FMath::Abs(Lx)` [src Source/IronwakeTest/IronwakeCollision.cpp:281].
2. *The solid's own sample points inside the hull polygon* (poles and corners poking through the hull side): the centre plus `8` ellipse points (every 45 deg) or the `8` surrounding box corners/edge midpoints; `Pen = DistToPoly + 30` cm, normal = from sample toward hull centre `Pen = DistToPoly(Samp[k], PolyW) + 30.f` [src Source/IronwakeTest/IronwakeCollision.cpp:294].
3. *Other hulls* (when mask bits `MaskPawns`/`MaskOthers` set): quick reject by `dist^2 > (R_other + RadiusCm)^2`; own dense points inside the other polygon and the other's dense points inside own polygon, `Pen = DistToPoly + 20` cm, normal = centre-to-centre `DistToPoly(P, PW) + 20.f` [src Source/IronwakeTest/IronwakeCollision.cpp:314]. `Self` is skipped.

`Consider` keeps only the largest `Pen`.

## 5.8 Impact response (hard solids and hulls)

If `H.bHit`: normal `N = H.N` (or `-V` direction if zero); `Vn = V . N` (negative = into the obstacle) `const float Vn = FVector2D::DotProduct(V2, N)` [src Source/IronwakeTest/IronwakeShipCollision.cpp:64]. When `Vn < 0`:

- `Impact = -Vn` (cm/s).
- New velocity `V2 - N * Vn * (1 + 0.10)` ("slide + a little bounce") `(1.f + 0.10f)` [src Source/IronwakeTest/IronwakeShipCollision.cpp:69].
- Speed loss: `NewV *= (1 - clamp(Impact / 1800, 0, 0.5))` ("loss grows with the impact speed"; at most `50` %) `Impact / 1800.f, 0.f, 0.5f` [src Source/IronwakeTest/IronwakeShipCollision.cpp:70].
- A "recorded" impact needs `Impact > 120` cm/s and at least `0.6` s since the last (`LastImpactWall` in wall-clock seconds) `Impact > 120.f && Tn - LastImpactWall > 0.6` [src Source/IronwakeTest/IronwakeShipCollision.cpp:71]. On a recorded impact: `CollisionHits++`, `LastImpactCmS`, `LastHitTag`; camera shake; pitch/roll kick; for the player a toast "COLLISION: <tag> (<kn>)" for `2.5` s with `kn = Impact / 51.4444 / max(SpeedCompression, 1)` `COLLISION: %s (%.1f kn)` [src Source/IronwakeTest/IronwakeShipCollision.cpp:79]; creak SFX `creak_storm_strain` at volume `clamp(Impact/600, 0.3, 1.4)` pitch `0.55` (a reused sample: "TODO: dedicated scrape/impact SFX") `creak_storm_strain` [src Source/IronwakeTest/IronwakeShipCollision.cpp:81]; if the other hull is a merchant or NPC craft its `SlowUntil = now + 4`.
- The new velocity is written to the plan.

Regardless of `Vn`, if overlapping now (`Now.bHit && Now.Pen > 0`): push-out velocity added `= Now.N * min(Pen * 3, 350)` cm/s `FMath::Min(Now.Pen * 3.f, 350.f)` [src Source/IronwakeTest/IronwakeShipCollision.cpp:91]. Then the pose a further `Look` ahead with the corrected velocity is re-tested: if it still hits with `Pen > 5` cm the yaw rate is damped `*= exp(-8 Dt)` ("no turning into the obstacle") `YawTest.bHit && YawTest.Pen > 5.f` [src Source/IronwakeTest/IronwakeShipCollision.cpp:95].

In knots, speed loss at a given impact (derived from the formula, nominal knots = cm/s / 51.4444 / 1.5 compression): impact `1800` cm/s (35 sim-kn, 23.3 nominal kn) loses `50` %; `900` cm/s loses `25` %; `120` cm/s (the recording threshold) loses `3.3` %.

### 5.8.1 Camera shake and kick

On a recorded wall impact: `ShakeAmpCm = max(ShakeAmpCm, clamp(Impact * 0.12, 8, 160) * clamp(HullLengthM/100, 0.5, 1))` and `ShakeT = 0` `FMath::Clamp(Impact * 0.12f, 8.f, 160.f)` [src Source/IronwakeTest/IronwakeShipCollision.cpp:75]. `PitchRate += +-clamp(Impact*0.004, 0.3, 4)` deg/s and `RollRate += +-clamp(Impact*0.006, 0.3, 6)` deg/s (random sign) `FMath::Clamp(Impact * 0.004f, 0.3f, 4.f)` [src Source/IronwakeTest/IronwakeShipCollision.cpp:77] `FMath::Clamp(Impact * 0.006f, 0.3f, 6.f)` [src Source/IronwakeTest/IronwakeShipCollision.cpp:78].

Applied in `CalcCamera`: while `ShakeAmpCm > 1`: `ShakeT += Dt`; `ShakeAmpCm *= exp(-3.2 Dt)`; camera location offset `(sin(41 t), cos(53 t), 0.6 sin(67 t)) * ShakeAmpCm` cm `FMath::Exp(-3.2f * Dt)` [src Source/IronwakeTest/IronwakeShipPawn.cpp:1033] `FMath::Sin(ShakeT * 41.f)` [src Source/IronwakeTest/IronwakeShipPawn.cpp:1034]. The shake applies to the camera of any pawn that has `ShakeAmpCm` set, so enemy impacts just set fields on the NPC (its camera is inactive).

## 5.9 Grounding and shoals

After the solids step the seabed step runs with `MinD` = minimum keel depth (5.6) `MinDepthCm = MinD` [src Source/IronwakeTest/IronwakeShipCollision.cpp:104]. The look-ahead depth `MinNext` is evaluated only when `MinD < 2.2 * Draught`, at `P + V * (Look * 2)` i.e. four ticks ahead `MinD < DraughtCm * 2.2f` [src Source/IronwakeTest/IronwakeShipCollision.cpp:106].

| State | Condition | Source |
|---|---|---|
| `bAground` | `MinD < DraughtCm` | `bAground = MinD < DraughtCm;` [src Source/IronwakeTest/IronwakeShipCollision.cpp:113] |
| `bShoal` | not aground and `MinD < 2 * DraughtCm` | `MinD < DraughtCm * 2.f` [src Source/IronwakeTest/IronwakeShipCollision.cpp:114] |
| `bToShallower` | `MinNext < MinD - 0.5` or (aground and `Speed > 20` and `MinNext <= MinD + 0.5`) | `MinNext < MinD - 0.5f` [src Source/IronwakeTest/IronwakeShipCollision.cpp:116] |

Thresholds per hull (derived by multiplying the draughts; the thresholds themselves are expressions of `DraughtCm`): 

| Hull | Draught (cm) | Aground when depth below (cm) | Shoal when depth below (cm) | Look-ahead armed below (cm) |
|---|---|---|---|---|
| Cutter | `200` | 200 | 400 | 440 |
| Frigate | `500` | 500 | 1000 | 1100 |
| Ironclad (player) | `700` | 700 | 1400 | 1540 |
| Enemy Kracken | `600` | 600 | 1200 | 1320 |

First grounding (transition `!bWasAground -> bAground`): `GroundingCount++`; shake `max(ShakeAmpCm, clamp(Speed * 0.10, 6, 120) * clamp(HullLengthM/100, 0.5, 1))`; `PitchRate += clamp(Speed*0.003, 0.2, 3)`; `RollRate += clamp(Speed*0.004, 0.2, 4)` random sign; creak SFX at volume `clamp(Speed/500, 0.3, 1.4)` pitch `0.5`; log "IWP9 AGROUND ..." `FMath::Clamp(Speed * 0.10f, 6.f, 120.f)` [src Source/IronwakeTest/IronwakeShipCollision.cpp:122] `FMath::Clamp(Speed * 0.003f, 0.2f, 3.f)` [src Source/IronwakeTest/IronwakeShipCollision.cpp:123].

While aground:
- If `bToShallower`: velocity `*= exp(-2.4 Dt)` ("~1.5 s to a standstill") and hard-zeroed below `40` cm/s `FMath::Exp(-2.4f * Dt)` [src Source/IronwakeTest/IronwakeShipCollision.cpp:129] `if (Speed < 40.f)` [src Source/IronwakeTest/IronwakeShipCollision.cpp:130].
- Yaw rate `*= exp(-7 Dt)` always ("rudder has no bite") `FMath::Exp(-7.f * Dt)` [src Source/IronwakeTest/IronwakeShipCollision.cpp:132].
- Toast "AGROUND - engines astern to back off", repeating every `1.2` s `Now2 - GroundMsgT > 1.2f` [src Source/IronwakeTest/IronwakeShipCollision.cpp:133]; scrape SFX `creak_wooden_hull` every `1.4` s when speed > `80` cm/s, volume `0.9`, pitch `0.7` `creak_wooden_hull` [src Source/IronwakeTest/IronwakeShipCollision.cpp:134].

In the shoal band while still closing on shallower water: `Frac = 1 - (MinD - Draught)/Draught` (0 at 2x draught, 1 at the draught), drag `K = 0.05 + 0.35 * Frac^2`, `V *= exp(-K Dt)`; toast "SHOAL WATER" every `2.5` s `0.05f + 0.35f * Frac * Frac` [src Source/IronwakeTest/IronwakeShipCollision.cpp:139] `SHOAL WATER` [src Source/IronwakeTest/IronwakeShipCollision.cpp:141]. Derived: at the draught `K = 0.40`/s, at 1.5 draughts `K = 0.1375`/s.

### 5.9.1 Astern release

There is no special "astern" code path. Because `bToShallower` becomes false when the bow is moving away from the shallows (`MinNext > MinD`), the exponential drag stops; only the yaw damping remains while `MinD < Draught`; the engine telegraph (astern) then accelerates the hull out, subject to the normal `1.3 x MaxDv` braking authority (chapter 2). The toast says "engines astern to back off". The P9 director's definition of "freed": not aground, `MinDepthCm > 1.5 * Draught`, travelled > `0.6 * hull length` from the contact point and `speed < -0.5` kn, with no hit (test criterion, not game logic) `P->MinDepthCm > P->DraughtCm * 1.5f` [src Source/IronwakeTest/IronwakeP9Test.cpp:167]. P9 log: "astern frees" PASS for all three hulls `astern frees` [src build/logs/P9.md:6].

## 5.10 Merchants and NPC craft (kinematic craft)

Merchants and P8 boats do not call `TickCollision`; each step they test the next pose: hit by any solid/hull (`TestPose`) or `MinDepthAt < DraughtCm` => hold position (merchants accumulate `BlockedS`; P8 craft restore their loop time `T = T0`) `Co->MinDepthAt(CollShape, PN, Yaw) < CollShape.DraughtCm` [src Source/IronwakeTest/IronwakeLivingSea.cpp:191] `T = T0; return;` [src Source/IronwakeTest/IronwakeP8.cpp:201]. When rammed by the player they crawl at `0.3` speed for `4` s. The route audit (`AuditPath`) runs once at start for every merchant and P8 boat loop and logs the first blocked segment (`IWP9 AUDIT ...`); the P9 log states all were clear `route audit logs at start (all clear)` [src build/logs/P9.md:7].

## 5.11 PathClear, AuditPath and AvoidLand

`PathClear(Start, YawDeg, LenCm, HalfBeamCm, DraughtCm)` returns the distance to the first problem along a heading, scanning every `1000` cm and at three lateral offsets (`-1, 0, +1` x `HalfBeamCm`):

- stop if `DepthAt(P) < DraughtCm * 1.5` (shoal threshold of `1.5` draughts, tighter than the `2.0` used by `bShoal`),
- stop if inside any solid (z filter as in 5.5) inflated by `Margin = 500` cm: ellipse `((Lx/(Hx+500))^2 + (Ly/(Hy+500))^2) < 1`, box `|Lx| < Hx + 500 and |Ly| < Hy + 500` `const float Margin = 500.f` [src Source/IronwakeTest/IronwakeCollision.cpp:327] `DepthAt(P) < DraughtCm * 1.5f` [src Source/IronwakeTest/IronwakeCollision.cpp:332].
Returns `LenCm` if clear.

`AuditPath(name, pts, bClosed, HalfBeam, Draught)` applies `PathClear` to every segment and logs the first blocked segment `segment %d blocked after %.0f m` [src Source/IronwakeTest/IronwakeCollision.cpp:364].

`AvoidLand(DesiredYaw, LookAheadCm = 150000)` (pawn): half-beam used `= CollShape.R * 0.18 + 600` cm; if the desired heading is clear for the look-ahead return it unchanged; else try offsets `+-15, +-30 ... +-120` degrees (alternating sides, nearest first) and return the first clear one; if none, `DesiredYaw + 90` `CollShape.R * 0.18f + 600.f` [src Source/IronwakeTest/IronwakeShipCollision.cpp:151] `Off <= 120.f` [src Source/IronwakeTest/IronwakeShipCollision.cpp:155] `DesiredYaw + 90.f` [src Source/IronwakeTest/IronwakeShipCollision.cpp:161]. Default look-ahead `150000` cm (1.5 km) `float LookAheadCm = 150000.f` [src Source/IronwakeTest/IronwakeShipPawn.h:543]. Callers:

| Caller | Look-ahead (cm) | Source |
|---|---|---|
| NPC ironclad AI | `120000` | `AvoidLand(AIDesiredYaw, 120000.f)` [src Source/IronwakeTest/IronwakeCombat.cpp:547] |
| Autopilot (`SteerAuto`) | `clamp(speedKn * 51.4444 * 1.5 * 40, 60000, 220000)` (about 40 s of travel) | `FMath::Clamp(P->GetSpeedKnots() * 51.4444f * 1.5f * 40.f, 60000.f, 220000.f)` [src Source/IronwakeTest/IronwakeAutopilot.cpp:404] |

`AvoidLand` returns the input unchanged when collision is disabled or the hull is unregistered `return DesiredYaw;` [src Source/IronwakeTest/IronwakeShipCollision.cpp:148]. The helmsman (J) does not call `AvoidLand` (it steers rudder steps toward the next point only) `Helmsman: steer for the next point of interest` [src Source/IronwakeTest/IronwakeVoyage.cpp:1038]; it relies on the collision response.

## 5.12 Hazard circles used by the autopilot planner

The autopilot has a separate, coarser avoidance layer: circular hazards (`FObs {Name, C, R}`) built in `BuildObstacles`; total `43` at runtime in the logs `IWA1 obstacles=43` [src build/review_opus/circle2_ironclad.log:363]. They are defined from fixed constants (radii in cm):

| Hazard | R (cm) | Source |
|---|---|---|
| St Brannoc island (centre `LivingSea::IslandCentre`, header default `(-50000, 190000)`) | `42000` | `St Brannoc island` [src Source/IronwakeTest/IronwakeAutopilot.cpp:67] `FVector IslandCentre = FVector(-50000.f, 190000.f, 0.f)` [src Source/IronwakeTest/IronwakeLivingSea.h:121] |
| The Needles (`StacksC` = `(110000, -90000)`) | `36000` | `The Needles` [src Source/IronwakeTest/IronwakeAutopilot.cpp:68] |
| Wreck of the Valiant (`WreckC` = `(-92000, -150000)`) | `17000` | `Wreck of the Valiant` [src Source/IronwakeTest/IronwakeAutopilot.cpp:69] |
| Greyhaven south breakwater | 300 m wall, circles every `40` m, `R 4500` each, centre `(60, -190)` yaw `12` (harbour frame, m) | `Greyhaven south breakwater` [src Source/IronwakeTest/IronwakeAutopilot.cpp:79] |
| Greyhaven north breakwater | 200 m wall, `R 4500`, centre `(10, 200)` yaw `-18` | `Greyhaven north breakwater` [src Source/IronwakeTest/IronwakeAutopilot.cpp:80] |
| Greyhaven quay | three circles `R 17000` at harbour-frame `(-170, -200)`, `(-170, 0)`, `(-170, 200)` m | `Greyhaven quay` [src Source/IronwakeTest/IronwakeAutopilot.cpp:81] |
| Greyhaven pier | `R 9000` at `(30, 10)` m | `Greyhaven pier` [src Source/IronwakeTest/IronwakeAutopilot.cpp:84] |
| 8 channel buoys (4 pairs) | `3000` each | `TEXT("channel buoy"), P, 3000.f` [src Source/IronwakeTest/IronwakeAutopilot.cpp:85] |
| Archipelago islands (14) | each island's `HazardR` | `I.HazardR` [src Source/IronwakeTest/IronwakeAutopilot.cpp:95] |

The harbour frame is yawed `45` deg with +X seaward `FRotator(0.f, 45.f, 0.f)` [src Source/IronwakeTest/IronwakeAutopilot.cpp:71]. Harbour centre `HarbourC = (-345000, -335000)`, `WreckC`, `StacksC` constants `FVector HarbourC = FVector(-345000.f, -335000.f, 0.f)` [src Source/IronwakeTest/IronwakeVoyage.h:189] `FVector WreckC = FVector(-92000.f, -150000.f, 0.f)` [src Source/IronwakeTest/IronwakeVoyage.h:190] `FVector StacksC = FVector(110000.f, -90000.f, 0.f)` [src Source/IronwakeTest/IronwakeVoyage.h:191].

Island `HazardR` (cm): default `max(R * 1.35 + 6000, MaxRs * 1.25 + 15000)` where `MaxRs` is the maximum shoreline radius over 72 bearings (P9 "the autopilot circle must clear the whole shelf a 7 m draught can ground on (rho ~1.12 of the shoreline at its widest bearing)") `I.HazardR = FMath::Max(I.R * 1.35f + 6000.f, MaxRs * 1.25f + 15000.f)` [src Source/IronwakeTest/IronwakeArchipelago.cpp:277]; overrides inside the per-kind builders: Rocky `R*1.5 + 6000` `I.HazardR = I.R * 1.5f + 6000.f` [src Source/IronwakeTest/IronwakeArchipelago.cpp:379], Reef `R*1.4 + 5000` `I.HazardR = I.R * 1.4f + 5000.f` [src Source/IronwakeTest/IronwakeArchipelago.cpp:421], Graveyard `R*1.3 + 6000` `I.HazardR = I.R * 1.3f + 6000.f` [src Source/IronwakeTest/IronwakeArchipelago.cpp:866]. (I1 log wording "1.35 x shoreline radius + 60 m" matches the default's first term.) `hazard circle = 1.35 x shoreline radius + 60 m` [src build/logs/I1.md:8] Logged results per island (m): Kestrel `762`, Fort Grimsby `370`, Morrow Head `411`, Gannet Stacks `330`, Sable Cay `329`, The Teeth `414`, Drowned Men's Shoal `424`, Fir Holm `330`, Seal Rocks `258`, Little Sable `214`, Heron Holm `262`, The Anvil `291`, Blackscar `303`, The Sisters `262` `hazard=762 m` [src build/review_opus/circle2_ironclad.log:334] `hazard=214 m` [src build/review_opus/circle2_ironclad.log:353].

Planner (`PlanLeg`): finds the first hazard circle the straight segment crosses (both ends outside `1.02 R`), builds a detour of tangent points plus arc at radius `1.15 R` on the side the segment already passes, recursion depth below `7` `O.R * 1.15f` [src Source/IronwakeTest/IronwakeAutopilot.cpp:138] `Depth < 7` [src Source/IronwakeTest/IronwakeAutopilot.cpp:125]. Look-ahead safety layer in `TickAutopilot`: ray `Look = 60000 + V*20` cm; inside a circle (`D < 0.98 R`): head straight out at SLOW; ray crossing a circle: steer to the nearer tangent bearing `Bc +- asin(1.15 R / D)` `60000.f + VCm * 20.f` [src Source/IronwakeTest/IronwakeAutopilot.cpp:631] `O.R * 0.98f` [src Source/IronwakeTest/IronwakeAutopilot.cpp:639]. The planner circles are independent of the solid registry; `AvoidLand` is the second safety net (5.11). Details of the autopilot are chapter 6.

## 5.13 Old POI guard ("HardRadiusCm")

`FIronwakePOI::HardRadiusCm` ("rocks/pier: the ship is stopped here (no hull collision with world statics)") is still set when POIs are added (wreck `9000`, Needles `26000`) but is only read by the S2 test director; the guard it used to drive was removed in P9 ("the old POI grounding guard is gone (real collision, IronwakeShipCollision.cpp)") `P9: the old POI grounding guard is gone` [src Source/IronwakeTest/IronwakeVoyage.cpp:1071] `Add(TEXT("Wreck of the Valiant"), TEXT("W"), WreckC, 60000.f, 9000.f)` [src Source/IronwakeTest/IronwakeVoyage.cpp:274]. The S1 log's guard text ("Guards stop the ship short of the stacks and the wreck hull") is therefore historical `Guards stop the ship short of the stacks` [src build/logs/S1.md:15].

## 5.14 Debug switches and test director

- `-iwnocoll`: sets `bNoCollision` at hull registration; `TickCollision` and `AvoidLand` become no-ops ("debug / old tours") `-iwnocoll: debug / old tours` [src Source/IronwakeTest/IronwakeShipPawn.h:534] `TEXT("iwnocoll")` [src Source/IronwakeTest/IronwakeShipCollision.cpp:20]. Merchants and P8 boats are NOT affected by this flag (they test the registry directly).
- `-iwp9=all|beach|stack|breakwater|moored|ship|autopilot [-iwp9quit]`, run with `build/tools/p9run.sh <hull> <mode>` (IronwakeP9Test.cpp; modes from the P9 log) `-iwp9=all` [src build/logs/P9.md:8]. Log keys: `IWP9 IMPACT`, `IWP9 AGROUND`, `IWP9 AUDIT`, `IWP9 FINAL`.
- Pawn test fields: `CollisionHits`, `LastImpactCmS`, `LastHitTag`, `MinDepthCm`, `GroundingCount`, `bAground`, `bShoal` `int32 CollisionHits = 0;` [src Source/IronwakeTest/IronwakeShipPawn.h:531].

## 5.15 Measured results (P9, flank into obstacle then astern)

| Hull | beach | rock stack | breakwater | pier + moored | ship vs ship | astern frees | autopilot round island |
|---|---|---|---|---|---|---|---|
| ironclad | PASS (aground 7.0 m < 7.0) | PASS* | PASS | PASS (hit pier first) | PASS | all PASS | PASS `137` s, 0 groundings |
| cutter | PASS | PASS | PASS | PASS | PASS | all PASS | PASS `117` s |
| frigate | PASS (aground 5.0) | PASS | PASS | PASS | PASS | all PASS | PASS `198` s |

`PASS 137 s, 0 groundings` [src build/logs/P9.md:13] `PASS 117 s` [src build/logs/P9.md:14] `PASS 198 s` [src build/logs/P9.md:15]. *Stack: first capture showed the ironclad bow inside the stack; fixed by the tall-rock full box + `3.5` m margin, bow flare in the outline and ellipse x1.18 (5.3.1, 5.3.3). Recaptured for the ironclad only; cutter/frigate stack frames predate the fix and were not re-shot `Cutter/frigate stack frames were taken BEFORE this fix` [src build/logs/P9.md:17]. Open-water circle unchanged at `208.8` s with collision on `208.8 s at 30 fps and at 18 fps` [src build/logs/P9.md:18]. Bench: P8 `33.0` / `33.3` vs P9 `33.6` / `33.4` median fps `P8 33.0 / 33.3, P9 33.6 / 33.4` [src build/logs/P9.md:19].

## 5.16 Known limitations (from the P9 log and code)

- Collision is outline-vs-bounds (ellipses/boxes), so concave rocks can leave visible gaps of a few metres `concave rocks can leave visible gaps of a few metres` [src build/logs/P9.md:25]. Inflation (x1.18, +3.5 m) trades gaps for tunnelling-free contact.
- The impact sound reuses a creak sample; no dedicated scrape/impact SFX exists `Impact sound is a reused creak` [src build/logs/P9.md:23].
- The "AGROUND" toast is drawn by the HUD, so photo-mode captures do not show it `the toast is drawn by the HUD in game, not in photo-mode shots` [src build/logs/P9.md:22].
- Vision-model judgement of side views is unreliable for "inside the rock"; overhead frames and `pen = 0` logs were used instead.
- Shell-versus-hull hits are a different system (projectiles test only `AIronwakeShipPawn` hulls; merchants "Not a combat target") `shells only test AIronwakeShipPawn hulls` [src Source/IronwakeTest/IronwakeLivingSea.h:20].

## 5.17 Gaps / unknown

- Total solid count in the current build and per-source breakdown: only `1108` (P9) is logged. Measure: grep the "IWP9 collision baked" line in a fresh run.
- Exact ZMin/ZMax of individual solids (derived from mesh bounds at runtime): not in source. Measure by logging `Obbs[i]` after `Bake()`.
- Whether the harbour town buildings (P12, `Place(... true)` with default `bSolid=true`) all exceed the `250` cm half-extent minimum (small props below it are silently dropped): measure by comparing the count of `Place` calls with the baked total.
- The sea reference `SeaZ()` is evaluated once at the origin; if the water plane is not flat at other places the z-range test is approximate (the I1 log notes `SeaOffCm`). Measure: compare `WaterZAt` at island centres.
- Real-time response of `ShakeAmpCm` in packaged builds (frame-rate independence of the `FRand` kicks): not measured.
- Collision with airships: none (airships have no body); player airship overflight is allowed by design (z-range only matters for solids).
- Whether `HardRadiusCm` has any remaining runtime effect: only found in the S2 test director.
