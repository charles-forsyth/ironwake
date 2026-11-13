# Chapter 3: Water

Scope: everything about the sea surface and how things sit on it. The Gerstner wave generator (`UIronwakeWaveGen`) and its per-sea-state tables (CALM..STORM), the wind model that is tied to sea state, the sea-scale API and legacy scale mapping, the weather-to-sea director (`AIronwakeStorm`) with its land fade (shelter) and the harbour calm slab, the ocean/zone extents, the water-height queries (plugin query, analytic `FastZ`, `RenderedZ`, the zero render offset), the ship hydrostatics that replace the plugin buoyancy (waterplane fit and 2nd-order spring), hull profiles used for hull-water contact, and how non-pawn floaters (merchants, buoys, debris) sample the sea. Ship wake / bow FX are in chapter 4; GPU cost is in chapter 11. Every number is cited to the file and line where it is written. "CONFLICT" marks two places that disagree; the entry that takes effect at runtime is named. Engine/plugin internals (Water plugin Gerstner maths, material graph contents inside `.uasset`) are not readable from the repo and are listed under Gaps.

## 3.1 Architecture in one page

- One ocean: a single `WaterBodyOcean` actor in `/Game/Maps/Ironwake_Sea`; one WaterZone; no landscape or plane under the play area (build report `W3.md`). Because the Water plugin ocean has one global spectrum, the whole visible sea follows the sea state at the player [src Source/IronwakeTest/IronwakeStorm.cpp:133-134].
- Wave generator: `UIronwakeWaveGen` replaces the ocean's `GerstnerWaveGenerator` at runtime (`Waves->GerstnerWaveGenerator = Gen`) [src Source/IronwakeTest/IronwakeHydro.cpp:146]; it holds a fixed set of components whose wavelengths and directions never change, only amplitude and steepness follow the sea state, so blends never jump [src Source/IronwakeTest/IronwakeHydro.h:32-33].
- Re-apply path: `Apply()` rebuilds the wave array, calls `RecomputeWaves(false)` and `RequestWaveDataRebuild()` and then sets the water material instance parameters [src Source/IronwakeTest/IronwakeHydro.cpp:166-167].
- Sea state is a continuous float `0..4`: `0` CALM, `1` MODERATE, `2` ROUGH, `3` GALE, `4` STORM [src Source/IronwakeTest/IronwakeHydro.h:45]. Names are `CALM`, `MODERATE`, `ROUGH`, `GALE`, `STORM` [src Source/IronwakeTest/IronwakeHydro.cpp:86]. `SeaStateName` rounds to nearest integer [src Source/IronwakeTest/IronwakeHydro.cpp:91]. All tables are interpolated linearly between integer states by `Lerp5` [src Source/IronwakeTest/IronwakeHydro.cpp:88].
- Default before any call: `GetSeaState()` returns `1.f` (MODERATE) while `GState < 0` [src Source/IronwakeTest/IronwakeHydro.cpp:90].
- `SetSeaState` clamps to 0..4 [src Source/IronwakeTest/IronwakeHydro.cpp:227] and returns early if the change is under `0.01` [src Source/IronwakeTest/IronwakeHydro.cpp:228].
- The wind sea travels toward yaw `120` degrees (ESE; wind from WNW) [src Source/IronwakeTest/IronwakeHydro.cpp:57].

## 3.2 Wave components (fixed set)

`NComp` is the array length of `Comps[]` (derived: 4+2+4+6+2+4 = 22 components) with group counts `{ 4, 2, 4, 6, 2, 4 }` [src Source/IronwakeTest/IronwakeHydro.cpp:70]. Each component is { wavelength m, direction offset from the wind (deg), group } [src Source/IronwakeTest/IronwakeHydro.cpp:60]. Absolute direction = WindYaw + offset [src Source/IronwakeTest/IronwakeHydro.cpp:161]. The "18 components" in the W2 log (build report `W2.md`) is stale: group F (4 waves) was added in W3 [src Source/IronwakeTest/IronwakeHydro.cpp:67] (derived: 18+4 = 22).

| Group | Role | Wavelengths (m) | Direction offsets (deg) | Source |
|---|---|---|---|---|
| A (4) | primary swell 150-250 m | `240` [src Source/IronwakeTest/IronwakeHydro.cpp:62], `205` [src Source/IronwakeTest/IronwakeHydro.cpp:62], `172` [src Source/IronwakeTest/IronwakeHydro.cpp:62], `150` [src Source/IronwakeTest/IronwakeHydro.cpp:62] | `-8` [src Source/IronwakeTest/IronwakeHydro.cpp:62], `6` [src Source/IronwakeTest/IronwakeHydro.cpp:62], `-3` [src Source/IronwakeTest/IronwakeHydro.cpp:62], `11` [src Source/IronwakeTest/IronwakeHydro.cpp:62] | [src Source/IronwakeTest/IronwakeHydro.cpp:62] |
| B (2) | crossing swell, second direction | `185` [src Source/IronwakeTest/IronwakeHydro.cpp:63], `128` [src Source/IronwakeTest/IronwakeHydro.cpp:63] | `62` [src Source/IronwakeTest/IronwakeHydro.cpp:63], `-55` [src Source/IronwakeTest/IronwakeHydro.cpp:63] | [src Source/IronwakeTest/IronwakeHydro.cpp:63] |
| C (4) | wind sea | `128` [src Source/IronwakeTest/IronwakeHydro.cpp:64], `104` [src Source/IronwakeTest/IronwakeHydro.cpp:64], `84` [src Source/IronwakeTest/IronwakeHydro.cpp:64], `66` [src Source/IronwakeTest/IronwakeHydro.cpp:64] | `18` [src Source/IronwakeTest/IronwakeHydro.cpp:64], `-22` [src Source/IronwakeTest/IronwakeHydro.cpp:64], `9` [src Source/IronwakeTest/IronwakeHydro.cpp:64], `-14` [src Source/IronwakeTest/IronwakeHydro.cpp:64] | [src Source/IronwakeTest/IronwakeHydro.cpp:64] |
| D (6) | chop | `31` [src Source/IronwakeTest/IronwakeHydro.cpp:65], `23` [src Source/IronwakeTest/IronwakeHydro.cpp:65], `16.5` [src Source/IronwakeTest/IronwakeHydro.cpp:65], `12` [src Source/IronwakeTest/IronwakeHydro.cpp:65], `9` [src Source/IronwakeTest/IronwakeHydro.cpp:65], `7` [src Source/IronwakeTest/IronwakeHydro.cpp:65] | `35` [src Source/IronwakeTest/IronwakeHydro.cpp:65], `-30` [src Source/IronwakeTest/IronwakeHydro.cpp:65], `12` [src Source/IronwakeTest/IronwakeHydro.cpp:65], `-41` [src Source/IronwakeTest/IronwakeHydro.cpp:65], `25` [src Source/IronwakeTest/IronwakeHydro.cpp:65], `-8` [src Source/IronwakeTest/IronwakeHydro.cpp:65] | [src Source/IronwakeTest/IronwakeHydro.cpp:65] |
| E (2) | ripples | `5` [src Source/IronwakeTest/IronwakeHydro.cpp:66], `3.8` [src Source/IronwakeTest/IronwakeHydro.cpp:66] | `48` [src Source/IronwakeTest/IronwakeHydro.cpp:66], `-36` [src Source/IronwakeTest/IronwakeHydro.cpp:66] | [src Source/IronwakeTest/IronwakeHydro.cpp:66] |
| F (4) | W3 short steep wind sea (GALE/STORM breakers); W4 wider spread so crest lines cross | `58` [src Source/IronwakeTest/IronwakeHydro.cpp:67], `44` [src Source/IronwakeTest/IronwakeHydro.cpp:67], `33` [src Source/IronwakeTest/IronwakeHydro.cpp:67], `24` [src Source/IronwakeTest/IronwakeHydro.cpp:67] | `-28` [src Source/IronwakeTest/IronwakeHydro.cpp:67], `37` [src Source/IronwakeTest/IronwakeHydro.cpp:67], `-48` [src Source/IronwakeTest/IronwakeHydro.cpp:67], `14` [src Source/IronwakeTest/IronwakeHydro.cpp:67] | [src Source/IronwakeTest/IronwakeHydro.cpp:67] |

Component order matters for the shader phase offset (`0.37 * i`, index into `Gen->Waves`) [src Source/IronwakeTest/IronwakeHydro.cpp:123].

### 3.2.1 Per-component conversion (what is written to the plugin wave struct)

