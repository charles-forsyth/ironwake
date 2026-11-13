# Chapter 2: Ships

Scope: every hull in the game and how it is built from source: the three player roster hulls (steam-sail frigate, steam cutter, turret ironclad), the enemy ironclad (RSS Kracken), the neutral merchants (Goliath tug / Aglae launch), the P8 NPC boats and airships, and the brass airship. For each: dimensions, mass, draught, boot-top rule, telegraph steps and speeds, the handling constants (MaxAccel, SpeedTimeConstant, MinTurnRadius, YawResponse, LateralDrag, RudderSlew), the sail model and auxiliary steam, guns, mesh/material assets, camera zoom steps and the measured results (circle times P3/P7/P8/P9). Every value is cited as `value` [src path:line]; where two places disagree the one that takes effect is documented and the other is marked CONFLICT. Water/hydro constants beyond the per-hull periods are chapters 3 and 4; collision constants are chapter 5.

Conventions: units are metres, seconds, knots unless "cm". "Sim speed" = nominal knots x `SpeedCompression` (default `1.5` [src Source/IronwakeTest/IronwakeShipPawn.h:95]); one knot is `51.4444` [src Source/IronwakeTest/IronwakeShipPawn.cpp:42] cm/s. All tables are in the world frame +X = north = bow at yaw 0.

## 2.1 Which code path sets a hull's numbers (read this first)

1. The pawn constructor sets the ironclad defaults: telegraph `{ -6.f, -3.f, 0.f, 5.f, 9.f, 13.f, 16.f }` `-6.f` [src Source/IronwakeTest/IronwakeShipPawn.cpp:58] and zoom steps `{ 0.8f, 1.15f, 1.5f, 2.2f, 3.2f, 4.6f }` `1.15f` [src Source/IronwakeTest/IronwakeShipPawn.cpp:60].
2. `BeginPlay` calls `ApplyHullDef()` first `ApplyHullDef` [src Source/IronwakeTest/IronwakeShipPawn.cpp:208]. For a non-NPC pawn it loads the roster id (`IronwakeFleet::GetSelected`) and, if the id is not `ironclad`, calls `ApplyFleetHull()` and returns `ApplyFleetHull` [src Source/IronwakeTest/IronwakeShipPawn.cpp:303]. So the **frigate and cutter take every constant from `FIronwakeShipDef`** (IronwakeFleet.cpp); the **ironclad (player) and the enemy (NPC) take the pawn header defaults** (IronwakeShipPawn.h). Fleet-def fields left at `0.f` for the ironclad (accel, time constant, yaw response, lateral drag, rudder slew) are never applied to it.
3. `ApplyFleetHull` copies the def only when the telegraph array has 7 entries `D.TelegraphKnots.Num() == 7` [src Source/IronwakeTest/IronwakeShipFleet.cpp:43] and always copies MaxAccel, SpeedTimeConstant, MinTurnRadiusM, YawResponse, LateralDrag, RudderSlew, hydrostatic periods and DraftBelowBootTopCm `MaxAccelMS2 = D.MaxAccelMS2` [src Source/IronwakeTest/IronwakeShipFleet.cpp:44].
4. The BP_PlayerShip Blueprint (`Content/Ironwake/Ships/BP_PlayerShip.uasset`) is the game's default pawn class `BP_PlayerShip` [src Source/IronwakeTest/IronwakeGameMode.cpp:21]. The pawn header says the tuning block is "editable in BP_PlayerShip" `Tuning (editable in BP_PlayerShip)` [src Source/IronwakeTest/IronwakeShipPawn.h:88]. A string search of the .uasset found none of the tuning property names (TelegraphKnots, MinTurnRadiusM, MassKg, YawResponse, HullLengthM, ZoomStepsL, SpeedTimeConstant), so no Blueprint overrides of these properties are present in the file; the C++ values below are therefore the ones in effect. (Binary property overrides could not be fully decoded: verify at runtime with the `IWS2 hull` log line.)

## 2.2 Player roster (FIronwakeShipDef, IronwakeFleet.cpp)

Roster order in code is frigate, cutter, ironclad. Default selected ship (no saved choice, no `-iwship`): `frigate` `TEXT("frigate")` [src Source/IronwakeTest/IronwakeFleet.cpp:111]. `IsUnlocked`: frigate and cutter always, ironclad after `10` `10` [src Source/IronwakeTest/IronwakeFleet.h:55] point-of-interest arrivals `ArrivalsForIronclad` [src Source/IronwakeTest/IronwakeFleet.cpp:133]; `-iwunlockall` unlocks all `iwunlockall` [src Source/IronwakeTest/IronwakeFleet.cpp:130].

