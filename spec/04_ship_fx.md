# Chapter 4: Ship water FX

Scope: everything the sea does visually around a ship and in the storm: the Kelvin wake trail (`UIronwakeWake`), the procedural bow-wave / hull-foam / curling-lip / green-water ribbons (`UIronwakeBowWave`, which also carries the "bow cushion" stem mound), the Niagara spray systems (bow sheets, slam spray, spume, rain, splash), funnel smoke per hull, the legacy flat wake planes that are now hidden, and the materials that drive them. For each system this chapter gives mesh topology, widths, opacity, speed gates, per-sea-state rates and the call that spawns it. Sea tables are in chapter 3, GPU/CPU cost in chapter 11, ship water audio loops are in chapter 10. Values come from `IronwakeHydro.cpp` (wake, bow wave), `IronwakeShipHydro.cpp` (spray / slam / smoke scale), `IronwakeStorm.cpp` (spume, rain), `IronwakeShipFleet.cpp` / `IronwakeFleet.cpp` (smoke stacks) and the Python scripts that last wrote the Niagara and material assets. Niagara and material assets are binary `.uasset`; the last values written by script are recorded, and anything an editor session might have changed afterwards is listed under Gaps.

## 4.1 Common inputs

| Input | Definition | Source |
|---|---|---|
| Nominal knots `Kn` [src Source/IronwakeTest/IronwakeShipHydro.cpp:157] | `max(GetSpeedKnots(), 0)`; sim speed / `SpeedCompression` | [src Source/IronwakeTest/IronwakeShipHydro.cpp:157] |
| Speed fraction `Frac` [src Source/IronwakeTest/IronwakeShipHydro.cpp:158] (bow FX, wake) | `Kn / 13` [src Source/IronwakeTest/IronwakeShipHydro.cpp:158], clamped to `1.2` [src Source/IronwakeTest/IronwakeShipHydro.cpp:158]; FULL AHEAD = 13 kn = 1.0 | [src Source/IronwakeTest/IronwakeShipHydro.cpp:158] |
| `SpeedCompression` [src Source/IronwakeTest/IronwakeShipPawn.h:95] | `1.5` [src Source/IronwakeTest/IronwakeShipPawn.h:95] (sim m/s = nominal kn x 0.5144 x 1.5) | [src Source/IronwakeTest/IronwakeShipPawn.h:95] |
| Compressed speed passed to bow wave | `Kn * KnCmS * SpeedCompression / 100` [src Source/IronwakeTest/IronwakeShipHydro.cpp:160] (m/s), `KnCmS = 51.4444` | [src Source/IronwakeTest/IronwakeShipHydro.cpp:160] |
| Compressed speed passed to wake | `Kn * 51.4444 * SpeedCompression / 100` (m/s) | [src Source/IronwakeTest/IronwakeShipPawn.cpp:858] |
| Hull profile | `HullProf` [src Source/IronwakeTest/IronwakeHydro.h:23-24] (chapter 3, 3.10.5): stem `X1` [src Source/IronwakeTest/IronwakeHydro.h:23-24], `HalfBreadthAt(x)`, length `Length()` [src Source/IronwakeTest/IronwakeHydro.h:23-24] | [src Source/IronwakeTest/IronwakeHydro.h:23-24] |
| Sea state | `IronwakeSea::GetSeaState()` 0..4 | [src Source/IronwakeTest/IronwakeHydro.h:47] |
| Ribbon scale `Scale` (bow wave) | `HullLengthM / 125` [src Source/IronwakeTest/IronwakeShipPawn.cpp:240] (125 m ironclad = 1.0) | [src Source/IronwakeTest/IronwakeShipPawn.cpp:240] |
| Hull length scale `Hs` [src Source/IronwakeTest/IronwakeShipHydro.cpp:185] (spray) | `HullLengthM / 70` [src Source/IronwakeTest/IronwakeShipHydro.cpp:185] | [src Source/IronwakeTest/IronwakeShipHydro.cpp:185] |

- Components: every `AIronwakeShipPawn` (player and NPC) creates one `UIronwakeBowWave` and one `UIronwakeWake` in `BeginPlay`, attached to `HullBody` [src Source/IronwakeTest/IronwakeShipPawn.cpp:237-246]. Wake `Init(HullLengthM, MaxB, !HasSails())` where `MaxB` is the profile's maximum half-breadth; the third argument marks a steamer [src Source/IronwakeTest/IronwakeShipPawn.cpp:246].
- Per-tick order in `Tick`: `TickWake`, `TickHullCosmetics`, then (unless sinking) `TickBowFX`, `TickWaterAudio` [src Source/IronwakeTest/IronwakeShipPawn.cpp:770-775].
- Both components use absolute location/rotation/scale, no collision, no shadow, no overlap events, async cooking [src Source/IronwakeTest/IronwakeHydro.cpp:338-345] [src Source/IronwakeTest/IronwakeHydro.cpp:531-534]. Translucency sort priority: bow wave `12` [src Source/IronwakeTest/IronwakeHydro.cpp:361], wake `11` [src Source/IronwakeTest/IronwakeHydro.cpp:551].
- Both meshes are built once with fixed topology (`CreateMeshSection_LinearColor`), then only vertices/colours are updated each tick via `UpdateMeshSection_LinearColor` [src Source/IronwakeTest/IronwakeHydro.cpp:540] [src Source/IronwakeTest/IronwakeHydro.cpp:642-643].
- Command-line kill switches: `-iwnobow`, -iwnosheet [src Source/IronwakeTest/IronwakeShipHydro.cpp:159]; -iwnowake [src Source/IronwakeTest/IronwakeShipPawn.cpp:857]; -iwnospume [src Source/IronwakeTest/IronwakeStorm.cpp:197]; debug logs `-iwwakedbg`, `-iwsmokedbg`, -iwcount [src Source/IronwakeTest/IronwakeHydro.cpp:644] [src Source/IronwakeTest/IronwakeShipHydro.cpp:166] [src Source/IronwakeTest/IronwakeShipHydro.cpp:206].

## 4.2 Kelvin wake trail (`UIronwakeWake`)

Three ribbons per ship: ribbon 0 = turbulent centre wake (prop boil on steamers), ribbons 1 and 2 = the two diverging Kelvin arms [src Source/IronwakeTest/IronwakeHydro.cpp:588]. Each ribbon is `WakeN` history rows by `WakeAcross` verts.

### 4.2.1 Constants and topology

| Item | Value | Source |
|---|---|---|
| History rows `WakeN` [src Source/IronwakeTest/IronwakeHydro.cpp:545] | `160` [src Source/IronwakeTest/IronwakeHydro.cpp:545] | [src Source/IronwakeTest/IronwakeHydro.cpp:545] |
| Base sample step `WakeStep` [src Source/IronwakeTest/IronwakeHydro.cpp:545] | `0.5` [src Source/IronwakeTest/IronwakeHydro.cpp:545] s | [src Source/IronwakeTest/IronwakeHydro.cpp:545] |
| Verts across `WakeAcross` [src Source/IronwakeTest/IronwakeHydro.cpp:545] | `6` [src Source/IronwakeTest/IronwakeHydro.cpp:545] | [src Source/IronwakeTest/IronwakeHydro.cpp:545] |
| Ribbons | `3` [src Source/IronwakeTest/IronwakeHydro.cpp:608] | [src Source/IronwakeTest/IronwakeHydro.cpp:608] |
| Kelvin half-angle | `19.47` [src Source/IronwakeTest/IronwakeShipWave.cpp:76] deg (`Kel = tan(19.47 deg)`) | [src Source/IronwakeTest/IronwakeShipWave.cpp:76] |
| Vertices (derived) | 3 x 160 x 6 = 2880 | derived |
| Triangles (derived) | 3 x 159 x 5 x 2 = 4770 | derived |
| Lift above sea | `6.f` [src Source/IronwakeTest/IronwakeHydro.cpp:660] cm (arg to `Emit` [src Source/IronwakeTest/IronwakeHydro.cpp:660]; was `70` cm pre-P27) | [src Source/IronwakeTest/IronwakeHydro.cpp:660] |
| UV | ribbon-local `(WkT[j], sL)` (`Emit(P, FVector2D(WkT[j], sL)` [src Source/IronwakeTest/IronwakeHydro.cpp:660]) | [src Source/IronwakeTest/IronwakeHydro.cpp:660] |
| Normals / tangent | up / `(1,0,0)` | [src Source/IronwakeTest/IronwakeHydro.cpp:586] |
| Across-profile T | `0, 0.2, 0.45, 0.65, 0.85, 1` | [src Source/IronwakeTest/IronwakeHydro.cpp:577] |
| Across-profile alpha | `0, 0.45, 1, 0.8, 0.3, 0` (soft both edges) | [src Source/IronwakeTest/IronwakeHydro.cpp:578] |