- Per-component amplitude `A = Hs_group / (2.83 * sqrt(N_group))` in metres [src Source/IronwakeTest/IronwakeHydro.cpp:156]; `Hs_group` is Lerp5(GroupHs[G], State) * AmpMul [src Source/IronwakeTest/IronwakeHydro.cpp:155].
- `WaveLength = WL * 100` (cm) [src Source/IronwakeTest/IronwakeHydro.cpp:158]; `Amplitude = max(A*100, 0.05)` (cm) [src Source/IronwakeTest/IronwakeHydro.cpp:159].
- `Steepness = StT * SteepShare[G] / GroupN[G]` when `AmpMul > 0.01`, else `0` [src Source/IronwakeTest/IronwakeHydro.cpp:160].
- `GHs = 2.83 * sqrt(sum(A^2))` is the published significant wave height [src Source/IronwakeTest/IronwakeHydro.cpp:165].
- `AmpMul` is `1.f` from SetSeaState [src Source/IronwakeTest/IronwakeHydro.cpp:229] and `0.f` for flat sea (`SetSeaScale(<=0.01)`) [src Source/IronwakeTest/IronwakeHydro.cpp:263].
- Deep-water phase speed used in analytic `FastZ`: `sqrt(K * 980) * T` with K = 2 pi / WL [src Source/IronwakeTest/IronwakeHydro.cpp:317]. Derived periods (T = sqrt(2 pi L / 980 cm/s^2)): 240 m 12.4 s, 150 m 9.8 s, 66 m 6.5 s, 31 m 4.5 s, 7 m 2.1 s, 3.8 m 1.6 s.

## 3.3 Group significant heights (m) per sea state

Rows are groups; columns CALM, MODERATE, ROUGH, GALE, STORM. Interpolated with `Lerp5` [src Source/IronwakeTest/IronwakeHydro.cpp:88].

| Group | CALM | MODERATE | ROUGH | GALE | STORM | Source |
|---|---|---|---|---|---|---|
| A | `0.85` [src Source/IronwakeTest/IronwakeHydro.cpp:73] | `1.5` [src Source/IronwakeTest/IronwakeHydro.cpp:73] | `2.6` [src Source/IronwakeTest/IronwakeHydro.cpp:73] | `4.8` [src Source/IronwakeTest/IronwakeHydro.cpp:73] | `8.4` [src Source/IronwakeTest/IronwakeHydro.cpp:73] | [src Source/IronwakeTest/IronwakeHydro.cpp:73] |
| B | `0.30` [src Source/IronwakeTest/IronwakeHydro.cpp:74] | `0.8` [src Source/IronwakeTest/IronwakeHydro.cpp:74] | `1.3` [src Source/IronwakeTest/IronwakeHydro.cpp:74] | `2.0` [src Source/IronwakeTest/IronwakeHydro.cpp:74] | `3.0` [src Source/IronwakeTest/IronwakeHydro.cpp:74] | [src Source/IronwakeTest/IronwakeHydro.cpp:74] |
| C | `0.18` [src Source/IronwakeTest/IronwakeHydro.cpp:75] | `1.0` [src Source/IronwakeTest/IronwakeHydro.cpp:75] | `2.4` [src Source/IronwakeTest/IronwakeHydro.cpp:75] | `3.8` [src Source/IronwakeTest/IronwakeHydro.cpp:75] | `5.0` [src Source/IronwakeTest/IronwakeHydro.cpp:75] | [src Source/IronwakeTest/IronwakeHydro.cpp:75] |
| D | `0.12` [src Source/IronwakeTest/IronwakeHydro.cpp:76] | `0.6` [src Source/IronwakeTest/IronwakeHydro.cpp:76] | `1.1` [src Source/IronwakeTest/IronwakeHydro.cpp:76] | `1.6` [src Source/IronwakeTest/IronwakeHydro.cpp:76] | `2.2` [src Source/IronwakeTest/IronwakeHydro.cpp:76] | [src Source/IronwakeTest/IronwakeHydro.cpp:76] |
| E | `0.05` [src Source/IronwakeTest/IronwakeHydro.cpp:77] | `0.15` [src Source/IronwakeTest/IronwakeHydro.cpp:77] | `0.25` [src Source/IronwakeTest/IronwakeHydro.cpp:77] | `0.3` [src Source/IronwakeTest/IronwakeHydro.cpp:77] | `0.35` [src Source/IronwakeTest/IronwakeHydro.cpp:77] | [src Source/IronwakeTest/IronwakeHydro.cpp:77] |
| F (W3) | `0.04` [src Source/IronwakeTest/IronwakeHydro.cpp:78] | `0.25` [src Source/IronwakeTest/IronwakeHydro.cpp:78] | `0.75` [src Source/IronwakeTest/IronwakeHydro.cpp:78] | `2.0` [src Source/IronwakeTest/IronwakeHydro.cpp:78] | `3.0` [src Source/IronwakeTest/IronwakeHydro.cpp:78] | [src Source/IronwakeTest/IronwakeHydro.cpp:78] |

Derived total Hs (root-sum-square of the six groups, equals `GHs` at integer states): CALM 0.93 m, MODERATE 2.08 m, ROUGH 4.01 m, GALE 6.94 m, STORM 10.89 m. In-game logs agree: CALM 0.9, MODERATE 2.1, ROUGH 4.0, STORM 10.5 (build report `W2.md`) (pre-F group); later STORM `10.9` m (build report `W3.md`); round-2 stats CALM Hs 0.9, ROUGH 3.9, STORM 10.4 (build report `W2.md`).

Derived per-component amplitudes (cm) at each integer state, groups A..F (from `Hs/(2.83 sqrt N)*100`):

| State | A | B | C | D | E | F |
|---|---|---|---|---|---|---|
| CALM | 15.0 | 7.5 | 3.2 | 1.7 | 1.2 | 0.7 |
| MODERATE | 26.5 | 20.0 | 17.7 | 8.7 | 3.7 | 4.4 |
| ROUGH | 45.9 | 32.5 | 42.4 | 15.9 | 6.2 | 13.3 |
| GALE | 84.8 | 50.0 | 67.1 | 23.1 | 7.5 | 35.3 |
| STORM | 148.4 | 75.0 | 88.3 | 31.7 | 8.7 | 53.0 |

(The lower bound `max(A*100, 0.05)` is never hit at these values.)

## 3.4 Steepness, foam and wind tables per state

| State | Total Gerstner steepness (sum over all waves) | Foam Boost | Foam Height Bias | Mean wind (kn) |
|---|---|---|---|---|
| CALM | `0.16` | `1.2` | `-0.7` | `8` |
| MODERATE | `0.45` | `2.6` | `-0.45` | `16` |
| ROUGH | `0.72` | `5.0` | `-0.15` | `24` |
| GALE | `0.92` | `8.0` | `0.1` | `36` |
| STORM | `1.05` | `11.f` | `0.3` | `48` |

Sources: steepness [src Source/IronwakeTest/IronwakeHydro.cpp:81]; foam boost [src Source/IronwakeTest/IronwakeHydro.cpp:83]; height bias [src Source/IronwakeTest/IronwakeHydro.cpp:84]; wind [src Source/IronwakeTest/IronwakeHydro.cpp:85]. Comment: ~1 total steepness means crests pinch to a point [src Source/IronwakeTest/IronwakeHydro.cpp:80].

Steepness share per group A..F: `0.17`, `0.06`, `0.30`, `0.16`, `0.03`, `0.28` (sum 1.0) [src Source/IronwakeTest/IronwakeHydro.cpp:82]. Derived per-wave steepness (total * share / N): STORM A 0.0446, C 0.0788, D 0.0280, F 0.0735; GALE C 0.069, F 0.0644; CALM C 0.012, F 0.0112.

`Foam Boost` is multiplied by clamp(AmpMul, 0.3, 1) [src Source/IronwakeTest/IronwakeHydro.cpp:172]. `FoamContrast` goes `1.0` to `1.6` over state 0..4 [src Source/IronwakeTest/IronwakeHydro.cpp:174].

### 3.4.1 Wind

- `WindForState(S)` = `Lerp5(WindKnS, S)` [src Source/IronwakeTest/IronwakeHydro.cpp:92]; header comment "Mean wind for a sea state (kn): 8 / 16 / 24 / 36 / 48" [src Source/IronwakeTest/IronwakeHydro.h:51].
- Actual wind used by gameplay is `IronwakeFleet::WindKn`: mean wind times gusts. Gust factor `G = 0.12 + 0.04*clamp(S-2, 0, 2)`; result = WindForState(S) * (1 + G*sin(0.21 T) + 0.08*sin(0.53 T + 1.3)) [src Source/IronwakeTest/IronwakeFleet.cpp:158-159]. Derived gust amplitude: CALM..ROUGH +-12 % (+8 % second term), GALE 16 %, STORM 20 %.
- Wind direction (FROM, UE yaw): 300 + 15*sin(2 pi T/660) + 4*sin(0.071 T) [src Source/IronwakeTest/IronwakeFleet.cpp:149]. Wave travel yaw `120` [src Source/IronwakeTest/IronwakeHydro.cpp:57] is consistent with a mean wind from 300 (derived: 300 - 180 = 120).
- `-iw` tests can freeze wind with `SetFixedWind(FromYaw, Kn)`; fixed defaults `300.f`, `16.f` [src Source/IronwakeTest/IronwakeFleet.cpp:94].
- `GetSeaScale()` = `WindForState / 16` ("wind-equivalent scale", kept for old callers) [src Source/IronwakeTest/IronwakeHydro.cpp:95].
- Rain slants downwind: rain system rotated to `WaveTravelYaw()` [src Source/IronwakeTest/IronwakeStorm.cpp:164].

## 3.5 Water material parameters driven per state