| Item | Frigate "Albatross" (id frigate) | Cutter "Kittiwake" (id cutter) | Ironclad "Devastation" (id ironclad) |
|---|---|---|---|
| In-game class line | Steam-sail frigate, 88 m, 2,200 t `2,200 t` [src Source/IronwakeTest/IronwakeFleet.cpp:26] | Steam cutter, 30 m, 60 t `60 t` [src Source/IronwakeTest/IronwakeFleet.cpp:60] | Turret ironclad, 125 m, 8,000 t `8,000 t` [src Source/IronwakeTest/IronwakeFleet.cpp:77] |
| Length LengthM (m) | `88.f` [src Source/IronwakeTest/IronwakeFleet.cpp:30] | `30.f` [src Source/IronwakeTest/IronwakeFleet.cpp:64] | `125.f` [src Source/IronwakeTest/IronwakeFleet.cpp:81] |
| Beam BeamM (m) | `16.4f` [src Source/IronwakeTest/IronwakeFleet.cpp:30] | `3.9f` [src Source/IronwakeTest/IronwakeFleet.cpp:64] | `25.8f` [src Source/IronwakeTest/IronwakeFleet.cpp:81] |
| Mass (kg) | `2.2e6f` [src Source/IronwakeTest/IronwakeFleet.cpp:30] | `6.0e4f` [src Source/IronwakeTest/IronwakeFleet.cpp:64] | `8.0e6f` [src Source/IronwakeTest/IronwakeFleet.cpp:81] (def); pawn default `8.0e6f` [src Source/IronwakeTest/IronwakeShipPawn.h:124] (this is what is applied) |
| Telegraph, 7 steps, index 0 = full astern ... 6 = flank (kn) | `{ -5.f, -2.5f, 0.f, 4.f, 6.f, 8.f, 9.f }` [src Source/IronwakeTest/IronwakeFleet.cpp:31] (aux steam only; sails add their own) | `{ -6.f, -3.f, 0.f, 6.f, 10.f, 14.f, 17.f }` [src Source/IronwakeTest/IronwakeFleet.cpp:65] | none in def (empty = keep pawn's own); pawn: `{ -6.f, -3.f, 0.f, 5.f, 9.f, 13.f, 16.f }` [src Source/IronwakeTest/IronwakeShipPawn.cpp:58] |
| MaxAccelMS2 (m/s^2 sim) | `0.35f` [src Source/IronwakeTest/IronwakeFleet.cpp:32] | `2.2f` [src Source/IronwakeTest/IronwakeFleet.cpp:32] | pawn `0.75f` [src Source/IronwakeTest/IronwakeShipPawn.h:103] |
| SpeedTimeConstant (s) | `6.f` [src Source/IronwakeTest/IronwakeFleet.cpp:32] | `1.2f` [src Source/IronwakeTest/IronwakeFleet.cpp:66] | pawn `3.f` [src Source/IronwakeTest/IronwakeShipPawn.h:99] |
| MinTurnRadiusM (m) | `185.f` [src Source/IronwakeTest/IronwakeFleet.cpp:32] | `55.f` [src Source/IronwakeTest/IronwakeFleet.cpp:66] | `330.f` [src Source/IronwakeTest/IronwakeFleet.cpp:82] in def AND pawn default `330.f` [src Source/IronwakeTest/IronwakeShipPawn.h:111] (agree; see 2.3) |
| YawResponse (1/s) | `2.2f` [src Source/IronwakeTest/IronwakeFleet.cpp:32] | `5.f` [src Source/IronwakeTest/IronwakeFleet.cpp:48] | pawn `2.8f` [src Source/IronwakeTest/IronwakeShipPawn.h:121] |
| LateralDrag (1/s) | `2.6f` [src Source/IronwakeTest/IronwakeFleet.cpp:32] | `4.5f` [src Source/IronwakeTest/IronwakeFleet.cpp:66] | pawn `3.5f` [src Source/IronwakeTest/IronwakeShipPawn.h:127] |
| RudderSlew (deg/s) | `18.f` [src Source/IronwakeTest/IronwakeFleet.cpp:32] | `45.f` [src Source/IronwakeTest/IronwakeFleet.cpp:66] | pawn `20.f` [src Source/IronwakeTest/IronwakeShipPawn.h:118] |
| Max rudder (deg) / steps | pawn-wide `35.f` [src Source/IronwakeTest/IronwakeShipPawn.h:114] / `3` [src Source/IronwakeTest/IronwakeShipPawn.cpp:43] | same | same |
| Heave period / zeta (s) | `5.5f` [src Source/IronwakeTest/IronwakeFleet.cpp:33] / `0.5f` [src Source/IronwakeTest/IronwakeFleet.cpp:33] | `3.2f` [src Source/IronwakeTest/IronwakeFleet.cpp:50] / `0.5f` [src Source/IronwakeTest/IronwakeFleet.cpp:50] | pawn `6.5f` [src Source/IronwakeTest/IronwakeShipPawn.h:480] / `0.5f` [src Source/IronwakeTest/IronwakeShipPawn.h:480] |
| Pitch period / zeta | `5.5f` [src Source/IronwakeTest/IronwakeFleet.cpp:33] / `0.55f` [src Source/IronwakeTest/IronwakeFleet.cpp:33] | `3.0f` [src Source/IronwakeTest/IronwakeFleet.cpp:67] / `0.5f` [src Source/IronwakeTest/IronwakeFleet.cpp:50] | pawn `6.8f` [src Source/IronwakeTest/IronwakeShipPawn.h:481] / `0.55f` [src Source/IronwakeTest/IronwakeShipPawn.h:481] |
| Roll period / zeta | `8.f` [src Source/IronwakeTest/IronwakeFleet.cpp:33] / `0.4f` [src Source/IronwakeTest/IronwakeFleet.cpp:33] | `4.5f` [src Source/IronwakeTest/IronwakeFleet.cpp:66] / `0.4f` [src Source/IronwakeTest/IronwakeFleet.cpp:50] | pawn `10.f` [src Source/IronwakeTest/IronwakeShipPawn.h:482] / `0.4f` [src Source/IronwakeTest/IronwakeShipPawn.h:482] |
| DraftBelowBootTopCm (cm sea above boot-top; negative = boot-top shows) | `-40.f` [src Source/IronwakeTest/IronwakeFleet.cpp:34] | `-25.f` [src Source/IronwakeTest/IronwakeFleet.cpp:68] | def `-50.f` [src Source/IronwakeTest/IronwakeFleet.cpp:82]; pawn `-60.f` [src Source/IronwakeTest/IronwakeShipPawn.h:345] (pawn value is the one applied, see 2.3) |
| Keel draught DraughtCm (collision, cm) | `500.f` [src Source/IronwakeTest/IronwakeShipCollision.cpp:22] | `200.f` [src Source/IronwakeTest/IronwakeShipCollision.cpp:21] | player `700.f` [src Source/IronwakeTest/IronwakeShipCollision.cpp:23] |
| Turrets / props / sail / smoke flags | bSail only `D.bSail = true` [src Source/IronwakeTest/IronwakeFleet.cpp:30] | bFunnelSmoke only `D.bFunnelSmoke = true` [src Source/IronwakeTest/IronwakeFleet.cpp:64] | bTurrets + bProps `D.bTurrets = true` [src Source/IronwakeTest/IronwakeFleet.cpp:81] |
| Hull mesh | `/Game/Ironwake/Ships/Frigate/SM_Frigate_Hull.SM_Frigate_Hull` [src Source/IronwakeTest/IronwakeFleet.cpp:29] | `/Game/Ironwake/Ships/Aglae/SM_Aglae_Hull.SM_Aglae_Hull` [src Source/IronwakeTest/IronwakeFleet.cpp:63] | `/Game/Ironwake/Ships/Devastation/SM_Devastation_Hull.SM_Devastation_Hull` [src Source/IronwakeTest/IronwakeFleet.cpp:80] |
| Unlock text | "Start ship" `Start ship` [src Source/IronwakeTest/IronwakeFleet.cpp:28] | "Harbour tender, always available" `Harbour tender, always available` [src Source/IronwakeTest/IronwakeFleet.cpp:62] | "Complete N voyages (point-of-interest arrivals)" `Complete %d voyages` [src Source/IronwakeTest/IronwakeFleet.cpp:79] |
| Hull profile | `Frigate()` `&FIronwakeHullProfile::Frigate` [src Source/IronwakeTest/IronwakeFleet.cpp:35] | `Aglae()` `&FIronwakeHullProfile::Aglae` [src Source/IronwakeTest/IronwakeFleet.cpp:71] | `Devastation()` `&FIronwakeHullProfile::Devastation` [src Source/IronwakeTest/IronwakeFleet.cpp:84] |
| Funnel smoke top (hull-mesh cm) / scale | none (aux stack made on B) | `FVector(60.f, 0.f, 965.f)` [src Source/IronwakeTest/IronwakeFleet.cpp:69] / `0.2f` [src Source/IronwakeTest/IronwakeFleet.cpp:51] | none in def (funnel smoke not flagged for ironclad); default SmokeScale `0.3f` [src Source/IronwakeTest/IronwakeFleet.h:40] |

Notes:
- The in-game ship name comes from `ShipDef->Name`; with no def: NPC "Kracken", player "Devastation" `Kracken` [src Source/IronwakeTest/IronwakeShipFleet.cpp:18].
- Header comments that disagree with the numbers: the pawn header comment on MaxAccelMS2 says "ironclad: Stop -> Half Ahead in ~10 s, Flank ~20 s" `Stop -> Half Ahead in ~10 s` [src Source/IronwakeTest/IronwakeShipPawn.h:101]; measured values are in 2.12.
- Header default `HullLengthM = 140` `140.f` [src Source/IronwakeTest/IronwakeShipPawn.h:135] versus constructor `HullLengthM = 125` `125.f` [src Source/IronwakeTest/IronwakeShipPawn.cpp:96]: **CONFLICT**, the constructor runs later and wins (and `ApplyHullDef` writes the table value again, 2.3). Header default `ArmLength = 18200` `18200.f` [src Source/IronwakeTest/IronwakeShipPawn.h:131] is overwritten in `BeginPlay` from zoom step x hull length `ZoomStepsL.IsValidIndex(ZoomIndex)` [src Source/IronwakeTest/IronwakeShipPawn.cpp:251].
- Dynamic model (identical for every hull, ApplyShipForces): see 2.4.

## 2.3 The ironclad and the enemy (ApplyHullDef table)

For the ironclad and the NPC the length/beam table is inside `ApplyHullDef`: default "Devastation scaled 87 m -> 125 m, beam 25.8 m" `LengthM = 125.f, BeamM = 25.8f` [src Source/IronwakeTest/IronwakeShipPawn.cpp:308]. The NPC enemy replaces them with the RSS Kracken hull: `LengthM = 120.f; BeamM = 20.f` `LengthM = 120.f` [src Source/IronwakeTest/IronwakeShipPawn.cpp:312] and mesh `/Game/Ironwake/Ships/Kracken/SM_Kracken_Hull` `/Game/Ironwake/Ships/Kracken/SM_Kracken_Hull.SM_Kracken_Hull` [src Source/IronwakeTest/IronwakeShipPawn.cpp:81].

| Item | Player ironclad | Enemy (Kracken, bIsNPC) |
|---|---|---|
| Hull profile | `Devastation()` | `Kracken()` `FIronwakeHullProfile::Kracken()` [src Source/IronwakeTest/IronwakeShipPawn.cpp:324] |
| HullBody box half-extent (cm) | `LengthM * 0.47 * 100`, `BeamM*0.5*100`, `700.f` [src Source/IronwakeTest/IronwakeShipPawn.cpp:326] | same formula with 120 / 20 |
| Mass | `8.0e6f` [src Source/IronwakeTest/IronwakeShipPawn.h:124] | same pawn default (no NPC override) |
| Handling | pawn defaults (2.2 table) | same pawn defaults (steered by rudder steps, 2.9) |
| Draught (collision) | `700` | `600.f` [src Source/IronwakeTest/IronwakeShipCollision.cpp:23] |
| Max HP | `160.f` [src Source/IronwakeTest/IronwakeCombat.cpp:110] | `120.f` [src Source/IronwakeTest/IronwakeGameMode.cpp:125] (set at spawn; pawn default `100.f` [src Source/IronwakeTest/IronwakeShipPawn.h:230]) |
| Reload (s) | pawn default `5.f` [src Source/IronwakeTest/IronwakeShipPawn.h:245] | `7.f` [src Source/IronwakeTest/IronwakeCombat.cpp:149] |
| Turrets | visible twin turrets (Devastation meshes) | turret pivots hidden; muzzle at `FVector(300.f, -150.f, 0.f)` [src Source/IronwakeTest/IronwakeShipPawn.cpp:322] (casemate ship) |
| Turret pivots (hull-mesh cm) | fore `FVector(2427.65f, 0.7f, 451.15f)` [src Source/IronwakeTest/IronwakeShipPawn.cpp:141] ; aft `FVector(-1968.51f, 0.7f, 451.15f)` [src Source/IronwakeTest/IronwakeShipPawn.cpp:142] yawed 180 | fore `FVector(4519.f, 0.8f, 496.f)` [src Source/IronwakeTest/IronwakeShipPawn.cpp:313] ; aft `FVector(-3997.f, 0.8f, 496.f)` [src Source/IronwakeTest/IronwakeShipPawn.cpp:314] |

MinTurnRadiusM resolution (the P8 finding, still the template for conflicts): the ironclad takes the PAWN default, not the fleet def; the P8 card changed `IronwakeShipPawn.h` from 280 to 330, and also set the def to 330 (no effect alone: "first run still R_coded=280") `IronwakeFleet.cpp:63 set too, no effect alone` (build report `P8.md`). Today both places read 330, so there is no live conflict; the def's value is documentation only `also the pawn default, IronwakeShipPawn.h` [src Source/IronwakeTest/IronwakeFleet.cpp:82]. The same applies to `DraftBelowBootTopCm`: def `-50` but pawn default `-60`, **CONFLICT**: `-60` is applied to the ironclad, because `ApplyFleetHull` is skipped for it (2.1); the `-60` came from A1 "50 -> -60" `**-60**` (build report `A1.md`).

Ironclad mesh composition: hull + 2 screws + rudder + 2 turret bases + 2 barrel meshes (assets in 2.11). Screw spin: `RevPerSec = EngineOrderKnots / 16 * 2.2` `2.2f` [src Source/IronwakeTest/IronwakeShipPawn.cpp:349], twin counter-rotating. Screw pivots (hull cm): port `FVector(-5481.9f, -376.6f, -650.4f)` [src Source/IronwakeTest/IronwakeShipPawn.cpp:112], starboard `FVector(-5481.9f, 376.6f, -650.4f)` [src Source/IronwakeTest/IronwakeShipPawn.cpp:113], rudder `FVector(-5733.7f, 0.7f, -559.7f)` [src Source/IronwakeTest/IronwakeShipPawn.cpp:114].

Boot-top / waterline rule (all hulls): library meshes are in cm with the boot-top at mesh z = 0, bow +X `boot-top (waterline paint) at mesh z=0` [src Source/IronwakeTest/IronwakeShipPawn.cpp:93]. The mesh is placed at body z `HullMeshZ = PontoonZCm + 0.1 * PontoonRadiusCm + RenderedSeaOffsetCm - DraftBelowBootTopCm` with `PontoonZCm = -150` and `PontoonRadiusCm = 800` `-150.f` [src Source/IronwakeTest/IronwakeShipPawn.cpp:50] `800.f` [src Source/IronwakeTest/IronwakeShipPawn.cpp:49], giving a static sea level on the body of `-70` cm `StaticSeaBodyZ = -70.f` [src Source/IronwakeTest/IronwakeShipPawn.h:479]. `RenderedSeaOffsetCm = 0` `0.f` [src Source/IronwakeTest/IronwakeShell.h:30]. Roster hulls use the same formula (`-150 + 0.1*800 + offset - draft`) `HullMeshZ = -150.f + 0.1f * 800.f` [src Source/IronwakeTest/IronwakeShipFleet.cpp:55] and then centre the waterplane on the body origin by shifting X by the profile mid-point `const float Xc` [src Source/IronwakeTest/IronwakeShipFleet.cpp:57].

## 2.4 Handling model (ApplyShipForces, identical for all pawns)

All steps below are in `AIronwakeShipPawn::ApplyShipForces`; the pawn ticks in `TG_PrePhysics` `TG_PrePhysics` [src Source/IronwakeTest/IronwakeShipPawn.cpp:56], with `Dt = min(DeltaSeconds, 0.1)` `0.1f` [src Source/IronwakeTest/IronwakeShipPawn.cpp:756].

| Stage | Formula / constant |
|---|---|
| Ordered speed | `Ordered = TelegraphKnots[TelegraphIndex]` (frigate with sails: see 2.6) |
| Engine room | `EngineOrderKnots` follows `Ordered` at `EngineResponse` = `6.f` [src Source/IronwakeTest/IronwakeShipPawn.h:107] kn/s (`FInterpConstantTo`) `FInterpConstantTo(EngineOrderKnots` [src Source/IronwakeTest/IronwakeShipPawn.cpp:675] |
| Target speed | `TargetV = EngineOrderKnots * 51.4444 * SpeedCompression` cm/s `TargetV` [src Source/IronwakeTest/IronwakeShipPawn.cpp:714] |
| Thrust limit | `MaxDv = MaxAccelMS2 * 100 * max(SpeedCompression/1.5, 1) * Dt` `MaxDv` [src Source/IronwakeTest/IronwakeShipPawn.cpp:716] |
| Exponential approach | `Want = (TargetV - VFwd) * clamp(Dt / max(SpeedTimeConstant, 0.2), 0, 1)` `0.2f` [src Source/IronwakeTest/IronwakeShipPawn.cpp:717] |
| New forward speed | `VFwd + clamp(Want, -1.3*MaxDv, MaxDv)`; braking/astern up to `1.3` x the thrust limit `1.3f` [src Source/IronwakeTest/IronwakeShipPawn.cpp:718] |
| Keel grip | `VLat *= exp(-LateralDrag * Dt)` `FMath::Exp(-LateralDrag * Dt * WaterFactor)` [src Source/IronwakeTest/IronwakeShipPawn.cpp:720] |
| Yaw rate wanted | `DesiredYawRateDeg = deg(SpeedMS / max(MinTurnRadiusM,1)) * (RudderAngle / MaxRudderDeg)` `SpeedMS / FMath::Max(MinTurnRadiusM, 1.f)` [src Source/IronwakeTest/IronwakeShipPawn.cpp:734] where `SpeedMS = NewVFwd / 100` (sim m/s, includes compression) |
| Yaw build-up | `AngVel.Z += (wanted + WaveYawRate(player only) - AngVel.Z) * (1 - exp(-YawResponse*Dt))` `1.f - FMath::Exp(-YawResponse * Dt)` [src Source/IronwakeTest/IronwakeShipPawn.cpp:742] |
| Wave yaw (A1) | `WaveYawGain` = `3.f` [src Source/IronwakeTest/IronwakeShipPawn.h:513] deg/s per unit slope difference; not applied to NPCs `(bIsNPC ? 0.f : WaveYawRate)` [src Source/IronwakeTest/IronwakeShipPawn.cpp:742] |
| Collision | `TickCollision(Dt, PlanVel, YR)` edits velocity and yaw rate (chapter 5) `TickCollision(Dt, PlanVel, YR)` [src Source/IronwakeTest/IronwakeShipPawn.cpp:744] |

Rudder (H1): the wheel is a held axis. Held A/D moves the commanded angle at `30` deg/s `RudderInput * 30.f * Dt` [src Source/IronwakeTest/IronwakeShipPawn.cpp:693] clamped to +-`MaxRudderDeg`; with the AUTO-CENTRE setting, step 0 and no input the command returns to 0 at `45` deg/s `45.f` [src Source/IronwakeTest/IronwakeShipPawn.cpp:697]. The blade then follows the command at `RudderSlewDegPerSec` `FInterpConstantTo(RudderAngleDeg` [src Source/IronwakeTest/IronwakeShipPawn.cpp:700]. Code that sets `RudderStep` (AI, helmsman, console) maps step s to `MaxRudderDeg * s / 3` `RudderCmdDeg = MaxRudderDeg * (float)RudderStep` [src Source/IronwakeTest/IronwakeShipPawn.cpp:602].

Telegraph names (non-sail hulls): FULL ASTERN, HALF ASTERN, STOP, SLOW AHEAD, HALF AHEAD, FULL AHEAD, FLANK `TEXT("FULL ASTERN")` [src Source/IronwakeTest/IronwakeShipPawn.cpp:41]]. Initial `TelegraphIndex = 2` (STOP) `int32 TelegraphIndex = 2` [src Source/IronwakeTest/IronwakeShipPawn.h:181]. W/S step the telegraph one per press, arrow keys also `EKeys::Up` [src Source/IronwakeTest/IronwakeShipPawn.cpp:402].

