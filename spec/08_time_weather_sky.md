# Chapter 8 — Time, weather, sky and lighting

**Scope.** This chapter specifies the living clock (`AIronwakeVoyage::TickLiving`, `IronwakeP11.cpp`), the sun-position maths for latitude 50 N, the six weather states and every per-state parameter, the weather drift chain and blend times, the lighting mixer that turns sun altitude plus weather into sun/sky/fog/exposure/grade values (including the P13 dawn/dusk ramps and the P14 clear-noon lift), the night star dome (`AIronwakeNightSky`, `IronwakeP8.cpp`, `M_IW_NightSky`), the moon (phase table, sprite selection, moonlight), the lamps/windows keyed to sun altitude, and the weather→sea/cloud/rain/lightning coupling (`AIronwakeStorm`). Citations are generated from the source (format `value` [src path:line]). Numbers marked *derived* are computed here from a cited formula and carry no tag. **CONFLICT** marks places where a log/comment disagrees with the code that actually runs. The legacy 6-preset weather (`SetWeather`) is described only where the live system reuses it.

## 8.1 Which system is live

- The living world (clock + drift weather) is started in `BeginPlay` unless any test mode is armed or `-iwlegacywx` is passed: `StartLiving()` [src Source/IronwakeTest/IronwakeVoyage.cpp:343] (condition lists A1/E1/P8/P9/S2/W2/storm/test modes `bOtherTest` [src Source/IronwakeTest/IronwakeVoyage.cpp:342]). Note `-iwp11=` / `-iwp12=` modes are *not* in `bOtherTest`, so P11/P12 test runs still start living mode.
- `SetWeather()` (legacy presets, tests) sets `bLiving = false` `bLiving = false` [src Source/IronwakeTest/IronwakeVoyage.cpp:728]; `L` while living calls `CycleWeatherLive`, not `SetWeather` `CycleWeatherLive` [src Source/IronwakeTest/IronwakeVoyage.cpp:737].
- Per-frame order: `ApplyWeather(RealDt)` → `TickLiving(Dt)` (if living) → apply sun/sky/fog/PPV/lamps `TickLiving(Dt)` [src Source/IronwakeTest/IronwakeVoyage.cpp:756]. `ApplyWeather` is called with **real** delta time `ApplyWeather(RealDt)` [src Source/IronwakeTest/IronwakeVoyage.cpp:1180]; it is skipped while the world is paused `IsPaused()` [src Source/IronwakeTest/IronwakeVoyage.cpp:1179].

## 8.2 Clock

| Parameter | Value | Source |
|---|---|---|
| Start hour (`ClockHour`) | `9.5` [src Source/IronwakeTest/IronwakeVoyage.h:97] | game clock 0..24 |
| Real minutes per game day at x1 (`DayLenMin`) | `48` [src Source/IronwakeTest/IronwakeVoyage.h:98] | `-iwdaymin=` overrides, min `0.5` `0.5` [src Source/IronwakeTest/IronwakeP11.cpp:85] |
| Hours per real second | `24 / (DayLenMin·60)` | `24.f / (DayLenMin * 60.f)` [src Source/IronwakeTest/IronwakeP11.cpp:131] — *derived*: 0.5 game h per real min (1 game h = 120 real s; 1 real s = 30 game s) |
| Speed index | `0` x1, `1` x10, `2` paused | `{ 1.f, 10.f, 0.f }` [src Source/IronwakeTest/IronwakeP11.cpp:129] (cycle with T) |
| Rollover | at ≥ 24 subtract 24 and `DayIdx = (DayIdx+1) % 365` | `% 365` [src Source/IronwakeTest/IronwakeP11.cpp:135] |
| Jump forward | `3` h (Shift+T or `]`) | `JumpHours(3.f)` [src Source/IronwakeTest/IronwakePlayerController.cpp:814] |
| Start overrides | `-iwhour=`, `-iwskyhour=` (also pauses), `-iwwx=`, `-iwnodrift` | `iwhour=` [src Source/IronwakeTest/IronwakeP11.cpp:86] `iwnodrift` [src Source/IronwakeTest/IronwakeP11.cpp:90] |
| Initial weather | state `1` (Fair) unless `-iwwx=` | `LiveState = 1` [src Source/IronwakeTest/IronwakeP11.cpp:89] |