All set in `Apply()` on the ocean MID (`GetWaterMaterialInstance`) [src Source/IronwakeTest/IronwakeHydro.cpp:169]. Material graph internals (`M_IW_WaterRoot` <- `MI_IW_WaterInst` <- `MI_IW_WaterOcean` <- `MI_Ocean_Storm`) are described in the W2 log (build report `W2.md`).

| Parameter | CALM | MODERATE | ROUGH | GALE | STORM | Source |
|---|---|---|---|---|---|---|
| `IW_Whitecap` base | `0.0` [src Source/IronwakeTest/IronwakeHydro.cpp:177] | `0.6` [src Source/IronwakeTest/IronwakeHydro.cpp:177] | `1.0` [src Source/IronwakeTest/IronwakeHydro.cpp:177] | `1.0` [src Source/IronwakeTest/IronwakeHydro.cpp:177] | `1.0` [src Source/IronwakeTest/IronwakeHydro.cpp:177] | [src Source/IronwakeTest/IronwakeHydro.cpp:177] |
| `IW_Streak` base | `0.0` [src Source/IronwakeTest/IronwakeHydro.cpp:178] | `0.0` [src Source/IronwakeTest/IronwakeHydro.cpp:178] | `0.25` [src Source/IronwakeTest/IronwakeHydro.cpp:178] | `0.75` [src Source/IronwakeTest/IronwakeHydro.cpp:178] | `1.0` [src Source/IronwakeTest/IronwakeHydro.cpp:178] | [src Source/IronwakeTest/IronwakeHydro.cpp:178] |
| `IW_CrestK` | `0.6` [src Source/IronwakeTest/IronwakeHydro.cpp:179] | `0.3` [src Source/IronwakeTest/IronwakeHydro.cpp:179] | `0.12` [src Source/IronwakeTest/IronwakeHydro.cpp:179] | `0.06` [src Source/IronwakeTest/IronwakeHydro.cpp:179] | `0.02` [src Source/IronwakeTest/IronwakeHydro.cpp:179] | [src Source/IronwakeTest/IronwakeHydro.cpp:179] |
| Whitecap coverage target (J < JT area) | `0.0f` [src Source/IronwakeTest/IronwakeHydro.cpp:113] | `0.023f` [src Source/IronwakeTest/IronwakeHydro.cpp:113] | `0.095f` [src Source/IronwakeTest/IronwakeHydro.cpp:113] | `0.19f` [src Source/IronwakeTest/IronwakeHydro.cpp:113] | `0.30f` [src Source/IronwakeTest/IronwakeHydro.cpp:113] | [src Source/IronwakeTest/IronwakeHydro.cpp:113] |
| `IW_FoamGlow` [src Source/IronwakeTest/IronwakeHydro.cpp:194] | `0.25` [src Source/IronwakeTest/IronwakeHydro.cpp:194] | `0.35` [src Source/IronwakeTest/IronwakeHydro.cpp:194] | `0.5` [src Source/IronwakeTest/IronwakeHydro.cpp:194] | `0.6` [src Source/IronwakeTest/IronwakeHydro.cpp:194] | `0.65` [src Source/IronwakeTest/IronwakeHydro.cpp:194] | [src Source/IronwakeTest/IronwakeHydro.cpp:194] |

- `IW_Whitecap = Lerp5(Whitecap) * clamp(AmpMul,0,1) * iwfoamdbg` and IW_Streak = Lerp5(Streak) * clamp(AmpMul,0,1) * iwstreakk [src Source/IronwakeTest/IronwakeHydro.cpp:182-184]. Command-line tuning keys: `iwfoamdbg=`, `iwstreakk=`, `iwlacek=`, `iwcovk=`, `iwfoammode=`, `iwplanebias=` [src Source/IronwakeTest/IronwakeHydro.cpp:181-197] (`iwplanebias` has no effect: `IW_PlaneZ` is set to 0 [src Source/IronwakeTest/IronwakeHydro.cpp:198]).
- Whitecap threshold `IW_JT` is calibrated from the Jacobian of the 16 shader waves on a deterministic 64x64 grid so the J < JT area equals the coverage target; coverage below `0.002` returns `-10` (no caps) [src Source/IronwakeTest/IronwakeHydro.cpp:116]. Coverage values were re-set in W4 to `0 / 2.3 / 9.5 / 19 / 30 %` and measured on the full mask as ROUGH 3.6-10 %, STORM 9-24 % (build report `W4.md`).
- The shader gets the 16 waves with the largest steepness (ties: larger amplitude) [src Source/IronwakeTest/IronwakeHydro.cpp:104], encoded per frame as `IW_Wave0..15` = (`dir.x*(1+steep)`, `dir.y*(1+steep)`, `wavelength`, `amplitude`) [src Source/IronwakeTest/IronwakeHydro.cpp:299-300]; unused slots are (1,0,10000,0) [src Source/IronwakeTest/IronwakeHydro.cpp:298]. Per-frame call `SetFoamCamera` also pushes `IW_CamPos`, `IW_WaterTime` [src Source/IronwakeTest/IronwakeHydro.cpp:290-291].
- Lace: `IW_Lace = clamp((State-0.8)/2, 0, 1) * 0.9 * iwlacek`, IW_LaceT = 2.2 [src Source/IronwakeTest/IronwakeHydro.cpp:191].
- Rim colour: `Sk = clamp((State-1)/3, 0, 1)`; Scattering lerps `(0.012,0.016,0.018)` to `(0.006,0.007,0.009)` by `Sk`, then toward night blue `(0.35,0.55,1.0)` by `GNightWater`; Absorption lerps `(0.35,0.08,0.06)` to (0.45,0.2,0.12) [src Source/IronwakeTest/IronwakeHydro.cpp:201-203]. `SetNightWater(K)` repeats the Scattering lerp [src Source/IronwakeTest/IronwakeHydro.cpp:279].
- Wind ripple normals: multiplier `Wk = 1 + 1.8 * clamp(State/4, 0, 1)` (CALM x1 .. STORM x2.8) applied to the captured defaults of `Default Near Normal Strength`, `Default Distant Normal Strength`, `Default Distant Normal StrengthB`, `Far Normal Strength` [src Source/IronwakeTest/IronwakeHydro.cpp:209-215]. The inline comment says `0.9 -> 1.8`, the code is `1.8` factor [src Source/IronwakeTest/IronwakeHydro.cpp:211]; P13 text says "x1..1.9" (build report `P13.md`) and P14 says "up to x2.8 at STORM" (build report `P14.md`). **CONFLICT** (comments/logs 1.9 vs code): the code (x2.8 at STORM) takes effect.
- Wind vector IW_Wind = (cos WindYaw, sin WindYaw) [src Source/IronwakeTest/IronwakeHydro.cpp:219].
- Night water glow `GNightWater` is driven by `SetNightWater(world, clamp(Up*Wn*(0.15+0.85*Clear)*(0.3+0.7*Illum)*Wg, 0, 1))` in the clock tick [src Source/IronwakeTest/IronwakeP11.cpp:277].

## 3.6 Sea-scale (legacy) API

- `SetSeaScale(World, Scale)`: `Scale <= 0.01` = flat sea (`Apply(World, 0, 0)`, `GState = -2`) [src Source/IronwakeTest/IronwakeHydro.cpp:263]. Otherwise `Scale <= 1` maps `(Scale-0.25)/0.75` clamped 0..1 to state 0..1; `Scale > 1` maps `Lerp(1, 2, clamp((Scale-1)/0.6, 0, 1.5))` (so 1.6 gives state 2 = ROUGH; values above 1.6 extend to state up to 2.5) [src Source/IronwakeTest/IronwakeHydro.cpp:264].
- Header summary: scale 1 = MODERATE, 1.6 = ROUGH, 0.65 = CALM-ish, 0 = flat [src Source/IronwakeTest/IronwakeHydro.h:62]. Callers: A1 autopilot test `1.6` [src Source/IronwakeTest/IronwakeAutopilot.cpp:832], W1 tests `1.6` and `1.f` [src Source/IronwakeTest/IronwakeShipHydro.cpp:343].
- `SetSeaBand(World, N, MinWL, MaxWL, MinA, MaxA)` is test-only: replaces the spectrum by N evenly spaced waves with total steepness `0.2 / N`, directions alternating by `25 * U` degrees; `GState = -2` forces the next `SetSeaState` to restore the normal sea [src Source/IronwakeTest/IronwakeHydro.cpp:249-255].
- Legacy weather presets in `AIronwakeVoyage` (used by test directors): Day `1`, Dusk `0`, Night `2`, Gale `3`, Storm `4` [src Source/IronwakeTest/IronwakeVoyage.cpp:666]; the clear moonlit night preset has `Sea = 0.5f` [src Source/IronwakeTest/IronwakeVoyage.cpp:714]. Preset assignment order: Day 644, Dusk 648, Night 652, Gale 656, Storm 660, clear night 664 [src Source/IronwakeTest/IronwakeVoyage.cpp:644-666].
- The W1 sea-scale table (scale 1.0 swell ~3.5 m, 0.65 ~2.3 m, 1.6 ~5.5 m) described a 24-wave spectrum (build report `W1.md`) that no longer exists; the W2 generator replaced it (chapter history: W2). Treat W1 sea heights as superseded.

## 3.7 Weather -> sea state (the director)

