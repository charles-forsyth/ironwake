# Chapter 11: Performance

Scope: how the game is made to hit its frame-rate target on the one machine it has been measured on (an Apple M5 Mac, Metal, 32 GB, UE 5.8.4): the LOW / MEDIUM / HIGH graphics presets and exactly which console variables each one sets, the physics sub-stepping settings and engine config keys, the known GPU costs of the volumetric cloud deck, water and post-processing, the paired benchmark method (scripts, command line, CSV statistics), and every benchmark result found in the logs and CSV files, grouped by build and by view. No second GPU/card has ever been benchmarked: the project has measured one machine only (see Gaps). Per-system CPU cost of the water and ship FX is estimated from code in chapters 3 and 4; nothing here claims a profile that was not run. Numbers marked "derived" were computed by running `build/tools/csvfps.py` or `csvstat.py` on the stored CSV (read-only); all others quote a log line.

## 11.1 Machine and build under test

- Hardware: "Apple M5 Mac, Metal, 32 GB", engine `$UE_ROOT` [src build/BRIEF.md:20]; "Apple M5" in the H1 performance card (build report `H1.md`). SPEC.md: UE `5.8.4`, Apple Silicon (Metal) [src SPEC.md:18].
- Nanite works on this GPU (NaniteVisBuffer/NaniteBasePass appear in the GPU CSV, about 1.5 ms together) (build report `R1.md`).
- Retina is not a factor: the windowed game runs at 1x backing (`Scale: 1.00`) (build report `H1.md`).
- Bench resolution: `1920x1080` windowed [src build/tools/e1openbench.sh:7].

## 11.2 Graphics presets (`IronwakeSettings`)

Source: `IronwakeSettings::ApplyGraphics`. Level mapping: `0` LOW, `1` MEDIUM (default), `2` HIGH [src Source/IronwakeTest/IronwakeSettings.cpp:44]. Stored in `GameUserSettings.ini` section `[Ironwake]` key `Graphics`; default `L = 1` [src Source/IronwakeTest/IronwakeSettings.cpp:51-52]; clamped 0..2 [src Source/IronwakeTest/IronwakeSettings.cpp:55]; `-iwgfx=N` overrides for benchmarks [src Source/IronwakeTest/IronwakeSettings.cpp:54]. Applied at game start by the game mode [src Source/IronwakeTest/IronwakeGameMode.cpp:69]; toggled MEDIUM -> HIGH -> LOW -> MEDIUM by the menu button (`G == 1 ? 2 : G == 2 ? 0 : 1`) [src Source/IronwakeTest/IronwakePlayerController.cpp:346]. Each `SetCVar` applies with `ECVF_SetByCode`; an unknown cvar logs `IWGFX unknown cvar` and is skipped [src Source/IronwakeTest/IronwakeSettings.cpp:21].