- Nominal time span of the history: 160 rows x 0.5 s = 80 s (derived); the header comment says 60 s [src Source/IronwakeTest/IronwakeHydro.h:94-95] and W3 says "60 s" [src build/logs/W3.md:34]. **CONFLICT**: 80 s of rows exist, but fade-out `Life` caps the visible life at 45 s (centre) / 35 s (arms) (see below), so nothing is visible past ~45 s; the 60 s is a comment value. The row cap 160 is what takes effect [src Source/IronwakeTest/IronwakeHydro.cpp:567]. At turns the step shrinks, so 160 rows can cover less time.
- Vertex colour: `R` = foam density (material input), `A` = alpha (clamped 0..1) [src Source/IronwakeTest/IronwakeHydro.cpp:584].

### 4.2.2 History sampling

- `Age += Dt; Acc += Dt` each tick [src Source/IronwakeTest/IronwakeHydro.cpp:554]. Each sample stores `{Stern xy, Bow xy, Fwd, T, V, F}` (stern/bow positions at hull-local X offsets `SternXM`, `BowXM` along the heading) [src Source/IronwakeTest/IronwakeHydro.h:107].
- Called with `SternXM = HullProf.X0 + 2` and `BowXM = HullProf.X1 - 2` m [src Source/IronwakeTest/IronwakeShipPawn.cpp:858].
- Adaptive step: `StepNow = clamp(WakeStep - TurnDeg * 0.12, 0.12, WakeStep)`, where `TurnDeg` is the angle between current heading and the newest sample's heading (0.5 s straight, 0.12 s at >= ~3.2 deg step) [src Source/IronwakeTest/IronwakeHydro.cpp:580]. A new sample is inserted at index 0 when `Acc >= StepNow` (or history empty); list capped at `WakeN` [src Source/IronwakeTest/IronwakeHydro.cpp:585].
- The live head sample `Hist[0]` is overwritten every frame with the current hull state [src Source/IronwakeTest/IronwakeHydro.cpp:570].
- Teleport guard: if the stern moved farther than `max(6000, SpeedMS*100*1.5 + 3000)` cm from the last sample, history is cleared (no straight band across a respawn) [src Source/IronwakeTest/IronwakeHydro.cpp:559].
- Rows beyond the stored history (`i >= Hist.Num()`) duplicate the oldest sample with alpha 0 [src Source/IronwakeTest/IronwakeHydro.cpp:595-596] [src Source/IronwakeTest/IronwakeHydro.cpp:630].
- The ribbon mesh component is moved to the hull location every tick; vertices are stored relative to it [src Source/IronwakeTest/IronwakeHydro.cpp:571-583].

### 4.2.3 Per-ribbon shape and opacity

Common: `A0 = Age - S.T` (seconds since the sample); `Rt` = right vector of the sample's heading; `SpeedOK = clamp(clamp(S.F, 0, 1.2)^1.5, 0, 1)`: the wake scales with speed in strength, width and lifetime, not on/off (Chuck 2026-11-10) [src Source/IronwakeTest/IronwakeHydro.cpp:599-600]. `SeaFoam = clamp(1 - SeaState*0.12, 0.5, 1)`: rough seas break the wake up sooner [src Source/IronwakeTest/IronwakeHydro.cpp:576].

| Quantity | Centre wake (ribbon 0) | Kelvin arms (ribbons 1, 2) |
|---|---|---|
| Centre line | `S.Stern` | `S.Bow + Rt * Side * max(Spread, HalfBeam*100)`; `Spread = V * A0 * tan(19.47) * 100` (V = compressed m/s of that sample) |
| Half width (cm) | `(HalfBeamM*(0.4 + 0.45*min(F,1)) + A0*0.45*min(F,1) + (steamer ? 1.5*F : 0)) * 100` | arm width `Wd = (2.0 + 0.1*V*A0) * 100 * LenK`, `LenK = clamp(LengthM/90, 0.4, 1.3)`; across offset `Off = max(Spread, HalfBeam*100) + Wd*(T_j - 0.15)` |
| Lifetime `Life` (s) | `Lerp(6, 45, min(F,1)) * SeaFoam` | `Lerp(4, 35, min(F,1)) * SeaFoam` |
| Peak alpha factor | `WkA[j] * 0.55` | `WkA[j] * 0.4` |
| Alpha | `Ac * SpeedOK * pow(clamp(1 - A0/Life, 0, 1), 1.4) * clamp(A0/0.6 + 0.3, 0, 1)` | `WkA[j] * 0.4 * SpeedOK * pow(clamp(1 - A0/Life, 0, 1), 1.3) * clamp(A0/2.5, 0, 1)` |
| Density `R` | `Lerp(steamer ? 0.8 : 0.7, 0.25, clamp(A0/Life, 0, 1))` | `Lerp(0.55, 0.15, clamp(A0/Life, 0, 1))` |

Sources (updated in P27 for LUT handoff): centre half width `(HalfBeamM * 0.34f + LwlM * 0.135f * FMath::Pow(sL, 0.42f))` [src Source/IronwakeTest/IronwakeHydro.cpp:637]; arm centre/spread/width [src Source/IronwakeTest/IronwakeHydro.cpp:648-652]; lifetimes `5.f` and `44.f` [src Source/IronwakeTest/IronwakeHydro.cpp:639], `4.f` and `35.f` [src Source/IronwakeTest/IronwakeHydro.cpp:653]; twin-screw profile `TwinA` (`0.90f` peak) [src Source/IronwakeTest/IronwakeHydro.cpp:640] and arm drive `0.42f` [src Source/IronwakeTest/IronwakeHydro.cpp:655]; alpha exponents `1.35f` [src Source/IronwakeTest/IronwakeHydro.cpp:644] and `1.3f` [src Source/IronwakeTest/IronwakeHydro.cpp:656]; straight-course LUT handoff `NearHandoff` [src Source/IronwakeTest/IronwakeHydro.cpp:642]. `Side` is `-1` for ribbon 1 and `+1` otherwise [src Source/IronwakeTest/IronwakeHydro.cpp:611]. `HalfBeamM` = the profile's max half-breadth passed at Init [src Source/IronwakeTest/IronwakeHydro.cpp:563].

Derived examples at FULL (F=1, SpeedOK=1), ironclad (HalfBeam 12.89 m [src Source/IronwakeTest/IronwakeHydro.cpp:23]): centre half width at the stern row `12.89*0.85 = 10.96` m, growing `0.45` m per second of age (hull widths +). Arm lifetime 35 s, centre 45 s. A ship at 13 kn compressed speed is 13 x 0.5144 x 1.5 = 10.03 m/s (derived), so arm spread at age 35 s = 10.03 x 35 x 0.3535 = 124 m (derived).