`AIronwakeStorm` (one per world) turns the open-sea target into the state at the player:

1. `SetTarget(State, Seconds)` clamps `0..4`, starts a smooth-step blend over max(Seconds, 0.05) [src Source/IronwakeTest/IronwakeStorm.cpp:97]; `Open` follows [src Source/IronwakeTest/IronwakeStorm.cpp:127]. `SetOpenImmediate` skips the blend (live weather already smooths) [src Source/IronwakeTest/IronwakeStorm.h:44].
2. Want = Open * LandFade(ShipLoc) [src Source/IronwakeTest/IronwakeStorm.cpp:136]; `Local` follows with FInterpTo(..., rate 0.5) [src Source/IronwakeTest/IronwakeStorm.cpp:137].
3. `SetSeaState(W, Local)` is called only if |Local - LastApplied| > 0.02 [src Source/IronwakeTest/IronwakeStorm.cpp:138-140].
4. Per frame `SetFoamCamera` and `TickAtmosphere` run [src Source/IronwakeTest/IronwakeStorm.cpp:143-144]; Tick delta is capped at `0.1` s [src Source/IronwakeTest/IronwakeStorm.cpp:126].
5. `Storminess() = clamp((Local - 2)/2, 0, 1)` (0 below ROUGH, 1 at STORM) [src Source/IronwakeTest/IronwakeStorm.h:38].

### 3.7.1 Live weather states (clock system, `IronwakeP11.cpp`)

The living-world clock feeds `Cur.Sea`; `StateWx` gives per-state values [src Source/IronwakeTest/IronwakeP11.cpp:46]:

| Weather | Cloud cover | Rain | Open-sea state | Source |
|---|---|---|---|---|
| clear | `0.02` [src Source/IronwakeTest/IronwakeP11.cpp:51] | `0.f` [src Source/IronwakeTest/IronwakeP11.cpp:51] | `0.55` [src Source/IronwakeTest/IronwakeP11.cpp:51] | [src Source/IronwakeTest/IronwakeP11.cpp:51] |
| fair | `0.30` [src Source/IronwakeTest/IronwakeP11.cpp:52] | `0.f` [src Source/IronwakeTest/IronwakeP11.cpp:52] | `0.95` [src Source/IronwakeTest/IronwakeP11.cpp:52] | [src Source/IronwakeTest/IronwakeP11.cpp:52] |
| overcast | `0.88` [src Source/IronwakeTest/IronwakeP11.cpp:53] | `0.f` [src Source/IronwakeTest/IronwakeP11.cpp:53] | `1.7` [src Source/IronwakeTest/IronwakeP11.cpp:53] | [src Source/IronwakeTest/IronwakeP11.cpp:53] |
| mist | `0.65` [src Source/IronwakeTest/IronwakeP11.cpp:54] | `0.f` [src Source/IronwakeTest/IronwakeP11.cpp:54] | `0.3` [src Source/IronwakeTest/IronwakeP11.cpp:54] | [src Source/IronwakeTest/IronwakeP11.cpp:54] |
| rain | `0.92` [src Source/IronwakeTest/IronwakeP11.cpp:55] | `0.7` [src Source/IronwakeTest/IronwakeP11.cpp:55] | `2.35` [src Source/IronwakeTest/IronwakeP11.cpp:55] | [src Source/IronwakeTest/IronwakeP11.cpp:55] |
| squall | `0.97` [src Source/IronwakeTest/IronwakeP11.cpp:56] | `1.f` [src Source/IronwakeTest/IronwakeP11.cpp:56] | `3.0` [src Source/IronwakeTest/IronwakeP11.cpp:56] | [src Source/IronwakeTest/IronwakeP11.cpp:56] |

- `Cur.Sea` = `SeaOverride` if >= 0 else `LiveCur.Sea` [src Source/IronwakeTest/IronwakeP11.cpp:285]; sent via `SetOpenImmediate(Cur.Sea)` [src Source/IronwakeTest/IronwakeP11.cpp:294].
- The P11 log lists overcast 1.4, rain 1.9, squall 2.7 (build report `P11.md`) and the P13 log says 1.7 / 2.35 / 3.0 (build report `P13.md`). **CONFLICT**: the code values above (1.7 / 2.35 / 3.0) take effect; P11 log is stale. The live system never reaches STORM; the comment says "0 CALM .. 3 GALE; never the parked STORM look" [src Source/IronwakeTest/IronwakeP11.cpp:45].
- HUD sea word thresholds on `GetLocalState`: calm `< 0.3`, slight `< 0.8`, moderate `< 1.5`, rough `< 2.4`, else very rough [src Source/IronwakeTest/IronwakeP11.cpp:76].
- Spawn/storm hooks: title-menu STORM button and `-iwstorm` start ~5.5 km out of Greyhaven already in storm (build report `W2.md`); K toggles drift, L forces next weather (build report `P11.md`).

### 3.7.2 Shelter / LandFade

`LandFade(P)` returns `1` at open sea and less near land; it is a multiplier on the open-sea state, taking the minimum over all sheltering terms [src Source/IronwakeTest/IronwakeStorm.cpp:101-118].

| Term | Geometry | Max shelter | Fade | Source |
|---|---|---|---|---|
| Harbour basin box | harbour frame rotated `45` [src Source/IronwakeTest/IronwakeStorm.cpp:111-114] deg about `HarbourC` [src Source/IronwakeTest/IronwakeStorm.cpp:111-114]; box x `-50` [src Source/IronwakeTest/IronwakeStorm.cpp:111-114]..`150` [src Source/IronwakeTest/IronwakeStorm.cpp:111-114] m, y `-160` [src Source/IronwakeTest/IronwakeStorm.cpp:111-114]..`170` [src Source/IronwakeTest/IronwakeStorm.cpp:111-114] m | state x `0.6` (code: `1 - 0.4`) | `SmoothStep(0, 250 m, D)` from the box edge | [src Source/IronwakeTest/IronwakeStorm.cpp:106-109] |
| Breakwater/stacks point | centre `StacksC` [src Source/IronwakeTest/IronwakeStorm.cpp:50], radius `50000` [src Source/IronwakeTest/IronwakeStorm.cpp:50] cm | x `0.7` | `SmoothStep(R, R + 30000, D)` | [src Source/IronwakeTest/IronwakeStorm.cpp:50] |
| St Brannoc island | `IslandCentre` [src Source/IronwakeTest/IronwakeStorm.cpp:54], radius `70000` [src Source/IronwakeTest/IronwakeStorm.cpp:54] cm | x `0.7` | same 300 m | [src Source/IronwakeTest/IronwakeStorm.cpp:54] |
| Terrain isles | every `FIsle` [src Source/IronwakeTest/IronwakeStorm.cpp:56] with `bTerrain` [src Source/IronwakeTest/IronwakeStorm.cpp:56], radius `I.R * 1.2` [src Source/IronwakeTest/IronwakeStorm.cpp:56] | x `0.7` | same 300 m | [src Source/IronwakeTest/IronwakeStorm.cpp:56] |

- Shore terms: `Lerp(1 - 0.3, 1, U)` with comment "0.3 max, gone 300 m past the shore" [src Source/IronwakeTest/IronwakeStorm.cpp:115-116].
- `HarbourC` default `(-345000, -335000, 0)` cm [src Source/IronwakeTest/IronwakeStorm.h:80] and `AIronwakeVoyage::HarbourC` is the same value [src Source/IronwakeTest/IronwakeVoyage.h:183]; `StacksC` (110000, -90000, 0) [src Source/IronwakeTest/IronwakeVoyage.h:185]; `IslandCentre` (-50000, 190000, 0) [src Source/IronwakeTest/IronwakeLivingSea.h:121]. `BeginPlay` overwrites `HarbourC` from the voyage actor [src Source/IronwakeTest/IronwakeStorm.cpp:50].
- **CONFLICT (comments vs code)**: the BeginPlay comment says the basin shelters "at most 0.6 and fades out over ~400 m" [src Source/IronwakeTest/IronwakeStorm.cpp:48] and the LandFade comment says "fades in 400 m" [src Source/IronwakeTest/IronwakeStorm.cpp:104]. The code lerps the multiplier from `1 - 0.4` (= 0.6 of the open state at the box edge) up to 1 over a `250` m smoothstep [src Source/IronwakeTest/IronwakeStorm.cpp:109]. The code takes effect: minimum multiplier 0.6, 250 m fade. (P13 logged it as "shelter 0.6 -> 0.4 max, 250 m fade" (build report `P13.md`), i.e. the reduction went from 0.6 to 0.4.) The 400 m in the comments is stale.
- Measured Hs at MODERATE: basin 1.0 m vs offshore 2.1 m (build report `P11.md`).
- Early W2 log said the fade went to 15 % at Greyhaven (build report `W2.md`); superseded by the code above.

### 3.7.3 Harbour calm slab (P14)