| CVar | LOW | MEDIUM | HIGH | Source |
|---|---|---|---|---|
| `t.MaxFPS` [src Source/IronwakeTest/IronwakeSettings.cpp:76] | `60` [src Source/IronwakeTest/IronwakeSettings.cpp:76] | `60` [src Source/IronwakeTest/IronwakeSettings.cpp:76] | `60` [src Source/IronwakeTest/IronwakeSettings.cpp:76] | [src Source/IronwakeTest/IronwakeSettings.cpp:76] |
| `r.VSync` [src Source/IronwakeTest/IronwakeSettings.cpp:77] | `1` [src Source/IronwakeTest/IronwakeSettings.cpp:77] | `1` [src Source/IronwakeTest/IronwakeSettings.cpp:77] | `1` [src Source/IronwakeTest/IronwakeSettings.cpp:77] | [src Source/IronwakeTest/IronwakeSettings.cpp:77] |
| `r.DynamicRes.OperationMode` [src Source/IronwakeTest/IronwakeSettings.cpp:78] | `2` [src Source/IronwakeTest/IronwakeSettings.cpp:78] | `2` [src Source/IronwakeTest/IronwakeSettings.cpp:78] | `2` [src Source/IronwakeTest/IronwakeSettings.cpp:78] | [src Source/IronwakeTest/IronwakeSettings.cpp:78] |
| `r.DynamicRes.FrameTimeBudget` [src Source/IronwakeTest/IronwakeSettings.cpp:79] (ms) | `15.0` [src Source/IronwakeTest/IronwakeSettings.cpp:79] | `15.0` [src Source/IronwakeTest/IronwakeSettings.cpp:79] | `33.3` [src Source/IronwakeTest/IronwakeSettings.cpp:79] | [src Source/IronwakeTest/IronwakeSettings.cpp:79] |
| `r.DynamicRes.MinScreenPercentage` [src Source/IronwakeTest/IronwakeSettings.cpp:80] | `45` [src Source/IronwakeTest/IronwakeSettings.cpp:80] | `50` [src Source/IronwakeTest/IronwakeSettings.cpp:80] | `50` [src Source/IronwakeTest/IronwakeSettings.cpp:80] | [src Source/IronwakeTest/IronwakeSettings.cpp:80] |
| `r.DynamicRes.MaxScreenPercentage` [src Source/IronwakeTest/IronwakeSettings.cpp:81] | `70` [src Source/IronwakeTest/IronwakeSettings.cpp:81] | `85` [src Source/IronwakeTest/IronwakeSettings.cpp:81] | `100` [src Source/IronwakeTest/IronwakeSettings.cpp:81] | [src Source/IronwakeTest/IronwakeSettings.cpp:81] |
| `r.VolumetricCloud.SampleMinCount` [src Source/IronwakeTest/IronwakeSettings.cpp:82] | `2` [src Source/IronwakeTest/IronwakeSettings.cpp:82] | `2` [src Source/IronwakeTest/IronwakeSettings.cpp:82] | `2` [src Source/IronwakeTest/IronwakeSettings.cpp:82] | [src Source/IronwakeTest/IronwakeSettings.cpp:82] |
| `r.VolumetricCloud.ViewRaySampleMaxCount` [src Source/IronwakeTest/IronwakeSettings.cpp:83] | `12` [src Source/IronwakeTest/IronwakeSettings.cpp:83] | `16` [src Source/IronwakeTest/IronwakeSettings.cpp:83] | `64` [src Source/IronwakeTest/IronwakeSettings.cpp:83] | [src Source/IronwakeTest/IronwakeSettings.cpp:83] |
| `r.VolumetricCloud.ReflectionRaySampleMaxCount` [src Source/IronwakeTest/IronwakeSettings.cpp:84] | `8` [src Source/IronwakeTest/IronwakeSettings.cpp:84] | `8` [src Source/IronwakeTest/IronwakeSettings.cpp:84] | `16` [src Source/IronwakeTest/IronwakeSettings.cpp:84] | [src Source/IronwakeTest/IronwakeSettings.cpp:84] |
| `r.VolumetricCloud.SkyAO` [src Source/IronwakeTest/IronwakeSettings.cpp:85] | `0` [src Source/IronwakeTest/IronwakeSettings.cpp:85] | `0` [src Source/IronwakeTest/IronwakeSettings.cpp:85] | `1` [src Source/IronwakeTest/IronwakeSettings.cpp:85] | [src Source/IronwakeTest/IronwakeSettings.cpp:85] |
| `r.VolumetricRenderTarget.Mode` [src Source/IronwakeTest/IronwakeSettings.cpp:86] | `2` [src Source/IronwakeTest/IronwakeSettings.cpp:86] | `2` [src Source/IronwakeTest/IronwakeSettings.cpp:86] | `0` [src Source/IronwakeTest/IronwakeSettings.cpp:86] | [src Source/IronwakeTest/IronwakeSettings.cpp:86] |
| `r.Water.WaterMesh.TessFactorBias` [src Source/IronwakeTest/IronwakeSettings.cpp:87] | `-2` [src Source/IronwakeTest/IronwakeSettings.cpp:87] | `-1` [src Source/IronwakeTest/IronwakeSettings.cpp:87] | `0` [src Source/IronwakeTest/IronwakeSettings.cpp:87] | [src Source/IronwakeTest/IronwakeSettings.cpp:87] |
| `r.DynamicGlobalIlluminationMethod` [src Source/IronwakeTest/IronwakeSettings.cpp:89] | `0` [src Source/IronwakeTest/IronwakeSettings.cpp:89] (none, sky light only) | `1` [src Source/IronwakeTest/IronwakeSettings.cpp:89] (Lumen) | `1` [src Source/IronwakeTest/IronwakeSettings.cpp:89] | [src Source/IronwakeTest/IronwakeSettings.cpp:89] |
| `r.ReflectionMethod` [src Source/IronwakeTest/IronwakeSettings.cpp:90] | `2` [src Source/IronwakeTest/IronwakeSettings.cpp:90] (SSR) | `1` [src Source/IronwakeTest/IronwakeSettings.cpp:90] (Lumen) | `1` [src Source/IronwakeTest/IronwakeSettings.cpp:90] | [src Source/IronwakeTest/IronwakeSettings.cpp:90] |
| `r.Shadow.Virtual.Enable` [src Source/IronwakeTest/IronwakeSettings.cpp:91] | `0` [src Source/IronwakeTest/IronwakeSettings.cpp:91] | `1` [src Source/IronwakeTest/IronwakeSettings.cpp:91] | `1` [src Source/IronwakeTest/IronwakeSettings.cpp:91] | [src Source/IronwakeTest/IronwakeSettings.cpp:91] |

Notes:
- Comment in code: "Every level: 60 fps cap with vsync; dynamic resolution (TSR upscale) holds the GPU frame budget" and "volumetric cloud deck was 40 of 57 ms GPU at 1600x900; the water ~4 ms" [src Source/IronwakeTest/IronwakeSettings.cpp:74-75].
- `r.VolumetricRenderTarget.Mode 0` on HIGH = full-resolution cloud tracing; Mode 2 = half-res trace + reconstruction (LOW/MEDIUM); Mode 1 was measured worse: 9.5 ms (build report `H1.md`).
- `SampleMinCount` is written as `L == 2 ? "2" : "2"`: the same value for every level (no-op conditional) [src Source/IronwakeTest/IronwakeSettings.cpp:82].
- Header summary: "Graphics 0 LOW / 1 MEDIUM (default, 60 fps on the M5 at 1080p) / 2 HIGH (cinematic clouds, ~30 fps class)" [src Source/IronwakeTest/IronwakeSettings.h:6].
- Preset effects per the H1 log: LOW = Lumen off (sky light only), SSR reflections, no virtual shadow maps, 45-70 % dynamic res, 12 cloud samples, water tess -2; MEDIUM = Lumen GI + reflections kept, VSM kept, 50-85 % dynamic res, 16 cloud samples, water tess -1; HIGH = full cloud quality (64 samples, full-res, sky AO), 100 % max resolution, 30 fps budget; "Use HIGH for stills" (build report `H1.md`).
- `sg.*` scalability groups are not touched by code; a leftover `GameUserSettings.ini` from before H1 had `sg.ShadowQuality=2`, `sg.GlobalIlluminationQuality=2`, `sg.ReflectionQuality=2`, other groups 3, `bUseVSync=False`, `bUseDynamicResolution=False`, `FrameRateLimit=0.000000` (build report `H1/GameUserSettings_before.ini`). There is no `DefaultScalability.ini` in `Config/` (files: `DefaultEditorPerProjectUserSettings.ini`, `DefaultEngine.ini`, `DefaultEngine.ini.bak_lumen`, `DefaultGame.ini`, `DefaultInput.ini`).
- The in-game graphics label reads `GRAPHICS:  LOW|MEDIUM|HIGH`.