### 4.2.4 Vertical placement on the sea

- Every vertex: z = FastZ(P) + RowD + Lift(70 cm) [src Source/IronwakeTest/IronwakeHydro.cpp:600].
- `RowD` is a per-row correction `clamp(RenderedZ - FastZ, -150, 150)` at the row centre, refreshed only on rows where `i % 2 == 0 || i < 4` and the row has history; other rows reuse the last [src Source/IronwakeTest/IronwakeHydro.cpp:624-625]. This prevents rows from dipping under the translucent sea (W4 item 3) [src build/logs/W4.md:12].
- Cost (derived): per ship per frame, 3 ribbons x 82 query rows (80 even rows + 4 head rows, capped by history length) = about 246 `RenderedZ` plugin queries, plus about 246 + 2880 `FastZ` calls; each `FastZ` loops all 22 waves with a `cos` and a `sqrt` (derived from [src Source/IronwakeTest/IronwakeHydro.cpp:314-320]). See chapter 11.

### 4.2.5 Wake material (`M_IW_Surf2`)

- Asset `/Game/Ironwake/Islands/M_IW_Surf2` (shared with the archipelago surf ribbons), MID created per wake [src Source/IronwakeTest/IronwakeHydro.cpp:558-560]. Unlit, translucent, two-sided, no depth fade; opacity = `saturate((foamTex1 + foamTex2 + 1.2*VertexR - Thresh) * Contrast) * VertexA`; emissive = `FoamColor` [src Content/Python/iw_i1_shelf.py:57-69]. The two panning foam samples use `T_WaterFlow_03_Foam_Tiled`; second layer UV x `2.7` [src Content/Python/iw_i1_shelf.py:48].
- Runtime overrides at `Init`: `Thresh` `1.35`, `Contrast` `2.4`, `FoamColor` (0.85, 0.88, 0.9) [src Source/IronwakeTest/IronwakeHydro.cpp:561-563]. Asset defaults: `Thresh` `0.6`, `Contrast` `2.5`, `FoamColor` (0.55, 0.58, 0.60) [src Content/Python/iw_i1_shelf.py:60].
- **CONFLICT**: W4 log says "Surf2 Thresh 0.55 -> 0.32" for the wake [src build/logs/W4.md:12], but the code still sets `Thresh` to `1.35` per wake MID at Init [src Source/IronwakeTest/IronwakeHydro.cpp:561]. The runtime override (`1.35`) takes effect for ship wakes; any asset-default edit (0.32) only affects other users of the material (island surf ribbons). Whether the W4 `9.5/10` wake capture used the override cannot be determined from the files: unknown; re-measure with `-iwwakedbg` and a 200 m-up astern shot.
- Island surf also uses it with `SurfLiftCm = 70.f` [src Source/IronwakeTest/IronwakeArchipelago.h:72].

### 4.2.6 Tuning history of the wake (what changed, from logs)

| Pass | Change | Source |
|---|---|---|
| P5c | 6 verts across with feathered edges (was 4); centre peak x0.55, arms x0.4 (P5b ~1.0 / 0.75); arm width 2 m + 0.1*V*age (was 1.5 + 0.06*V*age); Thresh 1.35 / Contrast 2.4, UV tile 1100; history 160 samples (was 120); step 0.5 -> 0.12 s in turns; reset on teleport | [src build/logs/P5c.md:5] |
| W3 | Kelvin trail replaces the flat stern plane; wider/longer hull churn | [src build/logs/W3.md:34-35] |
| W4 | per-vertex `FastZ` [src build/logs/W4.md:12] + row query correction; density decays to 0.4 not 0.1; arm alpha up | [src build/logs/W4.md:12] |

(P27 update: straight-course wake `0..4.1 L` is rendered directly in `M_IW_WaterRoot` via `T_IW_PropWashFoamLut` + `T_IW_CellularFoamTile`, while `UIronwakeWake` uses lit `M_IW_WakeLut` [src Source/IronwakeTest/IronwakeHydro.cpp:560] (`NearHandoff` [src Source/IronwakeTest/IronwakeHydro.cpp:642]) to carry curved turn trails and far-astern wake tails at `+6 cm` lift.)

## 4.3 Bow wave strip (`UIronwakeBowWave`)

A world-space procedural mesh rebuilt every tick from five ribbon kinds; all vertices sit on the sea (or on the deck for green water), 35 cm above `RenderedZ` plus a speed-dependent height. Material `M_IW_BowFoam` [src Source/IronwakeTest/IronwakeHydro.cpp:368].

### 4.3.0 Topology constants

| Constant | Value | Source |
|---|---|---|
| `NCrest` [src Source/IronwakeTest/IronwakeHydro.cpp:373] (stations per Kelvin crest) | `22` [src Source/IronwakeTest/IronwakeHydro.cpp:373] | [src Source/IronwakeTest/IronwakeHydro.cpp:373] |
| `NStrip` [src Source/IronwakeTest/IronwakeHydro.cpp:373] (waterline-foam stations) | `40` [src Source/IronwakeTest/IronwakeHydro.cpp:373] | [src Source/IronwakeTest/IronwakeHydro.cpp:373] |
| `NMound` [src Source/IronwakeTest/IronwakeHydro.cpp:373] (stem-mound stations) | `13` [src Source/IronwakeTest/IronwakeHydro.cpp:373] | [src Source/IronwakeTest/IronwakeHydro.cpp:373] |
| `NAcross` [src Source/IronwakeTest/IronwakeHydro.cpp:373] (verts across each ribbon) | `4` [src Source/IronwakeTest/IronwakeHydro.cpp:373] | [src Source/IronwakeTest/IronwakeHydro.cpp:373] |
| Curl-lip stations per side | `12` [src Source/IronwakeTest/IronwakeHydro.cpp:488] | [src Source/IronwakeTest/IronwakeHydro.cpp:488] |
| Deck-wash stations | `8` [src Source/IronwakeTest/IronwakeHydro.cpp:525] | [src Source/IronwakeTest/IronwakeHydro.cpp:525] |
| Across T | `0, 0.3, 0.6, 1` | [src Source/IronwakeTest/IronwakeHydro.cpp:375] |
| Across alpha weights | `0.55, 1, 0.6, 0` | [src Source/IronwakeTest/IronwakeHydro.cpp:376] |
| Across lift weights | `0.7, 1, 0.55, 0` | [src Source/IronwakeTest/IronwakeHydro.cpp:377] |
| Curl-lip alpha / height profile | `0.9, 1, 0.85, 0.15` / `1.05, 1.25, 1.0, 0.35` | [src Source/IronwakeTest/IronwakeHydro.cpp:479-480] |

Derived vertex count: mound 13x4 = 52, crests 2x22x4 = 176, lip 2x12x4 = 96, strip 2x40x4 = 320, deck wash 8x4 = 32; total 676, of which 644 take a `RenderedZ` plugin query per vertex per frame (deck wash uses the hull transform instead) (derived from the loops in [src Source/IronwakeTest/IronwakeHydro.cpp:380-415]).

Per-vertex values in `AddRibbon`: `P2 = Lerp(In, Out, AcrossT[j])`; `z = RenderedZ + 35 + H * PH[j] * 100` cm [src Source/IronwakeTest/IronwakeHydro.cpp:378]; alpha clamp(A * PA[j], 0, 1) [src Source/IronwakeTest/IronwakeHydro.cpp:381]; UV = (`AcrossT`, station fraction) [src Source/IronwakeTest/IronwakeHydro.cpp:382]. Deck wash vertices use the hull transform: DeckZ + 30 + H*PH*100 [src Source/IronwakeTest/IronwakeHydro.cpp:374].