- Problem: the ocean is one global spectrum, so a state-0.6 harbour still shows the open-sea chop of state 1+. Fix: a flat slab (`Block(-48, 140, -150, 158, 0.05, 0.42)` in harbour-frame metres: x `-48`..`140`, y `-150`..`158`, z `0.05`..`0.42`) carrying the ocean's own material with no Gerstner height or crest foam [src Source/IronwakeTest/IronwakeP12.cpp:106]. Material `MI_IW_WaterInst`, fallback path `/Game/Ironwake/Water/MI_IW_WaterInst`; if none loads it falls back to a dark tint (0.004, 0.012, 0.022) [src Source/IronwakeTest/IronwakeP12.cpp:111-113].
- Always created invisible, no collision, no shadow; `GBasinSlab` holds it [src Source/IronwakeTest/IronwakeP12.cpp:112-113]. `IronwakeSetBasinCalm(bool)` toggles visibility [src Source/IronwakeTest/IronwakeP12.cpp:22].
- Gate: visible when `Cur.Sea > 0.75` [src Source/IronwakeTest/IronwakeP11.cpp:286]. Slab (x -48..140) is narrower than the shelter box (x -50..150) [src Source/IronwakeTest/IronwakeStorm.cpp:107], so a thin strip at each end shows open-sea waves.
- Vision result: harbour high shot 1/10 -> `9` (build report `P14.md`).

## 3.8 Ocean and zone extents

- Original A card: ocean extents, collision extents and WaterZone `zone_extent` `4000` m (8 km tile), render target `1024` (build report `A.md`).
- I1: raised to `8.4` km (build report `I1.md`).
- P5c: raised ocean extents and zone extent to `300` km (`IWI1_SETEXT=30000`) to remove the seam at the end of the tessellated sea (build report `P5c.md`); the 300 km extent "costs nothing measurable" (build report `P7.md`). SPEC.md also says 300 km [src SPEC.md:28].
- Script to read/set: `iw_i1_water.py` reads `zone_extent`, `render_target_resolution`, `enable_local_only_tessellation`, `local_tessellation_extent` [src Content/Python/iw_i1_water.py:10] and `ocean_extents`, `collision_extents`, `height_offset` [src Content/Python/iw_i1_water.py:20]; `IWI1_SETEXT` is metres full width (x100 to cm) [src Content/Python/iw_i1_water.py:15].
- The night-sky dome is radius 400 km, "beyond the 300 km ocean" [src Source/IronwakeTest/IronwakeP8.cpp:33].
- **CONFLICT (stale constants)**: `AIronwakeGameMode` still documents "The tile is +/-4000 m" with `SpawnBoxHalfCm = 370000` [src Source/IronwakeTest/IronwakeGameMode.h:36-38] and the sea spawn `(-300000, -270000, 150)` "in the SW corner of the 8 km tile" [src Source/IronwakeTest/IronwakeGameMode.h:42]; the combat flee code bends back inside `300000` cm "on the 8 km ocean tile" [src Source/IronwakeTest/IronwakeCombat.cpp:524]. The map value written by the extent script (300 km, P5c) takes effect for rendering; the 3-3.7 km constants are gameplay limits that still apply for spawn and AI flee. The saved `.umap` is binary and was not read: actual saved extent = `300 km` per P5c log, verify with `iw_i1_water.py` (read-only mode).
- Far-distance water mesh beyond the tessellated tile carries the sea to the horizon (build report `A.md`); P5c noted "if the seam persists look at the far-distance water mesh material" (build report `P5c.md`). Water-mesh tessellation bias: see chapter 11.
- Old experiment scripts (20 km extents) `build_storm_v2.py` are superseded [src Content/Python/build_storm_v2.py:61].

## 3.9 Height queries (what "sea level" means in code)

| API | What it returns | Source |
|---|---|---|
| `AIronwakeShell::WaterZAt(World, P)` | flat water plane at XY (`GetWaterSurfaceInfoAtLocation(..., false)`, no waves); `-350` if no ocean | [src Source/IronwakeTest/IronwakeShell.cpp:74] |
| `AIronwakeShell::WaterZAtWaves(World, P)` | plugin query with `IncludeWaves` [src Source/IronwakeTest/IronwakeShell.cpp:94]; falls back to `WaterZAt` | [src Source/IronwakeTest/IronwakeShell.cpp:94] |
| `IronwakeSea::RenderedZ(World, P)` | `WaterZAtWaves + RenderedSeaOffsetCm` | [src Source/IronwakeTest/IronwakeHydro.cpp:97] |
| `IronwakeSea::FastZ(World, P, &J)` | analytic sum of the generator's waves (no query) + per-frame calibration offset; `J` [src Source/IronwakeTest/IronwakeHydro.cpp:306-332] = Jacobian-like crest sharpness (1 flat, `< 0.6` sharp) | [src Source/IronwakeTest/IronwakeHydro.cpp:306-332] |

- `RenderedSeaOffsetCm = 0.f`. R1 believed the render sat 4.7 m under the query; the gauge reader was counting post bands seen through translucent water; re-measured render == query [src Source/IronwakeTest/IronwakeShell.h:27-30]. Numbers: first (wrong) gauge reading rendered-minus-query mean +0.24 m sd 0.99 m with the old `-470` offset (build report `W2.md`); fixed reader rendered minus query `-0.08` m sd `0.09` (build report `W3.md`). The W2 text "RenderedSeaOffsetCm = -470" (build report `W2.md`) is obsolete (offset is 0).
- `FastZ` analytic calibration: once per frame (`GFrameCounter`), `GFastOff = RenderedZ(P) - Raw(P)` at the first sample of that frame; all later samples that frame reuse the same offset [src Source/IronwakeTest/IronwakeHydro.cpp:325-331]. Without generator/subsystem it falls back to `RenderedZ` with `J = 1` [src Source/IronwakeTest/IronwakeHydro.cpp:327].
- `FastZ` omits the Gerstner horizontal displacement (it evaluates `Z += A*cos(phase)` only, the cos phase uses the undisplaced XY) [src Source/IronwakeTest/IronwakeHydro.cpp:317-319]; the plugin query includes it. Gauge: rendered min `-5.4` max `+12.0` sd `2.43` vs FastZ min `-11.6` max `+1.6` sd `2.55` at one STORM post (pre-fix, with the old 4.7 m error) (build report `W2.md`). Treat FastZ vs query residual under the fixed offset as unknown (re-measure with `w2run.sh gauge`).
- Gerstner phase matches the plugin (`K*x - sqrt(K*980)*T`) and T = WaterSubsystem->GetWaterTimeSeconds() [src Source/IronwakeTest/IronwakeHydro.cpp:311].
- Archipelago uses the flat plane + `SeaOffCm = 0.f` for island anchors [src Source/IronwakeTest/IronwakeArchipelago.h:71]; surf ribbons lift `SurfLiftCm = 70` [src Source/IronwakeTest/IronwakeArchipelago.h:72] and anchor at `WaterZAt` [src Source/IronwakeTest/IronwakeArchipelago.cpp:228]. Collision caches sea z once from WaterZAt(0,0,0) [src Source/IronwakeTest/IronwakeCollision.cpp:194].
- Shell water impact test uses the flat `WaterZAt` only after `Age > 0.15` s [src Source/IronwakeTest/IronwakeShell.cpp:128-130], although the class comment says shells miss "below the live Gerstner water surface" [src Source/IronwakeTest/IronwakeShell.h:12-13]. **CONFLICT**: code (flat plane) takes effect; a shell can pass through a crest above the plane or splash early in a trough.
- Plugin query cost: each call is `TryQueryWaterInfoClosestToWorldLocation` [src Source/IronwakeTest/IronwakeShell.cpp:94]; per-ship per-frame query counts are estimated in chapter 11.

## 3.10 Hull-water contact (hydrostatics)

The plugin `UBuoyancyComponent` is created but disabled: pontoons reset, tick disabled, deactivated, gravity off [src Source/IronwakeTest/IronwakeShipPawn.cpp:229-232]. `AIronwakeShipPawn::TickHydro` replaces it [src Source/IronwakeTest/IronwakeShipHydro.cpp:41].

### 3.10.1 Waterplane fit

- Samples: `NStations = 9` along the keel, `3` across (centre and both sides at `j * HB * 0.8` m), via WaterZAtWaves [src Source/IronwakeTest/IronwakeShipHydro.cpp:31]; across offset `0.8` [src Source/IronwakeTest/IronwakeShipHydro.cpp:61]; sample weight = local waterline half-breadth `HB` (minimum `0.3` m) x `2` for the centre line [src Source/IronwakeTest/IronwakeShipHydro.cpp:57-64].
- Station position `U = (i + 0.5) / NStations` between `X0` and `X1` of the hull profile [src Source/IronwakeTest/IronwakeShipHydro.cpp:55-56].
- Least-squares plane gives `SeaFitZ` (at body origin), slope along keel `SlopeX` and across `SlopeY` (cm per m); `SeaFitPitch = atan(SlopeX/100)`, SeaFitRoll = -atan(SlopeY/100) [src Source/IronwakeTest/IronwakeShipHydro.cpp:84-85]. Short chop averages out over a 125 m hull; only swell longer than the hull moves it [src Source/IronwakeTest/IronwakeShipHydro.cpp:37].
- Wave yaw moment (A1): lateral slope vs keel station twist, `Raw = clamp(-Twist * Length * WaveYawGain, -1.5, 1.5)` deg/s, low-passed with tau `2` s [src Source/IronwakeTest/IronwakeShipHydro.cpp:81-82]; `WaveYawGain = 3.f` [src Source/IronwakeTest/IronwakeShipPawn.h:513]. NPCs ignore it [src Source/IronwakeTest/IronwakeShipPawn.cpp:739].
- Stem submerge: rendered sea at a point `(X1 - 3)` m forward and `HullMeshZ + BowDeckM*100` high; `BowSubmergeM = (BowSea - StemP.Z)/100` (green water when > 0) [src Source/IronwakeTest/IronwakeShipHydro.cpp:88-90].