### 11.2.1 What each preset changes, in plain terms

| Aspect | LOW | MEDIUM | HIGH |
|---|---|---|---|
| Frame target | 60 fps, 15 ms budget | 60 fps, 15 ms budget | 60 fps cap but 33.3 ms budget (30 fps class) |
| Internal resolution | 45-70 % of 1080p | 50-85 % | 50-100 % |
| Clouds | 12 view samples, half-res target | 16 samples, half-res | 64 samples, full-res, sky AO |
| Water mesh density | tess -2 | -1 | 0 |
| GI / reflections | none (sky light) / SSR | Lumen / Lumen | Lumen / Lumen |
| Shadows | no virtual shadow maps | virtual shadow maps | virtual shadow maps |

(Table is a restatement of the cvar table above; sources as listed there.)

## 11.3 Engine config keys relevant to performance (`Config/DefaultEngine.ini`)

These are the keys that take effect from the ini (the presets override the ones they set, at runtime, by code, which wins):

| Key | Value | Source |
|---|---|---|
| `r.GenerateMeshDistanceFields` [src Config/DefaultEngine.ini:2] | `True` [src Config/DefaultEngine.ini:2] | [src Config/DefaultEngine.ini:2] |
| `r.DynamicGlobalIlluminationMethod` [src Config/DefaultEngine.ini:3] | `1` [src Config/DefaultEngine.ini:3] (Lumen) | [src Config/DefaultEngine.ini:3] |
| `r.ReflectionMethod` [src Config/DefaultEngine.ini:4] | `1` [src Config/DefaultEngine.ini:4] (Lumen) | [src Config/DefaultEngine.ini:4] |
| `r.Lumen.HardwareRayTracing` [src Config/DefaultEngine.ini:5] | `False` [src Config/DefaultEngine.ini:5] | [src Config/DefaultEngine.ini:5] |
| `r.Shadow.Virtual.Enable` [src Config/DefaultEngine.ini:6] | `1` [src Config/DefaultEngine.ini:6] | [src Config/DefaultEngine.ini:6] |
| `r.CustomDepth` [src Config/DefaultEngine.ini:7] | `3` [src Config/DefaultEngine.ini:7] | [src Config/DefaultEngine.ini:7] |
| `GameDefaultMap` [src Config/DefaultEngine.ini:29] | `/Game/Maps/Ironwake_Sea.Ironwake_Sea` [src Config/DefaultEngine.ini:29] | [src Config/DefaultEngine.ini:29] |

- The ini values are the MEDIUM values; LOW overrides GI, reflections and VSM at runtime (11.2). The ini diff against `DefaultEngine.ini.bak_lumen`: the backup lacks `r.GenerateMeshDistanceFields`, `GameMapsSettings`, bundle id and the whole `PhysicsSettings` block (i.e. those were added later); the backup is not read by the engine.
- `r.Water.*` keys are not in the ini; the water mesh density is set only by code via `r.Water.WaterMesh.TessFactorBias` (11.2).

### 11.3.1 Physics sub-stepping

Added in P7 to make handling frame-rate independent: "handling is driven as velocities in Tick, but the physics step itself (angular damping, velocity application) ran at the variable frame delta" (build report `P7.md`). `[/Script/Engine.PhysicsSettings]`:

| Key | Value | Source |
|---|---|---|
| `bSubstepping` [src Config/DefaultEngine.ini:37] | `True` [src Config/DefaultEngine.ini:37] | [src Config/DefaultEngine.ini:37] |
| `bSubsteppingAsync` [src Config/DefaultEngine.ini:38] | `False` [src Config/DefaultEngine.ini:38] | [src Config/DefaultEngine.ini:38] |
| `MaxSubstepDeltaTime` [src Config/DefaultEngine.ini:39] | `0.016667` [src Config/DefaultEngine.ini:39] (1/60 s) | [src Config/DefaultEngine.ini:39] |
| `MaxSubsteps` [src Config/DefaultEngine.ini:40] | `6` [src Config/DefaultEngine.ini:40] | [src Config/DefaultEngine.ini:40] |
| `MaxPhysicsDeltaTime` [src Config/DefaultEngine.ini:41] | `0.1` [src Config/DefaultEngine.ini:41] | [src Config/DefaultEngine.ini:41] |