Physics body: mass override `MassKg`; linear damping `0`, angular damping `0.02` after BeginPlay `SetAngularDamping(0.02f)` [src Source/IronwakeTest/IronwakeShipPawn.cpp:226] (constructor `0.3` is overwritten `SetAngularDamping(0.3f)` [src Source/IronwakeTest/IronwakeShipPawn.cpp:72]); centre of mass lowered to z `-300` `FVector(0.f, 0.f, -300.f)` [src Source/IronwakeTest/IronwakeShipPawn.cpp:222]; gravity off and the plugin buoyancy replaced by waterplane hydrostatics `SetEnableGravity(false)` [src Source/IronwakeTest/IronwakeShipPawn.cpp:232]. Physics substepping (P7): `bSubstepping=True, MaxSubstepDeltaTime=1/60, MaxSubsteps=6, MaxPhysicsDeltaTime=0.1` in DefaultEngine.ini `MaxSubsteps=6` (build report `P7.md`).

## 2.5 Hull profiles (waterline outline data, IronwakeHydro.cpp / IronwakeFleet.cpp)

`FIronwakeHullProfile`: maximum half-breadth sampled at evenly spaced stations from stern X0 to stem X1 between -0.5 and +2.5 m of the boot-top; `BowDeckM` = forecastle deck height over boot-top near the stem; `KeelM` = keel depth under the boot-top "for the log" `KeelM = 7.f` [src Source/IronwakeTest/IronwakeHydro.h:22]. Station count per hull is not a named constant (the arrays are literal initializer lists; **unknown** as a constant, measure with `HalfB.Num()`).

| Hull | X0 stern (m) | X1 stem (m) | Waterline length X1-X0 (derived, m) | BowDeckM (m) | KeelM (m) | Max half-breadth (m, from table) |
|---|---|---|---|---|---|---|
| Devastation | `-62.5f` [src Source/IronwakeTest/IronwakeHydro.cpp:23] | `59.2f` [src Source/IronwakeTest/IronwakeHydro.cpp:23] | 121.7 | `3.2f` [src Source/IronwakeTest/IronwakeHydro.cpp:23] | `10.8f` [src Source/IronwakeTest/IronwakeHydro.cpp:23] | `12.89f` [src Source/IronwakeTest/IronwakeHydro.cpp:23] |
| Kracken (enemy) | `-56.6f` [src Source/IronwakeTest/IronwakeHydro.cpp:27] | `60.f` [src Source/IronwakeTest/IronwakeHydro.cpp:27] | 116.6 | `5.0f` [src Source/IronwakeTest/IronwakeHydro.cpp:27] | `7.5f` [src Source/IronwakeTest/IronwakeHydro.cpp:27] | `10.01f` [src Source/IronwakeTest/IronwakeHydro.cpp:27] |
| Goliath tug | `-25.f` [src Source/IronwakeTest/IronwakeHydro.cpp:31] | `24.9f` [src Source/IronwakeTest/IronwakeHydro.cpp:31] | 49.9 | `2.5f` [src Source/IronwakeTest/IronwakeHydro.cpp:31] | `5.0f` [src Source/IronwakeTest/IronwakeHydro.cpp:31] | `4.46f` [src Source/IronwakeTest/IronwakeHydro.cpp:31] |
| Aglae launch | `-14.94f` [src Source/IronwakeTest/IronwakeHydro.cpp:35] | `15.f` [src Source/IronwakeTest/IronwakeHydro.cpp:35] | 29.9 | `1.2f` [src Source/IronwakeTest/IronwakeHydro.cpp:35] | `1.6f` [src Source/IronwakeTest/IronwakeHydro.cpp:35] | `1.92f` [src Source/IronwakeTest/IronwakeHydro.cpp:35] |
| Frigate | `-40.51f` [src Source/IronwakeTest/IronwakeFleet.cpp:217] | `26.47f` [src Source/IronwakeTest/IronwakeFleet.cpp:217] | 67.0 | `6.0f` [src Source/IronwakeTest/IronwakeFleet.cpp:218] | `4.2f` [src Source/IronwakeTest/IronwakeFleet.cpp:219] | `8.21f` [src Source/IronwakeTest/IronwakeFleet.cpp:218] |

Source comment for the frigate profile: "max half-breadth between -0.5 and +2.5 m of the boot-top, 88 m game scale" (build/s2/clear_frigate.py) `88 m game scale` [src Source/IronwakeTest/IronwakeFleet.cpp:216]. The roster text says "scaled to 88 m overall, 67 m waterline" `88 m overall, 67 m waterline` [src Source/IronwakeTest/IronwakeFleet.cpp:24]. Note the profile `KeelM` (10.8 m on the Devastation) is a log value and is NOT the collision draught (7 m); the collision keel depth is `DraughtCm` (2.2).

Collision outline padding of the profile is in chapter 5.

## 2.6 Sail model (frigate Albatross only; `HasSails()` = SailRig present)

`FleetBeginPlay` builds the rig only for a non-NPC with `bSail` `SailRig = NewObject` [src Source/IronwakeTest/IronwakeShipFleet.cpp:73]. Rig contents: `9` yards, `8` square sails, `8` fore-and-aft sails, ensign + `3` masthead pennants (header comment) `9 yards that brace to the wind, 8 square sails and 8 fore-and-aft sails` [src Source/IronwakeTest/IronwakeFleet.h:70]; arrays `GYards`, `GSquare`, `GForeAft` `GYards[]` [src Source/IronwakeTest/IronwakeFleet.cpp:240] hold the mesh-space geometry (frigate masts: three; mast tops listed in `GMastTops` `GMastTops[]` [src Source/IronwakeTest/IronwakeFleet.cpp:272]). Grid resolution `SqU=14, SqV=10` square, `FaU=10, FaV=10` fore-aft, ensign `14x6`, pennant `18x1` `SqU = 14, SqV = 10` [src Source/IronwakeTest/IronwakeFleet.cpp:275].

Telegraph on this hull = sail plan. Names: FULL ASTERN (steam), HALF ASTERN (steam), FURLED, REEFED TOPSAILS, HALF SAIL, FULL SAIL, ALL SAIL + STAYSAILS `TEXT("REEFED TOPSAILS")` [src Source/IronwakeTest/IronwakeShipPawn.cpp:633]. Sails set by index: 3 topsails, 4 plus courses and spanker, 5 plus topgallants and jibs, 6 plus staysails `3 topsails, 4 + courses/spanker, 5 + topgallants/jibs, 6 + staysails` [src Source/IronwakeTest/IronwakeShipFleet.cpp:167].

Sail drive (TickSails):
| Quantity | Value |
|---|---|
| True wind angle | `TrueWindAngle = FindDeltaAngleDegrees(Yaw, WindFromYaw)`; + = wind from starboard `FMath::FindDeltaAngleDegrees(GetActorRotation().Yaw, From)` [src Source/IronwakeTest/IronwakeShipFleet.cpp:165] |
| Plan fraction PlanFrac[0..6] | `{ 0.f, 0.f, 0.f, 0.30f, 0.62f, 0.90f, 1.f }` [src Source/IronwakeTest/IronwakeShipFleet.cpp:182] |
| Raw drive | `Raw = 0.72 * WindKn * Polar(TWA) * sqrt(Plan)` `0.72f` [src Source/IronwakeTest/IronwakeShipFleet.cpp:62] |
| Sail knots | `SailKn = 14*(1-exp(-Raw/14))*1.25`, then `min(SailKn, Raw)` `1.25f` [src Source/IronwakeTest/IronwakeShipFleet.cpp:63] |
| Fill | `SailFill = clamp(Polar * clamp(WindKn/16, 0.3, 1.2), 0, 1)` `1.2f` [src Source/IronwakeTest/IronwakeShipFleet.cpp:188] |
| Heel | `SailHeelDeg = clamp(7 * fill * side * min(WindF^2, 2.2) * plan/0.9, 0, 8)` `0.f, 8.f` [src Source/IronwakeTest/IronwakeShipHydro.cpp:141] |
| Leeway | up to `6` deg close-hauled: `LeewayDeg = 6 * clamp((110-TWA)/70,0,1) * clamp(fill*1.5,0,1)` `6.f * FMath::Clamp((110.f - Twa) / 70.f` [src Source/IronwakeTest/IronwakeShipPawn.cpp:725] ; lateral relaxation `0.6`/s |
| Ordered speed | astern = steam `Ordered`; ahead = `max(SailKn, Steam) + 0.3 * min(SailKn, max(Steam,0))`, Steam only when `bAuxSteam` `0.3f * FMath::Min(SailKn` [src Source/IronwakeTest/IronwakeShipPawn.cpp:673] |
| Apparent wind | for the HUD only: boat speed added as a headwind `ApparentKn = A.Size()` [src Source/IronwakeTest/IronwakeShipFleet.cpp:195] |