### 3.10.2 Spring model (heave / pitch / roll)

`Spring(X, V, Target, Period, Zeta)`: `A = wn^2 (Target - X) - 2 zeta wn V`, semi-implicit Euler, sub-stepped to `0.02` s, max `8` sub-steps [src Source/IronwakeTest/IronwakeShipHydro.cpp:101-112].

| Quantity | Target | Period / zeta | Source |
|---|---|---|---|
| Heave Z | `SeaFitZ - StaticSeaBodyZ + 45 * clamp(State-2, 0, 2)` cm | `HeavePeriod * Lerp(1, 0.6, SeaQ)`, `HeaveZeta` [src Source/IronwakeTest/IronwakeShipHydro.cpp:115-117] | [src Source/IronwakeTest/IronwakeShipHydro.cpp:115-117] |
| Pitch | `SeaFitPitch*0.95 - Squat - DigDeg` | `PitchPeriod * Lerp(1, 0.6, SeaQ)`, `PitchZeta` [src Source/IronwakeTest/IronwakeShipHydro.cpp:130] | [src Source/IronwakeTest/IronwakeShipHydro.cpp:130] |
| Roll | `SeaFitRoll*0.55 + Heel` | `RollPeriod` [src Source/IronwakeTest/IronwakeShipHydro.cpp:144], `RollZeta` [src Source/IronwakeTest/IronwakeShipHydro.cpp:144] | [src Source/IronwakeTest/IronwakeShipHydro.cpp:144] |