- Derived: with sub-steps of at most 1/60 s and at most 6 per frame, the physics clock covers up to 0.1 s per frame (6 x 1/60), which equals `MaxPhysicsDeltaTime`; below 10 fps simulated physics time is dropped.
- Verified by full-circle time at flank + hard rudder (fixed delta via `-benchmark -fps=N`): ironclad `178.3` s at 30 fps and 18 fps after the fix, vs `178.4` s (60/30 fps) and `294.8` s (18 fps) before (build report `P7.md`); frigate `192.1` / `192.0` s; cutter `32.9` s (build report `P7.md`). Real-time frame limiter check, frigate at `t.MaxFPS 18`: `191.6` s (build report `P7.md`).
- Later numbers: the ironclad circle is `208.8` s at 30 and 18 fps after P8's `MinTurnRadiusM` 280 -> 330 (build report `P8.md`); the P7 text 2.97 min (178 s) is superseded.
- Ship gameplay itself caps its own tick delta at `0.1` s: `Dt = min(DeltaSeconds, 0.1)`; the storm director also `min(DtIn, 0.1)`; the shell caps at `0.1` and sub-steps up to `64` times [src Source/IronwakeTest/IronwakeShell.cpp:106].
- Ship hydro spring integrates in sub-steps of `0.02` s up to `8` (own integrator, not the physics engine) [src Source/IronwakeTest/IronwakeShipHydro.cpp:105]. Gravity is off on the hull body; heave comes from `TickHydro`.
- Substepping affects every physics body (ships only); airship/NPC fleets were not re-timed (build report `P7.md`).

## 11.4 Known GPU costs

All from csvprofile GPU columns (median over frames 120+ unless stated).

### 11.4.1 The cloud deck (the dominant cost)

| Build / setting | VolumetricCloud GPU ms | Source |
|---|---|---|
| v1 (before H1), 1920x1080 | `41.4` (build report `H1.md`) | (build report `H1.md`) |
| v1, 1600x900 | `41.3` (build report `H1.md`) | (build report `H1.md`) |
| H1 default (MEDIUM with 16 view samples, mode 2) | about 2-3 ms; derived median `3.22`-`3.42` ms in the capped runs | (build report `H1.md`) |
| `r.VolumetricRenderTarget.Mode 1` | `9.5` (build report `H1.md`) ms | (build report `H1.md`) |
| HIGH | derived median `14.19` ms (open_high) | `logs/H1/open_high.csv.gz` (derived) |
| LOW | derived median `2.60` ms (open_low) | `logs/H1/open_low.csv.gz` (derived) |
| I1 five-island bench | `14.00` (build report `I1.md`) ms (derived, i1_bench_gpu) and "14 ms" in the log | (build report `I1.md`) |

- Root cause: custom cloud deck material had unbounded ray samples (default 768): `ViewRaySampleMaxCount 16` (was unbounded/default 768), `SampleMinCount 2`, reflection/shadow samples 8, SkyAO off, `VolumetricRenderTarget.Mode 2`: 41 ms -> 2-3 ms (build report `H1.md`).
- Cloud deck was two thirds of the frame in v1 (build report `H1.md`).
- Clouds off (`c2_nocloud`) derived: 52.4 fps, GPU median 18.94 ms; cloud low (`c3_cloudlow`) 38.0 fps; cloud low + 67 % screen (`c4`) 49.5 fps; MEDIUM (`c5_med`) 51.5 fps (all uncapped, 1080p, editor closed; derived from `logs/H1/c*.csv.gz`). Order of the H1 experiments is documented in the card, one change at a time (build report `H1.md`).

### 11.4.2 Water

| Item | Value | Source |
|---|---|---|
| v1: SingleLayerWater at 1920x1080 | `5.7` (build report `H1.md`) ms | (build report `H1.md`) |
| v1: at 1600x900 | `5.0` (build report `H1.md`) ms | (build report `H1.md`) |
| H1 MEDIUM water | about `0.9` (build report `H1.md`) ms | (build report `H1.md`) |
| derived: MEDIUM uncapped (`open_medium_uncapped`) SingleLayerWater | `0.91` ms | derived from `logs/H1/open_medium_uncapped.csv.gz` |
| derived: LOW SingleLayerWater | `0.67` ms | derived from `logs/H1/open_low.csv.gz` |
| derived: HIGH SingleLayerWater | `1.56` ms | derived from `logs/H1/open_high.csv.gz` |
| I1 five-island bench: SingleLayerWater | `0.18` ms (+ depth prepass `0.34` ms) | derived from `logs/I1/i1_bench_gpu.csv` |
| Tess bias `-1` (build report `H1.md`) MEDIUM, `-2` (build report `H1.md`) LOW; `LODScaleBias +2` (build report `H1.md`) made it worse and is not used | | (build report `H1.md`) |
| Tried with no worthwhile gain: `sg.*` (build report `H1.md`) quality 2 (about 0.7 ms), water reflection/tiles cvars (0), motion blur off (0) | | (build report `H1.md`) |

- The SingleLayerWater pass is resolution-dependent (more pixels -> more cost), which dynamic resolution reduces. Whitecap/foam shading is inside the water material; its GPU cost vs foam coverage has never been isolated (Gaps).
- The 300 km ocean extent costs nothing measurable: P5c vs P5 warm pairs within 1.5 fps; far ring not reduced (build report `P7.md`).
- Vertex-count/CPU cost of water FX is separate: see chapter 4 (wake and bow wave rebuild per ship per frame).