### 4.3.1 Gates and frame

- Visible only when `SpeedFrac > 0.04` (about 0.5 kn) [src Source/IronwakeTest/IronwakeHydro.cpp:402]; hidden meshes keep their last geometry if already built [src Source/IronwakeTest/IronwakeHydro.cpp:404].
- F = clamp(SpeedFrac, 0, 1.2) [src Source/IronwakeTest/IronwakeHydro.cpp:405]. The caller passes `Kn/13` (clamped 1.2), so F = 1 at 13 kn [src Source/IronwakeTest/IronwakeShipHydro.cpp:158].
- Level frame: yaw only, z = 0, so foam lies on the sea not on the pitching deck [src Source/IronwakeTest/IronwakeHydro.cpp:406-408].
- Clearance: every inner edge is at `HalfBreadthAt(x) + Margin`, `Margin = 0.7 * Scale` m [src Source/IronwakeTest/IronwakeHydro.cpp:413-414].
- `Stem = Hull.X1`, `L = Hull.Length()` [src Source/IronwakeTest/IronwakeHydro.cpp:437].

### 4.3.2 Ribbon 1: stem mound ("bow cushion")

There is no symbol named "cushion" in code; the P5c request "small raised bow wave / cushion at the stem with a foam cap" [src build/cards2/P5c_ship_water.md:14] is implemented by this stem mound, and SPEC.md's "bow cushion from 8 kn" [src SPEC.md:29] describes its visibility.

- Arc: `13` stations at angles `-110`..`+110` deg around the stem [src Source/IronwakeTest/IronwakeHydro.cpp:424]; radii `R0 = 0.6*Scale` m to `R1 = (3.5 + 9*F)*Scale` m [src Source/IronwakeTest/IronwakeHydro.cpp:421].
- Inner edge pushed sideways outside the hull if it falls inside (`|y| < Clear(x)` and `x < Stem + 0.3`) [src Source/IronwakeTest/IronwakeHydro.cpp:446]; outer edge keeps at least `0.8 * (R1 - R0)` lateral spread when `0.3f` [src Source/IronwakeTest/IronwakeHydro.cpp:448].
- Alpha at a station: `clamp((F - 0.25)*2, 0, 1) * (0.65 + 0.35*Ahead)`, Ahead = cos(angle) [src Source/IronwakeTest/IronwakeHydro.cpp:433]. Gate starts at F = 0.25 (3.25 kn) and is full at F = 0.75 (9.75 kn) (derived: F x 13).
- Height (m): (0.2 + 2.0*F^2) * Scale * (0.5 + 0.5*max(Ahead, 0)) [src Source/IronwakeTest/IronwakeHydro.cpp:433]. P5c log: "stem mound height 0.2 + 2.0*F^2 (was 0.3 + 1.2F)" [src build/logs/P5c.md:6]. At FULL on the ironclad (Scale 1): crest `2.2` m. `LastStemCrestM = (0.2 + 2.0*F^2)*Scale` is stored for others to read [src Source/IronwakeTest/IronwakeHydro.cpp:436].
- Derived heights at nominal speeds on the ironclad: 4 kn (F 0.31) 0.39 m, 8 kn (F 0.62) 0.96 m, 13 kn (F 1) 2.2 m; alpha at 4 kn 0.12, 8 kn 0.73 (derived).

### 4.3.3 Ribbons 2 and 2b: diverging Kelvin crests and curling lip

- Kelvin = tan(19.47 deg) [src Source/IronwakeTest/IronwakeHydro.cpp:457]; crest length CrestLen = Lerp(0.35, 1.4, clamp(F, 0, 1)) * L [src Source/IronwakeTest/IronwakeHydro.cpp:458]. Ironclad L = 121.7 m (derived: 59.2 - (-62.5) from [src Source/IronwakeTest/IronwakeHydro.cpp:23]): crest 42.6 m at a crawl to 170 m at FULL.
- Per station `U = i/21` (22 stations): `Sx = U*CrestLen`; X = Stem - 2*Scale - Sx [src Source/IronwakeTest/IronwakeHydro.cpp:447-449]; width W0 = (2.5 + 7*F)*Scale*(0.6 + 1.4*U) [src Source/IronwakeTest/IronwakeHydro.cpp:450]; inner lateral Yin = max(1*Scale + Sx*Kelvin, Clear(X)) [src Source/IronwakeTest/IronwakeHydro.cpp:451-452].
- Station alpha: `clamp(F*1.6, 0, 1) * sqrt(Fade)` with `Fade = clamp(U < 0.08 ? U/0.08 : 1, 0, 1) * (1 - U)^2`; height `(0.8 + 3.2*F) * Scale * (1 - 0.8*U)` m [src Source/IronwakeTest/IronwakeHydro.cpp:453-454]. Outer edge x trails `0.25*W0` aft [src Source/IronwakeTest/IronwakeHydro.cpp:454].
- Curling lip (W4): over the first 45 % of each crest (`U = i/11 * 0.45`, 12 stations) [src Source/IronwakeTest/IronwakeHydro.cpp:469]; ribbon spans `Yin + 0.15*W0` to Yin + 0.75*W0 [src Source/IronwakeTest/IronwakeHydro.cpp:474]; alpha `clamp((F - 0.25)*2.5, 0, 1) * Fade` with `Fade = clamp(U/0.06, 0, 1) * (1 - U/0.45)^2`; height equals crest height `Hc` with the lip height profile (peak `1.25 Hc`) [src Source/IronwakeTest/IronwakeHydro.cpp:475-477]. Always emitted (fixed topology); alpha is 0 below 25 % speed [src Source/IronwakeTest/IronwakeHydro.cpp:460]. Vision: "no curling lip, painted foam" (score 4/10) [src build/logs/W4.md:14].
- Derived crest heights at FULL, ironclad: first station `4.0` m (0.8 + 3.2), last `0.8` m (x(1 - 0.8)); width at FULL 9.5 m x (0.6..2.0) = 5.7..19 m.

### 4.3.4 Ribbon 3: waterline foam strip

Two strips hugging the hull, 40 stations from the shoulder to past the stern [src Source/IronwakeTest/IronwakeHydro.cpp:483-501].

- `U = i/39`; `X = Stem - 3*Scale - U*1.05*L` (runs 5 % past the stern, W3) [src Source/IronwakeTest/IronwakeHydro.cpp:489]; `Yin = Clear(X) - 0.3*Scale`; width `Wd = (2.0 + 4.5*F)*Scale`, outer edge Yin + Wd*(0.5 + 0.5*U) [src Source/IronwakeTest/IronwakeHydro.cpp:490-491] [src Source/IronwakeTest/IronwakeHydro.cpp:498].
- Speed gate: `Speed = pow(clamp((F - 0.2)*1.5, 0, 1), 1.5)`: nothing below F = 0.2 (2.6 kn), full at F = 0.87 (11.3 kn) [src Source/IronwakeTest/IronwakeHydro.cpp:493].
- Density: `BowD = clamp(1 - U/0.22, 0, 1)`; `SternD = clamp((U - 0.72)/0.2, 0, 1) * (U > 0.97 ? 0.5 : 1)`; noise `Nz = 0.5 + 0.5*sin(i*2.3 + Side*1.7 + frame*0.02)*sin(i*0.9 - Side*0.6 + frame*0.013)`; `Dens = max(BowD,SternD) + (1 - max)*0.45*Nz`; alpha Speed * Dens * (U < 0.03 ? U/0.03 : 1) [src Source/IronwakeTest/IronwakeHydro.cpp:494-497]. (Foam densest at the bow and stern quarter, patchy amidships, P5c.)
- Height `(0.05 + 0.12*BowD)*F*Scale` m [src Source/IronwakeTest/IronwakeHydro.cpp:498].