Polar (`IronwakeFleet::Polar`): drive fraction by TWA 0..180 degrees, linear interpolation:
| TWA (deg) | `0`, `35`, `50`, `60`, `75`, `90`, `105`, `120`, `135`, `150`, `165`, `180` `{ 0.f, 35.f, 50.f, 60.f, 75.f, 90.f, 105.f, 120.f, 135.f, 150.f, 165.f, 180.f }` [src Source/IronwakeTest/IronwakeFleet.cpp:187] |
|---|---|
| Drive | `{ 0.f, 0.f, 0.40f, 0.62f, 0.80f, 0.92f, 0.99f, 1.0f, 0.96f, 0.90f, 0.84f, 0.80f }` [src Source/IronwakeTest/IronwakeFleet.cpp:188] |

No-go zone: drive is exactly 0 for TWA <= `35` (table entry 2). The in-code comment says "no drive inside 40 deg" `no drive inside 40 deg` [src Source/IronwakeTest/IronwakeFleet.cpp:185] and the header says "0 inside 35 deg (in irons)" `0 inside 35 deg` [src Source/IronwakeTest/IronwakeFleet.h:64]; the **table (35) takes effect**; at 40 deg the table gives 0.13 (derived, linear between 35->0 and 50->0.40). The SPEC.md controls line says "can't sail within 40 deg of wind" `can't sail within 40 deg of wind` [src SPEC.md:76] (**CONFLICT**: table = 35, sails stop completely only at <=35).

Points of sail labels (HUD, `PointOfSail`): IN IRONS < `35`, CLOSE-HAULED < `60`, CLOSE REACH < `80`, BEAM REACH < `110`, BROAD REACH < `155`, else RUNNING `if (TWA < 35.f) return TEXT("IN IRONS")` [src Source/IronwakeTest/IronwakeFleet.cpp:198] `if (TWA < 155.f) return TEXT("BROAD REACH")` [src Source/IronwakeTest/IronwakeFleet.cpp:202]. Compass points: 16-point, 22.5 deg steps `22.5f` [src Source/IronwakeTest/IronwakeFleet.cpp:210].

In irons (physics): when `|TWA| < 40` and `|SpeedMS| < 2.5` the sails back and pay her head off at `0.8` deg/s, rudder keeps `0.6 * RudderFrac` `0.8f * (1.f - FMath::Abs(SpeedMS) / 2.5f)` [src Source/IronwakeTest/IronwakeShipPawn.cpp:738]. Note this threshold (40) is different from the polar's zero zone (35); both are as coded.

Auto-reef (W2/W3): wind used is a slow-filtered `ReefWindKn` (rises with interp speed `2.0`, falls with `0.05`) `FInterpTo(ReefWindKn, WindKn, Dt, 0.05f)` [src Source/IronwakeTest/IronwakeShipFleet.cpp:173]; cap on sail plan: `3` if reef wind > `40` kn, `4` if > `30` kn, else `6` `ReefWindKn > 40.f ? 3 : ReefWindKn > 30.f ? 4 : 6` [src Source/IronwakeTest/IronwakeShipFleet.cpp:174]. Player override: ordering more sail again within 3 s (`bReefOverride`) `bReefOverride` [src Source/IronwakeTest/IronwakeShipPawn.h:564].

Auxiliary steam: key B toggles `bAuxSteam` `EKeys::B` [src Source/IronwakeTest/IronwakeShipPawn.cpp:427]; astern orders always use steam `Astern orders always use steam` [src Source/IronwakeTest/IronwakeShipPawn.h:557]; frigate aux stack smoke at hull-mesh `(-2300, 0, 2300)` cm scale `0.9` `FVector(-2300.f, 0.f, 2300.f)` [src Source/IronwakeTest/IronwakeShipFleet.cpp:209] `0.9f` [src Source/IronwakeTest/IronwakeShipFleet.cpp:149]. P7 moved the stack from (-15 m aft, 13 m up) to (-23 m aft, 23 m up), scale 0.6 -> 0.9 `-23 m aft, 23 m up` (build report `P7.md`). Smoke scale at speed: `0.9 * (0.45 + 0.55 * clamp(Kn/13,0,1))` for the sailing ship `0.45f + 0.55f` [src Source/IronwakeTest/IronwakeShipHydro.cpp:165].

Wind (shared, IronwakeFleet.cpp): from yaw `300` deg (WNW) + `15` sin(2 pi T/`660`) + `4` sin(0.071 T), mod 360 `300.f + 15.f * FMath::Sin(T * 2.f * PI / 660.f)` [src Source/IronwakeTest/IronwakeFleet.cpp:169]; speed = sea-state mean wind x (1 + G sin(0.21 T) + 0.08 sin(0.53 T + 1.3)), `G = 0.12 + 0.04 * clamp(S-2, 0, 2)` `0.12f + 0.04f` [src Source/IronwakeTest/IronwakeFleet.cpp:178]; mean wind per sea state CALM `8`, MODERATE `16`, ROUGH `24`, GALE `36`, STORM `48` kn `{ 8.f, 16.f, 24.f, 36.f, 48.f }` [src Source/IronwakeTest/IronwakeHydro.cpp:85]. `SetFixedWind` freezes it for tests `SetFixedWind` [src Source/IronwakeTest/IronwakeFleet.cpp:181].

Measured points of sail (S2 log, 15 kn wind, full sail): broad reach 120 deg `9.8-11.5` kn, beam 90 `9.3-10.6`, close-hauled 55 `5.8-6.1`, running 175 `8.5-9.4`, in irons 20-30 deg `0` kn, in irons + aux steam 21 deg `9.0` kn `9.8-11.5 kn` (build report `S2.md`) `5.8-6.1 kn` (build report `S2.md`) `9.0 kn` (build report `S2.md`). Yard brace: about 38 deg close-hauled, 27 beam reach, 18 broad reach, 1.5 running `38° close-hauled` (build report `S2.md`). Autopilot beating: holds minimum TWA `50` deg (Beat constant) `constexpr float Beat = 50.f` [src Source/IronwakeTest/IronwakeAutopilot.cpp:391], hysteresis `12` deg `FMath::Abs(Off) > 12.f` [src Source/IronwakeTest/IronwakeAutopilot.cpp:396]; S2 beat test result: min TWA 50 with 0 s in irons `minimum TWA of 50°` (build report `S2.md`).

## 2.7 Merchants and the moored steamer (AIronwakeMerchant, IronwakeLivingSea.*)

Kinematic actors (no physics): each follows a closed waypoint loop at fixed speed with a limited turn rate, sits on the live swell (centre height + pitch/roll from 4 water samples) and is NOT a combat target `Not a combat target` [src Source/IronwakeTest/IronwakeLivingSea.h:20]. Three are spawned in `SpawnTraffic`.

| Name | Waypoints (m, world, X north / Y east) | Knots | Start idx | Hull |
|---|---|---|---|---|
| SS Calloway | `M(-2100, 1200), M(1500, 3300), M(3200, 1200), M(2500, -2500), M(-1600, -1900)` [src Source/IronwakeTest/IronwakeLivingSea.cpp:623] | `8.f` [src Source/IronwakeTest/IronwakeLivingSea.cpp:623] | 0 | Goliath tug (idx 0) |
| SS Hesper | `M(450, 1900), M(170, 2570), M(-500, 2850), M(-1170, 2570), M(-1450, 1900), M(-1170, 1230), M(-500, 950), M(170, 1230)` [src Source/IronwakeTest/IronwakeLivingSea.cpp:625] | `7.f` [src Source/IronwakeTest/IronwakeLivingSea.cpp:625] | `7` [src Source/IronwakeTest/IronwakeLivingSea.cpp:625] | Aglae launch (idx 1) `bAglae = (Merchants.Num() == 1)` [src Source/IronwakeTest/IronwakeLivingSea.cpp:642] |
| SS Marrow Queen | `M(3300, -3200), M(3500, 2600), M(2100, 2500), M(1800, -2900)` [src Source/IronwakeTest/IronwakeLivingSea.cpp:627] | `9.f` [src Source/IronwakeTest/IronwakeLivingSea.cpp:627] | 1 | Goliath tug (idx 2) |

Comments in source: Calloway "Coastal packet: wide loop past the island and south of the player's spawn"; Hesper "Collier: circles the lighthouse island"; Marrow Queen "Grain steamer: eastern loop" `Grain steamer: eastern loop` [src Source/IronwakeTest/IronwakeLivingSea.cpp:626].

Per-merchant constants:
| Quantity | Goliath tug | Aglae launch | Source |
|---|---|---|---|
| Length | `50` m, `LengthCm = 5000` `LengthCm = 5000.f` [src Source/IronwakeTest/IronwakeLivingSea.cpp:133] | `30` m, `3000` `LengthCm = 3000.f` [src Source/IronwakeTest/IronwakeLivingSea.cpp:129] | BeginPlay |
| Speed | `SpeedCmS = knots * 51.44 * 1.5` (fixed 1.5 compression) `D.Knots * 51.44f * 1.5f` [src Source/IronwakeTest/IronwakeLivingSea.cpp:640] ; header default `690` (9 kn x 1.5) `690.f` [src Source/IronwakeTest/IronwakeLivingSea.h:41] | same | SpawnTraffic |
| Max yaw rate | `1.4f` [src Source/IronwakeTest/IronwakeLivingSea.h:42] deg/s | same | header |
| Waypoint reached | within `25000` cm (250 m) `25000.f` [src Source/IronwakeTest/IronwakeLivingSea.cpp:169] | same | Tick |
| Hull position | boot-top 0.4 m under rendered sea: z offset `-40` `RenderedSeaOffsetCm - 40.f` [src Source/IronwakeTest/IronwakeLivingSea.cpp:98] | `-25` `RenderedSeaOffsetCm - 25.f` [src Source/IronwakeTest/IronwakeLivingSea.cpp:127] | |
| Funnel smoke scale | `0.3f` [src Source/IronwakeTest/IronwakeLivingSea.cpp:154] | `0.2` | |
| Collision outline | `OutlineFromProfile(Goliath, pad 50 cm)`, draught `250` cm | `Aglae`, draught `200` cm `bAglae ? 200.f : 250.f` [src Source/IronwakeTest/IronwakeLivingSea.cpp:139] | |
| Route audit half-beam/draught | `500` / `250` cm | `250` / `200` cm `bAglae ? 250.f : 500.f` [src Source/IronwakeTest/IronwakeLivingSea.cpp:164] | |
| Wake strip | 160 m long (`Len = 16000`), width scale `12` `bAglae ? 7.f : 12.f` [src Source/IronwakeTest/IronwakeLivingSea.cpp:218] | width scale `7` | |
| Pitch/roll | sample half length `0.42 L`, half beam `440` cm (Goliath) / `190` cm (Aglae) `bAglae ? 190.f : 440.f` [src Source/IronwakeTest/IronwakeLivingSea.cpp:202] | | pitch gain 0.8, roll gain 0.35, roll clamp +-6 |