### 11.4.3 Post-processing and others

- Dominant remaining items in H1 MEDIUM: `Unaccounted` 4.9, Lumen reflections `1.0`, water `0.9` ms (build report `H1.md`); LOW: Lumen off, VSM off, TSR (build report `H1.md`).
- Derived (open_low): `TemporalSuperResolution` `4.13` ms, `ShadowDepths` `2.09` ms, `Postprocessing` `0.30` ms; (open_medium_uncapped): `Unaccounted` `4.95`, `LumenReflections` `0.99`, `LumenScreenProbeGather` `0.91`, `ShadowDepths` `0.90`, `LumenSceneUpdate` `0.53` ms; (open_high): `Unaccounted` `7.21`, `LumenReflections` `1.02`, `LumenScreenProbeGather` `0.88`, `ShadowDepths` `0.82` ms. (All derived from the CSVs named; MotionBlur reads `5.21` ms on `t_cap120` and `4.67` ms on `c12_tess-2`, a measurement-dependent artefact: in the final MEDIUM run it is `0.18` ms (W3 tour csv).)
- Ship Nanite hulls replaced high-poly non-Nanite meshes; ShadowDepths dropped `2.2` -> `1.5` ms (build report `R1.md`).
- Motion blur off: no gain (0) (build report `H1.md`).
- Retina no factor; `r.DynamicRes` beat a fixed 60-67 % screen percentage (build report `H1.md`).

## 11.5 Paired benchmark method

### 11.5.1 Why paired

- Machine load swung between runs; the first run of each session is slow. "Caveat: the first run of each session is slow, so take pairs" (build report `P7.md`). R1: "One early R1 run read 27.9 vs 47.4, the background load swung between runs, which is why the interleaved pairs above are the numbers to trust" (build report `R1.md`).
- Chuck's another Unreal Editor instance editor open on the same GPU halves throughput; benches taken with it open are not absolute (W1, W2, I1, R1, P3 logs). Absolute 60 fps claims come only from runs with the editor closed.
- Rule used by the logs: two builds are "the same" when interleaved warm pairs are within 1-3 fps (P5c vs P5 within 1.5, P8 loses nothing, P12 within 0.6, P13 within 3).

### 11.5.2 How to launch (packaged, player-style)

Launching the binary directly from a shell makes any capped run read about 18 fps because macOS throttles the frame limiter's sleeps for a background-treated process; the same build reads a flat 60.0 under `open -n`, which is how a player starts it. Uncapped GPU times are the same either way (build report `H1.md`). Hence all benches use `open -n`:

- `build/tools/h1openbench.sh <label> [args]`: `open -n ${IWAPP:-$HOME/Games/Ironwake/Ironwake.app} --args -iwtour -windowed -ResX=1920 -ResY=1080 -csvCaptureFrames=450`; polls up to `80` x `3` s for a CSV whose tail contains `HasHeaderRowAtEnd`; kills any `Packaged/Ironwake/Ironwake*.app`; copies the CSV to `logs/${BENCHDIR:-H1}/<label>.csv` and prints `csvstat.py ... 120`.
- `build/tools/e1openbench.sh`: same but no `-iwtour` (pass `-iwe1=bench` or `-iwtour`), `FRAMES` env = CSV frames (default `450`), adds `-nosound -unattended -nosplash -RenderOffScreen`, container id from `IWID` [src build/tools/e1openbench.sh:5-7]. Note on `-RenderOffScreen` benches: added in P13/P14 runs (build report `P14.md`).
- `build/tools/p7bench.sh a b ...`: per `<app>[:suffix]` runs `FRAMES=600` through e1openbench with `BENCHDIR=${BD:-P7}` and `IWID=io.github.charles-forsyth.ironwake`, app `Packaged/Ironwake/Ironwake_<app>.app`, `-iwtour`; labels `bench_<app>_<time>`. Pair usage: `p7bench.sh P12:a P13:a P12:b P13:b` interleaves.
- Test hooks for specific views: `-iwtour` (scripted tour), `-iwe1=bench` / `bench_island` (hero harbour / island), `-iwi1=bench` (most islands in view), `-iwp8=bench` (town), `-iww2=bench` (storm), `-iwp12=bench` (town day), `-iwp11=tw`, `-iwgfx=N` (preset override) (build report `E1.md`) (build report `I1.md`) (build report `P8.md`) (build report `W2.md`) [src Source/IronwakeTest/IronwakeSettings.cpp:54].
- Fixed-step capture runs (`-benchmark -fps=30`) are for movies/frame dumps, not FPS (build report `P4.md`).

### 11.5.3 Statistics