### 4.3.5 Ribbon 4: green water on the forecastle

- Eight stations from the stem back `0.16 * L`: `X = Stem - 1 - U*0.16*L`, half width max(HalfBreadthAt(X) - 0.4, 0.3) [src Source/IronwakeTest/IronwakeHydro.cpp:527]; alpha `A*(1 - 0.7*U)`, height `0.25*A` m with A = clamp(DeckWash, 0, 1) [src Source/IronwakeTest/IronwakeHydro.cpp:523] [src Source/IronwakeTest/IronwakeHydro.cpp:529]; transformed by the (pitching) hull frame at deck height `BowDeckZ` [src Source/IronwakeTest/IronwakeHydro.cpp:531].
- `DeckWash` input: `0` below sea state 2.8, else clamp((BowSubmergeM + 0.6)/1.2, 0, 1) [src Source/IronwakeTest/IronwakeShipHydro.cpp:160]; `BowDeckZ = HullMeshZ + BowDeckM*100` (BowDeckM per hull: Devastation 3.2, Kracken 5.0, Goliath 2.5, Aglae 1.2, Frigate 6.0; see chapter 3 3.10.5).

### 4.3.6 Bow-foam material (`M_IW_BowFoam`)

Created by `w1_assets.py`: lit translucent, volumetric non-directional lighting, two-sided [src Content/Python/w1_assets.py:11-14]; two world-position foam layers from `/Water/Textures/Foam/T_WaterFlow_03_Foam_Tiled` with UV divisors `900.0` and `2300.0` and panner speeds `(0.01, 0.004)` / (-0.006, 0.008) [src Content/Python/w1_assets.py:30-31]; `foam = t1 + t1*t2`; opacity = saturate((foam*1.4 + 0.3) * VertexColor.A * Opacity) [src Content/Python/w1_assets.py:38-48]; depth fade distance `120` cm [src Content/Python/w1_assets.py:49] reduced to `8` by `w1_foamfix.py` (foam lifted 35 cm) [src Content/Python/w1_foamfix.py:6]; base colour lerp `(0.55, 0.6, 0.63)` to `(0.92, 0.94, 0.95)` by the foam texture; roughness `0.85` [src Content/Python/w1_assets.py:53-58]. Vision after W1 fix: 7/10 on `W1_bow_wave` [src build/logs/W1.md:58].

## 4.4 Bow spray, slam and sheets (Niagara, `TickBowFX`)

All spawned from `AIronwakeShipPawn::TickBowFX` each tick [src Source/IronwakeTest/IronwakeShipHydro.cpp:155]. Spawn points are outside the hull (hull half-breadth + offset) so no sprite starts inside the mesh.

### 4.4.1 Continuous bow sheets (`NS_IW_BowSheet`)

- Chuck 2026-11-10: bow spray is small and tied to speed; only a big wave makes a big splash. Continuous sheets are light feathering at the shoulders, from ~8 kn up, sized by speed^2 [src Source/IronwakeTest/IronwakeShipHydro.cpp:176-177].
- SeaK = clamp(0.7 + 0.1*SeaState, 0.7, 1.1) [src Source/IronwakeTest/IronwakeShipHydro.cpp:178].
- Rate (bursts per second): `0` when `Frac <= 0.6` (7.8 kn), else Lerp(0.3, 1.5, clamp((Frac - 0.6)/0.4, 0, 1)) * SeaK [src Source/IronwakeTest/IronwakeShipHydro.cpp:179]. One accumulator `SprayAcc += Dt*Rate`; when `>= 1` a burst is spawned on **both** sides (the comment says "per side"; code spawns one per side per trigger) [src Source/IronwakeTest/IronwakeShipHydro.cpp:180-186].
- Conditions: not NPC, not `-iwnosheet`, system loaded [src Source/IronwakeTest/IronwakeShipHydro.cpp:181].
- Position per side `s = +-1`: `X = X1 - rand(5, 12) * Hs`; `Y = s*(HalfBreadthAt(X) + 1.2)`; `Z = RenderedZ + 60` cm [src Source/IronwakeTest/IronwakeShipHydro.cpp:188-190]. Axis (in yaw frame) `(0.15, s, 0.55)` normalised [src Source/IronwakeTest/IronwakeShipHydro.cpp:191]. Pooled auto-release spawn [src Source/IronwakeTest/IronwakeShipHydro.cpp:192].
- Scale: clamp((0.10 + 0.30*Frac^2) * clamp(Hs, 0.6, 1.3) * SeaK, 0.06, 0.5) [src Source/IronwakeTest/IronwakeShipHydro.cpp:193].

Derived rates: at 13 kn CALM (SeaK 0.7): 1.05 bursts/s => 2.1 sprite systems/s; STORM (SeaK 1.1): 1.65/s. At 9 kn (Frac 0.69): Lerp = 0.3 + 1.2 x 0.2308 = 0.58 x SeaK 0.8 (MODERATE 0.8) = 0.46 bursts/s.

### 4.4.2 Slam bursts (`NS_IW_BowSpray` + extra `NS_IW_BowSheet`)

Slam gate (P5c, then P5b original):

- `Rise = (BowSubmergeM - Prev)/max(Dt, 1e-3)` (m/s, per-ship static map) [src Source/IronwakeTest/IronwakeShipHydro.cpp:197-199].
- Gates: `SeaSt = GetSeaState()`; `GateSub = Lerp(-1.1, -0.5, clamp((SeaSt - 1)/1, 0, 1))`, GateRise = Lerp(1.5, 1.0, same) [src Source/IronwakeTest/IronwakeShipHydro.cpp:204].
- `bMeet = SeaSt > 0.7 && (BowSubmergeM > GateSub && Rise > GateRise) || (SeaSt > 0.7 && BowSubmergeM > 0)` (stem rising to meet the sea fast near the deck edge, or buried; calm `< 0.7` never slams) [src Source/IronwakeTest/IronwakeShipHydro.cpp:205].
- Impact `= clamp(Rise/3 + max(BowSubmergeM + 0.8, 0)*0.5, 0, 1.5)`; SlamStrength = Frac^2 * Impact [src Source/IronwakeTest/IronwakeShipHydro.cpp:207-208].
- Fire when: not `-iwnosheet`, `Kn > 4`, cooldown <= 0, `SlamStrength > 0.08`, and (`bMeet` or (`BowSubmergeM > 1.0` and sea state >= 3)) [src Source/IronwakeTest/IronwakeShipHydro.cpp:209]. Cooldown `Lerp(4, 1.5, clamp(Frac, 0, 1))` s [src Source/IronwakeTest/IronwakeShipHydro.cpp:211].
- Slam spray (`NS_IW_BowSpray`): one burst per side at `X = X1 - 4`, `Y = s*(HalfBreadthAt(X) + 2.5)`, `Z = RenderedZ + 50`, cone tilted outward/forward (vector `(0.35, 0.6 s, 1)`); world scale 0.05 + 0.12*Strength [src Source/IronwakeTest/IronwakeShipHydro.cpp:220-228].
- Slam sheets (`NS_IW_BowSheet`): per side at `X = X1 - 9*Hs`, `Y = s*(HalfBreadthAt(X) + 1.5)`, `Z = +80`; axis `(0.25, 0.95 s, 0.75)`; scale `clamp((0.12 + 0.6*Strength) * Sea * clamp(Hs, 0.6, 1.3), 0.08, 1.1)` with Sea = clamp(SeaState/4, 0.25, 1) [src Source/IronwakeTest/IronwakeShipHydro.cpp:236-245].
- Logged as IWP5B slam kn= strength= rise= bowSub= sea= [src Source/IronwakeTest/IronwakeShipHydro.cpp:248].
- `BowSubmergeM` = (rendered sea at the stem deck point minus its z)/100; green water when > 0 (chapter 3, 3.10.1).
- Measured slam counts per 60 s at 13 kn on the cutter (P5c): calm 0, moderate 8, rough 35 [src build/logs/P5c.md:8]. P5b measured 0 slams (BowSubmergeM max -0.57) under the older stricter gate [src build/logs/P5b.md:8].