Behaviour when blocked: a merchant tests its next pose against solids, hulls and `MinDepthAt < Draught` and holds position (`BlockedS` accumulates, log at 0.5 s) `BlockedS > 0.5f` [src Source/IronwakeTest/IronwakeLivingSea.cpp:195]. After being rammed it moves at `0.3` speed for `4` s (`SlowUntil`) `bSlow ? 0.3f : 1.f` [src Source/IronwakeTest/IronwakeLivingSea.cpp:185] `GetTimeSeconds() + 4.f` [src Source/IronwakeTest/IronwakeShipCollision.cpp:83].

Moored steamer "SS Wren (moored)": a static merchant with an empty route placed at harbour-frame (40, 10) m, yaw 45, funnel smoke switched off (P12) `SS Wren (moored)` [src Source/IronwakeTest/IronwakeVoyage.cpp:487]. It bobs with `0.4 sin(0.5 t)` pitch and `1.2 sin(0.37 t)` roll `0.4f * FMath::Sin(Now * 0.5f)` [src Source/IronwakeTest/IronwakeVoyage.cpp:1151].

Merchant mesh assets: `/Game/Ironwake/Ships/Goliath/SM_Goliath_Hull` and `/Game/Ironwake/Ships/Aglae/SM_Aglae_Hull` `/Game/Ironwake/Ships/Goliath/SM_Goliath_Hull.SM_Goliath_Hull` [src Source/IronwakeTest/IronwakeLivingSea.cpp:86]. Credit line: "Goliath Steam Tug 1903 ... martyn169" `Goliath Steam Tug 1903` [src Source/IronwakeTest/IronwakePlayerController.cpp:65].

## 2.8 NPC boats and airships on loops (AIronwakeNpcCraft, IronwakeP8.*)

"An NPC craft on a slow elliptical loop (boats ride the rendered sea, airships fly at a fixed altitude). No AI." `No AI` [src Source/IronwakeTest/IronwakeP8.h:50]. Centres are offsets from the harbour centre `HarbourC`; `Out = (0.7071, 0.7071)` (harbour yaw 45, seaward), `Side = (-0.7071, 0.7071)` `FVector(0.7071f, 0.7071f, 0.f)` [src Source/IronwakeTest/IronwakeP8.cpp:246]. Parameters: `Setup(mesh, centre, RadA, RadB, PeriodS, AltCm, KeelCm, Phase, bCW, bAir)`.

| Craft (mesh /Game/Ironwake/World/<name>) | Centre = HarbourC + Out*a + Side*b (cm) | RadA (cm) | RadB (cm) | Period (s) | Alt (cm) | Keel (cm) | Phase | CW | Air |
|---|---|---|---|---|---|---|---|---|---|
| PaddleSteamer (45 m) | Out*`90000.f` [src Source/IronwakeTest/IronwakeP8.cpp:260] + Side*`15000.f` [src Source/IronwakeTest/IronwakeP8.cpp:260] | `40000.f` [src Source/IronwakeTest/IronwakeP8.cpp:260] | `22000.f` [src Source/IronwakeTest/IronwakeP8.cpp:260] | `1500.f` [src Source/IronwakeTest/IronwakeP8.cpp:260] | 0 | `-150.f` [src Source/IronwakeTest/IronwakeP8.cpp:260] | 0.1 | true | false |
| SteamDrifter (24 m) | Out*`62000.f` [src Source/IronwakeTest/IronwakeP8.cpp:261] - Side*`30000.f` [src Source/IronwakeTest/IronwakeP8.cpp:261] | `26000.f` [src Source/IronwakeTest/IronwakeP8.cpp:261] | `15000.f` [src Source/IronwakeTest/IronwakeP8.cpp:261] | `1100.f` [src Source/IronwakeTest/IronwakeP8.cpp:261] | 0 | `-130.f` [src Source/IronwakeTest/IronwakeP8.cpp:261] | 0.55 | false | false |
| PatrolGunboat (32 m) | Out*`120000.f` [src Source/IronwakeTest/IronwakeP8.cpp:262] - Side*`10000.f` [src Source/IronwakeTest/IronwakeP8.cpp:262] | `52000.f` [src Source/IronwakeTest/IronwakeP8.cpp:262] | `30000.f` [src Source/IronwakeTest/IronwakeP8.cpp:262] | `1300.f` [src Source/IronwakeTest/IronwakeP8.cpp:262] | 0 | `-110.f` [src Source/IronwakeTest/IronwakeP8.cpp:262] | 0.8 | true | false |
| PatrolAirship (70 m) | Out*`100000.f` [src Source/IronwakeTest/IronwakeP8.cpp:263] + Side*`25000.f` [src Source/IronwakeTest/IronwakeP8.cpp:263] | `70000.f` [src Source/IronwakeTest/IronwakeP8.cpp:263] | `45000.f` [src Source/IronwakeTest/IronwakeP8.cpp:263] | `1500.f` [src Source/IronwakeTest/IronwakeP8.cpp:263] | `24000.f` [src Source/IronwakeTest/IronwakeP8.cpp:263] | 0 | 0.3 | true | true |
| SkyYacht (45 m) | Out*`70000.f` [src Source/IronwakeTest/IronwakeP8.cpp:264] - Side*`40000.f` [src Source/IronwakeTest/IronwakeP8.cpp:264] | `45000.f` [src Source/IronwakeTest/IronwakeP8.cpp:264] | `30000.f` [src Source/IronwakeTest/IronwakeP8.cpp:264] | `1100.f` [src Source/IronwakeTest/IronwakeP8.cpp:264] | `17000.f` [src Source/IronwakeTest/IronwakeP8.cpp:264] | 0 | 0.75 | false | true |

Craft sizes from the P8 log: paddle steamer 45 m, gunboat 32 m, drifter 24 m, launch 14 m, gig 8 m, patrol airship 70 m, sky yacht 45 m `paddle steamer 45 m, gunboat 32 m, drifter 24 m, launch 14 m, gig 8 m, patrol airship 70 m, sky yacht 45 m` (build report `P8.md`). Moored with the swell at the quay: PilotLaunch at harbour (-40, 103) m and RowingGig (-44, 78) m `Fl(TEXT("PilotLaunch"), -40, 103` [src Source/IronwakeTest/IronwakeP8.cpp:244].

Motion: angle `th = Sgn * 2 pi T / Period`, position `Centre + (A cos th, B sin th)`, yaw along the tangent `const float th = Sgn * 2.f * PI * T / Period` [src Source/IronwakeTest/IronwakeP8.cpp:176]. Boats: height `sea + RenderedSeaOffsetCm + Keel`, pitch `0.8 *` and roll `0.6 *` the atan of 4-sample slopes (samples at +-1200 cm along, +-400 cm across), rotation interpolated at `1.5` `FMath::RInterpTo(Cur, Want, Dt, 1.5f)` [src Source/IronwakeTest/IronwakeP8.cpp:212]. Airships: `Z = Alt + 2500 sin(0.05 T)`, pitch `1.5 sin(0.21 T)`, roll `2 sin(0.17 T)` `2500.f * FMath::Sin(T * 0.05f)` [src Source/IronwakeTest/IronwakeP8.cpp:183]. Collision (boats only): outline = padded bounds rectangle (pad `40` cm), draught `clamp(-Bounds.Min.Z*0.5, 150, 400)`; a blocked boat holds its loop time that tick; slow after a ram `0.3` for `4` s `FMath::Clamp(-Bx.Min.Z * 0.5f, 150.f, 400.f)` [src Source/IronwakeTest/IronwakeP8.cpp:161]. Airships have no collision body (`!bAir` guard).

## 2.9 Enemy AI steering (RSS Kracken)

`TickAI` thinks every `0.5` s `AIThink = 0.5f` [src Source/IronwakeTest/IronwakeCombat.cpp:475]; utilities: patrol `0.2`, engage `0.6 + 0.3*(1 - range/DetectRange)` inside `DetectRange`, flee `0.95 + (0.35 - HP fraction)` when HP < `35` % (sticky) `0.95f + (0.35f - HPFrac)` [src Source/IronwakeTest/IronwakeCombat.cpp:481]. Ranges: `DetectRange = 900000` cm, `CombatRange = 700000`, `BroadsideRange = 600000` `constexpr float DetectRange = 900000.f` [src Source/IronwakeTest/IronwakeCombat.cpp:38] `constexpr float BroadsideRange = 600000.f` [src Source/IronwakeTest/IronwakeCombat.cpp:40]. Patrol: square of 4 points at `90000` cm around the spawn point, waypoint accepted inside `30000` cm, telegraph index `4` (HALF AHEAD) `* 90000.f` [src Source/IronwakeTest/IronwakeCombat.cpp:493]. Engage: closing = telegraph `5` (FULL AHEAD) heading at bearing; inside broadside range: the beam heading nearer the current course, telegraph `4`, and `+-30` deg opening if range < `200000` cm. Flee: telegraph `6` (FLANK), heading away; bends back toward the tile centre beyond `300000` cm. `SteerToHeading`: rudder step 3 if error > `25` deg, 2 if > `10`, 1 if > `3` `A > 25.f ? 3 : A > 10.f ? 2 : A > 3.f ? 1 : 0` [src Source/IronwakeTest/IronwakeCombat.cpp:465]. The AI heading passes through `AvoidLand(AIDesiredYaw, 120000)` before steering `AvoidLand(AIDesiredYaw, 120000.f)` [src Source/IronwakeTest/IronwakeCombat.cpp:547]. Spawn: `EnemyRangeCm = 750000` ahead of the player bow, bow offset `-12` deg, spawn box half-size `370000` cm, `Loc.Z = 150`, heading `ToPlayer + 35` `float EnemyRangeCm = 750000.f` [src Source/IronwakeTest/IronwakeGameMode.h:30] `float EnemyBowOffsetDeg = -12.f` [src Source/IronwakeTest/IronwakeGameMode.h:34].

## 2.10 Guns and combat constants (pawn)

| Constant | Value |
|---|---|
| Muzzle velocity | `60000.f` [src Source/IronwakeTest/IronwakeShipPawn.h:236] cm/s (600 m/s) |
| Max gun range | `1000000.f` [src Source/IronwakeTest/IronwakeShipPawn.h:239] cm (10 km) |
| Turret traverse | `7.f` [src Source/IronwakeTest/IronwakeShipPawn.h:242] deg/s |
| Turret arc (HUD and limit) | +-`150.f` [src Source/IronwakeTest/IronwakeShipPawn.h:171] deg about the rest bearing (fore rest 0, aft rest 180) `{ 0.f, 180.f }` [src Source/IronwakeTest/IronwakeShipPawn.h:429] |
| Reload (player / NPC) | `5` s / `7` s (2.3) |
| Shell damage | `constexpr float ShellDamage = 6.f` [src Source/IronwakeTest/IronwakeCombat.cpp:42] (HP) |
| Fire burn | `constexpr float FireBurnPerSec = 0.35f` [src Source/IronwakeTest/IronwakeCombat.cpp:43] HP/s per fire |
| Repair cooldown | `45` s `RepairCooldown = 45.f` [src Source/IronwakeTest/IronwakeCombat.cpp:686] |
| NPC hold-fire | `HoldFireSeconds` `60.f` [src Source/IronwakeTest/IronwakeShipPawn.h:283] s unless range < `HoldFireRangeCm` `500000.f` [src Source/IronwakeTest/IronwakeShipPawn.h:286] or it is hit |
| Min aim range | `150000.f` [src Source/IronwakeTest/IronwakeShipPawn.h:290] cm |
| Muzzle points (barrel-mesh cm) | player `FVector(690.f, -185.f, 0.f)` [src Source/IronwakeTest/IronwakeShipPawn.h:347] and (690, 185, 0); NPC `(300, +-150, 0)` |
| Lead indicator / aim assist | `bool bLeadIndicator = true` [src Source/IronwakeTest/IronwakeShipPawn.h:248] / `bool bAimAssist = false` [src Source/IronwakeTest/IronwakeShipPawn.h:251] |
| Gun mesh per turret | one barrel mesh holding both guns, pitching about the trunnion; barrel offsets fore `(386.7, 0, 149.25)`, aft `(474.89, 0, 149.25)` `FVector(386.7f, 0.f, 149.25f)` [src Source/IronwakeTest/IronwakeShipPawn.cpp:141] |