- `build/tools/csvstat.py <csv[.gz]> [skip=60]`: UE csvprofile files keep the full header at the END (`HasHeaderRowAtEnd`); the script takes the last `EVENTS` row as header, drops `skip` leading frames (callers use `120`), and prints median and p90 of `FrameTime`, `GameThreadTime`, `RenderThreadTime`, `RHIThreadTime`, `GPUTime`, the median FPS = 1000/median(FrameTime), and the top 25 `GPU/*` columns above 0.05 ms [src build/tools/csvstat.py:1-4] [src build/tools/csvstat.py:19-33].
- `build/tools/csvfps.py <csv...>`: skips the first `120` frames, prints `n`, mean FPS = 1000/mean(FrameTime), 1 % low FPS (99th percentile frame time) and mean GPU ms [src build/tools/csvfps.py:6-14]. Averages from `csvfps` differ slightly from `csvstat` median FPS (e.g. P7 final: 59.0 mean vs 59.9 median).
- Default capture: `450` frames (330 after skip) for H1-W3 era; `600` frames (480 after skip) from P5/P7 on (`FRAMES=600`) [src build/tools/p7bench.sh:3].
- Pass criterion used in cards: median FPS 60.0 with p90 GPU under the 16.67 ms budget (E1: "frames > 17.5 ms" count) (build report `E1.md`).

## 11.6 Results (all benches found, on the one M5)

### 11.6.1 H1: presets (packaged Development build, editor closed, 1920x1080, `-iwtour`, medians of frames 120-450)

| Build / preset | Median FPS | Frame ms (p90) | GPU ms (p90) | Top GPU pass |
|---|---|---|---|---|
| v1 (before) 1920x1080 | `16.1` | `62.2` (80.1) | `61.9` (79.8) | VolumetricCloud 41.4, SingleLayerWater 5.7 |
| v1 1600x900 | `17.5` | `57.0` (75.8) | `56.8` (74.8) | VolumetricCloud 41.3 |
| H1 MEDIUM (60 cap + vsync) | `60.0` | `16.67` (16.74) | `12.9` (14.1) | Unaccounted 4.9 |
| H1 MEDIUM uncapped | `76.4` | `13.1` (14.9) | `12.8` (14.7) | same |
| H1 LOW (60 cap) | `60.0` | `16.67` (16.94) | `12.6` (14.1) | Lumen off, VSM off, TSR |
| H1 HIGH | `33.7` | `29.7` (32.1) | `29.5` (31.9) | VolumetricCloud |

Source: (build report `H1.md`). The orchestrator's earlier 6 FPS reading was taken with the editor open (build report `H1.md`).

The H1 experiment ladder (uncapped, one change at a time; derived median FPS from the `c*.csv.gz` files, for reproduction) is: `c1_raysamples` 26.0, `c2_nocloud` 52.4, `c3_cloudlow` 38.0, `c4_cloudlow_sp67` 49.5, `c5_med` 51.5, `c6_vs16` 57.3, `c7_water` 51.7, `c8_maxfps` 50.1, `c9_mode1` 43.7, `c10_watermesh` 55.7, `c11_sp58` 61.9, `c12_tess-2` 55.0, `c13_tess-3lod` 53.5, `c14_med_sp60` 62.7, `c15_dynres` 72.6, `c16_low` 68.6 (all derived by `csvstat.py ... 120`). Frame-cap experiments (`t_cap40`, `t_cap60_novsync`, `t_cap62`, `t_default_front`, `t_default_noappnap`) read 11.8 / 23.1 / 18.4 / 20.7 / 17.7 fps because of the macOS frame-limiter throttling of directly launched binaries (derived); the `open -n` runs (`t_default_open_front`, `open_low`) read `60.0` (derived).

### 11.6.2 Same-session pairs for every later build (packaged, `-iwtour` unless noted)

All values median/avg FPS. "Editor open" means Chuck's another Unreal Editor instance editor was running (absolute numbers not clean).