### 4.4.3 NPC behaviour

- `TickBowFX` runs for NPCs too, but continuous sheets and the funnel-smoke scale are `!bIsNPC` [src Source/IronwakeTest/IronwakeShipHydro.cpp:165] [src Source/IronwakeTest/IronwakeShipHydro.cpp:181]; slam spray has no NPC check [src Source/IronwakeTest/IronwakeShipHydro.cpp:209].
- NPC ships get the bow-wave and wake components (same code, own profile Kracken) [src Source/IronwakeTest/IronwakeShipPawn.cpp:324].
- Merchants (`AIronwakeMerchant`, not pawns) do not use these; they use one flat foam plane (below).

## 4.5 Niagara systems inventory

Last known parameter source per system. Assets: `/Game/Ironwake/FX/<name>`. "Dup of" means created by `EditorAssetLibrary.duplicate_asset` then retuned over MCP; the parent's emitter defaults remain unless a later script set them.

| System | Used by (spawn site) | Origin | Last-written key params | Source |
|---|---|---|---|---|
| `NS_IW_Splash` [src build/tools/fxsetup.py:46] | shell water impact; island/voyage splashes | base | burst 260, life 1.6-3.6, sprite 250-700, colour `(0.85, 0.88, 0.9, 0.55)` [src build/tools/fxsetup.py:46], speed 1200-3000, cone 12, drag 0.2-0.6, gravity -980; material `M_IW_Particle` | [src build/tools/fxsetup.py:46] |
| `NS_IW_Explosion2` [src build/tools/fxsetup.py:48] | shell hit | base | burst 160, life 0.5-1.4, sprite 400-1100, colour `(14, 5, 1.2, 0.9)` [src build/tools/fxsetup.py:48], speed 600-2400, cone 70 | [src build/tools/fxsetup.py:48] |
| `NS_IW_Fire` [src build/tools/fxsetup.py:50] | damage fires | base | loop 70/s, life 0.6-1.4, sprite 300-900, colour `(12, 3.5, 0.6, 0.8)` [src build/tools/fxsetup.py:50] | [src build/tools/fxsetup.py:50] |
| `NS_IW_Smoke` [src build/tools/fxsetup.py:52] | burning/sinking only; factory chimney | base | loop 22/s, life 6-10, sprite 900-2600, colour `(0.04, 0.04, 0.045, 0.7)` [src build/tools/fxsetup.py:52], radius 300 | [src build/tools/fxsetup.py:52] |
| `NS_IW_FunnelSmoke` [src Content/Python/p5b_assets.py:5] | all funnels + frigate aux stack | dup of `NS_IW_Smoke` [src Content/Python/p5b_assets.py:5] | see 4.6 | [src Content/Python/p5b_assets.py:5] |
| `NS_IW_BowSpray` [src Content/Python/w1_assets.py:79] | slam spray | dup of `NS_IW_Splash` [src Content/Python/w1_assets.py:79] | no later param edit in repo; material `M_IW_Particle` | [src Content/Python/w1_assets.py:79] |
| `NS_IW_BowSheet` | bow sheets, slam sheets | dup of `NS_IW_BowSpray` | material switched to `M_IW_Spray` (W4) | [src Content/Python/w2_assets.py:18] [src build/tools/w4fx.py:40] |
| `NS_IW_Spume` [src Content/Python/w2_assets.py:18] | storm director | dup of `NS_IW_BowSpray` [src Content/Python/w2_assets.py:18] | see 4.7 | [src Content/Python/w2_assets.py:18] |
| `NS_IW_Rain` [src Content/Python/w2_assets.py:18] | storm director | dup of `NS_IW_Smoke` [src Content/Python/w2_assets.py:18] | see 4.7 | [src Content/Python/w2_assets.py:18] |

- Spawn calls: shell splash [src Source/IronwakeTest/IronwakeShell.cpp:134-136]; hit `NS_IW_Explosion2` [src Source/IronwakeTest/IronwakeCombat.cpp:362]; sink/burn `NS_IW_Smoke` and NS_IW_Fire [src Source/IronwakeTest/IronwakeCombat.cpp:402] [src Source/IronwakeTest/IronwakeCombat.cpp:416].
- The old bow splash spawn in `AIronwakeVoyage::TickSailing` was removed in W1 [src Source/IronwakeTest/IronwakeVoyage.cpp:1018].
- Soft spray material `M_IW_Spray` (W4): unlit translucent; opacity = `radial^1.6 * (0.35 + 0.9*foamTex) * ParticleColor.A * depthfade(60 cm)`; emissive = particle colour; used with Niagara sprites and ribbons [src Content/Python/w4_spray.py:17-48] with exponent `1.6` [src Content/Python/w4_spray.py:32], breakup constants `0.9` and `0.35` [src Content/Python/w4_spray.py:39] [src Content/Python/w4_spray.py:41], depth fade `60` [src Content/Python/w4_spray.py:46], panner `0.08`, `0.03` [src Content/Python/w4_spray.py:37].
- Chimney smoke in the town does not use Niagara (sphere plumes, Niagara ignores component scale) [src build/logs/P12.md:16]; the factory `NS_IW_Smoke` is scaled `0.045` and deactivated/hidden [src Source/IronwakeTest/IronwakeVoyage.cpp:347-349].

## 4.6 Funnel smoke

### 4.6.1 Asset parameters (`NS_IW_FunnelSmoke`)

Duplicated from `NS_IW_Smoke` [src Content/Python/p5b_assets.py:5], then edited in order:

| Edit | Values | Source |
|---|---|---|
| P5b | colour `(0.35, 0.35, 0.37, 0.3)` [src build/tools/p5b_smoke.py:4-8], uniform sprite size min `800` [src build/tools/p5b_smoke.py:4-8], max `2000` [src build/tools/p5b_smoke.py:4-8], spawn rate `9` [src build/tools/p5b_smoke.py:4-8], sphere radius `150` [src build/tools/p5b_smoke.py:4-8] | [src build/tools/p5b_smoke.py:4-8] |
| P5c (final) | colour `(0.62, 0.62, 0.64, 0.75)` [src build/tools/p5c_smoke.py:4-7], spawn rate `14` [src build/tools/p5c_smoke.py:4-7], lifetime min `4` [src build/tools/p5c_smoke.py:4-7], max `7` [src build/tools/p5c_smoke.py:4-7] | [src build/tools/p5c_smoke.py:4-7] |

The P5c edit does not touch sprite size or radius, so P5b `800`..`2000` (cm, i.e. 8-20 m at scale 1) and radius `150` persist [src build/logs/P5b.md:3] (asset binary not read; if edited later in the editor this is unknown). P5c log: previous alpha 0.3 / 9 per s was invisible against fog; now 0.62 alpha 0.75 14/s life 4-7 s [src build/logs/P5c.md:9]. Derived: at 14/s and mean life 5.5 s there are ~77 live sprites per funnel at steady state.