- `SeaQ = clamp((State-2)/2, 0, 1)`: at STORM heave and pitch periods shrink to 0.6 x [src Source/IronwakeTest/IronwakeShipHydro.cpp:116].
- Ride-height lift above plane: `45` cm per state above ROUGH, max `90` cm at GALE+ [src Source/IronwakeTest/IronwakeShipHydro.cpp:115] (W3 log: storm ride lift reduced `2.75 -> 0.9` m (build report `W3.md`)).
- Bow dig (only at telegraph index >= 5 = FULL): `DigDeg = 1.0 * clamp(CrestOverPlane/1.2, 0, 1) * clamp(SpeedFrac, 0, 1)` where `CrestOverPlane` is the rendered sea at `X1 - 4` m minus the fitted plane there; squat 0.25 * SpeedFrac [src Source/IronwakeTest/IronwakeShipHydro.cpp:124-129]. SpeedFrac = clamp(Kn/13, 0, 1.3) [src Source/IronwakeTest/IronwakeShipHydro.cpp:121]. (W1 log said 2.5 deg dig (build report `W1.md`); code max is `1.0` deg after the A1 'looks like it's sinking' fix [src Source/IronwakeTest/IronwakeShipHydro.cpp:126].)
- Sail heel (frigate): `SailHeelDeg = clamp(7 * SailFill * sin(TWA) * min(WindF^2, 2.2) * SailPlanFrac/0.9, 0, 8)`; turn heel DesiredYawRate * 1.1 [src Source/IronwakeTest/IronwakeShipHydro.cpp:131-141].
- Output clamps: pitch `+-14`, roll `+-10` deg [src Source/IronwakeTest/IronwakeShipHydro.cpp:145-146]. Vertical velocity is written as `HeaveVel`; angular velocity kept only about Z [src Source/IronwakeTest/IronwakeShipHydro.cpp:149-151].
- First call teleports to the fit with no spring transient (`bHydroInit`) [src Source/IronwakeTest/IronwakeShipHydro.cpp:93-99].
- Static sea reference on the body: `StaticSeaBodyZ = -70` cm [src Source/IronwakeTest/IronwakeShipPawn.h:479] (= `PontoonZCm -150` + 0.1 x `PontoonRadiusCm 800`) [src Source/IronwakeTest/IronwakeShipPawn.cpp:49-50].

### 3.10.3 Natural periods per hull

| Hull | Heave period / zeta | Pitch period / zeta | Roll period / zeta | Source |
|---|---|---|---|---|
| Ironclad (Devastation, pawn defaults; Kracken NPC also) | `6.5f` [src Source/IronwakeTest/IronwakeShipPawn.h:480] / `0.5f` [src Source/IronwakeTest/IronwakeShipPawn.h:480] | `6.8f` [src Source/IronwakeTest/IronwakeShipPawn.h:481] / `0.55f` [src Source/IronwakeTest/IronwakeShipPawn.h:481] | `10.f` [src Source/IronwakeTest/IronwakeShipPawn.h:482] / `0.4f` [src Source/IronwakeTest/IronwakeShipPawn.h:482] | [src Source/IronwakeTest/IronwakeShipPawn.h:480-484] |
| Frigate (Albatross) | `5.5f` [src Source/IronwakeTest/IronwakeFleet.cpp:33] / `0.5f` [src Source/IronwakeTest/IronwakeFleet.cpp:33] | `5.5f` [src Source/IronwakeTest/IronwakeFleet.cpp:33] / `0.55f` [src Source/IronwakeTest/IronwakeFleet.cpp:33] | `8.f` [src Source/IronwakeTest/IronwakeFleet.cpp:33] / `0.4f` [src Source/IronwakeTest/IronwakeFleet.cpp:33] | [src Source/IronwakeTest/IronwakeFleet.cpp:33] |
| Cutter (Kittiwake) | `3.2f` [src Source/IronwakeTest/IronwakeFleet.cpp:50] / `0.5f` [src Source/IronwakeTest/IronwakeFleet.cpp:50] | `3.0f` [src Source/IronwakeTest/IronwakeFleet.cpp:67] / `0.5f` [src Source/IronwakeTest/IronwakeFleet.cpp:50] | `4.5f` [src Source/IronwakeTest/IronwakeFleet.cpp:66] / `0.4f` [src Source/IronwakeTest/IronwakeFleet.cpp:50] | [src Source/IronwakeTest/IronwakeFleet.cpp:50] |

### 3.10.4 Draft (boot-top vs sea) and hull mesh height

- Mesh z on the body: `HullMeshZ = PontoonZCm + 0.1*Radius + RenderedSeaOffsetCm - DraftBelowBootTopCm` for the ironclad/NPC [src Source/IronwakeTest/IronwakeShipPawn.cpp:340]; fleet hulls use -150 + 0.1*800 + offset - Draft [src Source/IronwakeTest/IronwakeShipFleet.cpp:52].
- `DraftBelowBootTopCm` (negative = boot-top shows above the sea): frigate `-40.f` [src Source/IronwakeTest/IronwakeFleet.cpp:34], cutter `-25.f` [src Source/IronwakeTest/IronwakeFleet.cpp:68], ironclad `-50.f` [src Source/IronwakeTest/IronwakeFleet.cpp:82]. Pawn default `-60.f` [src Source/IronwakeTest/IronwakeShipPawn.h:345]. Resulting boot-top over sea 0.41 / 0.23 / 0.60 m (frigate / cutter / ironclad) (build report `W3.md`). The pawn default `-60` applies to the Kracken NPC (no roster entry).
- W1 draft measurement (superseded reference): boot-top `0.46-0.54` m under rendered sea, keel draft `11.3` m, measured against the then-wrong -4.7 m offset (build report `W1.md`); ignore.
- Frigate mesh is shifted so its waterplane centre is the body origin: `HullMesh` relative x = `-Xc`, profile `X0/X1` shifted by Xc/100 [src Source/IronwakeTest/IronwakeShipFleet.cpp:54-56].

### 3.10.5 Hull profiles (waterline half-breadth, m)

`FIronwakeHullProfile`: `X0..X1` (m, stern..stem; hull-local, +X bow), `HalfB[25]` evenly spaced, `BowDeckM` (forecastle deck over boot-top near stem), `KeelM`; measured in Blender between 0.5 m below and 2.5 m above the boot-top [src Source/IronwakeTest/IronwakeHydro.h:13-22]. `HalfBreadthAt` linearly interpolates and returns 0 beyond X0-0.5 / X1+0.5 [src Source/IronwakeTest/IronwakeHydro.cpp:39-42]. Used for waterplane weighting, bow-wave clearance, and spray offsets.

| Hull | X0 | X1 | Max half-breadth | BowDeckM | KeelM | Source |
|---|---|---|---|---|---|---|
| Devastation (player ironclad) | `-62.5f` [src Source/IronwakeTest/IronwakeHydro.cpp:23] | `59.2f` [src Source/IronwakeTest/IronwakeHydro.cpp:23] | `12.89f` [src Source/IronwakeTest/IronwakeHydro.cpp:23] | `3.2f` [src Source/IronwakeTest/IronwakeHydro.cpp:23] | `10.8f` [src Source/IronwakeTest/IronwakeHydro.cpp:23] | [src Source/IronwakeTest/IronwakeHydro.cpp:23] |
| Kracken (NPC enemy) | `-56.6f` [src Source/IronwakeTest/IronwakeHydro.cpp:27] | `60.f` [src Source/IronwakeTest/IronwakeHydro.cpp:27] | `10.01f` [src Source/IronwakeTest/IronwakeHydro.cpp:27] | `5.0f` [src Source/IronwakeTest/IronwakeHydro.cpp:27] | `7.5f` [src Source/IronwakeTest/IronwakeHydro.cpp:27] | [src Source/IronwakeTest/IronwakeHydro.cpp:27] |
| Goliath (tug, merchant) | `-25.f` [src Source/IronwakeTest/IronwakeHydro.cpp:31] | `24.9f` [src Source/IronwakeTest/IronwakeHydro.cpp:31] | `4.46f` [src Source/IronwakeTest/IronwakeHydro.cpp:31] | `2.5f` [src Source/IronwakeTest/IronwakeHydro.cpp:31] | `5.0f` [src Source/IronwakeTest/IronwakeHydro.cpp:31] | [src Source/IronwakeTest/IronwakeHydro.cpp:31] |
| Aglae (cutter/launch) | `-14.94f` [src Source/IronwakeTest/IronwakeHydro.cpp:35] | `15.f` [src Source/IronwakeTest/IronwakeHydro.cpp:35] | `1.92f` [src Source/IronwakeTest/IronwakeHydro.cpp:35] | `1.2f` [src Source/IronwakeTest/IronwakeHydro.cpp:35] | `1.6f` [src Source/IronwakeTest/IronwakeHydro.cpp:35] | [src Source/IronwakeTest/IronwakeHydro.cpp:35] |
| Frigate (Albatross) | `-40.51f` [src Source/IronwakeTest/IronwakeFleet.cpp:217-219] | `26.47f` [src Source/IronwakeTest/IronwakeFleet.cpp:217-219] | `8.21f` [src Source/IronwakeTest/IronwakeFleet.cpp:218-220] | `6.0f` [src Source/IronwakeTest/IronwakeFleet.cpp:218-220] | `4.2f` [src Source/IronwakeTest/IronwakeFleet.cpp:219-221] | [src Source/IronwakeTest/IronwakeFleet.cpp:200-202] |

(Frigate line 198 carries the 25 samples, max `8.21`; Devastation `HalfB` first `3.49f`, last `3.55f`.) Roster hull lengths/beams: frigate `88` m x `16.4` m, cutter `30` m x `3.9` m, ironclad `125` m x `25.8` m [src Source/IronwakeTest/IronwakeFleet.cpp:30]; cutter [src Source/IronwakeTest/IronwakeFleet.cpp:46]; ironclad [src Source/IronwakeTest/IronwakeFleet.cpp:62]. NPC Kracken is `120` m x `20` m [src Source/IronwakeTest/IronwakeShipPawn.cpp:310].

### 3.10.6 Measured sea-keeping (superseded spectra; indicative only)

| Run | Result | Source |
|---|---|---|
| W1 short chop only (7-35 m) | hull heave / sea heave ratio `0.05` (build report `W1.md`) | (build report `W1.md`) |
| W1 long swell only (100-200 m) | ratio `0.96` (build report `W1.md`) | (build report `W1.md`) |
| W1 moderate full spectrum stopped | ratio `0.36` (build report `W1.md`) | (build report `W1.md`) |
| W1 settle after 2.5 m / 4 deg kick | `4.7` (build report `W1.md`) s | (build report `W1.md`) |
| W2 round 2 STORM | Hs `10.4` (build report `W2.md`) m, hull heave sd `2.99` (build report `W2.md`) m, pitch sd `3.0` (build report `W2.md`) deg, roll sd `2.9` (build report `W2.md`) deg | (build report `W2.md`) |
| W3 STORM | Hs `10.9` (build report `W3.md`) m, hull heave `2.1` (build report `W3.md`) m, pitch `3.2` (build report `W3.md`), roll `2.0` (build report `W3.md`) deg | (build report `W3.md`) |

Measurement hook: `-iww1` director measures bands via `SetSeaBand` (needs `SetSeaBand`/`SetSeaScale` restore) [src Source/IronwakeTest/IronwakeShipHydro.cpp:376-384].

## 3.11 Other floaters

- Merchants (`AIronwakeMerchant`): kinematic. Four samples (bow `0.42*L`, stern, both beams `HalfB = 190` Aglae / `440` Goliath cm) by `WaterZAtWaves`; centre height = mean of four, `FInterpTo` rate `1.5`; target pitch = slope `* 0.8` (`FInterpTo 1.2`); target roll = beam slope `* 0.35` clamped `+-6` deg (`FInterpTo 0.8`) [src Source/IronwakeTest/IronwakeLivingSea.cpp:194-204]. Hydro hull profiles Goliath/Aglae exist for them but are not wired (W1 open item 3) (build report `W1.md`). Merchant speed `690` cm/s (9 kn x 1.5) [src Source/IronwakeTest/IronwakeLivingSea.h:41].
- Islands' floating debris (`Floats`): updated only within `200000` cm of the camera, `FInterpTo` rate `1.5` toward SeaZ + keel offset [src Source/IronwakeTest/IronwakeArchipelago.cpp:1008-1015].
- Speed compression: the sim speed is nominal knots x SpeedCompression 1.5 [src Source/IronwakeTest/IronwakeShipPawn.h:95]; `KnotToCmS = 51.4444` [src Source/IronwakeTest/IronwakeShipPawn.cpp:42]. All speed-gated water FX use nominal knots (`GetSpeedKnots`), wake advection uses compressed m/s (chapter 4).

## Gaps / unknown

- Actual saved ocean/zone extents in the `.umap`: unknown (binary). Measure: `iw_i1_water.py` in read-only mode (no `IWI1_SETEXT`) logs `IWI1W` lines.
- Plugin Gerstner internals (displacement formula, steepness normalisation, `maxWaveHeight`): not in repo; `GetMaxWaveHeight()` is logged by `IWW2 sea state` lines [src Source/IronwakeTest/IronwakeHydro.cpp:232]. Per-state max crest values are unknown in this spec; read them from a run log.
- Whether `FastZ` residual vs the plugin query is small under the fixed offset: unknown. Measure with `build/tools/w2run.sh gauge` and `gaugeread.py`.
- Water material graph (`M_IW_WaterRoot`, `IW_Foam` custom node): defined in `.uasset`; build scripts are `Content/Python/w2_foammat.py`, `w3_foammat.py`, `w4_foammat.py`; their internals are not reproduced here.
- Sea-state behaviour of the sea-state-to-wind-to-ship systems for NPC ships (they use the same hydro): per-hull sea-keeping beyond W1/W2/W3 numbers above is unmeasured.
- Where the "Thresh"/"Contrast" for wake foam come from at asset level is chapter 4.

## 3.12 P16: measured swell, livelier hydro (2026-11-11)

- Measurement (`-iwp16=measure`, logs/P16.md): analytic `IronwakeSea::AnalyticZ` (no calibration) vs the water-body query, 10 points x 120 s x 10 Hz: peak-to-peak CALM `0.97` m, MODERATE `2.34`, ROUGH `4.82`, GALE `8.78`; dominant period `9.5`-`11` s; correlation `1.000`; ratio `1.00` [src logs/P16.md]. The rendered waterline read from pixels on a striped post follows the query within ~0.3 m [src logs/P16.md]. The sea always had the amplitude; it read flat because of long period swell and an under-responsive hull.
- New API `AnalyticZ(World, P)` returns the uncalibrated Gerstner sum plus the flat plane [src Source/IronwakeTest/IronwakeHydro.cpp:AnalyticZ].
- Waterplane fit window: stations are placed over the central `FitSpan` of the hull (`0.6` ironclad, `0.7` frigate, `1.0` cutter) so 60-100 m waves register [src Source/IronwakeTest/IronwakeShipHydro.cpp:55].
- Follow gains (fraction of the fitted plane the hull target uses): heave/pitch/roll ironclad `0.7 / 0.7 / 0.6`, frigate `1 / 1 / 0.85`, cutter `1 / 1 / 1` (were pitch x0.95, roll x0.55) [src Source/IronwakeTest/IronwakeFleet.cpp:33].
- Output clamps now pitch `+-8`, roll `+-12` deg (were `14`/`10`) [src Source/IronwakeTest/IronwakeShipHydro.cpp:145-146].
- New periods/zeta: ironclad heave `6.5` s / `0.5`, pitch `6.8` s / `0.55`, roll `10` s / `0.4` [src Source/IronwakeTest/IronwakeShipPawn.h:480-482]; frigate `5.5/0.5`, `5.5/0.55`, `8/0.4`; cutter zeta `0.5/0.5/0.4` (periods unchanged `3.2/3.0/4.5`) [src Source/IronwakeTest/IronwakeFleet.cpp:33,49]. This supersedes the table in 3.10.3 and the clamps in 3.10.2.
- Measured response, ironclad stopped across the sea (sd heave m / pitch deg / roll deg): CALM `0.14/0.18/0.24`, MODERATE `0.35/0.36/0.62`, ROUGH `0.62/0.80/1.07`; cutter ROUGH `0.98/1.91/4.00` [src logs/P16.md].

## 3.13 P19: ship-wave field (hull-driven surface disturbance) (2026-11-11)

- One analytic function, two evaluators. `IronwakeSea::ShipWaveEval` (`Source/IronwakeTest/IronwakeShipWave.cpp`) and its HLSL twin in the water material (`Content/Python/p19_watermat.py`, nodes `IW_ShipWaveV` = vertex WPO, `IW_ShipWaveP` = pixel normal tilt + foam alpha, spliced into `M_IW_WaterRoot` after the W4 foam blend) sum, per source: bow crest, midbody trough, quarter crest, and a Kelvin wedge (transverse + divergent crest families from stationary phase, cusp line at `WedgeHalfAngleDeg(Fn)` = 19.47 deg at Fn <= 0.25 narrowing linearly to 12 deg at Fn >= 0.5) [src Source/IronwakeTest/IronwakeShipWave.cpp].
- Sources: every `AIronwakeShipPawn` registers each tick (`RegisterShipWave`, entry expires after 0.6 s, stale entries pruned after 5 s); NPC craft register from their loop speed and mesh bounds (`AIronwakeNpcCraft::Tick`). The material receives the 4 sources nearest the camera each frame (`IW_SA0..3`, `IW_SB`, `IW_SC`, `IW_SD`; pushed by `SetFoamCamera`) [src Source/IronwakeTest/IronwakeHydro.cpp].
- Shape (all lengths in LWL, metres): bow crest height = `CrestK * freeboard * Gk * (0.8 + 0.4 * bluff)`, `Gk = clamp(Fn/0.32)^1.5`, Gaussian widths 0.09 L aft / 0.05 L ahead of the stem; trough = `-TroughK * draft * Gk` centred 36 % LWL (sigma 0.13 L); quarter crest = `QuarterK * bow crest` at 64 % LWL (sigma 0.07 L); wedge amplitude `WedgeK * bow crest`, decays as sqrt(0.6 L / (0.6 L + rho)), faded in over 0.15-0.9 L behind the stem, and band-limited per pixel footprint so crests fade instead of aliasing at distance.
- Hull-class data in `FIronwakeShipDef` (`WaveCrestK, WaveTroughK, WaveQuarterK, WaveWedgeK, WaveBluff, FreeboardM, DraftM`) [src Source/IronwakeTest/IronwakeFleet.h]: frigate 0.30/0.08/0.50/1.0, bluff 0.3, freeboard 4.0, draft 4.2; cutter 0.22/0.0/0.55/1.1, bluff 0.15, freeboard 1.3, draft 1.4 (no trough: planing); ironclad 0.45/0.10/0.60/0.9, bluff 0.8, freeboard 3.2, draft 8.0 [src Source/IronwakeTest/IronwakeFleet.cpp].
- CPU consumers: `ShipWaveZ(P, Skip)` adds other ships' waves to the buoyancy fit samples at 0.5 x (own wave excluded) [src Source/IronwakeTest/IronwakeShipHydro.cpp], to the bow-wave and Kelvin-wake ribbon vertices [src Source/IronwakeTest/IronwakeHydro.cpp], and to the P19 director measurements. `-iwnoshipwave` switches the whole field off (A/B control).
- Foam: ship foam is `max(old foam alpha, ship foam)`: bow foam starts 3 % LWL behind the stem (glassy first), hull-side strip foam fades through the trough window (`UIronwakeBowWave` strip alpha x (1 - 0.85 trough window), smoothstep 2-7 % LWL), quarter froth, wedge cusp foam `Gk^2 * ...`.
- Ribbons: Kelvin arm half-angle now follows `WedgeHalfAngleDeg(Fn)` (bow-wave crest ribbons and wake arms); prop wash band exists only when `Wake3->SetPropWash(!HasSails() || bAuxSteam, twin = ironclad)`: transom beam x 0.85 half-width + 4.5 deg widening, twin-screw alpha profile for the ironclad; sailing ships get no centre band (alpha 0) and weaker arms.
- Measured (calm, director `-iwp19`, same maths as the material; `logs/P19/p19all_*.txt`):

| Hull | kn | Fn | bow crest (% freeboard) | trough (% draft) | trough at | quarter crest at | wedge half angle (expected) |
|---|---|---|---|---|---|---|---|
| ironclad | 8 | 0.18 | 21 | 4.2 | 36 % | 64 % | 18.9 (19.5) |
| ironclad | 13 | 0.29 | 44 | 8.6 | 36 % | 64 % | 17.8 (18.3) |
| ironclad | 16 | 0.36 | 50 | 10.0 | 36 % | 64 % | 15.7 (16.3) |
| frigate (steam) | 8 / 13 / 16 | 0.24 / 0.39 / 0.48 | 18 / 28 / 28 | 5.2 / 8.0 / 8.0 | 36 % | 64 % | 19.2 / 13.5 / 11.8 |
| cutter | 8 / 13 / 16 | 0.36 / 0.59 / 0.72 | 19 (flat) | 0 | - | 64 % | 16.2 / 11.4 / 11.5 |

  The wedge angle is the sampled maximum-divergent-crest position at 1.4-2.8 L behind the stem. It tracks the formula (expected) within 0.7 deg, which proves the function, not the render.
- Cost: paired bench P17 33.2 / 33.5 fps, P19 31.7 / 32.3 fps (1.4 fps lower) [src logs/P19bench].
- Not done: ship waves do not yet enter `AnalyticZ` / `FastZ` (the wake ribbons add `ShipWaveZ` on top instead); only the 4 nearest sources displace the water mesh; moored-ship lapping and the crossing-wake rocking are wired through the buoyancy fit but were not measured.

## 3.14 P27: GPU batch Havelock Kelvin LUT, 7-ship ocean shader & turbulent Voronoi prop-wash LUT (2026-11-13)

- **Pre-solved GPU batch LUTs (`Content/Python/iw_p27_water_lut.py`, `/Game/Ironwake/Water/`):** replaced the P19 analytic stationary-phase trig loops with `1024x1024` HPC spectral Havelock free-surface integral `T_IW_KelvinWaveLut` (`TC_VECTOR_DISPLACEMENTMAP`, `sRGB=False`, `TA_CLAMP`), `T_IW_PropWashFoamLut` (`TC_MASKS`, `sRGB=False`, `TA_CLAMP`), and a seamless 3-octave periodic KD-tree Voronoi cellular bubble-web texture `T_IW_CellularFoamTile` (`TC_MASKS`, `TA_WRAP`, `33.9%` zero-value open-water cell cores). CPU `IronwakeSea::ShipWaveEval` loads `Content/Data/kelvin_lut_256.bin` (`262144` bytes, staged via `+DirectoriesToAlwaysStageAsUFS=(Path="Data")` [src Config/DefaultGame.ini:9]) so CPU height queries and GPU `M_IW_WaterRoot` sample the identical Havelock wave field (`SampleLut256` [src Source/IronwakeTest/IronwakeShipWave.cpp:39]).
- **7 simultaneous ships + camera-centred selection:** expanded `M_IW_WaterRoot` (`PushShipWaveParams` [src Source/IronwakeTest/IronwakeShipWave.h:42], `IW_SA0..6 .. IW_SD0..6` [src Source/IronwakeTest/IronwakeShipWave.h:41]) to **7 simultaneous ship wakes** (player slot `0` + nearest `6` moving NPCs). Both `SetFoamCamera` [src Source/IronwakeTest/IronwakeHydro.cpp:284] and `AIronwakeNpcDirector` (`FxRadiusCm` [src Source/IronwakeTest/IronwakeNpcDirector.cpp:287]) measure distance from `PlayerCameraManager->GetCameraLocation()` (with pawn fallback) so Photo Mode and aerial harbour views render wakes on every moving NPC in frame (`sheet_speed_scaling_and_npc6.jpg` `9.5 / 10` (build report `P27.md`)).
- **Froude interpolation & echelon envelope:** ship-relative coordinates `x_lut in [-6.5, +1.5]`, `y_lut in [-3.0, +3.0]` interpolate `R/G/B` channels across `Fn = {0.15, 0.24, 0.36}` (`Fn - 0.15f` [src Source/IronwakeTest/IronwakeShipWave.cpp:153]), gated cleanly at crawl speeds (`Smooth01(2.2f, 7.0f, KnApprox)` [src Source/IronwakeTest/IronwakeShipWave.cpp:110]), with echelon cusp boost `FMath::Lerp(0.76f, 1.26f, Smooth01(0.18f, 0.30f, Ray))` [src Source/IronwakeTest/IronwakeShipWave.cpp:157] and downstream decay `exp(-s_aft * 0.28)` (`8.5 / 10` (build report `P27.md`)).
- **Metal SM6 safety & performance:** zero `MaterialExpressionClamp` nodes in `M_IW_WaterRoot`, `M_IW_WakeLut`, or `M_IW_BowFoam` (`0` `Failed to compile Material` warnings (build report `P27.md`)); paired 600-frame 1920x1080 bench `Ironwake_P27.app` (`36.5`, `33.1` FPS) vs `Ironwake_P26.app` (`32.7`, `33.4` FPS) = **`+1.75 FPS` faster** (build report `P27.md`).