Roster hulls other than the ironclad keep the turret pivots but hide the turret/barrel/screw/rudder meshes `K->SetVisibility(false)` [src Source/IronwakeTest/IronwakeShipFleet.cpp:34]; they are placed at `+-Lw*25` along X and `300` cm up `Lw * 25.f` [src Source/IronwakeTest/IronwakeShipFleet.cpp:37]. Whether they can fire from those invisible pivots is **unknown** (combat is parked); check `TickTurrets` with `IWFire` on the frigate. Combat is PARKED per SPEC.md.

## 2.11 Mesh and material assets

| Role | Asset path | Source line |
|---|---|---|
| Player/ironclad hull | `/Game/Ironwake/Ships/Devastation/SM_Devastation_Hull` | `/Game/Ironwake/Ships/Devastation/SM_Devastation_Hull.SM_Devastation_Hull` [src Source/IronwakeTest/IronwakeShipPawn.cpp:80] ("HMS Devastation (martyn169, CC-BY)") `HMS Devastation (martyn169, CC-BY)` [src Source/IronwakeTest/IronwakeShipPawn.h:66] |
| Enemy hull | `/Game/Ironwake/Ships/Kracken/SM_Kracken_Hull` | ("RSS Kracken (martyn169, CC-BY)") `RSS Kracken (martyn169, CC-BY)` [src Source/IronwakeTest/IronwakeShipPawn.h:67] |
| Cutter hull | `/Game/Ironwake/Ships/Aglae/SM_Aglae_Hull` | `/Game/Ironwake/Ships/Aglae/SM_Aglae_Hull.SM_Aglae_Hull` [src Source/IronwakeTest/IronwakeShipPawn.cpp:85] |
| Frigate hull | `/Game/Ironwake/Ships/Frigate/SM_Frigate_Hull` | `/Game/Ironwake/Ships/Frigate/SM_Frigate_Hull.SM_Frigate_Hull` [src Source/IronwakeTest/IronwakeShipPawn.cpp:86] |
| Sail cloth | `/Game/Ironwake/Ships/Materials/M_IW_Cloth` | `/Game/Ironwake/Ships/Materials/M_IW_Cloth.M_IW_Cloth` [src Source/IronwakeTest/IronwakeShipPawn.cpp:87] |
| Screws / rudder | `SM_Devastation_PropPort`, `SM_Devastation_PropStbd`, `SM_Devastation_Rudder` | `SM_Devastation_PropPort` [src Source/IronwakeTest/IronwakeShipPawn.cpp:98] |
| Turrets / barrels | `SM_Devastation_TurretFore/Aft`, `SM_Devastation_BarrelsFore/Aft` | `SM_Devastation_TurretFore` [src Source/IronwakeTest/IronwakeShipPawn.cpp:118] |
| Merchant tug | `/Game/Ironwake/Ships/Goliath/SM_Goliath_Hull` | 2.7 |
| Brass airship | `/Game/Ironwake/World/BrassAirshipRemesh/SM_BrassAirshipRemesh`, prop `/Game/Ironwake/Ships/BrassAirship/SM_BrassAirship_PropStern` | `/Game/Ironwake/World/BrassAirshipRemesh/SM_BrassAirshipRemesh.SM_BrassAirshipRemesh` [src Source/IronwakeTest/IronwakeLivingSea.cpp:228] |
| Wake / foam | `/Game/Ironwake/FX/M_WakeFoam`; wake planes use `/Engine/BasicShapes/Plane` | `/Game/Ironwake/FX/M_WakeFoam.M_WakeFoam` [src Source/IronwakeTest/IronwakeShipPawn.cpp:265] |
| Funnel smoke | `/Game/Ironwake/FX/NS_IW_FunnelSmoke` | `/Game/Ironwake/FX/NS_IW_FunnelSmoke.NS_IW_FunnelSmoke` [src Source/IronwakeTest/IronwakeShipFleet.cpp:80] |

Other ship folders present on disk (not referenced by the code read for this chapter): `Content/Ironwake/Ships/Ironclad/SM_Ironclad`, `Content/Ironwake/Ships/Airship/SM_Airship`, `SM_Airship110` (the `AirshipNoseYaw` comment mentions SM_Airship) `SM_Airship's long axis is mesh Y` [src Source/IronwakeTest/IronwakeLivingSea.cpp:76]. Whether they are used anywhere else is **unknown** (grep Blueprints / levels). Material masters named in comments: `M_IW_ShipTex2`-based instances `M_IW_ShipTex2` [src Source/IronwakeTest/IronwakeWorld.h:19]. Frigate hull credit: Svjatoi Nikolai scan (Finnish Heritage Agency, CC-BY 4.0), baked sails/yards removed, scaled 55 -> 88 m `scaled 55 -> 88 m` (build report `S2.md`). Brass airship credit "Pigcraft, CC-BY", 110 m `brass airship (Pigcraft, CC-BY), 110 m` [src Source/IronwakeTest/IronwakeLivingSea.cpp:233].

Wake/foam planes on the pawn: three absolute-rotation planes (stern wake, two bow strips) parented to HullBody, translucent sort priority `10` `SetTranslucentSortPriority(10)` [src Source/IronwakeTest/IronwakeShipPawn.cpp:155].

## 2.12 Brass airship (AIronwakeAirship) and the mooring mast

| Constant | Header default | Value set in `SpawnTraffic` (takes effect) |
|---|---|---|
| Loop radius (cm) | `220000.f` [src Source/IronwakeTest/IronwakeLivingSea.h:72] | `190000.f` [src Source/IronwakeTest/IronwakeLivingSea.cpp:652] |
| Altitude (cm) | `42000.f` [src Source/IronwakeTest/IronwakeLivingSea.h:73] | `26000.f` [src Source/IronwakeTest/IronwakeLivingSea.cpp:653] |
| Speed (cm/s) | `1400.f` [src Source/IronwakeTest/IronwakeLivingSea.h:74] | `1500.f` [src Source/IronwakeTest/IronwakeLivingSea.cpp:654] |
| Loop centre (cm) | origin | `FVector(60000.f, 160000.f, 0.f)` [src Source/IronwakeTest/IronwakeLivingSea.cpp:651] |
| Start angle / direction | 0 / +1 | `FMath::DegreesToRadians(-35.f)` [src Source/IronwakeTest/IronwakeLivingSea.cpp:655] / clockwise `+1` |
| Dock hold (s) | `45.f` [src Source/IronwakeTest/IronwakeLivingSea.h:84] | `SetupDock(Top, seaward (0.7071,0.7071), 150)` => `150.f` [src Source/IronwakeTest/IronwakeVoyage.cpp:468] s |

These are **CONFLICT**-class pairs: header defaults are overwritten by the spawn code, the spawn values apply. Cruise altitude for docking: `CruiseAltCm = 26000`; nose offset from actor origin: `NoseXCm = 5500`, `NoseZCm = 3130` `NoseXCm = 5500.f, NoseZCm = 3130.f` [src Source/IronwakeTest/IronwakeLivingSea.h:89]. Docking: approach slows within `40000` cm of the mast (speed factor `clamp(Ahead/40000, 0.04, 1)`), docks when within `150` cm, holds for `DockHoldS`, then departs (factor `clamp(Behind/40000, 0.08, 1)`) until `150000` cm away `Ahead < 40000.f` [src Source/IronwakeTest/IronwakeLivingSea.cpp:264] `Behind > 150000.f` [src Source/IronwakeTest/IronwakeLivingSea.cpp:263]; altitude blends over `160000` cm of path `SmoothStep(0.f, 160000.f, D)` [src Source/IronwakeTest/IronwakeLivingSea.cpp:270]. Bob `600 sin(0.21 T)` cm; yaw = tangent + `6` deg crab; roll `-2.5 * Clockwise` plus wobble; weathervane on the mast `600.f * Bob * FMath::Sin(T * 0.21f)` [src Source/IronwakeTest/IronwakeLivingSea.cpp:275]. Stern prop spins at `PropRevPerSec` = `4.f` [src Source/IronwakeTest/IronwakeLivingSea.h:70] rev/s about the long axis. The mast: `MooringMast` placed at harbour-frame (-150, 262) m, top at `Q + 1.2 + 53` m `P(TEXT("MooringMast"), -150, 262` [src Source/IronwakeTest/IronwakeVoyage.cpp:463]. Airships have no collision body or hazard circle in the autopilot (chart draws them as `~` on the minimap only).

## 2.13 Camera zoom steps and rig per hull

Player zoom steps are re-set in `ApplyHullDef` to ten steps (hull-length multiples): `{ 0.2, 0.3, 0.42, 0.6, 0.8, 1.15, 1.5, 2.2, 3.2, 4.6 }` `{ 0.2f, 0.3f, 0.42f, 0.6f, 0.8f, 1.15f, 1.5f, 2.2f, 3.2f, 4.6f }` [src Source/IronwakeTest/IronwakeShipPawn.cpp:299]; default index `6` (1.5 L) `DefaultZoomIndex = 6` [src Source/IronwakeTest/IronwakeShipPawn.h:143]. Overrides: cutter steps x`1.8` `Z *= 1.8f` [src Source/IronwakeTest/IronwakeShipFleet.cpp:61]; frigate steps `{ 0.36, 0.45, 0.58, 0.75, 0.95, 1.25, 1.7, 2.5, 3.6, 5.2 }` `{ 0.36f, 0.45f, 0.58f, 0.75f, 0.95f, 1.25f, 1.7f, 2.5f, 3.6f, 5.2f }` [src Source/IronwakeTest/IronwakeShipFleet.cpp:63] ("57 m masts, 26 m yards: closest ~32 m, outside the sails"). Arm length = step x `HullLengthM` x 100 cm. Derived default arm (index 6): ironclad 1.5 x 125 = 187.5 m; frigate index 6 = 1.7 x 88 = 149.6 m; cutter 1.5 x 1.8 x 30 = 81 m (derived arithmetic, not a source constant). Wheel moves one index per notch `ZoomIndex + (W > 0.f ? -1 : 1)` [src Source/IronwakeTest/IronwakeShipPawn.cpp:588]; arm eases with rate `7` `1.f - FMath::Exp(-7.f * Dt)` [src Source/IronwakeTest/IronwakeShipPawn.cpp:959].