### 4.6.2 Stack positions, base scale and colour per hull

| Hull | Funnel smoke? | Attach / local position (cm) | Base scale | Source |
|---|---|---|---|---|
| Cutter Kittiwake (steam launch) | yes (`bFunnelSmoke = true`) | `HullMesh`, `FunnelTop = (60, 0, 965)` | `SmokeScale 0.2`, x`1.2` since `LengthM < 60` => `0.24` | [src Source/IronwakeTest/IronwakeFleet.cpp:46] [src Source/IronwakeTest/IronwakeFleet.cpp:51] [src Source/IronwakeTest/IronwakeShipHydro.cpp:165] |
| Ironclad Devastation | no `FunnelSmoke` component (`bFunnelSmoke` default false) | none | n/a | [src Source/IronwakeTest/IronwakeFleet.h:31] [src Source/IronwakeTest/IronwakeFleet.cpp:62] |
| Frigate Albatross | aux steam stack created on first `B` (steam toggle) | `HullMesh`, `(-2300, 0, 2300)` (P7: -23 m aft, 23 m up) | `0.9` (sail ship) | [src Source/IronwakeTest/IronwakeShipFleet.cpp:136-137] [src Source/IronwakeTest/IronwakeShipHydro.cpp:165] |
| Merchant Goliath tug | yes | funnel component at `(330, 0, 1140)`, smoke local `(0, 0, 55)` | `0.3` | [src Source/IronwakeTest/IronwakeLivingSea.cpp:105] [src Source/IronwakeTest/IronwakeLivingSea.cpp:152-154] |
| Merchant Aglae | yes | funnel at `(60, 0, 940)`, smoke local `(0, 0, 55)` | `0.2` | [src Source/IronwakeTest/IronwakeLivingSea.cpp:128] [src Source/IronwakeTest/IronwakeLivingSea.cpp:154] |
| NPC enemy (Kracken) | none (`FleetBeginPlay` [src Source/IronwakeTest/IronwakeShipFleet.cpp:70] returns for NPCs) | n/a | n/a | [src Source/IronwakeTest/IronwakeShipFleet.cpp:66] |

- `FleetBeginPlay` spawns the smoke only for the player ship (`bIsNPC` returns early) [src Source/IronwakeTest/IronwakeShipFleet.cpp:66] with `SpawnSystemAttached(..., KeepRelativeOffset, false)` and scale `ShipDef->SmokeScale` [src Source/IronwakeTest/IronwakeShipFleet.cpp:87-88].
- Sail ships: the stack exists only after toggling auxiliary steam; SetActive(bAuxSteam || !SailRig) [src Source/IronwakeTest/IronwakeShipFleet.cpp:139]. Default for the frigate is steam OFF (no smoke) until `B` is pressed [src Source/IronwakeTest/IronwakeShipFleet.cpp:132].
- The frigate stack has moved three times: between the masts [src build/logs/P5c.md:9] (-15 m aft, 13 m up, scale 0.6) then (-23 m aft, 23 m up, 0.9) [src build/logs/P7.md:27]. Vision after P7: score about 5/10, visible in chase, faint [src build/logs/P7.md:27].
- Moored steamer's funnel smoke is deactivated (white glare pillar) [src Source/IronwakeTest/IronwakeVoyage.cpp:428].

### 4.6.3 Scale vs speed

`FunnelSmoke->SetWorldScale3D(base * (0.45 + 0.55 * clamp(Frac, 0, 1)))`, each tick, player ship only; base = SailRig ? 0.9 : (ShipDef ? SmokeScale * (LengthM < 60 ? 1.2 : 1) : 0.3) [src Source/IronwakeTest/IronwakeShipHydro.cpp:165]. So smoke is 45 % size at a stop and 100 % at 13 kn (Frac 1) (derived). Cutter: `0.24 x (0.45..1.0)` = `0.108..0.24`. Frigate `0.405..0.9`. P5b: "funnel scale 45% -> 100% with speed" [src build/logs/P5b.md:3]. Emission rate and lifetime do not vary with speed (fixed in the asset).

Note: the P12 log says Niagara ignores component scale for the town chimney plumes [src build/logs/P12.md:16]; for ship funnels the scale demonstrably changes the smoke (chase view scoring) per P5c/P7 logs, so treat scale as honoured here, unverified numerically.

## 4.7 Storm spray: spume and rain (`AIronwakeStorm`)

### 4.7.1 Spume (`NS_IW_Spume`)

- Active when `G = clamp((Local - 1.8)/2.2, 0, 1) > 0` (whitecap spray from ROUGH, full by GALE/STORM; Local = sea state at the player) and the asset loaded and not -iwnospume [src Source/IronwakeTest/IronwakeStorm.cpp:195-198].
- Live-burst budget `Lerp(40, 160, G)` (W3) [src Source/IronwakeTest/IronwakeStorm.cpp:199]; candidate tests per second Lerp(60, 260, G) [src Source/IronwakeTest/IronwakeStorm.cpp:200].
- Candidate position: radius `Lerp(30, 500, FRand^2)` m (squared distribution, more candidates near the camera) at an angle within `+-50` deg of the camera yaw [src Source/IronwakeTest/IronwakeStorm.cpp:207-209]; rejected if within `2500` cm of the player ship [src Source/IronwakeTest/IronwakeStorm.cpp:211].
- Crest test: reject unless Jacobian `J <= Lerp(0.55, 0.72, G)` and height `>= FastZ(ship) - 150` cm [src Source/IronwakeTest/IronwakeStorm.cpp:215]; spawn at `Z + 30` [src Source/IronwakeTest/IronwakeStorm.cpp:225].
- Orientation: axis `normalize(DownV*0.97 + Up*0.1)` (low, long, blown downwind, W4); scale Lerp(0.7, 1.4, G) * clamp(R/15000, 0.8, 2.2) [src Source/IronwakeTest/IronwakeStorm.cpp:217-219]. Each burst counts as live for `3.5` s [src Source/IronwakeTest/IronwakeStorm.cpp:221].
- W4 asset tuning (last script): burst count `60`, lifetime `2.5`..`4.5`, sprite size min `(120, 1500)` max `(320, 4200)` (velocity-aligned: Y along travel), colour `(0.42, 0.44, 0.46, 0.3)`, cone velocity `1400`..`2800`, cone angle `12`, gravity `(0, 0, -120)`, drag `0.2`..`0.5`, material M_IW_Spray [src build/tools/w4fx.py:25-37]. W3 values (burst 46, life 2-4, sprite 700x450..2200x1100, colour 1.5/1.55/1.6/0.5, strength 900-2400, cone 18) [src build/tools/w3fx.py:27-35] are superseded.
- Measured: 2,242 bursts in one W3 storm run [src build/logs/W3.md:33]; W4 vision still sees no airborne spume (1-2/10): too faint/thin [src build/logs/W4.md:13]. Budget with the original W2 setup was 25-90 live [src build/logs/W2.md:18]; code now 40-160.

### 4.7.2 Rain (`NS_IW_Rain`)

- Active when `RainAmount > 0.05` or `Local >= 2.6`; placed `1800` cm ahead of the camera yaw and `1800` cm above, rotated to the wave travel yaw (slant downwind) [src Source/IronwakeTest/IronwakeStorm.cpp:151-155].
- W3 asset parameters: spawn rate `24000`/s, lifetime `0.9`..`1.3`, non-uniform sprite `(2.6, 110)` cm, colour `(2.2, 2.4, 2.7, 0.55)` (HDR), sphere radius `4200`, cone axis `(0.55, 0, -1)`, speed `1300`..`1900`, velocity-aligned [src build/tools/w3fx.py:14-24]. Vision: rain reads (prominent diagonal streaks) [src build/logs/W3.md:32].
- Rain audio layer gain `bRain ? Lerp(0.25, 0.9, max(RainAmount, Storminess)) : 0` (chapter 10) [src Source/IronwakeTest/IronwakeStorm.cpp:262].