| Card | Condition | Result | Source |
|---|---|---|---|
| W1 | editor/another Unreal Editor instance running; W1 vs R1 pairs 1-4 | W1 `35.9` (build report `W1.md`), `22.6` (build report `W1.md`), `36.3` (build report `W1.md`), `35.5` (build report `W1.md`); R1 `33.3` (build report `W1.md`), `24.4` (build report `W1.md`), `36.9` (build report `W1.md`), `36.6` (build report `W1.md`); GPU median about 26.5 ms both | (build report `W1.md`) |
| W2 | STORM bench `-iww2=bench` (build report `W2.md`), editor open | median 25.5 fps, GPU 38.6 ms, game thread 1.8 ms; derived mean 25.6 fps, 1% low 22.0 | (build report `W2.md`) |
| W3 | tour, editor closed at 10:35 | 33.8 fps median, GPU 29.4 ms, game 1.5 ms (derived csvstat: median 33.8, GPU p90 32.03) | (build report `W3.md`) |
| W4 | bench skipped (editor open) | none | (build report `W4.md`) |
| P5c vs P5 (cold first run discarded) | P5 #1 `26.2` (build report `P7.md`) (17.6 1%), P5c #1 `51.5` (build report `P7.md`) (42.7), P5 #2 `53.0` (build report `P7.md`) (44.6), P5c #2 `53.1` (build report `P7.md`) (44.8), P7 final `59.0` (build report `P7.md`) (49.6); GPU mean `37.2` (build report `P7.md`), `18.9` (build report `P7.md`), `18.4` (build report `P7.md`), `18.4` (build report `P7.md`), `16.4` (build report `P7.md`) | (build report `P7.md`) |
| P8 vs P7 | P7 `33.1` (build report `P8.md`), P8 `33.5` (build report `P8.md`), P7 `33.5` (build report `P8.md`), P8 `33.1` (build report `P8.md`); town hero view `-iwp8=bench` (build report `P8.md`) 32.5 fps GPU 30 ms | (build report `P8.md`) |
| P9 vs P8 | P8 `33.0` (build report `P9.md`) / `33.3` (build report `P9.md`), P9 `33.6` (build report `P9.md`) / `33.4` (build report `P9.md`) | (build report `P9.md`) |
| P10 vs P9 | P9 `33.4` (build report `P10.md`), P10 `~33.1` (build report `P10.md`) (30.2 ms), P10 `33.7` (build report `P10.md`), P9 `33.4` (build report `P10.md`) | (build report `P10.md`) |
| P11 vs P10 | P10 `33.0` (build report `P11.md`) / `33.6` (build report `P11.md`), P11 `33.7` (build report `P11.md`) / `33.3` (build report `P11.md`) | (build report `P11.md`) |
| P12 vs P11 | P11 `33.3` (build report `P12.md`) / `33.3` (build report `P12.md`), P12 `32.7` (build report `P12.md`) / `32.9` (build report `P12.md`) | (build report `P12.md`) |
| P13 vs P12 | P12 `32.6` (build report `P13.md`) / `33.2` (build report `P13.md`), P13 `31.3` (build report `P13.md`) / `32.1` (build report `P13.md`) (about -1.1) | (build report `P13.md`) |
| P14 vs P13 | P13 `32.4` (build report `P14.md`) / `32.8` (build report `P14.md`), P14 `32.4` (build report `P14.md`) / `32.6` (build report `P14.md`) | (build report `P14.md`) |
| E1 (editor closed, no another Unreal Editor instance) | A1 tour `60.0` (build report `E1.md`) GPU 13.2 / 13.8; E1 harbour hero `60.0` (build report `E1.md`) 14.4 / 16.0; island `60.0` (build report `E1.md`) 15.5 / 17.2 (4/480 frames above 17.5 ms); tour `60.0` (build report `E1.md`) 13.2 / 13.9; after fog change: harbour 14.3 / 15.2, island 15.3 / 16.4, tour 13.1 / 14.1 | (build report `E1.md`) |
| I1 (editor open, half speed) | E1 tour `32.6` (build report `I1.md`), W2 `32.8` (build report `I1.md`), S2 `31.4` (build report `I1.md`), I1 tour `32.7` (build report `I1.md`); E1 island bench `25.6-26.5` (build report `I1.md`); I1 five-island bench `22.7-24.0` (build report `I1.md`) (GPU 41 ms, VolumetricCloud 14 ms) | (build report `I1.md`) |
| P3 (editor open, rclone busy) | P3 `13.3` (build report `P3.md`) fps (GPU 70 ms), Ironwake.app `12.8` (build report `P3.md`) (GPU 77 ms), five-island view; "not clean, no claim" | (build report `P3.md`) |
| R1 (editor open) | capped pair 1: H1 `24.2` (build report `R1.md`) vs R1 `32.0` (build report `R1.md`); pair 2: `23.4` (build report `R1.md`) vs `31.9` (build report `R1.md`); GPU-stats pair `28.2` (build report `R1.md`) (GPU 33.4 ms) vs `30.1` (build report `R1.md`) (GPU 31.0 ms) | (build report `R1.md`) |

Derived FPS from the stored CSVs (`csvfps.py`, mean FPS, 1 % low, mean GPU): P11 bench `P10:a` 32.9 / 29.1 / 29.9 ms; `P10:b` 33.1 / 27.5 / 29.7; `P11:a` 33.4 / 29.1 / 29.5; `P11:b` 33.0 / 28.6 / 32.8 (`logs/P11bench/`). P12 bench: `P11:a` 33.1 / 29.4 / 29.7; `P11:b` 33.0 / 28.7 / 30.1; `P12:a` 32.8 / 26.9 / 30.3; `P12:b` 32.7 / 27.9 / 36.1 (`logs/P12bench/`). P13 bench: `P12:a` 32.3 / 26.8 / 30.5; `P12:b` 32.7 / 27.5 / 30.4; `P13:a` 31.4 / 25.6 / 31.3; `P13:b` 32.5 / 26.5 / 30.4 (`logs/P13bench/`). P14: `P13_101351` 32.5 / 26.2 / 30.6; `P13_101438` 32.7 / 26.2 / 30.4; `P14_101415` 32.3 / 26.5 / 30.5; `P14_101502` 32.2 / 27.0 / 30.6 (`logs/P14/`). P7 folder: `P7_224535` 32.7 / 27.8 / 30.1; `P7_224622` 32.9 / 26.9 / 30.2; `P8_013130` 33.1 / 27.6 / 29.8; `P8_013224` 33.0 / 26.8 / 30.1; `P9_013154` 33.1 / 26.8 / 29.9; `P9_013248` 33.1 / 26.7 / 30.0 (`logs/P7/`). Medians (`csvstat`) for the P14 pair: 32.4 / 32.8 / 32.4 / 32.6 fps, GameThread 2.55-2.60 ms, RenderThread 30.5-30.8 ms, RHI 1.43-1.46 ms, GPU 30.3-30.7 ms.

### 11.6.3 Reading the table