- **Day of year (season)** is taken from the **real wall-clock date** at launch: `DayIdx = clamp(DayOfYear − 1, 0, 364)` `GetDayOfYear() - 1` [src Source/IronwakeTest/IronwakeP8.cpp:48], override `-iwskyday=` `iwskyday=` [src Source/IronwakeTest/IronwakeP8.cpp:50]. The day index only advances at midnight rollover during play. Fallback `313` is used by the sun if no NightSky exists `313` [src Source/IronwakeTest/IronwakeP11.cpp:162]. Logged runs show `day=313` (10 Nov) `day=313` [src build/logs/P8/p7_circle_ironclad_30-backup-2026.11.11-03.31.42.log:1505].
- Because the season follows the calendar, the game is **not deterministic across dates**: sun altitude at noon varies from about 16.6° (late Dec) to 63.4° (late June) at 50 N (*derived*, see 8.3). The code comments assume a low November sun (`~22 deg`) `~22 deg` [src Source/IronwakeTest/IronwakeP11.cpp:188].
- Clock label (HUD) by hour: `<4.5` small hours, `<7` dawn, `<11` morning, `<13.5` noon, `<15.5` afternoon, `<18` dusk, `<21` evening, else night `H < 4.5f` [src Source/IronwakeTest/IronwakeP11.cpp:67] `H < 18.f` [src Source/IronwakeTest/IronwakeP11.cpp:68]. Suffix ` x10` / ` (paused)`.
- The star dome's own hour mirrors the clock every frame and is `bHold = true` in living mode `bHold = true` [src Source/IronwakeTest/IronwakeP11.cpp:137]. When not held the dome's own clock would run at `ClockRate` `60` game s per real s `ClockRate = 60.f` [src Source/IronwakeTest/IronwakeP8.h:33] (unused in living mode; **CONFLICT in meaning**: that rate is 30× faster than the living clock's 30 game s/real s).
- Controls (from the help panel): `T` clock speed x1/x10/pause, `Shift+T` jump +3 h, `L` next weather, `K` weather drift on/off `T          clock speed: x1 / x10 / pause` [src Source/IronwakeTest/IronwakeP11.cpp:375] `K          weather drift on / off` [src Source/IronwakeTest/IronwakeP11.cpp:378].

## 8.3 Sun position (latitude 50 N)

Observer: latitude `Phi` = `50.0` [src Source/IronwakeTest/IronwakeP11.cpp:161] deg (the star dome has `LatDeg` `50.f` [src Source/IronwakeTest/IronwakeP8.h:36] — same value, two independent constants). World axes +X north, +Y east, +Z up.

```
Doy  = DayIdx + 1
LamS = 360 * (Doy - 80) / 365.25   (deg, sun ecliptic longitude; vernal equinox at day 80)
Eps  = 23.44 deg
Dec  = asin( sin(LamS) * sin(Eps) )
Ha   = (ClockHour - 12) * 15 deg                  (solar time; no equation of time)
SunDir.X = cos(Phi) sin(Dec) - sin(Phi) cos(Dec) cos(Ha)
SunDir.Y = -cos(Dec) sin(Ha)
SunDir.Z = sin(Phi) sin(Dec) + cos(Phi) cos(Dec) cos(Ha)
Alt = asin(SunDir.Z);  light rotation = (-SunDir).Rotation()
```
Source: `Doy - 80.0` [src Source/IronwakeTest/IronwakeP11.cpp:163] and the vector line `FMath::Cos(Phi) * FMath::Sin(Dec)` [src Source/IronwakeTest/IronwakeP11.cpp:166]; light rotation `(-SunDir).Rotation()` [src Source/IronwakeTest/IronwakeP11.cpp:169]. Hour angle: `(ClockHour - 12.0) * 15.0` [src Source/IronwakeTest/IronwakeP11.cpp:165]. Local noon is at clock `12:00` exactly (no longitude, no equation of time). The sun's yaw is stored in `LiveSunYaw` and used for the light's yaw `LiveSunYaw = LightRot.Yaw` [src Source/IronwakeTest/IronwakeP11.cpp:170] `LiveSunYaw : SunYaw` [src Source/IronwakeTest/IronwakeVoyage.cpp:779]; pitch is `Cur.SunPitch = LightRot.Pitch` `Cur.SunPitch = LightRot.Pitch` [src Source/IronwakeTest/IronwakeP11.cpp:178].

*Derived* sun altitude (deg) from the formula, Doy 315 (11 Nov, "today" in the build logs), 172 (summer), 355 (winter):

| Clock | 0 | 6 | 7 | 8 | 9.5 | 12 | 15 | 16 | 17 | 18 | 21 |
|---|---|---|---|---|---|---|---|---|---|---|---|
| Doy 315 | −58.2 | −13.8 | −4.6 | 3.8 | 14.2 | 21.8 | 11.1 | 3.8 | −4.6 | −13.8 | −42.1 |
| Doy 172 | −16.6 | 17.7 | 27.2 | 36.8 | 50.6 | 63.4 | 46.2 | 36.8 | 27.2 | 17.7 | −6.4 |
| Doy 355 | −63.4 | −17.7 | −8.7 | −0.6 | 9.4 | 16.6 | 6.5 | −0.6 | −8.7 | −17.7 | −46.2 |

Doy 315: declination −18.2°, sunrise (alt 0) 07:32, sunset 16:28, i.e. 8.9 h of daylight (*derived*). Altitude thresholds used by the lighting code reach (Doy 315): alt 3° at 07:54 / 16:06; −5° at 06:58 / 17:02; −9° at 06:31 / 17:29; −16° at 05:46 / 18:14 (*derived*).

## 8.4 Weather: the two systems

### 8.4.1 Baseline presets (`Presets[6]`, `CaptureWeatherBaseline`)

The baseline reads the level's actors (DirectionalLight, SkyLight, ExponentialHeightFog, unbound PostProcessVolume) `CaptureWeatherBaseline` [src Source/IronwakeTest/IronwakeVoyage.cpp:673]. `Day` starts from hard-coded defaults which are then overwritten from the level actors; the **actual** values come from the map and are only visible in the log line. The defaults and the logged level values:

| Field | Code default | Logged level value | Source |
|---|---|---|---|
| SunPitch | `-22.f` [src Source/IronwakeTest/IronwakeVoyage.cpp:680] | `-22.0` | `pitch=-22.0` [src build/logs/P4/clip_try1.log:1506] |
| SunI (lux) | `3.f` [src Source/IronwakeTest/IronwakeVoyage.cpp:605] | `3.00` | `sunI=3.00` [src build/logs/P4/clip_try1.log:1506] |
| SkyI | `1.5` [src Source/IronwakeTest/IronwakeVoyage.cpp:680] | `1.50` | `skyI=1.50` [src build/logs/P4/clip_try1.log:1506] |
| Fog density | `0.02` [src Source/IronwakeTest/IronwakeVoyage.cpp:680] | `0.014` after ×`0.7` | `0.7` [src Source/IronwakeTest/IronwakeVoyage.cpp:691] `fog=0.014` [src build/logs/P4/clip_try1.log:1506] |
| Bias (EV) | `-0.5` [src Source/IronwakeTest/IronwakeVoyage.cpp:680] | `-0.50` | `bias=-0.50` [src build/logs/P4/clip_try1.log:1506] |
| SunC | `(0.8f, 0.84f, 0.9f)` [src Source/IronwakeTest/IronwakeVoyage.cpp:681] | **unknown** (read from level light; not logged) | measure: `ADirectionalLight` colour in the map |
| FogC | `(0.35f, 0.38f, 0.41f)` [src Source/IronwakeTest/IronwakeVoyage.cpp:681] | unknown (read from fog `FogInscatteringLuminance`) | measure in map |
| SunYaw | `-150.f` [src Source/IronwakeTest/IronwakeVoyage.h:288] | read from level light yaw | legacy only |

Fog thinning: day fog 0.020 → 0.014 (E1: so the Needles read at 2.6 km) `0.020 -> 0.014` [src build/logs/E1.md:33]. The log values are from the P4 build; later cards did not change these constants.

Presets (formulas on `Day`; `Sea` is the sea-state index 0 CALM … 4 STORM):

| # | Name (legacy label) | SunPitch | SunI | SkyI | Fog | Bias | Night | SunC | FogC | Sea | Source |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 0 | Day / "OVERCAST DAY (moderate)" | Day | Day | Day | Day | Day | `0` | level | level | `1.f` [src Source/IronwakeTest/IronwakeVoyage.cpp:657] | `Day.Night = 0.f` [src Source/IronwakeTest/IronwakeVoyage.cpp:680] |
| 1 | Dusk "DUSK (calm)" | `-2.5` [src Source/IronwakeTest/IronwakeVoyage.cpp:697] | ×`0.7` [src Source/IronwakeTest/IronwakeVoyage.cpp:691] | ×`0.5` [src Source/IronwakeTest/IronwakeVoyage.cpp:680] | ×`0.8` [src Source/IronwakeTest/IronwakeVoyage.cpp:681] | −`0.3` [src Source/IronwakeTest/IronwakeVoyage.cpp:681] | `0.45` [src Source/IronwakeTest/IronwakeVoyage.cpp:697] | `(1.f, 0.42f, 0.16f)` [src Source/IronwakeTest/IronwakeVoyage.cpp:698] | `(0.32f, 0.16f, 0.08f)` [src Source/IronwakeTest/IronwakeVoyage.cpp:698] | `0.f` [src Source/IronwakeTest/IronwakeVoyage.cpp:654] | |
| 2 | Night "FOGGY NIGHT (rough)" | `6.f` [src Source/IronwakeTest/IronwakeVoyage.cpp:701] | ×`0.03` [src Source/IronwakeTest/IronwakeVoyage.cpp:701] | ×`0.35` [src Source/IronwakeTest/IronwakeVoyage.cpp:681] | ×`1.6` [src Source/IronwakeTest/IronwakeVoyage.cpp:701] | +`1.0` [src Source/IronwakeTest/IronwakeVoyage.cpp:654] | `1.f` [src Source/IronwakeTest/IronwakeVoyage.cpp:635] | `(0.55f, 0.66f, 1.f)` [src Source/IronwakeTest/IronwakeVoyage.cpp:702] | `(0.012f, 0.016f, 0.026f)` [src Source/IronwakeTest/IronwakeVoyage.cpp:702] | `2.f` [src Source/IronwakeTest/IronwakeVoyage.cpp:656] | |
| 3 | Gale "GALE" | `-14.f` [src Source/IronwakeTest/IronwakeVoyage.cpp:706] | ×`0.35` [src Source/IronwakeTest/IronwakeVoyage.cpp:681] | ×`0.55` [src Source/IronwakeTest/IronwakeVoyage.cpp:702] | ×`4.f` [src Source/IronwakeTest/IronwakeVoyage.cpp:706] | −`0.5` [src Source/IronwakeTest/IronwakeVoyage.cpp:680] | `0.15` [src Source/IronwakeTest/IronwakeVoyage.cpp:706] | `(0.62f, 0.68f, 0.76f)` [src Source/IronwakeTest/IronwakeVoyage.cpp:707] | `(0.16f, 0.18f, 0.2f)` [src Source/IronwakeTest/IronwakeVoyage.cpp:707] | `3.f` [src Source/IronwakeTest/IronwakeVoyage.cpp:680] | |
| 4 | Storm "STORM" | `-40.f` [src Source/IronwakeTest/IronwakeVoyage.cpp:710] | ×`0.1` [src Source/IronwakeTest/IronwakeVoyage.cpp:698] | ×`0.55` [src Source/IronwakeTest/IronwakeVoyage.cpp:702] | ×`9.f` [src Source/IronwakeTest/IronwakeVoyage.cpp:710] | −`0.9` [src Source/IronwakeTest/IronwakeVoyage.cpp:681] | `0.3` [src Source/IronwakeTest/IronwakeVoyage.cpp:681] | `(0.52f, 0.58f, 0.68f)` [src Source/IronwakeTest/IronwakeVoyage.cpp:711] | `(0.11f, 0.12f, 0.13f)` [src Source/IronwakeTest/IronwakeVoyage.cpp:711] | `4.f` [src Source/IronwakeTest/IronwakeVoyage.cpp:706] | |
| 5 | Clear night "CLEAR NIGHT (calm)" | (Night) | (Night) | ×`0.5` [src Source/IronwakeTest/IronwakeVoyage.cpp:680] | ×`0.45` [src Source/IronwakeTest/IronwakeVoyage.cpp:697] | +`1.2` [src Source/IronwakeTest/IronwakeVoyage.cpp:714] | (Night) `Clear = 1` `Clr.Clear = 1.f` [src Source/IronwakeTest/IronwakeVoyage.cpp:714] | (Night) | `(0.010f, 0.014f, 0.030f)` [src Source/IronwakeTest/IronwakeVoyage.cpp:715] | `0.5` [src Source/IronwakeTest/IronwakeVoyage.cpp:680] | |

Row-5 `Clr` is a copy of `Night` (`FWx Clr = Night`) `FWx Clr = Night` [src Source/IronwakeTest/IronwakeVoyage.cpp:713]. Using `Day` = (3, 1.5, 0.014, −0.5) the preset absolute values are (*derived*): Dusk SunI 2.1, SkyI 0.75, Fog 0.0112, Bias −0.8; Night SunI 0.09, SkyI 0.525, Fog 0.0224, Bias +0.5; Gale SunI 1.05, SkyI 0.825, Fog 0.056, Bias −1.0; Storm SunI 0.3, SkyI 0.825, Fog 0.126, Bias −1.4; Clear night SkyI 0.75, Fog 0.0063, Bias +0.7. Note the Storm line carries a stray comment that swallowed `Storm.Bias`/`Storm.Night` on the same source line: `// W3: visibility ~1-2 km Storm.Bias` [src Source/IronwakeTest/IronwakeVoyage.cpp:710]. **CONFLICT/BUG:** because of the `//`, `Storm.Bias = Day.Bias - 0.9f; Storm.Night = 0.3f;` never execute; Storm therefore inherits `Day.Bias` (−0.5) and `Night` = Day's `0` (copy of Day). The table row above lists the intended values; the effective Storm Bias is −0.5 and Night 0. Presets 3 and 4 are used **only** by legacy `SetWeather` (tests, `-iwstorm`); the living mixer reads only Presets 0, 1, 2, 5 (8.6).

Legacy blend: `SetWeather(preset, blend)` lerps all fields with smoothstep over `BlendDur`; sea target blends over `max(2·blend, 8)` s `FMath::Max(BlendSeconds * 2.f, 8.f)` [src Source/IronwakeTest/IronwakeVoyage.cpp:729]; auto-cycle (K when not living) steps `{1,0,2,3,4,3,2,0}` every `120` s with `45` s blends `{ 1, 0, 2, 3, 4, 3, 2, 0 }` [src Source/IronwakeTest/IronwakeVoyage.cpp:753] `AutoCycleT >= 120.f` [src Source/IronwakeTest/IronwakeVoyage.cpp:753].

### 8.4.2 Live weather states (`StateWx`) — what actually runs

State names `TEXT("Clear"), TEXT("Fair"), TEXT("Overcast"), TEXT("Mist"), TEXT("Rain"), TEXT("Squall")` [src Source/IronwakeTest/IronwakeP11.cpp:43]. `FLiveWx` fields: cloud cover, SunMul, SkyMul, FogMul, rain, open-sea state index (default struct `float Cover = 0.3f, SunMul = 1.f, SkyMul = 1.f, FogMul = 1.f, Rain = 0.f, Sea = 1.f` [src Source/IronwakeTest/IronwakeVoyage.h:102]).

| # | State | Cover | SunMul | SkyMul | FogMul | Rain | Sea (0–4) | Sea label (code comment) |
|---|---|---|---|---|---|---|---|---|
| 0 | Clear | `0.02` [src Source/IronwakeTest/IronwakeP11.cpp:51] | `1.25` [src Source/IronwakeTest/IronwakeP11.cpp:51] | `1.0` [src Source/IronwakeTest/IronwakeP11.cpp:51] | `0.6` [src Source/IronwakeTest/IronwakeP11.cpp:51] | `0.f` [src Source/IronwakeTest/IronwakeP11.cpp:51] | `0.55` [src Source/IronwakeTest/IronwakeP11.cpp:51] | slight |
| 1 | Fair | `0.30` [src Source/IronwakeTest/IronwakeP11.cpp:52] | `1.1` [src Source/IronwakeTest/IronwakeP11.cpp:52] | `1.0` [src Source/IronwakeTest/IronwakeP11.cpp:52] | `0.9` [src Source/IronwakeTest/IronwakeP11.cpp:52] | `0.f` [src Source/IronwakeTest/IronwakeP11.cpp:52] | `0.95` [src Source/IronwakeTest/IronwakeP11.cpp:52] | slight–moderate |
| 2 | Overcast | `0.88` [src Source/IronwakeTest/IronwakeP11.cpp:53] | `0.45` [src Source/IronwakeTest/IronwakeP11.cpp:53] | `0.9` [src Source/IronwakeTest/IronwakeP11.cpp:53] | `0.9` [src Source/IronwakeTest/IronwakeP11.cpp:53] | `0.f` [src Source/IronwakeTest/IronwakeP11.cpp:53] | `1.7` [src Source/IronwakeTest/IronwakeP11.cpp:53] | moderate–rough |
| 3 | Mist | `0.65` [src Source/IronwakeTest/IronwakeP11.cpp:54] | `0.6` [src Source/IronwakeTest/IronwakeP11.cpp:54] | `0.9` [src Source/IronwakeTest/IronwakeP11.cpp:54] | `4.5` [src Source/IronwakeTest/IronwakeP11.cpp:54] | `0.f` [src Source/IronwakeTest/IronwakeP11.cpp:54] | `0.3` [src Source/IronwakeTest/IronwakeP11.cpp:54] | calm, glassy |
| 4 | Rain | `0.92` [src Source/IronwakeTest/IronwakeP11.cpp:55] | `0.3` [src Source/IronwakeTest/IronwakeP11.cpp:55] | `0.8` [src Source/IronwakeTest/IronwakeP11.cpp:55] | `2.5` [src Source/IronwakeTest/IronwakeP11.cpp:55] | `0.7` [src Source/IronwakeTest/IronwakeP11.cpp:55] | `2.35` [src Source/IronwakeTest/IronwakeP11.cpp:55] | rough |
| 5 | Squall | `0.97` [src Source/IronwakeTest/IronwakeP11.cpp:56] | `0.18` [src Source/IronwakeTest/IronwakeP11.cpp:56] | `0.7` [src Source/IronwakeTest/IronwakeP11.cpp:56] | `2.4` [src Source/IronwakeTest/IronwakeP11.cpp:56] | `1.f` [src Source/IronwakeTest/IronwakeP11.cpp:56] | `3.0` [src Source/IronwakeTest/IronwakeP11.cpp:56] | very rough (gale) |

**CONFLICT:** `build/logs/P11.md` says sea overcast 1.4 / rain 1.9 / squall 2.7 `overcast 1.4, mist 0.3 (glassy), rain 1.9, squall 2.7` [src build/logs/P11.md:11]; P13 raised them to 1.7 / 2.35 / 3.0 `overcast 1.7, rain 2.35, squall 3.0` [src build/logs/P13.md:11]. The code values above take effect. Squall fog multiplier `2.4` (P14 lowered it from 4.0) `fog 4.0 -> 2.4` [src build/logs/P14.md:9].

Derived sea numbers per state (from the Hydro tables, 8.9): significant wave height Hs and wind: Clear Hs 1.54 m / 12.4 kn; Fair 2.02 m / 15.6 kn; Overcast 3.42 m / 21.6 kn; Mist 1.25 m / 10.4 kn; Rain 5.02 m / 28.2 kn; Squall 6.94 m / 36.0 kn (*derived*; the P14 log independently states "Hs 6.9 m in the sea table" for squall `Hs 6.9 m` [src build/logs/P14.md:9]).

Labels shown on the HUD: the sea word uses the **local** sea state `S` : `<0.3` calm, `<0.8` slight, `<1.5` moderate, `<2.4` rough, else very rough `S < 0.3f ? TEXT("calm")` [src Source/IronwakeTest/IronwakeP11.cpp:76] `S < 2.4f ? TEXT("rough")` [src Source/IronwakeTest/IronwakeP11.cpp:76].

### 8.4.3 Drift chain, probabilities and blend times

- A decision is taken every `WxNextIn` **game minutes**, uniform `10–30`: `FRandRange(10.f, 30.f)` [src Source/IronwakeTest/IronwakeP11.cpp:117]; the hold timer advances `Dt·HoursPerRealSec·60·Speed` game minutes `WxHoldT += Dt * HoursPerRealSec * 60.f * Sp` [src Source/IronwakeTest/IronwakeP11.cpp:139]. At x1 one real second is 0.5 game minute, so a decision comes every **20–60 real seconds** (*derived*; x10 → 2–6 s; paused → never). Drift runs only if `bWxDrift` (default `true` `bool bWxDrift = true` [src Source/IronwakeTest/IronwakeVoyage.h:101], toggled with K or `-iwnodrift`).
- At each decision: with probability `0.5` do nothing `FRand() > 0.5f` [src Source/IronwakeTest/IronwakeP11.cpp:144]; otherwise pick one of three neighbours uniformly from the table below; if the pick is Squall (5) it is re-rolled to Rain (4) with probability `0.6` `FRand() > 0.4f` [src Source/IronwakeTest/IronwakeP11.cpp:147]; if the result equals the current state nothing happens.
- Neighbour table (rows = current state, columns = three equal-weight options): `{ { 1, 1, 1 }, { 0, 2, 3 }, { 1, 4, 3 }, { 1, 2, 2 }, { 2, 5, 1 }, { 4, 2, 2 } }` [src Source/IronwakeTest/IronwakeP11.cpp:143].

| From | Options (index → state) | Effective per-decision P(next state) (*derived*; includes the 0.5 no-op and Squall re-roll) |
|---|---|---|
| Clear | 1,1,1 → Fair×3 | Clear 0.500, Fair 0.500 |
| Fair | 0,2,3 → Clear, Overcast, Mist | Clear 0.167, Fair 0.500, Overcast 0.167, Mist 0.167 |
| Overcast | 1,4,3 → Fair, Rain, Mist | Fair 0.167, Overcast 0.500, Mist 0.167, Rain 0.167 |
| Mist | 1,2,2 → Fair, Overcast×2 | Fair 0.167, Overcast 0.333, Mist 0.500 |
| Rain | 2,5,1 → Overcast, Squall, Fair | Fair 0.167, Overcast 0.167, Rain 0.600, Squall 0.067 |
| Squall | 4,2,2 → Rain, Overcast×2 | Overcast 0.333, Rain 0.167, Squall 0.500 |

Stationary distribution of that chain (*derived*, power iteration; decisions are equally spaced in game time so occupancy = visit probability): Clear `0.098`, Fair `0.295`, Overcast `0.277`, Mist `0.191`, Rain `0.122`, Squall `0.016`. Squall is reached only from Rain (never from Overcast/Fair directly). `build/logs/P11.md` records that drift was **not** soaked over a full day `Drift not soaked for a full 48 min day` [src build/logs/P11.md:32]; the distribution is therefore unmeasured in play.
- Blend: on a drift step `LiveBlendDur = uniform(120, 240)` real seconds `FRandRange(120.f, 240.f)` [src Source/IronwakeTest/IronwakeP11.cpp:150]; progress `+= Dt·max(Speed,1)/Dur` (so x10 blends 10× faster and a *paused* clock still blends in real time) `FMath::Max(Sp, 1.f) / LiveBlendDur` [src Source/IronwakeTest/IronwakeP11.cpp:155]; each field is lerped with smoothstep `A = SmoothStep(0,1,t)` (Cover, SunMul, SkyMul, FogMul, Rain, Sea) `FMath::SmoothStep(0.f, 1.f, LiveBlendT)` [src Source/IronwakeTest/IronwakeP11.cpp:156]. `L` (forced next state) and `ForceWeatherState` use an **8 s** blend `ForceWeatherState(LiveState + 1, 8.f)` [src Source/IronwakeTest/IronwakeP11.cpp:123]; test snaps use `0.05` `FMath::Max(BlendSeconds, 0.05f)` [src Source/IronwakeTest/IronwakeP11.cpp:116]. `ForceWeatherState` also re-rolls `WxNextIn` and zeroes the hold timer. The RNG is seeded from `FPlatformTime::Cycles()` — weather is **not** reproducible between runs `FPlatformTime::Cycles()` [src Source/IronwakeTest/IronwakeP11.cpp:83].
- First decision after start: `WxNextIn = uniform(10, 30)` game minutes `WxNextIn = WxRng.FRandRange(10.f, 30.f)` [src Source/IronwakeTest/IronwakeP11.cpp:92]. **CONFLICT (docs):** the declared default `WxNextIn = 300.f` in the header is overwritten at `StartLiving` `WxNextIn = 300.f` [src Source/IronwakeTest/IronwakeVoyage.h:294].

## 8.5 Lighting mixer (`TickLiving`) — sun altitude × weather

Everything is derived each frame from `Alt` (sun altitude, 8.3) and the live weather `LiveCur`. Three base looks are blended by altitude weights:

| Weight | Formula | Meaning |
|---|---|---|
| `Wn` | `1 − SmoothStep(−9, −1, Alt)` | night: 1 below −9°, 0 above −1° |
| `Wd` | `SmoothStep(2, 10, Alt)` | day: 0 below 2°, 1 above 10° |
| `Wk` | `max(0, 1 − Wn − Wd)` | dusk/dawn band (uses the Dusk preset) |
| `Clear` | `clamp(1 − (Cover − 0.2)/0.3, 0, 1)` | 1 for cover ≤ 0.2, 0 for ≥ 0.5 |

Source: `1.f - FMath::SmoothStep(-9.f, -1.f, Alt)` [src Source/IronwakeTest/IronwakeP11.cpp:173] `FMath::SmoothStep(2.f, 10.f, Alt)` [src Source/IronwakeTest/IronwakeP11.cpp:173] `(LiveCur.Cover - 0.2f) / 0.3f` [src Source/IronwakeTest/IronwakeP11.cpp:174].

Sources for the blend: night base = `Presets[2]` (Night) with its `SunI, SkyI, Fog, Bias, FogC` lerped toward `Presets[5]` (Clear night) by `Clear` `Presets[5]` [src Source/IronwakeTest/IronwakeP11.cpp:176]; day = `Presets[0]`, dusk = `Presets[1]` `&NightBase, &Presets[0], &Presets[1]` [src Source/IronwakeTest/IronwakeP11.cpp:177]. Then (weights normalised by their sum):

```
Cur.SunPitch = LightRot.Pitch                      (real sun, not preset)
Cur.SunI  = Mix(SunI)  * LiveCur.SunMul
Cur.SkyI  = Mix(SkyI)  * LiveCur.SkyMul
Cur.Fog   = Mix(Fog)   * LiveCur.FogMul
Cur.Bias  = Mix(Bias)                              (+ P13/P14 terms below)
Cur.Night = Mix(Night)                              (then clamped, 8.7)
Cur.SunC / Cur.FogC = weighted colours
Cur.Clear = Clear
```
Source: `Cur.SunI = Mix` [src Source/IronwakeTest/IronwakeP11.cpp:179] `Cur.SkyI = Mix` [src Source/IronwakeTest/IronwakeP11.cpp:180] `Cur.Fog = Mix` [src Source/IronwakeTest/IronwakeP11.cpp:181]. The mixer weights ignore Presets 3/4 entirely.

*Derived* weights and outputs against sun altitude (Wn/Wd/Wk from the formulas; `Night` after the P13 clamp; star `Fade` from 8.10; `Lamp` from 8.12; `Tw` twilight weight from 8.7):

| Alt° | Wn | Wd | Wk | Night (clear) | Star Fade | Lamp | Tw |
|---|---|---|---|---|---|---|---|
| −20…−16 | 1 | 0 | 0 | 1.000 | 1 | 1 | 0 |
| −14 | 1 | 0 | 0 | 0.931 | 1 | 1 | 0 |
| −12 | 1 | 0 | 0 | 0.771 | 0.677 | 1 | 0.028 |
| −10 | 1 | 0 | 0 | 0.593 | 0.231 | 1 | 0.216 |
| −8 | 0.957 | 0 | 0.043 | 0.469 | 0 | 1 | 0.500 |
| −6 | 0.684 | 0 | 0.316 | 0.450 | 0 | 1 | 0.784 |
| −4 | 0.316 | 0 | 0.684 | 0.450 | 0 | 0.957 | 0.972 |
| −2 | 0.043 | 0 | 0.957 | 0.450 | 0 | 0.684 | 1 |
| 0 | 0 | 0 | 1 | 0.450 | 0 | 0.316 | 1 |
| 2 | 0 | 0 | 1 | 0.450 | 0 | 0.043 | 1 |
| 4 | 0 | 0.156 | 0.844 | 0.380 | 0 | 0 | 0.957 |
| 6 | 0 | 0.5 | 0.5 | 0.225 | 0 | 0 | 0.684 |
| 8 | 0 | 0.844 | 0.156 | 0.070 | 0 | 0 | 0.316 |
| ≥10 | 0 | 1 | 0 | 0 | 0 | 0 | ≤0.043 |

Applied to the scene each frame by `ApplyWeather` (only when `bApply`): directional light rotation `(Cur.SunPitch, LiveSunYaw)` roll 0, `SetIntensity(Cur.SunI)`, `SetLightColor(Cur.SunC)`; sky light `SetIntensity(Cur.SkyI)`; height fog `SetFogDensity(Cur.Fog)` and inscattering colour `Cur.FogC`; post-process volume `AutoExposureBias = Cur.Bias` `S->SetActorRotation(FRotator(Cur.SunPitch` [src Source/IronwakeTest/IronwakeVoyage.cpp:779] `L->SetIntensity(Cur.SunI)` [src Source/IronwakeTest/IronwakeVoyage.cpp:780] `L->SetIntensity(Cur.SkyI)` [src Source/IronwakeTest/IronwakeVoyage.cpp:782] `C->SetFogDensity(Cur.Fog)` [src Source/IronwakeTest/IronwakeVoyage.cpp:785] `AutoExposureBias = Cur.Bias` [src Source/IronwakeTest/IronwakeVoyage.cpp:797]. Volumetric fog is enabled only when `Cur.Night > 0.3`, distance `250000` cm, extinction scale `0.4` `Cur.Night > 0.3f` [src Source/IronwakeTest/IronwakeVoyage.cpp:787] `SetVolumetricFogDistance(250000.f)` [src Source/IronwakeTest/IronwakeVoyage.cpp:791] `SetVolumetricFogExtinctionScale(0.4f)` [src Source/IronwakeTest/IronwakeVoyage.cpp:792].

### 8.5.1 Absolute values per weather state at fixed altitudes (*derived*)

Using the logged baseline `Day = (SunI 3.0, SkyI 1.5, Fog 0.014, Bias −0.5)` and ignoring the P13/P14 post-terms (8.6, 8.7) the **pre-twilight mixer** gives, for full day (`Wd = 1`, alt ≥ 10°): `SunI = 3·SunMul`, `SkyI = 1.5·SkyMul`, `Fog = 0.014·FogMul`:

| State | SunI | SkyI | Fog density | Cover |
|---|---|---|---|---|
| Clear | 3.75 | 1.50 | 0.0084 | 0.02 |
| Fair | 3.30 | 1.50 | 0.0126 | 0.30 |
| Overcast | 1.35 | 1.35 | 0.0126 | 0.88 |
| Mist | 1.80 | 1.35 | 0.0630 | 0.65 |
| Rain | 0.90 | 1.20 | 0.0350 | 0.92 |
| Squall | 0.54 | 1.05 | 0.0336 | 0.97 |

For full night (`Wn = 1`): `SunI = 0.09·SunMul`, `SkyI = 0.525` (overcast) to `0.75` (clear, `Clear=1`), then raised to the ambient floor `max(SkyI, 1.5·0.9·Wn·SkyMul)` = `1.35·SkyMul` (8.7). Bias at night `Day.Bias + 1.0` = +0.5 (overcast) up to +0.7 (clear).

## 8.6 P14 clear/fair noon lift

Block after the mixer: defaults `DS = 2.2` (sky), `DU = 1.2` (sun), `DB = 0.5` (bias), overridable by `-iwday=sky,sun,bias` `static float DS = 2.2f, DU = 1.2f, DB = 0.5f` [src Source/IronwakeTest/IronwakeP11.cpp:190]. Weight `Dl = Wd·clamp(1 − (Cover − 0.15)/0.6, 0, 1)` `(LiveCur.Cover - 0.15f) / 0.6f` [src Source/IronwakeTest/IronwakeP11.cpp:192]. Then `Fog ×= lerp(1, 0.55, Dl)`, `SkyI ×= lerp(1, DS, Dl)`, `SunI ×= lerp(1, DU, Dl)`, `Bias += DB·Dl` `L(1.f, 0.55f, Dl)` [src Source/IronwakeTest/IronwakeP11.cpp:193]. So at full day: Clear (Cover 0.02 → factor 1) SkyI 1.5·1.0·2.2 = 3.30, SunI 3.75·1.2 = 4.50, fog 0.0084·0.55 = 0.0046, bias −0.5+0.5 = 0.0; Fair (Cover 0.30 → clamp(1 − 0.25) = 0.75): lerp factors sky 1.9, sun 1.15, fog 0.6625 → SkyI 2.85, SunI 3.80, Fog 0.0083 (*derived*). Overcast (Cover 0.88) → `Dl = 0`: no lift. The comment explains the intent (low winter sun ≈ 22° + grey grade read dark/flat) `Low winter sun (~22 deg)` [src Source/IronwakeTest/IronwakeP11.cpp:188].

## 8.7 P13 dawn/dusk ramps, clear-day blue, night floor (all altitude-driven)

Twilight weight `Tw = SmoothStep(−13, −3, Alt)·(1 − SmoothStep(3, 11, Alt))` `SmoothStep(-13.f, -3.f, Alt)` [src Source/IronwakeTest/IronwakeP11.cpp:197]. `bDawn = ClockHour < 12` `ClockHour < 12.f` [src Source/IronwakeTest/IronwakeP11.cpp:198]. Warm-horizon weight `Ww = SmoothStep(−6, 0, Alt)·(1 − SmoothStep(1, 9, Alt))` `SmoothStep(-6.f, 0.f, Alt)` [src Source/IronwakeTest/IronwakeP11.cpp:201].

| Quantity | Formula / value | Source |
|---|---|---|
| Night factor clamp | `Cur.Night = min(Cur.Night, 1 − 0.55·SmoothStep(−16, −7, Alt))` (twilight is not night: releases locked exposure, fades stars) | `0.55f * FMath::SmoothStep(-16.f, -7.f, Alt)` [src Source/IronwakeTest/IronwakeP11.cpp:200] |
| Twilight sun colour | dawn `(1, 0.86, 0.80)`, dusk `(1, 0.80, 0.60)` (near-white: the atmosphere does the reddening) | `FLinearColor(1.f, 0.86f, 0.80f)` [src Source/IronwakeTest/IronwakeP11.cpp:204] `FLinearColor(1.f, 0.80f, 0.60f)` [src Source/IronwakeTest/IronwakeP11.cpp:204] |
| Twilight sun intensity | `TwI = Presets[0].SunI·0.85·SmoothStep(−13, 5, Alt)·SunMul`; `SunI = lerp(SunI, max(TwI, SunI·Wd), Tw)` | `DaySunI * 0.85f` [src Source/IronwakeTest/IronwakeP11.cpp:205] |
| Twilight sun colour mix | `SunC = lerp(SunC, TwSun, Tw)` | `Cur.SunC = L(Cur.SunC, TwSun, Tw)` [src Source/IronwakeTest/IronwakeP11.cpp:207] |
| Horizon fog blue (blue hour) | `(0.030, 0.045, 0.115)` | `FogBlue(0.030f, 0.045f, 0.115f)` [src Source/IronwakeTest/IronwakeP11.cpp:209] |
| Horizon warm | dawn `(0.34, 0.19, 0.24)` (pink), dusk `(0.40, 0.20, 0.075)` (orange) | `FLinearColor(0.34f, 0.19f, 0.24f)` [src Source/IronwakeTest/IronwakeP11.cpp:209] `FLinearColor(0.40f, 0.20f, 0.075f)` [src Source/IronwakeTest/IronwakeP11.cpp:209] |
| Fog colour | `FogC = lerp(FogC, lerp(FogBlue, FogWarm, Ww), Tw·(0.45 + 0.55·Clear))` | `Tw * (0.45f + 0.55f * Clear)` [src Source/IronwakeTest/IronwakeP11.cpp:211] |
| Fog density in twilight | `×lerp(1, 1.3 (dawn) / 0.85 (dusk), Tw)` | `bDawn ? 1.3f : 0.85f` [src Source/IronwakeTest/IronwakeP11.cpp:212] |
| Clear-day haze | `FogC → (0.34, 0.46, 0.68)` with weight `clamp(clamp(1 − (Cover−0.15)/0.5)·Wd·0.85·(1−Tw))` | `FLinearColor(0.34f, 0.46f, 0.68f)` [src Source/IronwakeTest/IronwakeP11.cpp:214] |
| Night horizon glow | `NightHz = lerp((0.028, 0.032, 0.042), (0.034, 0.050, 0.100), Clear)`, weight `Wn·(1−Tw)` | `FLinearColor(0.028f, 0.032f, 0.042f)` [src Source/IronwakeTest/IronwakeP11.cpp:216] `FLinearColor(0.034f, 0.050f, 0.100f)` [src Source/IronwakeTest/IronwakeP11.cpp:216] |
| Ambient floor at night | `SkyI = max(SkyI, Presets[0].SkyI·0.9·Wn·SkyMul)` | `Presets[0].SkyI * 0.9f * Wn` [src Source/IronwakeTest/IronwakeP11.cpp:218] |

**Atmosphere tint** (sky + aerial perspective only): found by `TActorIterator<ASkyAtmosphere>`; `SetSkyAndAerialPerspectiveLuminanceFactor(lerp(White, TwTint, Tw·(0.35 + 0.65·Clear)))`, with `TwTint` = dawn `TD (0.62, 0.78, 1.55)` (cool violet) / dusk `TK (0.80, 0.80, 1.25)`; `OtherAbsorptionScale = lerp(0.001881, 0.001881·Oz, Tw)` with `Oz = 3` (`-iwozone=`). The luminance factor is then multiplied again by `lerp(1, TwLumK, Tw)` with `TwLumK = 5.0` (4th value of `-iwtw=`) `FLinearColor TD(0.62f, 0.78f, 1.55f)` [src Source/IronwakeTest/IronwakeP11.cpp:225] `TK(0.80f, 0.80f, 1.25f)` [src Source/IronwakeTest/IronwakeP11.cpp:225] `Oz = 3.f` [src Source/IronwakeTest/IronwakeP11.cpp:225] `0.001881` [src Source/IronwakeTest/IronwakeP11.cpp:231] `float V = 5.0f` [src Source/IronwakeTest/IronwakeP11.cpp:252]. (The twilight sky is therefore ×5 brighter at peak twilight on top of the tint — *derived* from the two cited statements.)

**Twilight grade** (post-process volume, the unbound volume found by `bUnbound`): defaults `K = {sat 1.05, contrast 1.0, toe 0.30, lumK 5.0, biasK 0.8}` (`-iwtw=`) `static float K[5] = { 1.05f, 1.0f, 0.30f, 5.0f, 0.8f }` [src Source/IronwakeTest/IronwakeP11.cpp:235]. Initial grade is captured once from the volume: contrast default `1.12`, toe `0.55` (the map's saturation is read from the actor; the code comment records it as 0.6) `C0 = 1.12f, T0 = 0.55f` [src Source/IronwakeTest/IronwakeP11.cpp:241] `sat 0.6, toe 0.55, contrast 1.12` [src Source/IronwakeTest/IronwakeP11.cpp:233]. With `Tk = Tw·(0.4 + 0.6·Clear)`: saturation lerps S0 → 1.05, contrast `1.12 → 1.0`, toe `0.55 → 0.30`; `Bias += 0.8·Tk` `Cur.Bias += K[4] * Tk + 0.7f * Dk` [src Source/IronwakeTest/IronwakeP11.cpp:249]. With `Dk = Wd·clamp(1 − (Cover − 0.15)/0.5)·(1 − Tw)` (clear/fair day) the saturation lerps further to `1.05` (blue channel `1.12`) and bias gets `+0.7·Dk` `Dk = Wd` [src Source/IronwakeTest/IronwakeP11.cpp:245]: the grey storm grade is lifted so the sky reads blue on clear/fair days. Overcast/mist/rain keep the grey grade.

Total effective Bias (clear day, *derived*): mixer `−0.5` + P14 `0.5·Dl` + P13 `0.7·Dk` ≈ `+0.7`.

## 8.8 Exposure lock, bloom and volumetric fog at clear night

`K = clamp((Cur.Night − 0.5)/0.4, 0, 1)·Cur.Clear`; when `K > 0.01` the post volume overrides `AutoExposureMinBrightness = MaxBrightness = NightEV` (default `0.4`, flag `-iwev=`) and `BloomIntensity = 0.08`; otherwise the captured baseline values are restored (defaults before capture `DMin 0.03`, `DMax 8`, `DBloom 0.675`) `NightEV = FParse::Value(FCommandLine::Get(), TEXT("iwev=")` [src Source/IronwakeTest/IronwakeVoyage.cpp:801] `0.4` [src Source/IronwakeTest/IronwakeVoyage.cpp:792] `BloomIntensity = 0.08f` [src Source/IronwakeTest/IronwakeVoyage.cpp:806] `DBloom = 0.675f` [src Source/IronwakeTest/IronwakeVoyage.cpp:799]. **CONFLICT:** `P10.md` states the lock is `1.0` with bloom `0.15` `AutoExposureMin = Max = 1.0` [src build/logs/P10.md:7]; `P13.md` records the change to EV `0.4`, bloom `0.08` `locked night EV 1.0 -> 0.4` [src build/logs/P13.md:27]; the code (0.4/0.08) governs. `Cur.Clear` is the mixer's `Clear` (cover-based), so the lock holds only when Cover ≲ 0.3 (Clear 0.0 → lock fully off when Cover ≥ 0.5).

## 8.9 Weather → clouds, rain, lightning, sea (`AIronwakeStorm`)

Per frame (living): `St->CloudCover = Cover·(1 − 0.8·NightK·Clear)` with `NightK = clamp((Cur.Night − 0.5)/0.5)`, `St->RainAmount = Rain`, `St->ClearSky = 0` then `SetOpenImmediate(Cur.Sea)` `LiveCur.Cover * (1.f - 0.8f * NightK * Clear)` [src Source/IronwakeTest/IronwakeP11.cpp:291] `St->SetOpenImmediate(Cur.Sea)` [src Source/IronwakeTest/IronwakeP11.cpp:294]. **CONFLICT (ordering):** `ApplyWeather` later sets `St->ClearSky = Cur.Clear` (after `TickLiving` returns) `St->ClearSky = Cur.Clear` [src Source/IronwakeTest/IronwakeVoyage.cpp:816], so the effective `ClearSky` seen by the storm director is `Cur.Clear`, not `0`.

Storm director effects (`TickAtmosphere`): volumetric cloud material (`Cloud_GlobalCoverage`) = `lerp(Base, max(Base, 0.95), Storminess)` where `Base = CloudCover` (when ≥ 0, else the map's value; logged `cloudCov=0.30`) `Cloud_GlobalCoverage` [src Source/IronwakeTest/IronwakeStorm.cpp:180] `cloudCov=0.30` [src build/logs/P5c/cutter_10_13kn-backup-2026.11.10-23.21.33.log:1494]. Density `Cloud_GlobalDensity = lerp(0.008, 0.022, Ov)` and albedo lerp `(0.98,0.98,0.98,0.5) → (0.52,0.56,0.62,0.5)` with `Ov = SmoothStep(0.5, 0.95, Cover)·(1 − ClearSky)` `FMath::Lerp(0.008f, 0.022f, Ov)` [src Source/IronwakeTest/IronwakeStorm.cpp:186] `FLinearColor(0.52f, 0.56f, 0.62f, 0.5f)` [src Source/IronwakeTest/IronwakeStorm.cpp:187] `SmoothStep(0.5f, 0.95f, CloudCover)` [src Source/IronwakeTest/IronwakeStorm.cpp:185]. Storminess = `clamp((Local − 2)/2, 0, 1)` `(Local - 2.f) / 2.f` [src Source/IronwakeTest/IronwakeStorm.h:38].

- **Local sea** at the player: `Local` chases `Open × LandFade` with `FInterpTo` rate `0.5` `FInterpTo(Local, Want, Dt, 0.5f)` [src Source/IronwakeTest/IronwakeStorm.cpp:146]. `LandFade`: harbour basin (harbour-frame box X −50…150 m, Y −160…170 m) min factor `1 − 0.4 = 0.6`, smoothstep out over `250` m; every terrain island and the Needles/St Brannoc: factor `1 − 0.3 = 0.7`, smoothstep from radius to radius + `30000` cm; land radii: Needles `50000`, St Brannoc `70000`, islands `R·1.2` `FMath::Lerp(1.f - 0.4f, 1.f, FMath::SmoothStep(0.f, 250.f, D))` [src Source/IronwakeTest/IronwakeStorm.cpp:112] `FMath::Lerp(1.f - 0.3f, 1.f, U)` [src Source/IronwakeTest/IronwakeStorm.cpp:125] `LandR.Add(50000.f)` [src Source/IronwakeTest/IronwakeStorm.cpp:50] `LandR.Add(70000.f)` [src Source/IronwakeTest/IronwakeStorm.cpp:54] `LandR.Add(I.R * 1.2f)` [src Source/IronwakeTest/IronwakeStorm.cpp:56]. The ocean has a single global spectrum so the **whole visible sea** follows the player's local state (`ST` comment).
- **Rain** (Niagara `NS_IW_Rain`): active when `RainAmount > 0.05` or `Local ≥ 2.6`; spawned 1800 cm ahead of the camera and 1800 cm up, rotated to `WaveTravelYaw` (slant downwind) `RainAmount > 0.05f || Local >= 2.6f` [src Source/IronwakeTest/IronwakeStorm.cpp:160] `Vector() * 1800.f + FVector(0, 0, 1800.f)` [src Source/IronwakeTest/IronwakeStorm.cpp:163]. In living mode Rain state (`0.7`) and Squall (`1.0`) have `RainAmount > 0.05` ⇒ rain; Overcast does not rain unless the local sea ≥ 2.6.
- **Lightning** needs `Storminess ≥ 0.6`, i.e. `Local ≥ 3.2` `St < 0.6f` [src Source/IronwakeTest/IronwakeStorm.cpp:246]. The live Squall's sea is `3.0` (Storminess 0.5, before the land fade), so **no lightning occurs in living mode** (*derived*); lightning is reachable only via the legacy Gale/Storm presets (sea 3 and 4: Storm sea 4 → Storminess 1). Bolt parameters when active: interval `7–18 s / max(St,0.6)`, distance `900–7000` m, flash `0.5–1.4`, thunder delay `dist/343` s, light intensity `Flash·30`, colour `(0.75, 0.82, 1.0)` `FRandRange(7.f, 18.f)` [src Source/IronwakeTest/IronwakeStorm.cpp:248] `FRandRange(900.f, 7000.f)` [src Source/IronwakeTest/IronwakeStorm.cpp:249] `Flash * 30.f` [src Source/IronwakeTest/IronwakeStorm.cpp:193].
- **Open sea state → waves**: states index 0 CALM, 1 MODERATE, 2 ROUGH, 3 GALE, 4 STORM; mean wind `{8, 16, 24, 36, 48}` kn; wave travel yaw `120`° (wind from WNW); group Hs of the primary swell A `{0.85, 1.5, 2.6, 4.8, 8.4}` m; total Gerstner steepness `{0.16, 0.45, 0.72, 0.92, 1.05}`; foam boost `{1.2, 2.6, 5, 8, 11}` `{ 8.f, 16.f, 24.f, 36.f, 48.f }` [src Source/IronwakeTest/IronwakeHydro.cpp:85] `constexpr float WindYaw = 120.f` [src Source/IronwakeTest/IronwakeHydro.cpp:57] `{ 0.85f, 1.5f, 2.6f, 4.8f, 8.4f }` [src Source/IronwakeTest/IronwakeHydro.cpp:73] `{ 0.16f, 0.45f, 0.72f, 0.92f, 1.05f }` [src Source/IronwakeTest/IronwakeHydro.cpp:81] `{ 1.2f, 2.6f, 5.0f, 8.0f, 11.f }` [src Source/IronwakeTest/IronwakeHydro.cpp:83]. Wind gusts `1 + G·sin(0.21 t) + 0.08 sin(0.53 t + 1.3)`, `G = 0.12 + 0.04·clamp(S−2, 0, 2)` `0.12f + 0.04f` [src Source/IronwakeTest/IronwakeFleet.cpp:212]. Hs per weather state is tabulated in 8.4.2.
- **Basin slab**: shown when `Cur.Sea > 0.75` `IronwakeSetBasinCalm(Cur.Sea > 0.75f)` [src Source/IronwakeTest/IronwakeP11.cpp:286] (ch.7 §7.7.2). Test override `SeaOverride` (default `-1`) forces the open sea `float SeaOverride = -1.f` [src Source/IronwakeTest/IronwakeVoyage.h:112].

## 8.10 Night: the star dome (`AIronwakeNightSky`, card P8/P10)

**Actor.** Static-mesh sphere (`/Engine/BasicShapes/Sphere`), scale `800000` ⇒ 400 km radius "beyond the 300 km ocean" `SetRelativeScale3D(FVector(800000.f))` [src Source/IronwakeTest/IronwakeP8.cpp:33]; no collision, no shadow, no DF/indirect lighting influence, hidden until needed; tick group `TG_PostUpdateWork`; location snapped to the camera every tick `SetActorLocation(CM->GetCameraLocation())` [src Source/IronwakeTest/IronwakeP8.cpp:99]. Spawned by `AIronwakeVoyage::BeginPlay` after the weather baseline `AIronwakeNightSky>` [src Source/IronwakeTest/IronwakeVoyage.cpp:266].

**Visibility/fade.** `Fade = clamp((Night − 0.5)/0.4, 0, 1)·Clear`; dome visible when `Fade > 0.003` `(Night - 0.5f) / 0.4f` [src Source/IronwakeTest/IronwakeP8.cpp:81] `Fade > 0.003f` [src Source/IronwakeTest/IronwakeP8.cpp:82]. In living mode `Night, Clear` are `Cur.Night, Cur.Clear` via `SetState` `NightSky->SetState(Cur.Night, Cur.Clear)` [src Source/IronwakeTest/IronwakeVoyage.cpp:815]. From the table in 8.5: `Fade` is 0 for sun altitude ≳ −8.5°, 0.05 at −9°, 0.23 at −10°, 0.45 at −11°, 0.68 at −12°, 0.89 at −13°, 1.0 at ≤ −14° (*derived*; a clear sky is required: with Cover 0.30 (Fair) `Clear = 0.667`, with Cover ≥ 0.5 stars are off). **CONFLICT (comment vs code):** the P13 code comment says stars are "gone by ~−7 deg, full below ~−16" `stars gone by ~-7 deg, full below ~-16` [src Source/IronwakeTest/IronwakeP11.cpp:199]; the clamp reaches 1.0 near −14° and 0 at −8.5°. The material also fades with the moon (`MoonFade = Fade`).

**Textures.** Star map: NASA/Goddard SVS *Deep Star Maps 2020* 8k EXR (`build/assets/p10/starmap_2020_8k.exr`) imported as `/Game/Ironwake/Sky/T_IW_StarHDR`: `srgb = False`, compression `TC_HDR` (RGBA16F, uncompressed), mips `TMGS_SIMPLE_AVERAGE`, `max_texture_size 8192`, address X wrap / Y clamp, filter bilinear `starmap_2020_8k.exr` [src Content/Python/iw_p10_sky.py:24] `TC_HDR` [src Content/Python/iw_p10_sky.py:30] `TMGS_SIMPLE_AVERAGE` [src Content/Python/iw_p10_sky.py:31] `8192` [src Content/Python/iw_p10_sky.py:32] `TA_WRAP` [src Content/Python/iw_p10_sky.py:33] `TF_BILINEAR` [src Content/Python/iw_p10_sky.py:34]. (P8's first attempt, an 8-bit sRGB PNG, BC7, was dropped: `8-bit PNG path dropped` [src build/logs/P10.md:5].) The P8 baked glow map `T_IW_StarGlow.exr` is **not** used by the final material: in `iw_p10_sky.py` the `GlowTex` parameter is bound to the same HDR star texture (`gt ... texture, star`) `set_editor_property("texture", star)` [src Content/Python/iw_p10_sky.py:57] — the "glow" is a blurred (mip `GlowMip`) sample of the HDR. Moon sprites: `T_IW_MoonPhase0..7` loaded at BeginPlay `T_IW_MoonPhase%d` [src Source/IronwakeTest/IronwakeP8.cpp:47].

**Material** `/Game/Ironwake/Sky/M_IW_NightSky` (made by `iw_p10_sky.py`): Unlit, `BLEND_ADDITIVE`, two-sided; one Custom HLSL node into Emissive `MSM_UNLIT` [src Content/Python/iw_p10_sky.py:42] `BLEND_ADDITIVE` [src Content/Python/iw_p10_sky.py:43]. Shader (verbatim logic):

```
d  = normalize(WorldPos - CameraPos)
q  = (dot(R0,d), dot(R1,d), dot(R2,d))
al = atan2(q.y, q.x);  de = asin(clamp(q.z,-1,1))
uv = (0.5 - al/2pi, 0.5 - de/pi)                       // equirectangular, RA right-to-left
s0 = StarTex @mip0;  s1 = StarTex @mip1 * 2;  sm = max(s0, s1)
gl = GlowTex @mip GlowMip
s  = max(sm - Gamma, 0) * PtGain + gl * GlowGain
ext = smoothstep(saturate(d.z / 0.12))                 // horizon extinction below ~7 deg
col = s * StarGain * StarFade * ext
moon disc blended over col (below)
```
Source: `float2 uv = float2(0.5 - al / 6.2831853, 0.5 - de / 3.14159265)` [src Content/Python/iw_p10_sky.py:72] `Texture2DSampleLevel(StarTex, StarTexSampler, uv, 1.0).rgb * 2.0` [src Content/Python/iw_p10_sky.py:76] `max(sm - Gamma, 0.0) * PtGain + gl * GlowGain` [src Content/Python/iw_p10_sky.py:80] `saturate(d.z / 0.12)` [src Content/Python/iw_p10_sky.py:81].

**Parameters pushed each visible frame** (C++ values override the asset defaults):

| Parameter (material name) | C++ value | Material default (asset) | Source (C++) | Source (default) |
|---|---|---|---|---|
| `Gamma` (point threshold) = `SkyGamma` | `0.08` | `0.55` | `float SkyGamma = 0.08f` [src Source/IronwakeTest/IronwakeP8.h:38] | `S("Gamma", 880, 0.55)` [src Content/Python/iw_p10_sky.py:55] |
| `PtGain` | `5` | `1.6` | `PtGain = 5.f` [src Source/IronwakeTest/IronwakeP8.h:37] | `S("PtGain", 940, 1.6)` [src Content/Python/iw_p10_sky.py:55] |
| `GlowGain` | `0.6` | `0.03` | `GlowGain = 0.6f` [src Source/IronwakeTest/IronwakeP8.h:37] | `S("GlowGain", 1000, 0.03)` [src Content/Python/iw_p10_sky.py:55] |
| `GlowMip` | `4.5` | `4.0` | `GlowMip = 4.5f` [src Source/IronwakeTest/IronwakeP8.h:37] | `S("GlowMip", 1060, 4.0)` [src Content/Python/iw_p10_sky.py:55] |
| `StarGain` | `1` | `8` | `StarGain = 1.f` [src Source/IronwakeTest/IronwakeP8.h:39] | `S("StarGain", 360, 8.0)` [src Content/Python/iw_p10_sky.py:55] |
| `MoonGain` | `1.0` | `30` | `MoonGain = 1.0f` [src Source/IronwakeTest/IronwakeP8.h:39] | `S("MoonGain", 540, 30.0)` [src Content/Python/iw_p10_sky.py:55] |
| `MoonCos` | `cos(1°)` (MoonRadiusDeg `1.0`) | `0.9999` | `MoonRadiusDeg = 1.0f` [src Source/IronwakeTest/IronwakeP8.h:39] | `S("MoonCos", 480, 0.9999)` [src Content/Python/iw_p10_sky.py:55] |
| `StarFade`, `MoonFade` | `Fade` | `1` | `SetScalarParameterValue(TEXT("StarFade"), Fade)` [src Source/IronwakeTest/IronwakeP8.cpp:129] | |
| `R0,R1,R2,MoonDir` | rotation rows, moon dir | – | `SetVectorParameterValue(TEXT("R0"), R0)` [src Source/IronwakeTest/IronwakeP8.cpp:122] | |

`PtThresh` (`0.05`) is a C++ member and `-iwptth=` flag but **is never pushed** to the material; the material's thresholding parameter is `Gamma` (= `SkyGamma`, flag `-iwskygamma=`) `float PtThresh = 0.05f` [src Source/IronwakeTest/IronwakeP8.h:37] `iwptth=` [src Source/IronwakeTest/IronwakeP8.cpp:55] — **CONFLICT/dead parameter** (`P10.md` quotes `SkyGamma 0.08 / PtGain 5 / GlowGain 0.6` consistent with the pushed values) `SkyGamma 0.08` [src build/logs/P10.md:6]. Mips/taps: two taps (mip `0` and mip `1`×`2`, max) plus one glow tap at mip `GlowMip`; the HDR texture has mips generated by simple averaging, 8192×4096 (`max_texture_size 8192`).

**Rotation maths.** (world +X north, +Y east, +Z up; observer latitude `LatDeg` = 50)
```
Doy   = DayIdx + 1
LamS  = 360 * (Doy - 80) / 365.25     (sun ecliptic longitude), Eps = 23.44
RaS   = atan2(sin(LamS) cos(Eps), cos(LamS))
Theta = RaS + (Hour - 12) * 15 deg    (local sidereal time = sun RA + sun hour angle)
R0 = (-cT sP, -sT,  cT cP)    R1 = (-sT sP, cT, sT cP)    R2 = (cP, 0, sP)     (cT=cos Theta, sT=sin Theta, cP=cos Phi, sP=sin Phi)
```
`(Doy - 80.0) / 365.25` [src Source/IronwakeTest/IronwakeP8.cpp:104] `Theta = RaS + (Hour - 12.0) * 15.0 * D2R` [src Source/IronwakeTest/IronwakeP8.cpp:90] `FLinearColor R0((float)(-cT * sP)` [src Source/IronwakeTest/IronwakeP8.cpp:109]. The dome's own `Hour` equals the live `ClockHour` every frame (8.2). The dome ignores the sun's declination-vs-hour except through `RaS`; same `Phi` and `Eps` as the sun block, so sun and stars stay consistent. `DirOf(RA, Dec)` maps a J2000 RA/Dec into world space with the same rows (test aiming) `FVector AIronwakeNightSky::DirOf` [src Source/IronwakeTest/IronwakeP8.cpp:63]. Cross-check recorded in `P8.md`: Orion's belt at alt 23°, az 123° at 23:00 on day 313 `alt 23 deg / az 123` [src build/logs/P8.md:14].

**Exposure at clear night** is locked (8.8) and bloom lowered so stars stay points. Weather coupling: overcast/mist/rain/squall give `Clear = 0` ⇒ `Fade = 0` ⇒ dome hidden; the volumetric clouds are thinned at clear night by `CloudCover·(1 − 0.8·NightK·Clear)` (8.9).

**Test aiming** (P8 shot list): Orion at hour `23`, RA `83.8°`, Dec `−1.2°`; Cygnus at hour `21`, RA `305°`, Dec `40°`; FOV 90 for both `{ TEXT("P8_sky_orion"), 5, 450.f, 55.f, true, 23.f, -998.f, 83.8f }` [src Source/IronwakeTest/IronwakeP8.cpp:304] `{ TEXT("P8_sky_cygnus"), 5, 450.f, 55.f, true, 21.f, -997.f, 305.f }` [src Source/IronwakeTest/IronwakeP8.cpp:305] `-1.2f : 40.f` [src Source/IronwakeTest/IronwakeP8.cpp:344]. Vision scores recorded: Orion 9.5/10, Cygnus/Milky Way 9–10/10 (P10) `Orion 9.5/10` [src build/logs/P10.md:4].

## 8.11 Moon

**Phase table.** `GMoonPhaseDeg[365]` — moon phase angle by day of year for **2027** (0 = new, 180 = full), generated from `models/sky_final/moon_2027.json` (skyfield DE421) `moon phase angle by day of year 2027` [src Source/IronwakeTest/IronwakeMoonTable.inc:1]. First entry `284.29` (1 Jan), last `34.78` (31 Dec) `284.29` [src Source/IronwakeTest/IronwakeMoonTable.inc:2]. Indexed by `DayIdx` (0..364), so the *sun's date* is the real current day while the moon table is the 2027 almanac — the phase on a given calendar date is a **2027** phase regardless of the real year. Derived from the table: new moon (phase nearest 0) at Doy 8, 38, 67, 97, 126, 156, 185, 214, 244, 273, 303, 332, 362; full (nearest 180) at Doy 23, 52, 81, 111, 140, 170, 200, 229, 259, 289, 318, 348 (*derived*). Entry for DayIdx 313 (14 Nov-ish) = `130.25` (log: "moon phase 130 deg") `130 deg` [src build/logs/P8/p7_circle_ironclad_30-backup-2026.11.11-03.31.42.log:1505].

**Direction.** Elongation = phase east of the sun along the ecliptic (inclination ignored): `LamM = LamS + phase`; `RaM = atan2(sin LamM cos Eps, cos LamM)`; `DecM = asin(sin LamM sin Eps)`; `H = Theta − RaM`; `MoonDirNEU = (cP sin Dec − sP cos Dec cos H, −cos Dec sin H, sP sin Dec + cP cos Dec cos H)` `LamM = LamS + MoonPhaseDeg * D2R` [src Source/IronwakeTest/IronwakeP8.cpp:92] `DecM = FMath::Asin(FMath::Sin(LamM) * FMath::Sin(Eps))` [src Source/IronwakeTest/IronwakeP8.cpp:94] `MoonDirNEU = FVector(cP * FMath::Sin(DecM)` [src Source/IronwakeTest/IronwakeP8.cpp:96]. The direction stays current while the dome is hidden (P13): same maths in the `!bVis` branch `moon's direction/phase stays current` [src Source/IronwakeTest/IronwakeP8.cpp:86]. Example (*derived*): DayIdx 314 (Doy 315, phase 141.75°), moon altitude at hour 0/3/6/9/12/15/18/21/23 = 35.6, 9.2, −18.3, −34.3, −26.2, −1.1, 27.0, 44.8, 41.6°.

**Phase sprite.** `MoonSprite = round(phase/45) & 7` → one of 8 textures `T_IW_MoonPhase0..7` (0 new … 4 full … 7 waning crescent); member default `4` `RoundToInt(MoonPhaseDeg / 45.f)) & 7` [src Source/IronwakeTest/IronwakeP8.cpp:120] `int32 MoonSprite = 4` [src Source/IronwakeTest/IronwakeP8.h:43]. Phase 130.25 → sprite `3` (log: "phase 130 deg -> sprite 3") `phase 130 deg -> sprite 3` [src build/logs/P8.md:14]. Sprites come from `moon_phase_%d.png` (LROC) copied by `build/p8/skyprep.py` `moon_phase_%d.png` [src build/p8/skyprep.py:15]. The sprite PNGs live outside the repo (`~/Projects/game-concepts/ironwake/models/sky_final/`): **unknown** pixel size (imported at max 512 in the first script `512` [src Content/Python/iw_p8_sky.py:35]).

**Disc rendering** (shader): disc radius `MoonRadiusDeg` = `1.0`° (`MoonCos = cos 1°`; the real Moon is ≈ 0.5°, so it is drawn 2× size); sprite UV from the disc-plane coordinates (`Rt = normalize(cross(Z, M))`, `Up = cross(M, Rt)`); edge softness `saturate((1 − |p|)·20)`; horizon extinction `mex = saturate((M.z + 0.02)/0.1)`; `col = col·(1 − disc·mo.a) + mo.rgb·mo.a·disc·MoonGain·MoonFade·mex` `float disc = saturate((1.0 - length(p)) * 20.0)` [src Content/Python/iw_p10_sky.py:94] `float mex = saturate((M.z + 0.02) / 0.1)` [src Content/Python/iw_p10_sky.py:95] `mo.rgb * mo.a * disc * MoonGain * MoonFade * mex` [src Content/Python/iw_p10_sky.py:96].

**Moonlight (`UDirectionalLightComponent "MoonLight"`)** (P13/P14): colour `(0.55, 0.68, 1.0)`, movable, not an atmosphere sun light, casts shadows unless `-iwmoonnoshadow`; peak `MoonMax` default **`4.0`** (`-iwmoon=`) `MoonLight->SetLightColor(FLinearColor(0.55f, 0.68f, 1.f))` [src Source/IronwakeTest/IronwakeP11.cpp:263] `SetAtmosphereSunLight(false)` [src Source/IronwakeTest/IronwakeP11.cpp:264] `float MoonMax = 4.f` [src Source/IronwakeTest/IronwakeP11.cpp:268]. (**CONFLICT:** `P13.md` quotes 2.5 for the P13 default; P14 raised it to 4.0 `moon 4.0` [src build/logs/P14.md:8]; code governs.) Intensity:

```
MAlt  = asin(MoonDir.Z)   (deg)
Illum = 0.5 * (1 - cos(phase))                         (0 new … 1 full)
Up    = SmoothStep(-2, 9, MAlt)
LiveMoonI = MoonMax * (0.10 + 0.90*Illum) * Up * Wn * (0.22 + 0.78*Clear)
rotation  = (-MoonDir).Rotation()
```
`0.5f * (1.f - FMath::Cos(FMath::DegreesToRadians(NightSky->MoonPhaseDeg)))` [src Source/IronwakeTest/IronwakeP11.cpp:273] `FMath::SmoothStep(-2.f, 9.f, MAlt)` [src Source/IronwakeTest/IronwakeP11.cpp:274] `(0.10f + 0.90f * Illum)` [src Source/IronwakeTest/IronwakeP11.cpp:275]. Full moon high at clear night: `4.0·1.0·1·1·1 = 4.0`; new moon: `0.4`; overcast (`Clear 0`) multiplies by `0.22`.

**Moon-lit water.** `IronwakeSea::SetNightWater(K)` with `K = clamp(Up·Wn·(0.15 + 0.85·Clear)·(0.3 + 0.7·Illum)·Wg)` (`Wg` = `-iwwater=`, default 1) lerps the water `Scattering` from the state colour `lerp((0.012, 0.016, 0.018), (0.006, 0.007, 0.009), Sk)` (`Sk = clamp((state−1)/3)`) toward `(0.35, 0.55, 1.0)` `(0.15f + 0.85f * Clear) * (0.3f + 0.7f * Illum)` [src Source/IronwakeTest/IronwakeP11.cpp:277] `FLinearColor(0.35f, 0.55f, 1.0f)` [src Source/IronwakeTest/IronwakeHydro.cpp:202].

## 8.12 Lamps, windows, beacons vs sun altitude

- `LiveLamp = 1 − SmoothStep(−5, 3, Alt)` (UE `SmoothStep` needs A<B; an earlier inverted call put lamps on by day — fixed in P12) `1.f - FMath::SmoothStep(-5.f, 3.f, Alt)` [src Source/IronwakeTest/IronwakeP11.cpp:283] `SmoothStep(3, -5, Alt)` [src build/logs/P12.md:14]. Heavy cloud bonus: if `Cover > 0.8`, `LiveLamp = max(LiveLamp, (1 − SmoothStep(−2, 8, Alt))·0.8)` `LiveCur.Cover > 0.8f` [src Source/IronwakeTest/IronwakeP11.cpp:284] `FMath::SmoothStep(-2.f, 8.f, Alt)) * 0.8f` [src Source/IronwakeTest/IronwakeP11.cpp:284]. So lamps are 0 by 3°, 0.32 at 0°, 0.68 at −2°, ≈1 at −5°. Doy 315: lamps start fading in at 16:06 and are full at 17:02 (*derived*; 8.3). Cloud states (Overcast/Rain/Squall: Cover 0.88/0.92/0.97) reach ≥ 0.4 while the sun is at 3° and zero only above 8°.
- Applied value `N = bLiving ? LiveLamp : clamp((Cur.Night − 0.2)/0.8)` `bLiving ? LiveLamp : FMath::Clamp((Cur.Night - 0.2f) / 0.8f, 0.f, 1.f)` [src Source/IronwakeTest/IronwakeVoyage.cpp:819].
- **Windows**: shared dynamic material scalar `On = N·0.12` (P14: window emissive ×0.12) `N * 0.12f` [src Source/IronwakeTest/IronwakeVoyage.cpp:820]. Base window colour `(2.4, 1.15, 0.38)` (ch.7).
- **Point lights** (`NightLamps`): each light's `cd=<candelas>` tag gives full intensity `Cd`; applied intensity `Cd·N·0.18·Blink` (`-iwnolamps` zeroes) `Cd * N * 0.18f * Blink` [src Source/IronwakeTest/IronwakeVoyage.cpp:836] `iwnolamps` [src Source/IronwakeTest/IronwakeVoyage.cpp:828]. Resulting full-night candelas (*derived*): quay/pier/village lamps `9000 → 1620`; wall lamps `4000 → 720`; town street lamps `2200 → 396`; channel buoys `15000 → 2700` (blink); wreck buoy `20000 → 3600`; harbour red `60000 → 10800`; north green `40000 → 7200`; Morrow Head beacon `80000 → 14400`. Default `Cd` if the tag is missing `10000` `float Cd = 10000.f` [src Source/IronwakeTest/IronwakeVoyage.cpp:832]. Light radii and colours: ch.7.
- **Buoy blink**: `Blink = 1` for the first 1 s of every 3 s (phase offset `X·0.0001`), else `0.08` `Fmod(LampBlink + L->GetComponentLocation().X * 0.0001f, 3.f) < 1.f ? 1.f : 0.08f` [src Source/IronwakeTest/IronwakeVoyage.cpp:835].
- **Voyage `Beam`** (St Brannoc rotating spot): intensity `N·2.0e6`, yaw rate `30°/s`, pitch −1.5 `Beam->SetIntensity(N * 2.0e6f)` [src Source/IronwakeTest/IronwakeVoyage.cpp:842] `Dt * 30.f` [src Source/IronwakeTest/IronwakeVoyage.cpp:840] `FRotator(-1.5f, BeamYaw, 0.f)` [src Source/IronwakeTest/IronwakeVoyage.cpp:841]. **St Brannoc `LampLight`** is a separate point light pulsing `15000 + 160000·max(0, sin 0.9t)^8` **regardless of time of day** (set every tick in `TickAmbience`, not gated by `N`) `15000.f + 160000.f * FMath::Pow` [src Source/IronwakeTest/IronwakeLivingSea.cpp:690]. **CONFLICT/behaviour:** the lamp glows by day (its emissive mesh `(4, 2.6, 1.2)` too); only the rotating `Beam` follows the night factor.
- Legacy (non-living) lamp curve: `N = clamp((Cur.Night − 0.2)/0.8, 0, 1)` using the preset `Night` field.

## 8.13 Control surface, flags, logs

| Flag / key | Effect | Source |
|---|---|---|
| `-iwdaymin=` | game-day length (real minutes) | `iwdaymin=` [src Source/IronwakeTest/IronwakeP11.cpp:85] |
| `-iwhour=` / `-iwskyhour=` | start hour / start hour + pause | `iwskyhour=` [src Source/IronwakeTest/IronwakeP11.cpp:87] |
| `-iwwx=` | start weather state 0–5 | `iwwx=` [src Source/IronwakeTest/IronwakeP11.cpp:88] |
| `-iwnodrift` | disable drift | `iwnodrift` [src Source/IronwakeTest/IronwakeP11.cpp:90] |
| `-iwskyday=` / `-iwskyrate=` | day index 0..364 / dome clock rate (unused when held) | `iwskyrate=` [src Source/IronwakeTest/IronwakeP8.cpp:52] |
| `-iwday=sky,sun,bias` | P14 noon lift | `iwday=` [src Source/IronwakeTest/IronwakeP11.cpp:191] |
| `-iwground=` | town ground tint ×K (ch.7) | `iwground=` [src Source/IronwakeTest/IronwakeP12.cpp:50] |
| `-iwtw=sat,contrast,toe,lumK,biasK` | twilight grade | `iwtw=` [src Source/IronwakeTest/IronwakeP11.cpp:236] |
| `-iwtintdawn=` / `-iwtintdusk=` / `-iwozone=` | atmosphere tint / absorption | `iwozone=` [src Source/IronwakeTest/IronwakeP11.cpp:228] |
| `-iwmoon=` / `-iwmoonnoshadow` / `-iwwater=` | moonlight peak / shadows / water glow | `iwmoonnoshadow` [src Source/IronwakeTest/IronwakeP11.cpp:265] |
| `-iwev=` | night EV lock | `iwev=` [src Source/IronwakeTest/IronwakeVoyage.cpp:801] |
| `-iwstargain=`, `-iwmoongain=`, `-iwptgain=`, `-iwglow=`, `-iwglowmip=`, `-iwskygamma=` | star dome parameters | `iwstargain=` [src Source/IronwakeTest/IronwakeP8.cpp:53] `iwglowmip=` [src Source/IronwakeTest/IronwakeP8.cpp:59] |
| `-iwlegacywx` | skip living mode | `iwlegacywx` [src Source/IronwakeTest/IronwakeVoyage.cpp:343] |
| Keys | `T` speed, `Shift+T`/`]` +3 h, `L` weather, `K` drift | `Shift+T    jump forward 3 hours (also ])` [src Source/IronwakeTest/IronwakeP11.cpp:376] |

Log lines to measure from: `IWP11 living world on:`, `IWP11 tick ... alt= wx= cover= rain= sea= night= clear= sunI= fog=` every 15 s, `IWP13 moon alt= illum= I= tw= sunI= skyI= lamp=` every 15 s, `IWP8 night sky up: day= (moon phase) hour= lat=`, `IWS1 weather baseline` `IWP11 tick %s alt=%.1f` [src Source/IronwakeTest/IronwakeP11.cpp:300] `IWP13 moon alt=%.1f` [src Source/IronwakeTest/IronwakeP11.cpp:280].

## 8.14 Gaps / unknown

- **Level-actor baseline** `SunC`, `FogC`, map post-process saturation/colour grade, sky-atmosphere and volumetric-cloud base parameters: stored in the map (`.umap`), not readable here. Measure with a packaged run and log `Day.SunC/FogC` (the `IWS1 weather baseline` line prints only pitch/intensity/fog/bias) or inspect the map in-editor.
- **Absolute lighting numbers** (sun lux, sky intensity, fog) at each state/hour are only *derived* from presets × multipliers (8.5.1); the real values depend on the baseline above and the P13/P14 post-terms; measure from `IWP11 tick` log lines (`sunI=`, `fog=`).
- **Weather drift statistics** over a full 48-minute day were never soaked (`P11.md`); the stationary distribution in 8.4.3 is a derivation.
- **Date dependence**: sun altitude depends on the real calendar date at launch (8.2); midday altitude ranges 16.6°–63.4°. Whether a fixed date is intended: unknown (design decision).
- **Moon sprite images** (`moon_phase_*.png`, LROC) and their resolution, and the star map file (`starmap_2020_8k.exr`) contents: external to the repo tree for sprites; the EXR is under `build/assets/p10/` but is gitignored binary (not read).
- **`PtThresh`** dead parameter; **`ClockRate`/`iwskyrate`** unused in living mode; **`Storm` preset line** swallowed by a comment (8.4.1) — all candidate cleanups, not changed here (read-only task).
- **Lighthouse (St Brannoc) lamp** glows by day (8.12); intended? unknown.
- Lightning is unreachable in living mode (8.9); intended? unknown.
- **Sun colour at twilight** vs. rendered sky colour — vision scores only (dawn ~6.5, dusk ~7.5 per `P13.md`); not numerically measured.