| Camera constant | Value |
|---|---|
| Default / min / max pitch (deg) | `-6.f` [src Source/IronwakeTest/IronwakeShipPawn.h:149] / `-35.f` [src Source/IronwakeTest/IronwakeShipPawn.h:152] / `3.f` [src Source/IronwakeTest/IronwakeShipPawn.h:156] |
| Normal FOV / scope FOV start | `70` `WantFov = bScope ? ScopeFov : 70.f` [src Source/IronwakeTest/IronwakeShipPawn.cpp:960] / `12.f` [src Source/IronwakeTest/IronwakeShipPawn.h:159]; wheel clamps scope FOV to `4`-`20` deg `FMath::Clamp(ScopeFov * (W > 0.f ? 0.75f : 1.333f), 4.f, 20.f)` [src Source/IronwakeTest/IronwakeShipPawn.cpp:585] |
| FOV ease | `14` `1.f - FMath::Exp(-14.f * Dt)` [src Source/IronwakeTest/IronwakeShipPawn.cpp:949] |
| Pivot height | `max(HullTopAboveOrigin, 0.2*L) + 0.06*L` far; deck pivot `clamp(0.11 L, 3, 12) m` (sail ship `7 m`) near; storm factor drops to `0.6` `FMath::Lerp(1.f, 0.6f, StormCam)` [src Source/IronwakeTest/IronwakeShipPawn.cpp:940] |
| Pivot smoothing | XY rate `14`, Z rate `1.2` `1.f - FMath::Exp(-1.2f * Dt)` [src Source/IronwakeTest/IronwakeShipPawn.cpp:950] |
| Camera floor | never within `200` cm of the rendered sea `+ 200.f` [src Source/IronwakeTest/IronwakeShipPawn.cpp:1035] |
| Scope eye | `0.55 L` ahead along the aim `0.55f * HullLengthM * 100.f` [src Source/IronwakeTest/IronwakeShipPawn.cpp:966] |
| Impact shake | see chapter 5 |

The close-zoom fraction runs from step index 5 (far) to index 1 (near) with a smoothstep `ZoomStepsL.IsValidIndex(5) ? ZoomStepsL[5]` [src Source/IronwakeTest/IronwakeShipPawn.cpp:1177]; at close zoom the pitch range opens to -60..+15 `FMath::Lerp(CamPitchMin, -60.f, Cz)` [src Source/IronwakeTest/IronwakeShipPawn.cpp:569].

## 2.14 Measured results (circle times and handling)

Circle test: flank (frigate on steam at telegraph 5) with hard rudder (`IWRudder(3)`), calm sea, no wind, unwrapped heading 360 deg, `-iws2=circle` via `build/tools/p3circle.sh` / `p7circle.sh`; result line `IWP3 RESULT` `IWP3 RESULT ship=%s full_circle` [src Source/IronwakeTest/IronwakeS2Test.cpp:441]. Test sets `SetFixedWind(0, 0)`, telegraph `5` `IronwakeFleet::SetFixedWind(0.f, 0.f)` [src Source/IronwakeTest/IronwakeS2Test.cpp:409].

| Card | Hull | Circle (s) | Diameter (m) | Yaw rate (deg/s) | R coded (m) | Condition | Source |
|---|---|---|---|---|---|---|---|
| P3 before | ironclad | `240.8` | | | `320` | pre-P3 | `240.8 s (320 m)` (build report `P3.md`) |
| P3 after | ironclad | `201.4` | ~563 | | `280` | 13 kn | `201.4 s (280 m)` (build report `P3.md`) |
| P3 after | frigate | `222.2` | ~373 | | `185` | 8 kn steam | `222.2 s (185 m)` (build report `P3.md`) |
| P3 | cutter | `38.3` | | | `55` | unchanged | `38.3 s (55 m)` (build report `P3.md`) |
| phase_review (pre-substep, variable fps) | ironclad | `288.0` / `221.1` | 562 / 563 | 2.04 / 2.05 | 280 | fps-dependent | `full_circle=288.0 s` (pre-substep turn log) |
| phase_review | frigate | `331.6` / `224.5` | 372 / 373 | 1.91 / 1.88 | 185 | fps-dependent | `full_circle=331.6 s` (pre-substep turn log) |
| phase_review | cutter | `58.0` | 109 | 11.20 | 55 | variable fps | `full_circle=58.0 s` (pre-substep turn log) |
| P7 before fix | ironclad | `178.4` (60/30 fps) / `294.8` (18 fps) | | | 280 | pre-substepping | `178.4` (build report `P7.md`) |
| P7 after fix (30/18 fps) | ironclad | `178.3` / `178.3` | | | 280 | substepping | `178.3` (build report `P7.md`) |
| P7 after | frigate | `192.1` / `192.0` | | | 185 | | `192.1` (build report `P7.md`) |
| P7 after | cutter | `32.9` / `32.9` | | | 55 | | `32.9` (build report `P7.md`) |
| P8 (R 330) | ironclad | `208.8` (30 fps) / `208.8` (18 fps) | ~665 | 1.66 | 330 | pass band 195-225 | `full_circle=208.8 s` (build report `P8/p7_circle_ironclad_30.log`) |
| P9 | ironclad | `208.8` (30 and 18 fps) | | | 330 | with collision on, open water | `208.8 s at 30 fps` (build report `P9.md`) |

Ironclad R=280 case (before P8): `IWP3 RESULT ... full_circle=178.4 s ... yawrate=2.04 R_coded=280` at 60 fps `full_circle=178.4 s` (build report `P8/p7_circle_ironclad_60.log`); the P7 log notes the 3-4 min target was not met at R=280 (2.97 min) and left the decision to the owner `ironclad is 2.97 min (178 s)` (build report `P7.md`). Chuck approved 280 -> 330 (P8) to reach ~3.5 min `Chuck OK 21:25` [src Source/IronwakeTest/IronwakeShipPawn.h:111]. Frigate is `3.2` min at R=185 (P7) `Frigate 3.2 min` (build report `P7.md`). Ideal `2 pi R / v` times are shorter than measured: P3 states "Measured circle time is ~1.6x the ideal" `~1.6x the ideal 2*pi*R/v` (build report `P3.md`) because yaw builds with slew and lateral drag. **No current (post-P8) circle result exists for the frigate and cutter** beyond the P7 numbers; frigate and cutter constants were unchanged since (P8: "Frigate/cutter unchanged") `Frigate/cutter unchanged` (build report `P8.md`).

Acceleration and turn rate measures:
| Card / test | Result | Source |
|---|---|---|
| H1 STOP -> HALF AHEAD 50 % | `4.9` s (before ~20 s) | `**4.9 s**` (build report `H1.md`) |
| H1 STOP -> HALF AHEAD 90 % | `10.0` s | `**10.0 s**` (build report `H1.md`) |
| H1 bow swing after A at FULL (13 kn) | `0.84` s | `**0.84 s**` (build report `H1.md`) |
| H1 hard-turn yaw rate / radius | 1.8 deg/s, ~380 m at 13 kn (pre-P3 R) | `1.8 deg/s (turning circle ~380 m radius at 13 kn)` (build report `H1.md`) |
| S2 cutter 90 % of full | `5.1` s (12.6 kn) | `time_to_90pct_full=5.1 s (12.6 kn)` (build report `S2/s2_handling_cutter.log`) |
| S2 ironclad 90 % of full | `13.0` s (11.7 kn) | `time_to_90pct_full=13.0 s (11.7 kn)` (build report `S2/s2_handling_ironclad.log`) |
| S2 ironclad hard turn (R 280) | yaw 1.99 deg/s, radius 288-289 m (sim, compression 1.5) | `turn_radius=289 m` (build report `S2/s2_handling_ironclad.log`) |
| S2 roster table claims | frigate "turn R 210 m", ironclad "turn R ~290 m", cutter "90 % speed in 5.1 s" | stale: frigate R is 185 since P3 (**CONFLICT**, code wins) `turn R 210 m` (build report `S2.md`) |

Waterline results: A1 calm HALF boot-top clearance `+0.66` m, storm `+0.40` m (ironclad, -60 draft) `midships boot-top clearance **+0.66 m**` (build report `A1.md`); S2 final drafts logged as "frigate +100 cm, cutter +300 cm, ironclad -60 cm" at the time (S2) `frigate +100 cm, cutter +300 cm, ironclad -60 cm` (build report `S2.md`); **CONFLICT**: W3 later set the frigate to `-40` and the cutter to `-25` ("true sea reference ... old +100 was tuned against a 4.7 m-too-low sea reference") `W3 fix: boot-top 0.4 m above the true sea` [src Source/IronwakeTest/IronwakeFleet.cpp:34] `W3 fix: true sea reference` [src Source/IronwakeTest/IronwakeFleet.cpp:68]; the code values apply and the S2 log values are historical.

## 2.15 Gaps / unknown

- BP_PlayerShip Blueprint property overrides: no tuning property names found in the .uasset; if the Blueprint binary stores differing defaults they would override the C++ header defaults for the ironclad and NPC. Measure: log `IWR1 hull` / `IWS2 hull` and `IWTEL` lines at start, or read `Default__BP_PlayerShip_C` through the editor's property API.
- Real speed of the P8 NPC boats and airships (m/s): derivable from ellipse perimeter / period but not logged. Measure with a position log on `AIronwakeNpcCraft`.
- Whether the frigate/cutter combat pivots can fire (turret meshes are hidden): test `IWFire` on those hulls.
- Collision draught of NPC craft (`clamp(-Bounds.Min.Z*0.5, 150, 400)`) per mesh: depends on mesh bounds, not in code. Measure by logging `CollShape.DraughtCm`.
- Number of hull-profile stations: arrays are literal initializer lists (25 values each in the source read); not a named constant.
- Kracken and Devastation roll/pitch/heave periods: only the pawn defaults are present; no per-NPC override was found.
- Frigate/cutter circle times after P8: not re-measured (only P3/P7 values).
- `SM_Ironclad`, `SM_Airship`, `SM_Airship110` mesh assets exist but no code reference was found in the files read here.

## 2.x P16 hydro tuning (2026-11-11)

Periods, zeta, plane follow gains, fit span and the new pitch/roll caps (`8`/`12` deg) are in chapter 3 section 3.12. `FIronwakeShipDef` gained `SeaFollowHeave/Pitch/Roll` and `FitSpan` [src Source/IronwakeTest/IronwakeFleet.h:34]. Ironclad circle time unchanged: `208.8` s at 30 fps and `208.7` s at 18 fps with `-iwcircleat=60000,40000` [src logs/P16.md]. The default circle start (-50000,-300000) now ends aground on the P15 hillside (IWP9 AGROUND at (-74,-3531) m), so use `-iwcircleat=`.

## 2.y P19 hull-class wave data (2026-11-11)