- Two regimes appear: about 59-60 fps (clean machine; H1 final, E1, P7 final) and about 33 fps (P7 re-run, P8-P14, W3). The P8 log attributes the 33 to a "contended" machine (P7 reads 33 too) (build report `P8.md`). For P7 itself: a clean run `59.0` and a later run of the same app `32.7`/`32.9` (derived from `bench_P7_224535/224622`) differ by 26 fps with no code change. Treat 33 as "machine at half speed", not a regression.
- In the 33 fps regime the frame is GPU-bound: GPU about 30 ms equals frame time about 30.8 ms, game thread about 2.6 ms, render thread about 30.8 ms (derived from `bench_P14_*`). CPU headroom is large (game thread 2.6 ms of 16.7 ms).
- The game thread is never the limit: 1.5-2.6 ms in every run (W3 `1.47` ms median, P14 `2.55` ms; derived).
- Water and ship FX never showed up as a measurable GPU line: wake/bow-wave work is CPU (game thread), and the P5c/P7 pairs found no difference (within 1.5 fps) (build report `P7.md`). There is no isolated "wake on vs off" measurement: `-iwnowake`, `-iwnobow`, `-iwnosheet`, `-iwnospume` exist (chapter 4) but have not been benchmarked.
- STORM (W2) costs about 8 ms more GPU than the tour in the same session class (38.6 vs 29.4 ms) but with the editor open for the first and closed for the second, so not comparable (Gaps).

## 11.7 Per-card verdicts (what the logs conclude)

| Card | Verdict on FPS | Source |
|---|---|---|
| H1 | 60 fps MEDIUM and LOW at 1080p, HIGH 33.7 by design | (build report `H1.md`) |
| W1 | same as R1 within noise; flat-60 idle check still to do | (build report `W1.md`) |
| W2 | not clean, re-bench on idle machine | (build report `W2.md`) |
| W3 | not 60; GPU-limited at 33.8 | (build report `W3.md`) |
| P5c | not slower than P5 | (build report `P7.md`) |
| E1 | holds 60 but island view uses about 93 % of the GPU budget (p90 17.2 ms) | (build report `E1.md`) |
| I1 | NOT verified at 60 (honest); by ratio the five-island view should land about 50-55 idle | (build report `I1.md`) (build report `I1.md`) |
| P8-P14 | within 1-3 fps of the previous build each time | (build report `P14.md`) |

## 11.8 Budget guidance derivable from the data (not measured as such)

- Frame budget at 60 fps = 16.67 ms (`FrameTime` at the cap `16.67` in capped runs) (build report `H1.md`). Idle-machine MEDIUM GPU is about 13-15 ms (H1 `12.9` p90 `14.1`; E1 harbour `14.4`, island `15.5`) leaving about 1.5-3.5 ms of GPU headroom; islands with vegetation use about 1.1 ms more than the harbour (E1) and I1's five-island view adds 3-4 ms (build report `I1.md`) (relative cost; absolute is on the contended machine).
- Anything added that costs GPU > 1-2 ms in MEDIUM at 60 fps drops frames; dynamic resolution (15 ms budget, 50-85 %) absorbs this by lowering screen percentage first (derived from the cvars, not a measurement).
- STORM at GALE/STORM sea states has more pixels in foam shading and spume sprites; measured only once (25.5 fps with the editor open).

## Gaps / unknown

- Other GPUs / Macs: unknown. Only one Apple M5 machine has ever been benched. To measure on another card: copy `build/tools/h1openbench.sh` / `p7bench.sh`, run paired `-iwtour` benches with the editor closed and `-iwgfx=0|1|2`.
- Idle-machine numbers for builds after E1: measured in **P30** with Unreal Editor closed (`1920x1080`, `600` frames, `-iwtour`, `open -g -n`): default `MEDIUM` (`-iwgfx=1`) `Ironwake_P29.app` `54.0`, `52.2` (mean `53.10` (build report `P30.md`), `+1.15 FPS` (build report `P30.md`) faster than `Ironwake_P19.app` `53.0`, `50.9` and `+18.15 FPS` (build report `P30.md`) faster than W3 `Ironwake.app` `33.3`, `33.8`); cinematic `HIGH` (`-iwgfx=2`) `33.6`, `32.9` (mean `33.25` (build report `P30.md`), `+1.40 FPS` (build report `P30.md`) faster than `Ironwake_P19.app` `32.0`, `31.7`).
- STORM / GALE cost on an idle machine: unknown (W2 bench 25.5 fps with the editor open; W4 skipped).
- Cost of the wake / bow-wave / spray systems in isolation: unknown. Measure A/B with `-iwnowake -iwnobow -iwnosheet` against a plain run in the same session, then read `GameThreadTime` in `csvstat.py`.
- GPU cost of the whitecap/foam shader vs coverage: unknown; `-iwcovk=` scales coverage for such a test (chapter 3).
- Cost of the 22-wave / 16-wave analytic wave sum at the shader level: unknown (not separated in `GPU/` columns; SingleLayerWater only).
- Memory and load time: not measured in the repo.
- Per-system CPU numbers (game thread by actor) are not collected; `GameThreadTime` is whole-frame only.
- LOW preset on a clean machine at 1080p: `60.0` fps median reported (build report `H1.md`); its GPU cost relative to MEDIUM (12.6 vs 12.9 ms) is nearly equal on the GPU, so LOW buys little on this GPU. A weaker card has not been tried.
- `DefaultEngine.ini.bak_lumen` is a historic snapshot (before the later keys); it is not referenced by code; keep out of any "what takes effect" reading.