### 4.7.3 Lightning flash (not Niagara)

Directional light `Bolt` with atmosphere light index 1 (lights the clouds, sun disk colour scale black); intensity `Flash * 30`; flash decays at `4`/s; main stroke plus return stroke after `0.08`..`0.16` s at 0.8 x peak; strikes begin when `Storminess >= 0.6`, spacing `rand(7, 18)/max(St, 0.6)` s; strike distance `900`..`7000` m, thunder delayed `dist/343` s [src Source/IronwakeTest/IronwakeStorm.cpp:80] [src Source/IronwakeTest/IronwakeStorm.cpp:182-184] [src Source/IronwakeTest/IronwakeStorm.cpp:237-244].

## 4.8 Legacy flat planes (hidden) and merchant wakes

- `WakeStern`, `WakeBowL`, `WakeBowR` are static-mesh planes with `M_WakeFoam` MIDs on every ship; since W1/W3 they are forced hidden every tick [src Source/IronwakeTest/IronwakeShipPawn.cpp:855-856]. The code after `if (!bShow) return;` (`Kn > 0.5`) still positions/scales them and sets their `Opacity` MID param each tick: dead work (stern strip `Lerp(15000, 90000, Frac16) * sqrt(WakeSc)` cm long, width `Lerp(2400, 3400, Frac16) * WakeSc`; bow planes at 19 deg `Lerp(4000, 11000)` x `Lerp(500, 1100)`; opacity `clamp(Frac16*1.3, 0, 1)`, where `Frac16 = clamp(Kn/16, 0, 1)`) [src Source/IronwakeTest/IronwakeShipPawn.cpp:863-885]. They have no visible effect.
- Merchants still use one flat stern plane: length `16000` cm, width scale `7` (Aglae) or `12` (Goliath) (x100 cm plane), `Opacity 0.75`, translucent sort priority 10 [src Source/IronwakeTest/IronwakeLivingSea.cpp:208-210] [src Source/IronwakeTest/IronwakeLivingSea.cpp:147]. Merchants have no bow wave and no Kelvin wake (W1 open item 3) [src build/logs/W1.md:82].
- Wreck waterline foam planes reuse `M_IW_BowFoam` and "barely show from 170 m" [src build/logs/E1.md:63].

## 4.9 Vision-score history (readability, not metrics)

| System | Result | Source |
|---|---|---|
| Wake from 200 m up astern | 9.5/10 after W4 (W3 `wake_high` [src build/logs/W4.md:12] 0/10 had wrong framing) | [src build/logs/W4.md:12] |
| Bow wave close, rough | 4/10, "painted foam", no curl | [src build/logs/W4.md:14] |
| Cutter side 13 kn | 3/10 flat band (P5b), 7 after P5c wake texture fix | [src build/logs/P5b.md:6] [src build/logs/P5c.md:14] |
| Frigate chase wake | unstable 3-7 | [src build/logs/P5c.md:14] |
| Spume | 1-2/10 | [src build/logs/W4.md:13] |
| Rain | reads | [src build/logs/W3.md:32] |
| Funnel smoke (frigate chase) | about 5/10 after P7 | [src build/logs/P7.md:27] |

Vision scores jitter by +-1.5 between identical frames [src build/logs/P5c.md:13].

## Gaps / unknown

- Niagara asset values as they exist today: unknown. The `.uasset` files are binary; the numbers above are the last values written by repo scripts. Measure with `build/tools/niag.py` / `nset.py` readback over the editor MCP, or `ndump.py`.
- Whether the `NS_IW_FunnelSmoke` sprite size (`800`-`2000`) and radius (`150`) from P5b remain: P5c did not rewrite them; assumed to persist.
- `NS_IW_BowSpray` emitter parameters after duplication from `NS_IW_Splash`: no tuning script exists in the repo; slam spray size is controlled only through the spawn scale (`0.05 + 0.12*Strength`). Look in the asset to confirm burst count/lifetime.
- Whether the shipped wake uses `Thresh 1.35` (code) or the W4 `0.32`: see the CONFLICT in 4.2.5. Measure with `-iwwakedbg` plus a 200 m astern capture at both values.
- Kelvin arm geometry uses compressed speed; there is no measurement of the visible arm angle versus the intended 19.47 deg on screen (sea waves displace the ribbon). Measure with a top-down capture at FULL.
- Cost of the wake and bow wave (CPU vertex rebuild + 890 plugin/analytic height queries) is only estimated, not profiled: see chapter 11.
- Frigate funnel smoke when the frigate runs under auxiliary steam at gale: not tested.
- Ironclad has no funnel smoke at all (no `bFunnelSmoke`); whether that is intended is a design question, not answered by the repo.

## 4.10 P27: Turbulent twin-vortex prop-wash LUT, Voronoi cellular lace & zero hull-climbing foam (2026-11-13)

- **Propulsion-aware turbulent boil (`M_IW_WaterRoot`, `IronwakeHydro.cpp`):** replaced the solid white `M_IW_Surf2` ribbon with `T_IW_PropWashFoamLut` (`R` = twin-vortex boil, `G` = echelon crest lace, `B` = conformal bow/shoulder contact) modulated by 3 distance-adaptive octaves (`2.6 m`, `11.5 m`, `36 m`) of periodic Voronoi cellular foam (`T_IW_CellularFoamTile`) and aerated turquoise subsurface scattering (`FLinearColor(0.055, 0.235, 0.255)`).
- **Twin-screw vs single-screw vs pure sail:** `FIwShipWaveSrc` carries `ThrustSteam` [src Source/IronwakeTest/IronwakeShipWave.h:20] and `bTwinScrew` [src Source/IronwakeTest/IronwakeShipWave.h:23] (`ShipId == TEXT("ironclad")` [src Source/IronwakeTest/IronwakeShipPawn.cpp:875], `false` on single-screw steamers, `0.f` thrust on `TallBrig` and `Frigate` under pure sail). Twin-screw hulls emit two counter-rotating jets at `+/- 0.30 * B/2` spreading as `(1 + s * 0.85)^0.42`; sailing ships under canvas emit zero screw boil (`sheet_sail_vs_steam_p27.jpg` `9.5 / 10` [src build/logs/P27.md:7], prop wash `8.5 / 10` [src build/logs/P27.md:5]).
- **Zero mid-body hull climb:** clamped `UIronwakeWake` vertex lift from `70` cm down to `6.f` cm [src Source/IronwakeTest/IronwakeHydro.cpp:660] with `SetCastShadow(false)` [src Source/IronwakeTest/IronwakeHydro.cpp:551] and `bReceivesDecals = false; bAffectDynamicIndirectLighting = false` [src Source/IronwakeTest/IronwakeHydro.cpp:552], switched curved turn ribbons to lit `M_IW_WakeLut` (`DepthFade = 45` cm, `VCR`/`VCA` scalar inputs), raised `M_IW_BowFoam` `DepthFade` from `8` cm to `65` cm, capped `UIronwakeBowWave` crest height at `0.55f` m [src Source/IronwakeTest/IronwakeHydro.cpp:473], shifted cutwater foam `+0.03 L` forward, and masked shader foam inside the conformal hull waterline (`d_outboard >= 0`, `sheet_hull_waterline_p19_vs_p27.jpg` `9.5 / 10` [src build/logs/P27.md:8]).