Each roster def carries ship-wave data used by the ship-wave field (chapter 3.13): `WaveCrestK` (bow crest as a fraction of freeboard), `WaveTroughK` (midbody trough as a fraction of draft), `WaveQuarterK`, `WaveWedgeK`, `WaveBluff`, `FreeboardM`, `DraftM` [src Source/IronwakeTest/IronwakeFleet.h]. Values: frigate 0.30 / 0.08 / 0.50 / 1.0 / 0.3 / 4.0 m / 4.2 m; cutter 0.22 / 0 / 0.55 / 1.1 / 0.15 / 1.3 m / 1.4 m; ironclad 0.45 / 0.10 / 0.60 / 0.9 / 0.8 / 3.2 m / 8.0 m [src Source/IronwakeTest/IronwakeFleet.cpp]. The pawn registers itself as a wave source every tick with the real hull profile (length, stem X, max half-beam) and world speed `Kn * 0.514444 * SpeedCompression` [src Source/IronwakeTest/IronwakeShipPawn.cpp]. Prop wash: steamers and the frigate under auxiliary steam only (`bAuxSteam`); twin-screw alpha profile for the ironclad. Not done: cutter bow-rise 2-4 deg at flank, ironclad/cutter drift angle and outward heel in turns (existing P16 heel unchanged).

## 2.z P21 hulls and moving parts (2026-11-12)
- New meshes (`/Game/Ironwake/World/<Name>/SM_<Name>`): HarbourTug 22 m (9/10 source), CoastalSteamer 55 m scaled 0.91 to 50 m (8/10), PilotCutter 14 m (7/10, rigging fused). Tall brig keeps the re-skinned Frigate hull (source scored 3/10).
- Parts (separate static meshes, pivot at the axle): CargoAirshipBody_prop_{fwd,aft}_{port,stbd} (4), GigBody_oar_{0,1,2}_{port,stbd} (6), WindmillBody_sails (1); patrol airship props unchanged (3). `moving_parts` is logged in `IWNPC t=` lines (35-46 with live bodies plus 1 windmill).
- Oar stroke: 2 s cycle (catch, drive, release, recover), only when the gig is under way. Props turn with thrust; windmill with wind speed.

## 2.aa P22 moving parts (2026-11-12)
- Gig oars: the FBX oar mesh has its blade on the -Y end (looms +Y inboard at the rowlock origin), so the port/starboard pivot Y signs were swapped in P21 (blades ended up inboard). Now port pivot Y=+160, stbd Y=-162 (hull-mesh cm) with the sign flipped to match. Stroke = yaw sweep (-28..+30 deg) plus a roll dip of 8..33 deg about the oar's long axis' pivot (`Dip`, blade down on the drive, clear on the recovery), never a spin about the long axis. Numeric check (`-iwoarlog`, `IWNPC oar side=... blade_rel= grip_rel=`): blade ~2.1 m outboard / grip ~1.2 m inboard of the rowlock, blade z -153..-49 cm below the rowlock on the stroke. Gig draft 1.2 m (was 0.35: hull floated on top of the water). Three seated rowers (Kind 5, torso cylinder + head + thighs) rock with the stroke.
- Propellers: patrol airship prop meshes (cut from the Hunyuan hull) render as dark jagged slabs; replaced with clean procedural blades (3 on the side props, 4 on the stern) at the measured hubs. Every prop (procedural and cargo mesh props) has a blades group and a faint translucent disc (M_IW_Smk, Op 0.10); above 1.7 rev/s (rate follows thrust) the blades hide and the disc shows. Cargo props get distinct rates (x1.0..1.5) and phases. `-iwproplog` prints `IWNPC prop_rate=<rev/s> idx= disc= angle=` twice a second per prop.
- Cargo-airship prop meshes (nacelle props) are still mesh props: a pair on one nacelle can look frozen when both turn at nearly the same rate in a 0.1 s frame pair (stroboscopic), see logs/P22.md.

## 2.ab P25 sailing brig wide-turn, harbour auxiliary & NPC steerage (2026-11-13)
- Square-riggers (`D.bSail && D.bRig`) use a `1.55`x wide turn radius outside the roads `D.TurnRadiusM * 1.55f` [src Source/IronwakeTest/IronwakeNpcShip.cpp:519] and engage auxiliary power (`R.bAux = true`) within `450` m (`45000` cm) of the Greyhaven harbour mouth `MouthDistCm < 45000.f` [src Source/IronwakeTest/IronwakeNpcBrain.cpp:1026] so wind shifts never blow them into irons across the throat.
- Basin tug-assisted steerage (`Rad *= 0.36f`) now covers `Depart`, `Transit` and `Arrive` `R->State == ENpcState::Arrive` [src Source/IronwakeTest/IronwakeNpcShip.cpp:520], sail leeway (`6` deg max) is suppressed whenever `R->bAux` is active `if (D.bSail && !R->bAux && SailFill > 0.01f)` [src Source/IronwakeTest/IronwakeNpcShip.cpp:532], and sailing craft keep steerage across large heading turns (`|Err| > 22` deg) `FMath::Abs(Err) > 22.f` [src Source/IronwakeTest/IronwakeNpcShip.cpp:522].
- Active NPC roster holds `30` ledger vessels with `MaxLive = 26` `int32 MaxLive = 26;` [src Source/IronwakeTest/IronwakeNpc.h:230], `SpawnRadiusCm = 520000.f` [src Source/IronwakeTest/IronwakeNpc.h:228], `FxRadiusCm = 110000.f` and `MaxFx = 12` `float FxRadiusCm = 110000.f; int32 MaxFx = 12` [src Source/IronwakeTest/IronwakeNpc.h:229]. Legacy `AIronwakeMerchant` ships (`-iwoldnpc`) register in `Contacts` with `RecId = -2` and run a `6` s collision look-ahead yield `SpeedCmS * 6.f` [src Source/IronwakeTest/IronwakeLivingSea.cpp:175].

## 2.ac P26 playable & NPC 42 m Tall Brig (17 moving parts + gaff spanker + WPO wind sails) (2026-11-13)
- Playable Shipyard roster expanded from 3 to 4 hulls (`ShipBtn0..5` in `IronwakePlayerController`), adding `brig` (`Curlew`, `Merchant brig  |  42 m  |  480 t` [src Source/IronwakeTest/IronwakeFleet.cpp:42], `LengthM = 42.f; D.BeamM = 9.5f; D.MassKg = 4.8e5f` [src Source/IronwakeTest/IronwakeFleet.cpp:46], `MinTurnRadiusM = 95.f` [src Source/IronwakeTest/IronwakeFleet.cpp:48], `DraftBelowBootTopCm = 310.f` [src Source/IronwakeTest/IronwakeFleet.cpp:51], `DraftM = 3.1f` [src Source/IronwakeTest/IronwakeFleet.cpp:52], profile `&FIronwakeHullProfile::TallBrig` [src Source/IronwakeTest/IronwakeFleet.cpp:53]).
- NPC `"brig"` class (`Brig Prosper`, `Brig Lady Merriweather`, `Brig Constant`, `Brig Halcyon`) switched from the re-skinned Frigate hull to `D.WorldMesh = TEXT("TallBrig"); D.BootTopCm = -310.f` [src Source/IronwakeTest/IronwakeNpcDirector.cpp:82] (`LengthM = 42.f; D.BeamM = 9.5f; D.DraftM = 3.1f` [src Source/IronwakeTest/IronwakeNpcDirector.cpp:82]) with full `UIronwakeSailRig` (`bTallBrig = true`).
- `SetupTallBrig` attaches all 17 imported `SM_TallBrig_*` spar and sail static meshes at their exact `parts.json` pivots: 6 square yards braced `+/- 32` deg around Z from apparent wind, 6 square sails (`course`, `topsail`, `topgallant` on fore and main masts) with Z-scale furling + `CanvasRoll` bundles, mainmast `boom_spanker` + `gaff_spanker` + `sail_spanker` sheeted `0..38` deg to leeward with tack-flipped camber, `fore_staysail` + `jib` trimmed around their stay axes, and dark-tarred standing shrouds/stays/backstays (`AddStandingRig`) plus masthead pennants.
- Dynamic `M_IW_SailCanvas` WPO parameters (`WindBelly` [src Source/IronwakeTest/IronwakeFleet.cpp:592], `LuffAmp`, `WindSign`, `WindPhase`) add smooth leeward belly on a reach and luffing ripple inside the `35` deg no-go zone without `Clamp` expressions on `SF_METAL_SM6`.
- Harbour reduced-canvas rule on NPC brigs: under `R->bAux` (`<= 450` m of harbour mouth or inside basin `Depart`/`Transit`/`Arrive`) or `WindKn > 36`, NPC brigs reef down to `StepLvl = 3` [src Source/IronwakeTest/IronwakeNpcShip.cpp:861] (topsails + jib set, lower courses & topgallants furled on yards) and furl all sails (`StepLvl = 2` [src Source/IronwakeTest/IronwakeNpcShip.cpp:857]) when moored or holding station.

## 2.ad P28 moving paddle/crane/bell/airship parts + 3D crew, rowers & quarterdeck helmsmen (2026-11-13)
- **`PaddleSteamerV3` (`Duchess of Kestrel`):** `D.WorldMesh = TEXT("PaddleSteamerV3")` [src Source/IronwakeTest/IronwakeNpcDirector.cpp:70], `D.BootTopCm = -393.f` [src Source/IronwakeTest/IronwakeNpcDirector.cpp:70] (`DraftM = 2.45f`), port & starboard `PaddleSteamerV3_wheel_{port,stbd}` scaled `0.86`x inside the sponson wheel-wells (`Kind = 1`, lateral `Y` spin proportional to speed) plus speed-gated `PaddleChurnPads` (`M_IW_BowFoam`) and raked stern jackstaff ensign (`8.5 / 10` (build report `P28.md`)).
- **`SalvageTenderV3` (`Dredger's Hope`):** `D.WorldMesh = TEXT("SalvageTenderV3")` [src Source/IronwakeTest/IronwakeNpcDirector.cpp:74] (`BootTopCm = -304.f`), articulated `SalvageTenderV3_crane` (`Kind = 6`, slewing `+/- 26` deg around `Z` at `Om = 1.15` rad/s during `Work`/moored) + child `SalvageTenderV3_bell` (`Kind = 7`, `0.76`x scale, `+77` cm clear of stern rail, lagged pendulum swing) + two `SailorCrewStand` salvage riggers (`9.5 / 10` (build report `P28.md`)).
- **`PatrolAirshipV4` (`Aurora Watch`):** `D.WorldMesh = TEXT("PatrolAirshipV4")` [src Source/IronwakeTest/IronwakeNpcDirector.cpp:90] clean bladeless-pod airship with 3 `PatrolAirshipV4_prop_{port,stbd,stern}` 4-blade brass/mahogany propellers + `Op = 0.055` translucent motion disc above `1.7` rev/s (`7.5 / 10` (build report `P28.md`)).
- **3D `SailorCrewRower` gig crew & quarterdeck helmsmen:** `GigBody` seats 3 Bone-Heat posed `SailorCrewRower` figures on the sculpted thwarts (`104.f, -16.f, -126.f` [src Source/IronwakeTest/IronwakeNpcShip.cpp:310], rocking `+/- 17` deg across the `1.50` s stroke) + stern coxswain, with all 6 oars in brass U-rowlocks `60` cm aft of each thwart (`+12` cm gunwale clearance, `9.5 / 10` (build report `P28.md`)). Player hulls (`IronwakeShipFleet.cpp`) and NPC surface ships carry a `SailorCrewStand` helmsman (`35000.f` cm cull) + rudder-linked spoked brass helm wheel; NPC ships also spawn live `MooringLines` strictly when `ENpcState::Docked` at a quay berth (`moving_parts=122` (build report `P28.md`)).
