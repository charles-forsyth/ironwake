# Chapter 9: Input, HUD, menus, settings

Scope: every key and gamepad binding the game registers (Enhanced Input mapping contexts, their priorities and triggers), what each action does when it fires, the in-game HUD (Canvas HUD in `IronwakeHUD.cpp` plus the voyage HUD in `IronwakeVoyage.cpp`), the CONTROLS help panel, the title/pause menu, end screens, saved settings (file, keys, defaults) and the LOW/MEDIUM/HIGH graphics presets. Everything is read from source; line numbers refer to the files at the time of writing. Citations use `value` [src path:line]. Where two places disagree the behaviour that actually executes is documented and the other marked **CONFLICT**.

## 9.1 Input architecture

All input is Enhanced Input (project default classes set in Config/DefaultInput.ini). Four mapping contexts are built in code (no Input Action assets on disk); three belong to the controller and one to the ship pawn.

| Item | Value | Source |
|---|---|---|
| Player input class | `/Script/EnhancedInput.EnhancedPlayerInput` [src Config/DefaultInput.ini:86] | Config/DefaultInput.ini |
| Input component class | `/Script/EnhancedInput.EnhancedInputComponent` [src Config/DefaultInput.ini:87] | Config/DefaultInput.ini |
| Ship context `IMC_Ship`, priority | `IMC_Ship` [src Source/IronwakeTest/IronwakeShipPawn.cpp:390] added at priority `AddMappingContext(ShipContext, 0)` [src Source/IronwakeTest/IronwakeShipPawn.cpp:447] (re-added on every `PawnClientRestart`) | IronwakeShipPawn.cpp |
| Menu/voyage context `IMC_Menu`, priority | `IMC_Menu` [src Source/IronwakeTest/IronwakePlayerController.cpp:709] added at `AddMappingContext(MenuContext, 10)` [src Source/IronwakeTest/IronwakePlayerController.cpp:783] | IronwakePlayerController.cpp |
| Photo context `IMC_Photo`, priority | `IMC_Photo` [src Source/IronwakeTest/IronwakePlayerController.cpp:758] added at `AddMappingContext(PhotoContext, 20)` [src Source/IronwakeTest/IronwakePlayerController.cpp:913] only while photo mode is on | IronwakePlayerController.cpp |
| Chart context `IMC_Chart`, priority | `IMC_Chart` [src Source/IronwakeTest/IronwakePlayerController.cpp:746] added at `AddMappingContext(ChartContext, 30)` [src Source/IronwakeTest/IronwakePlayerController.cpp:875] only while the chart is open | IronwakePlayerController.cpp |
| Controller ticks while paused | `bShouldPerformFullTickWhenPaused = true` [src Source/IronwakeTest/IronwakePlayerController.cpp:659] | IronwakePlayerController.cpp |
| Pause action fires while paused | `bTriggerWhenPaused = true` [src Source/IronwakeTest/IronwakePlayerController.cpp:708] | IronwakePlayerController.cpp |
| Console key | `+ConsoleKeys=Tilde` [src Config/DefaultInput.ini:90] | Config/DefaultInput.ini |
| Mouse axis sensitivity (MouseX, MouseY, Mouse2D) | `Sensitivity=0.070000` [src Config/DefaultInput.ini:13] (also lines 14-15) | Config/DefaultInput.ini |
| Gamepad stick dead zone (LeftX/LeftY/RightX/RightY) | `DeadZone=0.250000` [src Config/DefaultInput.ini:9] (lines 9-12) | Config/DefaultInput.ini |
| Fullscreen toggles | `bAltEnterTogglesFullscreen=True` [src Config/DefaultInput.ini:64], `bF11TogglesFullscreen=True` [src Config/DefaultInput.ini:65] | Config/DefaultInput.ini |
| Mouse capture | `DefaultViewportMouseCaptureMode=CapturePermanently_IncludingInitialMouseDown` [src Config/DefaultInput.ini:81], `DefaultViewportMouseLockMode=LockOnCapture` [src Config/DefaultInput.ini:82] | Config/DefaultInput.ini |
| Mouse smoothing flag | `bEnableMouseSmoothing=True` [src Config/DefaultInput.ini:67] | Config/DefaultInput.ini |

**CONFLICT (smoothing):** DefaultInput.ini sets `bEnableMouseSmoothing=True` [src Config/DefaultInput.ini:67] but `OnLook` documents "raw 1:1 mouse (no smoothing)" [src Source/IronwakeTest/IronwakeShipPawn.cpp:557]. The code path reads the Mouse2D value directly and never applies smoothing itself; whether the legacy flag changes the Mouse2D value fed to Enhanced Input is unknown (measure by logging `V.Get<FVector2D>()` per frame in `OnLook` with the flag toggled).

Input modes: title/pause/end screens use `FInputModeGameAndUI` with cursor shown and no mouse lock [src Source/IronwakeTest/IronwakePlayerController.cpp:641-645]; leaving the menu returns to `FInputModeGameOnly()` [src Source/IronwakeTest/IronwakePlayerController.cpp:889] with the cursor hidden. The chart uses GameAndUI with `DoNotLock` and `SetHideCursorDuringCapture(false)` [src Source/IronwakeTest/IronwakePlayerController.cpp:883].

### Gating (when keys are ignored)

| Rule | Source |
|---|---|
| Voyage and autopilot keys run only if `bStarted && !IsMenuOpen() && !IsEndScreenOpen() && !World->IsPaused()` | `bStarted && !IsMenuOpen() && !IsEndScreenOpen()` [src Source/IronwakeTest/IronwakePlayerController.cpp:790] |
| In photo mode only F (index 0, exit) and L (index 5, weather) work among the voyage keys | `Which != 0 && Which != 5` [src Source/IronwakeTest/IronwakePlayerController.cpp:797] |
| On the chart, L (index 5) toggles route loop instead of weather; F (index 0) is ignored | `Vy->ToggleLoop()` [src Source/IronwakeTest/IronwakePlayerController.cpp:798], `Vy->bChart && Which == 0` [src Source/IronwakeTest/IronwakePlayerController.cpp:799] |
| Fire does nothing while a menu or end screen is open or before START | `PC->IsMenuOpen() || PC->IsEndScreenOpen() || !PC->bStarted` [src Source/IronwakeTest/IronwakeCombat.cpp:666] |
| Scope toggle ignored while menu open or not started | `PC->IsMenuOpen() || !PC->bStarted` [src Source/IronwakeTest/IronwakeShipPawn.cpp:513] |
| Esc/Tab while help is open closes the help first | `IsHelpOpen()` [src Source/IronwakeTest/IronwakePlayerController.cpp:947] |
| Esc/Tab on the title menu does nothing (`!bStarted`) | `if (!bStarted) return;` [src Source/IronwakeTest/IronwakePlayerController.cpp:935] |
| Esc/Tab on SHIP LOST does nothing; on VICTORY it is CONTINUE | `Esc on VICTORY = Continue` [src Source/IronwakeTest/IronwakePlayerController.cpp:938] |

## 9.2 Binding table: ship context (`IMC_Ship`, priority 0)

Action value types: Throttle, Rudder, Wheel are `Axis1D`; Look is `Axis2D`; the rest `Boolean` [src Source/IronwakeTest/IronwakeShipPawn.cpp:375-385]. "Pressed" = `UInputTriggerPressed` (one event per key down); "held" = no trigger (fires every frame while down). "Negate" = `UInputModifierNegate`.

| Key | Action | Trigger / modifier | Handler effect | Source |
|---|---|---|---|---|
| `EKeys::W` | IA_Throttle +1 | Pressed | one telegraph step up (|value| must exceed `0.5`) | `EKeys::W, false` [src Source/IronwakeTest/IronwakeShipPawn.cpp:400] [src Source/IronwakeTest/IronwakeShipPawn.cpp:487] |
| `EKeys::S` | IA_Throttle -1 | Pressed + Negate | one telegraph step down | `EKeys::S, true` [src Source/IronwakeTest/IronwakeShipPawn.cpp:401] |
| `EKeys::Up` | IA_Throttle +1 | Pressed | as W | `EKeys::Up, false` [src Source/IronwakeTest/IronwakeShipPawn.cpp:402] |
| `EKeys::Down` | IA_Throttle -1 | Pressed + Negate | as S | `EKeys::Down, true` [src Source/IronwakeTest/IronwakeShipPawn.cpp:403] |
| `EKeys::D` | IA_Rudder +1 | held | wheel slews while held (`RudderInput`), holds on release unless auto-centre | `EKeys::D, false` [src Source/IronwakeTest/IronwakeShipPawn.cpp:410] |
| `EKeys::A` | IA_Rudder -1 | held + Negate | wheel slews other way | `EKeys::A, true` [src Source/IronwakeTest/IronwakeShipPawn.cpp:411] |
| `EKeys::Right` | IA_Rudder +1 | held | as D | `EKeys::Right, false` [src Source/IronwakeTest/IronwakeShipPawn.cpp:412] |
| `EKeys::Left` | IA_Rudder -1 | held + Negate | as A | `EKeys::Left, true` [src Source/IronwakeTest/IronwakeShipPawn.cpp:413] |
| `EKeys::Q` | IA_CenterRudder | Pressed | rudder midships (`CenterRudder()`) | `EKeys::Q` [src Source/IronwakeTest/IronwakeShipPawn.cpp:414] |
| `EKeys::E` | IA_CenterRudder | Pressed | rudder midships | `EKeys::E` [src Source/IronwakeTest/IronwakeShipPawn.cpp:415] |
| `EKeys::Mouse2D` | IA_Look | - | orbit camera / aim guns; control rotation yaw += X, pitch += Y; scaled by `CurFov/70` (min `0.08`) in the scope | `EKeys::Mouse2D` [src Source/IronwakeTest/IronwakeShipPawn.cpp:416], `0.08f` [src Source/IronwakeTest/IronwakeShipPawn.cpp:563] |
| `EKeys::RightMouseButton` | IA_Zoom | - | Started: exit the scope (RMB no longer zooms) | `EKeys::RightMouseButton` [src Source/IronwakeTest/IronwakeShipPawn.cpp:418], `OnZoomStart` [src Source/IronwakeTest/IronwakeShipPawn.cpp:575] |
| `EKeys::LeftShift` | IA_Scope | Pressed | toggle binoculars / gun sight | `EKeys::LeftShift` [src Source/IronwakeTest/IronwakeShipPawn.cpp:419] |
| `EKeys::RightShift` | IA_Scope | Pressed | toggle scope | `EKeys::RightShift` [src Source/IronwakeTest/IronwakeShipPawn.cpp:420] |
| `EKeys::C` | IA_FreeLook | held | Started: save aim rotation, free look; Completed: snap back | `EKeys::C` [src Source/IronwakeTest/IronwakeShipPawn.cpp:421] |
| `EKeys::MiddleMouseButton` | IA_FreeLook | held | as C | `EKeys::MiddleMouseButton` [src Source/IronwakeTest/IronwakeShipPawn.cpp:422] |
| `EKeys::MouseWheelAxis` | IA_Wheel | - | scope: magnification (FOV x`0.75` in / x`1.333` out, clamp `4`..`20`); else camera zoom step | `EKeys::MouseWheelAxis` [src Source/IronwakeTest/IronwakeShipPawn.cpp:423], `0.75f` [src Source/IronwakeTest/IronwakeShipPawn.cpp:585] |
| `EKeys::LeftMouseButton` | IA_Fire | Pressed | `TryFire()` | `EKeys::LeftMouseButton` [src Source/IronwakeTest/IronwakeShipPawn.cpp:424] |
| `EKeys::Gamepad_RightTrigger` | IA_Fire | Pressed | `TryFire()` | `EKeys::Gamepad_RightTrigger` [src Source/IronwakeTest/IronwakeShipPawn.cpp:425] |
| `EKeys::R` | IA_Repair | Pressed | damage control, puts out fires, cooldown `45` s | `EKeys::R` [src Source/IronwakeTest/IronwakeShipPawn.cpp:420], `RepairCooldown = 45.f` [src Source/IronwakeTest/IronwakeCombat.cpp:686] |
| `EKeys::B` | IA_Steam | Pressed | auxiliary steam (sailing ship) | `EKeys::B` [src Source/IronwakeTest/IronwakeShipPawn.cpp:427] |
| `EKeys::Gamepad_DPad_Up` | IA_Throttle +1 | Pressed | telegraph up | `Gamepad_DPad_Up` [src Source/IronwakeTest/IronwakeShipPawn.cpp:429] |
| `EKeys::Gamepad_DPad_Down` | IA_Throttle -1 | Pressed + Negate | telegraph down | `Gamepad_DPad_Down` [src Source/IronwakeTest/IronwakeShipPawn.cpp:430] |
| `EKeys::Gamepad_DPad_Right` | IA_Rudder +1 | held | wheel right | `Gamepad_DPad_Right` [src Source/IronwakeTest/IronwakeShipPawn.cpp:431] |
| `EKeys::Gamepad_DPad_Left` | IA_Rudder -1 | held + Negate | wheel left | `Gamepad_DPad_Left` [src Source/IronwakeTest/IronwakeShipPawn.cpp:432] |
| `EKeys::Gamepad_Right2D` | IA_Look | - | camera look | `Gamepad_Right2D` [src Source/IronwakeTest/IronwakeShipPawn.cpp:433] |
| `EKeys::Gamepad_LeftTrigger` | IA_Zoom | - | exits the scope (Started) | `Gamepad_LeftTrigger` [src Source/IronwakeTest/IronwakeShipPawn.cpp:434] |

Bound events (which trigger event each action uses): Throttle, Rudder, Steam, Scope, Center, Look, Wheel, Fire, Repair use `Triggered`; Rudder additionally `Completed` (release); FreeLook `Started`/`Completed`; Zoom `Started`/`Completed` [src Source/IronwakeTest/IronwakeShipPawn.cpp:464-477].

Rudder details: release zeroes `RudderInput`; with auto-centre ON the step goes to `0` so `ApplyShipForces` springs the wheel back, otherwise the step mirrors the held angle [src Source/IronwakeTest/IronwakeShipPawn.cpp:502-505]. Binoculars: `ScopeFov = 12.f` [src Source/IronwakeTest/IronwakeShipPawn.cpp:521] on entry (`12` deg, 70/12 = 5.8x), range `4`..`20` deg (`17x`..`3.5x`).

## 9.3 Binding table: menu/voyage context (`IMC_Menu`, priority 10)

All Pressed. The 11 "voyage" actions are `IA_Voyage0..10` indexed in the order of the key array; the 4 autopilot actions are `IA_Ap0..3`.

| Key | Index | Handler | Effect | Source |
|---|---|---|---|---|
| `EKeys::Tab` | IA_Pause | `OnPauseAction` | toggle pause menu | `EKeys::Tab` [src Source/IronwakeTest/IronwakePlayerController.cpp:710] |
| `EKeys::Escape` | IA_Pause | same | toggle pause menu (closes help first) | `EKeys::Escape` [src Source/IronwakeTest/IronwakePlayerController.cpp:710] |
| `EKeys::Gamepad_Special_Right` | IA_Pause | same | toggle pause menu (the only gamepad menu binding) | `Gamepad_Special_Right` [src Source/IronwakeTest/IronwakePlayerController.cpp:710] |
| `EKeys::F` | Voyage 0 | `OnVoyageKey` | `TogglePhoto()` (ignored while the chart is open) | `EKeys::F, EKeys::H` [src Source/IronwakeTest/IronwakePlayerController.cpp:716], `Vy->TogglePhoto()` [src Source/IronwakeTest/IronwakePlayerController.cpp:802] |
| `EKeys::H` | Voyage 1 | | `ToggleHelp()` | `ToggleHelp(); break;` [src Source/IronwakeTest/IronwakePlayerController.cpp:803] |
| `EKeys::N` | Voyage 2 | | `NextPoint()` (next point of interest) | `Vy->NextPoint()` [src Source/IronwakeTest/IronwakePlayerController.cpp:804] |
| `EKeys::X` | Voyage 3 | | `ToggleAnchor()` | `Vy->ToggleAnchor()` [src Source/IronwakeTest/IronwakePlayerController.cpp:805] |
| `EKeys::G` | Voyage 4 | | `ToggleFastSail()` (x4, peaceful) | `Vy->ToggleFastSail()` [src Source/IronwakeTest/IronwakePlayerController.cpp:806] |
| `EKeys::L` | Voyage 5 | | `CycleWeather()`; on the chart `ToggleLoop()` | `Vy->CycleWeather()` [src Source/IronwakeTest/IronwakePlayerController.cpp:807] |
| `EKeys::K` | Voyage 6 | | `ToggleAutoCycle()` (live world: weather drift on/off) | `Vy->ToggleAutoCycle()` [src Source/IronwakeTest/IronwakePlayerController.cpp:808] |
| `EKeys::J` | Voyage 7 | | `ToggleHelm()` helmsman steers for next point | `Vy->ToggleHelm()` [src Source/IronwakeTest/IronwakePlayerController.cpp:809] |
| `EKeys::T` | Voyage 8 | | live world: `CycleClockSpeed()`; with Shift held `JumpHours(3)` and it also leaves the scope | `IsInputKeyDown(EKeys::LeftShift)` [src Source/IronwakeTest/IronwakePlayerController.cpp:811], `Vy->CycleClockSpeed()` [src Source/IronwakeTest/IronwakePlayerController.cpp:816] |
| `EKeys::F1` | Voyage 9 | | `ToggleHelp()` | `case 9: ToggleHelp()` [src Source/IronwakeTest/IronwakePlayerController.cpp:818] |
| `EKeys::RightBracket` | Voyage 10 | | live world: `JumpHours(3)` | `case 10` [src Source/IronwakeTest/IronwakePlayerController.cpp:819] |
| `EKeys::P` | Ap 0 | `OnApKey` | `CycleAutopilot()` (works in photo mode) | `EKeys::P, EKeys::M` [src Source/IronwakeTest/IronwakePlayerController.cpp:736], `Vy->CycleAutopilot()` [src Source/IronwakeTest/IronwakePlayerController.cpp:850] |
| `EKeys::M` | Ap 1 | | `ToggleChart()` (not in photo mode) | `Vy->ToggleChart()` [src Source/IronwakeTest/IronwakePlayerController.cpp:851] |
| `EKeys::Enter` | Ap 2 | | `ChartStart()` only while the chart is open | `Vy->ChartStart()` [src Source/IronwakeTest/IronwakePlayerController.cpp:853] |
| `EKeys::BackSpace` | Ap 3 | | `ClearWaypoints()` only while the chart is open | `Vy->ClearWaypoints()` [src Source/IronwakeTest/IronwakePlayerController.cpp:856] |

Autopilot cycle: P steps off -> hold course (`1`) -> waypoints (`2`) -> tour (`3`) -> off [src Source/IronwakeTest/IronwakeAutopilot.cpp:281-284]; with waypoints on the open chart P starts the route instead [src Source/IronwakeTest/IronwakeAutopilot.cpp:277].

## 9.4 Binding table: chart context (`IMC_Chart`, priority 30, only while the chart is open)

| Key | Action | Effect | Source |
|---|---|---|---|
| `EKeys::LeftMouseButton` | IA_ChartL (Pressed) | add waypoint at the mouse position (`ChartClick(pos, false)`) | `EKeys::LeftMouseButton).Triggers` [src Source/IronwakeTest/IronwakePlayerController.cpp:749], `OnChartClick, false` [src Source/IronwakeTest/IronwakePlayerController.cpp:776] |
| `EKeys::RightMouseButton` | IA_ChartR (Pressed) | remove waypoint (`ChartClick(pos, true)`) | `EKeys::RightMouseButton).Triggers` [src Source/IronwakeTest/IronwakePlayerController.cpp:752], `OnChartClick, true` [src Source/IronwakeTest/IronwakePlayerController.cpp:777] |
| `Mouse2D`, `MouseWheelAxis`, `LeftShift`, `A`, `D`, `Left`, `Right` | IA_ChartSink (Axis2D, no binding) | swallowed (camera look, zoom, scope, rudder, fire-by-mouse cannot reach the ship context) | `EKeys::Mouse2D, EKeys::MouseWheelAxis, EKeys::LeftShift, EKeys::A, EKeys::D, EKeys::Left, EKeys::Right` [src Source/IronwakeTest/IronwakePlayerController.cpp:755] |

W/S still set telegraph with the chart open (comment at [src Source/IronwakeTest/IronwakePlayerController.cpp:453]); Q/E, B, R and C are not swallowed. Enter/Backspace/P/M act through the menu context (9.3).

## 9.5 Binding table: photo context (`IMC_Photo`, priority 20, only in photo mode)

| Key(s) | Action | Effect | Source |
|---|---|---|---|
| `W A S D Q E Up Down Left Right Mouse2D LeftMouseButton RightMouseButton R LeftShift SpaceBar LeftControl` | IA_PhotoSink (Axis2D, no handler) | swallows the ship's keys so only the free camera moves | `EKeys::W, EKeys::A, EKeys::S, EKeys::D, EKeys::Q, EKeys::E` [src Source/IronwakeTest/IronwakePlayerController.cpp:761], `EKeys::R, EKeys::LeftShift, EKeys::SpaceBar, EKeys::LeftControl` [src Source/IronwakeTest/IronwakePlayerController.cpp:762] |
| `MouseWheelAxis` | IA_PhotoWheel (Axis1D) | accumulates `PhotoFovDelta`; each notch changes FOV by `3` deg, clamp `15`..`100` deg | `EKeys::MouseWheelAxis` [src Source/IronwakeTest/IronwakePlayerController.cpp:755], `15.f, 100.f` [src Source/IronwakeTest/IronwakeVoyage.cpp:970] |
| `T` | IA_PhotoTime (Pressed) | sets `bPhotoTimeCycle` (photo slow-mo / time cycle) | `EKeys::T` [src Source/IronwakeTest/IronwakePlayerController.cpp:769] |

Photo-mode movement is polled in `PlayerTick` with `IsInputKeyDown`: W/S forward-back, D/A strafe, E/Q up-down, LeftShift = fast [src Source/IronwakeTest/IronwakePlayerController.cpp:868-869]. Mouse look in photo mode = mouse delta x `0.6f` [src Source/IronwakeTest/IronwakePlayerController.cpp:923].

## 9.6 Gamepad summary

Complete list of gamepad keys registered anywhere: DPad Up/Down (telegraph), DPad Left/Right (rudder), Right stick 2D (look), Right trigger (fire), Left trigger (exits scope), Special Right (pause). There is **no** gamepad binding for: entering the scope (IA_Scope is keyboard Shift only), centre rudder, repair, steam, photo, help, chart, autopilot, weather, anchor, freelook. Stick dead zone `0.25` [src Config/DefaultInput.ini:9]. Help panel and old controls text both state "D-pad telegraph / rudder, R trigger fire, R stick look" [src Source/IronwakeTest/IronwakeP11.cpp:377-378]. `bEnableMotionControls=True` [src Config/DefaultInput.ini:71] has no code consumer found.

## 9.7 Console (Exec) commands (typed in `~` console)

| Command | Defined in | Effect | Source |
|---|---|---|---|
| `IWMenu <mode>` | PlayerController | `-1` hides, `0` title, `1` pause | `IWMenu(int32 Mode)` [src Source/IronwakeTest/IronwakePlayerController.cpp:1102] |
| `IWCredits` | PlayerController | title menu with credits card | `IWCredits()` [src Source/IronwakeTest/IronwakePlayerController.cpp:1103] |
| `IWYard [i]` | PlayerController | open shipyard card | `IWYard(int32 Highlight` [src Source/IronwakeTest/IronwakePlayerController.cpp:1085] |
| `IWShot <name>` | PlayerController | PNG to `Saved/shots/<name>.png` | `TEXT("shots")` [src Source/IronwakeTest/IronwakePlayerController.cpp:1305] |
| `IWEnd <0|1|-1>` | PlayerController | SHIP LOST / VICTORY / hide | `IWEnd(int32 Mode)` [src Source/IronwakeTest/IronwakePlayerController.cpp:1310] |
| `IWStart <0|1>` | PlayerController | start voyage (1 = peaceful) | `IWStart(int32 Peaceful)` [src Source/IronwakeTest/IronwakePlayerController.cpp:1108] |
| `IWGfx <0|1|2>` | PlayerController | set graphics preset (saved) | `IWGfx(int32 Level)` [src Source/IronwakeTest/IronwakePlayerController.cpp:1109] |
| `IWMusic <0|1>` | PlayerController | music setting (saved) and re-apply to the player ship | `IWMusic(int32 On)` [src Source/IronwakeTest/IronwakePlayerController.cpp:1110] |
| `IWPhoto`, `IWWeather <n>`, `IWHelm`, `IWAnchor`, `IWFastSail` | PlayerController | same functions as F, L, J, X, G; IWWeather uses a `6` s blend | `Vy->SetWeather(Preset, 6.f)` [src Source/IronwakeTest/IronwakePlayerController.cpp:927] |
| `IWAutopilot <0-3>`, `IWChart` | PlayerController | autopilot mode (clamped `1`..`3`) / chart toggle | `FMath::Clamp(Mode, 1, 3)` [src Source/IronwakeTest/IronwakePlayerController.cpp:899] |
| `IWZoom <i>`, `IWScope <0|1>`, `IWH1` | ShipPawn | camera step, scope, H1 handling test | `IWZoom(int32 Index)` [src Source/IronwakeTest/IronwakeShipPawn.cpp:539] |
| `IWTelegraph <i>`, `IWRudder <step>`, `IWDemo` | ShipPawn | set telegraph / rudder step / demo | `IWTelegraph(int32 Index)` [src Source/IronwakeTest/IronwakeShipPawn.cpp:612] |
| `IWFire`, `IWAimAssist <0|1>`, `IWLead <0|1>`, `IWDamage <amt> [self]`, `IWSteam <0|1>` | ShipPawn | fire; aim assist (default off); lead indicator (default on); damage enemy or self; aux steam | `bLeadIndicator = true` [src Source/IronwakeTest/IronwakeShipPawn.h:248], `bAimAssist = false` [src Source/IronwakeTest/IronwakeShipPawn.h:251] |

## 9.8 Title menu and pause menu (P18 rebuild; the older single-column description below is superseded)

P18 replaced the single vertical button column with a paged, scaled menu [src Source/IronwakeTest/IronwakePlayerController.cpp:UIronwakeMenuWidget]. The old tables below are kept for history; where they disagree the P18 section (9.x P18 at the end of this chapter) wins.

### 9.8-old Title menu and pause menu (pre-P18)

One widget class (`UIronwakeMenuWidget`), `Mode` `0` = title, `1` = pause. Pure C++ UMG (no assets). Added to viewport at z-order `AddToViewport(50)` [src Source/IronwakeTest/IronwakePlayerController.cpp:1028]. Card width `CardW` [src Source/IronwakeTest/IronwakePlayerController.cpp:180], max height `980.f` [src Source/IronwakeTest/IronwakePlayerController.cpp:180], backdrop alpha `0.84f` [src Source/IronwakeTest/IronwakePlayerController.cpp:183]. Title text `IRONWAKE` [src Source/IronwakeTest/IronwakePlayerController.cpp:190] font Bold `72` [src Source/IronwakeTest/IronwakePlayerController.cpp:191].

| # | Button label | Handler | Behaviour | Source |
|---|---|---|---|---|
| 1 | `START VOYAGE` (pause: `RESUME`) | OnStart | title: `StartGame(PC->bPeaceful)`; pause: `TogglePause()` | `START VOYAGE` [src Source/IronwakeTest/IronwakePlayerController.cpp:221], `RESUME` [src Source/IronwakeTest/IronwakePlayerController.cpp:366] |
| 2 | `STORM AT SEA` | OnStormAtSea | start (or resume) then `Voyage->StartStormAtSea()` | `STORM AT SEA` [src Source/IronwakeTest/IronwakePlayerController.cpp:222] |
| 3 | `PEACEFUL` / `CAPTAIN` | OnToggleMode | toggles `bPeaceful`; button disabled when paused (mode chosen per voyage) | `PEACEFUL` [src Source/IronwakeTest/IronwakePlayerController.cpp:247], `ModeButton->SetIsEnabled(Mode == 0)` [src Source/IronwakeTest/IronwakePlayerController.cpp:367] |
| 4 | `PRESET` (`LOW` / `MEDIUM` / `HIGH`) | OnGraphics | settings tab cycles graphics quality | `PRESET` [src Source/IronwakeTest/IronwakePlayerController.cpp:256], `MEDIUM` [src Source/IronwakeTest/IronwakePlayerController.cpp:259] |
| 5 | `AUDIO` tab (8 sound class sliders) | OnAudio | 8 volume sliders replace the pre-P18 music button | `AUDIO` [src Source/IronwakeTest/IronwakePlayerController.cpp:232] |
| 6 | `RUDDER` (`HOLDS` / `AUTO-CENTRE`) | OnRudderMode | toggles saved RudderAutoCentre | `RUDDER` [src Source/IronwakeTest/IronwakePlayerController.cpp:249], `AUTO-CENTRE` [src Source/IronwakeTest/IronwakePlayerController.cpp:251] |
| 7 | `SHIPYARD` | OnYard | toggles the shipyard card (right side) | `SHIPYARD` [src Source/IronwakeTest/IronwakePlayerController.cpp:223] |
| 8 | `CONTROLS` | OnControls | closes other cards and calls `ToggleHelp()` (the P11 help panel) | `CONTROLS` [src Source/IronwakeTest/IronwakePlayerController.cpp:225], `PC->ToggleHelp()` [src Source/IronwakeTest/IronwakePlayerController.cpp:632] |
| 9 | `CREDITS` | OnCredits | toggles the credits card (bottom-right) | `CREDITS` [src Source/IronwakeTest/IronwakePlayerController.cpp:226] |
| 10 | `QUIT` | OnQuit | `QuitGame` | `QUIT` [src Source/IronwakeTest/IronwakePlayerController.cpp:227] |

Subtitle text: paused `Paused.  The sea waits for no one.` [src Source/IronwakeTest/IronwakePlayerController.cpp:371]; peaceful `Peaceful voyage: open sea, no enemy ships.` [src Source/IronwakeTest/IronwakePlayerController.cpp:371]; captain `Captain: an enemy ironclad hunts these waters.` [src Source/IronwakeTest/IronwakePlayerController.cpp:371]. Button style: brass colour `0.86f`, `0.74f`, `0.47f` [src Source/IronwakeTest/IronwakePlayerController.cpp:57].

Default mode: `bPeaceful` initial value `true` (the game opens in PEACEFUL) [src Source/IronwakeTest/IronwakePlayerController.h:108]. Pause: `ShowMenu(1)` calls `SetPause(true)`; `HideMenu` unpauses [src Source/IronwakeTest/IronwakePlayerController.cpp:646,656].

**Shipyard card** (title `SHIPYARD  -  GREYHAVEN` [src Source/IronwakeTest/IronwakePlayerController.cpp:376]): one button per roster ship, at most `i < IronwakeFleet::Roster().Num() && i < 6` [src Source/IronwakeTest/IronwakePlayerController.cpp:324] shown; each shows `[ IN COMMAND ]` / `[ LOCKED ]`, class, blurb and `Unlock:` text; LAUNCH button reads `LOCKED`, `CLOSE` (current ship) or `LAUNCH  %s` [src Source/IronwakeTest/IronwakePlayerController.cpp:406]. Launching a different ship saves the selection and reopens the map with options `iwrestart?iwpeaceful=1` [src Source/IronwakeTest/IronwakePlayerController.cpp:1096].

**Credits card**: static text list (`CreditsList`) with CC-BY attributions, shown bottom-right [src Source/IronwakeTest/IronwakePlayerController.cpp:47-78]; attribution content belongs to chapter 12.

**Dead text:** removed in P18 (the unused `ControlsList` text is gone; the help panel in 9.10 is the only controls list).

**Stale comments:** [src Source/IronwakeTest/IronwakePlayerController.h:136] says "H helmsman"; the code binds H to help and J to helmsman [src Source/IronwakeTest/IronwakePlayerController.cpp:433-435].

## 9.9 End screens

| Screen | Trigger | Buttons / keys | Source |
|---|---|---|---|
| `SHIP LOST` | player `bSinking` for `8` s | `RESTART`, `MAIN MENU`; Esc ignored | `Now - PlayerSinkSeen >= 8.f` [src Source/IronwakeTest/IronwakePlayerController.cpp:1289], `RESTART` [src Source/IronwakeTest/IronwakePlayerController.cpp:1096] |
| `VICTORY` | enemy sinking, `3` s after first seen | `CONTINUE` (Esc also continues) | `VictoryAt = Now + 3.f` [src Source/IronwakeTest/IronwakePlayerController.cpp:1297], `CONTINUE` [src Source/IronwakeTest/IronwakePlayerController.cpp:1200] |

Stats line `Shots %d   Hits %d   Hull %.0f%%` [src Source/IronwakeTest/IronwakePlayerController.cpp:1219]. Sub-lines: `She went down with her colours flying.` [src Source/IronwakeTest/IronwakePlayerController.cpp:1185] / `The enemy ironclad is going down.` [src Source/IronwakeTest/IronwakePlayerController.cpp:1185]. Z-order `AddToViewport(60)` [src Source/IronwakeTest/IronwakePlayerController.cpp:1058]. SHIP LOST card centred, VICTORY banner at anchor y `0.22` [src Source/IronwakeTest/IronwakePlayerController.cpp:1164]. No end screens during a tour/clip (`bTour`) or movie dumping [src Source/IronwakeTest/IronwakePlayerController.cpp:875-876]. RESTART reopens the current map with `iwrestart` (plus `iwpeaceful=1` if peaceful), MAIN MENU reopens it without options [src Source/IronwakeTest/IronwakePlayerController.cpp:848,859].

## 9.10 Help (CONTROLS) panel

Opened by H, F1 or the menu CONTROLS button; closed by H, F1 or Esc/Tab. Centred (anchor `0.5,0.5`), three columns separated by `44` px, backdrop alpha `0.90`, padding `40`/`28` [src Source/IronwakeTest/IronwakeP11.cpp:313-338]. Z-order `AddToViewport(60)` [src Source/IronwakeTest/IronwakePlayerController.cpp:1058]; widget is `HitTestInvisible`. While open the HUD draws nothing [src Source/IronwakeTest/IronwakeHUD.cpp:20]. Title `CONTROLS` Bold size `30` [src Source/IronwakeTest/IronwakeP11.cpp:324], footer `H / F1 / Esc: close` [src Source/IronwakeTest/IronwakeP11.cpp:386].

| Column | Section | Exact content (mono text) | Source |
|---|---|---|---|
| 1 | HELM | `W / S`: telegraph, one step per press (sailing ship: sail plan); `A / D` hold to turn the wheel; `Q / E` rudder midships; `J` helmsman steers to next point; `X` drop / weigh anchor; `G` fast sail x4 (peaceful) | `HELM` [src Source/IronwakeTest/IronwakeP11.cpp:340] |
| 1 | ENGINE & SAILS | `B` auxiliary steam (sailing ship); can't sail within `40` deg of wind; `R` damage control (fires); Left click fire when guns READY and ON | `ENGINE & SAILS` [src Source/IronwakeTest/IronwakeP11.cpp:348], `can't sail within 40 deg of wind` [src Source/IronwakeTest/IronwakeP11.cpp:350] |
| 2 | NAVIGATION | `M` chart: left-click add waypoint, right-click remove, Enter start, Backspace clear, L loop route; `P` autopilot off / hold course / waypoints / tour (hold: A / D nudge course `5` deg); `N` next point of interest | `NAVIGATION` [src Source/IronwakeTest/IronwakeP11.cpp:353], `nudge course 5 deg` [src Source/IronwakeTest/IronwakeP11.cpp:359] |
| 2 | CAMERA | Mouse orbit camera / aim the guns; Wheel camera zoom; Shift binoculars (wheel = zoom), Shift or right-click exits; `C` / Mid hold free look; `F` photo mode: WASD / QE fly, wheel FOV, T slow-mo, Shift fast | `CAMERA` [src Source/IronwakeTest/IronwakeP11.cpp:364] |
| 3 | VIEW & WORLD | `T` clock speed x1 / x10 / pause; `Shift+T` jump forward 3 hours (also `]`); `L` next weather; `K` weather drift on / off; `H / F1` this help; `Tab / Esc` pause menu | `VIEW & WORLD` [src Source/IronwakeTest/IronwakeP11.cpp:374] |
| 3 | GAMEPAD | D-pad telegraph / rudder; R trigger fire, R stick look; L trigger exit binoculars | `GAMEPAD` [src Source/IronwakeTest/IronwakeP11.cpp:381] |

Check against bindings: every key in the panel matches 9.2-9.4 (verified against the tables above). Not listed in the panel but bound: Up/Down/Left/Right arrow keys (telegraph/rudder aliases), RightShift (scope), Gamepad_Special_Right (pause), `Right bracket` (listed as `]` inside the Shift+T line).

## 9.11 In-game HUD (Canvas, `AIronwakeHUD::DrawHUD`)

UI scale `S = Clamp(ClipY/1080, min, max)` with `0.75f, 2.5f` [src Source/IronwakeTest/IronwakeHUD.cpp:23] applied to every offset and font scale. Font: `GEngine->GetMediumFont()`.

Suppression rules (nothing drawn): no ship pawn; view target is not the ship (cinematic demo cameras) [src Source/IronwakeTest/IronwakeHUD.cpp:17]; menu open, or end screen open with player sinking [src Source/IronwakeTest/IronwakeHUD.cpp:18]; help open [src Source/IronwakeTest/IronwakeHUD.cpp:20].

### Ship block (bottom-left)

Anchor `X = 28*S`, base `Canvas->ClipY - 205.f * S` [src Source/IronwakeTest/IronwakeHUD.cpp:83]; peaceful voyage (peaceful and no enemy) adds `54*S` (no HULL/GUNS rows) [src Source/IronwakeTest/IronwakeHUD.cpp:74]; extra rows `3` on a sailing ship else `1` [src Source/IronwakeTest/IronwakeHUD.cpp:75], each `27*S` tall. Backdrop rect alpha `0.55`, width `430*S` sailing / `340*S` otherwise [src Source/IronwakeTest/IronwakeHUD.cpp:77]. Label column at X, value column at X + `120*S`. Font scale `1.1*S`.

| Row | Shown when | Format / data | Colour | Source |
|---|---|---|---|---|
| `SHIP` | always | `GetShipName()` | brass | `Line(TEXT("SHIP")` [src Source/IronwakeTest/IronwakeHUD.cpp:100] |
| `WIND` | sailing ship | `from <compass> <deg>  <kn> kn   app <kn> kn <angle>S/P` (true wind from `IronwakeFleet::WindFromYaw`, apparent from ship) | brass | `Line(TEXT("WIND")` [src Source/IronwakeTest/IronwakeHUD.cpp:104] |
| `POINT` | sailing ship | point of sail, `\|TWA\|` deg, `stbd`/`port`, `STEAM ON` / `steam off (B)` | red if `\|TWA\| < 40`, else brass | `Line(TEXT("POINT")` [src Source/IronwakeTest/IronwakeHUD.cpp:107], `FMath::Abs(Twa) < 40.f` [src Source/IronwakeTest/IronwakeHUD.cpp:108] |
| `SAIL` / `TELEGRAPH` | always (SAIL on sailing ship) | `GetTelegraphName()` | brass | `TEXT("SAIL")` [src Source/IronwakeTest/IronwakeHUD.cpp:110] |
| `RUDDER` | always | `<name>  (+N deg)` from `RudderAngleDeg` | brass | `Line(TEXT("RUDDER")` [src Source/IronwakeTest/IronwakeHUD.cpp:140] |
| `SPEED` | always | `%.1f kn` | brass | `Line(TEXT("SPEED")` [src Source/IronwakeTest/IronwakeHUD.cpp:141] |
| `HEADING` | always | `%03.0f` degrees | brass | `Line(TEXT("HEADING")` [src Source/IronwakeTest/IronwakeHUD.cpp:142] |
| `HULL` | not peaceful | `HP%  fires N` | red if HP fraction < `0.4` | `Line(TEXT("HULL")` [src Source/IronwakeTest/IronwakeHUD.cpp:146] |
| `GUNS` | not peaceful | `READY`/`LOADING`, `ON`/`TRAINING`, aim range km | brass if ready else dim | `Line(TEXT("GUNS")` [src Source/IronwakeTest/IronwakeHUD.cpp:148] |

**CONFLICT (in-irons threshold):** three different numbers describe the no-go zone. HUD wind-dial red arc uses `35` deg half-angle [src Source/IronwakeTest/IronwakeHUD.cpp:103]; the POINT row turns red below `40` deg [src Source/IronwakeTest/IronwakeHUD.cpp:92]; `PointOfSail()` returns `IN IRONS` below `35` deg [src Source/IronwakeTest/IronwakeFleet.cpp:178]; help text says "within `40` deg" [src Source/IronwakeTest/IronwakeP11.cpp:350]. The value that gates the sailing physics is not in these files (see chapter 2 / `IronwakeShipFleet.cpp`); measure by logging thrust vs TWA near 35-40 deg.

### Wind dial (sailing ship, right of block)

Centre `X + 470*S`, `ClipY - 150*S`, radius `R = 62.f * S` [src Source/IronwakeTest/IronwakeHUD.cpp:114]; circle drawn as `48` segments of `7.5` deg [src Source/IronwakeTest/IronwakeHUD.cpp:100-102]; segments within `35` deg of the true-wind angle are red and thick (in irons), others dim; ship silhouette pointing up in brass; blue arrow = true wind (length `0.55`), grey arrow = apparent wind (length `0.38`); caption `WIND  true / app` [src Source/IronwakeTest/IronwakeHUD.cpp:138-140].

### Turret panel (right edge, mid-height; non-peaceful)

Centre `PX = ClipX - 170*S`, `PY = ClipY * 0.50`, backdrop `300.f * S, 150.f * S` [src Source/IronwakeTest/IronwakeHUD.cpp:154]. Draws ship hull outline (bow up), each turret's `150` deg arc (`TurretArcDeg` [src Source/IronwakeTest/IronwakeHUD.cpp:165] = `150` [src Source/IronwakeTest/IronwakeShipPawn.h:171]), current bearing line, a camera-look line, and per-turret status: `ON` (green, ready and on target), `TRAIN` (brass, ready not on target), or reload seconds `%.1fs` (red) [src Source/IronwakeTest/IronwakeHUD.cpp:155-159]. Labels `FWD`, `AFT`.

### Aiming aids (non-peaceful)

| Element | Description | Source |
|---|---|---|
| Crosshair | gap `5*S`, arm `14*S`, brass, three arms (no top arm) at screen centre | `const float L = 14.f * S, Gp = 5.f * S` [src Source/IronwakeTest/IronwakeHUD.cpp:188] |
| Aim ring | small grey ring (radius `6*S`) at `AimPoint` where the guns actually point | `Ring(Ship->AimPoint, 6.f * S` [src Source/IronwakeTest/IronwakeHUD.cpp:232] |
| Enemy marker | text `IRONCLAD  %.1f km  %.0f%%` or `SINKING` in red above the enemy plus a chevron | `IRONCLAD  %.1f km  %.0f%%` [src Source/IronwakeTest/IronwakeHUD.cpp:214] |
| Lead indicator | orange ring (radius `11*S`) with `LEAD  %.1fs` flight time, plus a faint line from the enemy; drawn if `bLeadIndicator && bHasLead` (default on) | `LEAD  %.1fs` [src Source/IronwakeTest/IronwakeHUD.cpp:226] |

### Binocular / gun-sight overlay (Shift)

Replaces the whole HUD while scoped [src Source/IronwakeTest/IronwakeHUD.cpp:26-65]: black mask outside a circle of radius `ClipY * 0.46f` [src Source/IronwakeTest/IronwakeHUD.cpp:29], mask built from `400` strips [src Source/IronwakeTest/IronwakeHUD.cpp:34], reticle arms with `8` mil ticks per axis (`k <= 8` [src Source/IronwakeTest/IronwakeHUD.cpp:49]), magnification text `x%.1f` = `70 / FOV` (`70.f / FMath::Max` [src Source/IronwakeTest/IronwakeHUD.cpp:58]), `RANGE  %.2f km` (or `RANGE  ---`), `BRG %03.0f` bearing, bottom hint `SHIFT / RIGHT-CLICK: exit     WHEEL: magnification` [src Source/IronwakeTest/IronwakeHUD.cpp:79]. Colours: reticle dark `0.05` alpha `0.9`, centre cross brass.

## 9.12 Voyage HUD (`AIronwakeVoyage::DrawVoyageHUD`, drawn from the HUD at line 127)

Called after the ship block [src Source/IronwakeTest/IronwakeHUD.cpp:127]; if the chart is open it draws the chart instead (plus any toast) [src Source/IronwakeTest/IronwakeVoyage.cpp:1098].

| Element | Description | Source |
|---|---|---|
| Compass tape | top centre, width `640*S`, height `34*S`, top `18*S`; shows +/-`60` deg around the camera yaw (`120` deg across), tick every `5` deg, labels at majors; POI diamonds (next POI highlighted, visited dim); ship-heading caret; heading text right of the tape | `TapeW = 640.f * S, TapeH = 34.f * S, TY = 18.f * S` [src Source/IronwakeTest/IronwakeVoyage.cpp:1212], `PxPerDeg = TapeW / 120.f` [src Source/IronwakeTest/IronwakeVoyage.cpp:1214] |
| NEXT line | `NEXT  <name>   <km> km   brg <deg>  (<dead ahead\|N deg to port/starboard>)` plus `[HELMSMAN]` when J is on | `NEXT  %s   %.1f km   brg %03.0f` [src Source/IronwakeTest/IronwakeVoyage.cpp:1271] |
| AUTOPILOT chip | `AUTOPILOT: <label>`; labels `HOLD COURSE %03.0f`, `WAYPOINTS n/m` (+` LOOP`), tour, `WAYPOINTS (planning: M chart)`; teal when on | `AUTOPILOT: %s` [src Source/IronwakeTest/IronwakeVoyage.cpp:1279], `HOLD COURSE %03.0f` [src Source/IronwakeTest/IronwakeAutopilot.cpp:195] |
| Status chips | `ANCHORED`, `FAST SAIL x4`, and (living world) clock + weather text e.g. `09:30 morning   Fair   sea slight`; extra debug (`[preset]`, `SEA: ... m`) only with `-iwdebug` | `ANCHORED   ` [src Source/IronwakeTest/IronwakeVoyage.cpp:1291], `FAST SAIL x4   ` [src Source/IronwakeTest/IronwakeVoyage.cpp:1292], `iwdebug` [src Source/IronwakeTest/IronwakeVoyage.cpp:1288] |
| Minimap | bottom right, side `230*S`, +/-`4` km, north up, merchants as grey squares, airship `~`, POIs as squares | `MS = 230.f * S` [src Source/IronwakeTest/IronwakeVoyage.cpp:1328] |
| Hint line | bottom-left under the ship block: `H: help   M: chart   P: autopilot   Esc: menu` | `H: help   M: chart   P: autopilot   Esc: menu` [src Source/IronwakeTest/IronwakeVoyage.cpp:1388] |
| Toast | centred at `0.30` of screen height, backdrop alpha `0.55`; default duration `4` s | `ClipY * 0.30f` [src Source/IronwakeTest/IronwakeVoyage.cpp:1394], `Seconds = 4.f` [src Source/IronwakeTest/IronwakeVoyage.h:128] |

Clock strings (`ClockString`): parts of day `small hours` (<`4.5`), `dawn` (<`7`), `morning` (<`11`), `noon` (<`13.5`), `afternoon` (<`15.5`), `dusk` (<`18`), `evening` (<`21`), `night`; speed suffix ``, ` x10`, ` (paused)` [src Source/IronwakeTest/IronwakeP11.cpp:67-70]. Weather names: Clear, Fair, Overcast, Mist, Rain, Squall; sea words calm (<`0.3`), slight (<`0.8`), moderate (<`1.5`), rough (<`2.4`), very rough [src Source/IronwakeTest/IronwakeP11.cpp:43,76].

Toasts raised by gameplay (examples with source): `COLLISION: <tag> (<kn> kn)` for `2.5` s [src Source/IronwakeTest/IronwakeShipCollision.cpp:79]; `AGROUND - engines astern to back off` [src Source/IronwakeTest/IronwakeShipCollision.cpp:133]; `Weather: %s` [src Source/IronwakeTest/IronwakeVoyage.cpp:733]; `Weather drifts on its own` / `Weather held (L changes it)` [src Source/IronwakeTest/IronwakeVoyage.cpp:741]; `Clock x1|x10|paused` and `Time: <clock>` [src Source/IronwakeTest/IronwakeP11.cpp:100,108].

## 9.13 Saved settings

Store: `GGameUserSettingsIni`, section `Ironwake` [src Source/IronwakeTest/IronwakeSettings.cpp:10], flushed on every write [src Source/IronwakeTest/IronwakeSettings.cpp:28-36]. File path on the Mac (sandboxed packaged app, observed): `~/Library/Containers/io.github.charles-forsyth.ironwake/Data/Library/Application Support/Epic/IronwakeTest/Saved/Config/Mac/GameUserSettings.ini` (another build uses bundle id `com.YourCompany.IronwakeTest`, see chapter 13). Observed sample content: `[Ironwake]` (macOS container `GameUserSettings.ini`) with `PoiMask=7`, `Arrivals=3`, `Ship=frigate`, `Graphics=2` (macOS container `GameUserSettings.ini`).

| Key | Type | Default | Written by | Read by | Source |
|---|---|---|---|---|---|
| `Graphics` | int `0..2` (LOW/MEDIUM/HIGH) | `1` (MEDIUM); `-iwgfx=N` on the command line overrides the saved value for that run | `SetGraphics` (menu, `IWGfx`) | `GetGraphics`, cached after first read | `int32 L = 1` [src Source/IronwakeTest/IronwakeSettings.cpp:51], `iwgfx=` [src Source/IronwakeTest/IronwakeSettings.cpp:54], `FMath::Clamp(L, 0, 2)` [src Source/IronwakeTest/IronwakeSettings.cpp:55] |
| `Music` | bool | `false` (off by default; Chuck's standing rule) | `SetMusic` | `GetMusic` | `IronwakeAudio::MusicOn()` [src Source/IronwakeTest/IronwakeSettings.cpp:98], `Standing rule` [src build/BRIEF.md:148] |
| `RudderAutoCentre` | bool | `false` (rudder holds) | `SetRudderAutoCentre` | `OnRudderRelease`, `ApplyShipForces`, menu label | `ReadBool(TEXT("RudderAutoCentre"), false)` [src Source/IronwakeTest/IronwakeSettings.cpp:101] |
| `Ship` | string id | `frigate`; falls back to frigate if unknown or locked; `-iwship=<id>` overrides | `SetSelected` (shipyard LAUNCH) | `GetSelected` | `FString S = TEXT("frigate")` [src Source/IronwakeTest/IronwakeFleet.cpp:111], `iwship=` [src Source/IronwakeTest/IronwakeFleet.cpp:110] |
| `Arrivals` | int | `0` | `WriteInt(Arrivals, +1)` when a POI is reached | unlock logic | `GetArrivals() { return ReadInt(TEXT("Arrivals"), 0)` [src Source/IronwakeTest/IronwakeFleet.cpp:125] |
| `PoiMask` | int bitmask (bit i = POI i visited, i < `31`) | `0` | on POI arrival | unlock logic / minimap visited state | `GetVisitedMask() { return ReadInt(TEXT("PoiMask"), 0)` [src Source/IronwakeTest/IronwakeFleet.cpp:126] |

`-iwunlockall` makes every roster ship unlocked [src Source/IronwakeTest/IronwakeFleet.cpp:110]. The frigate and cutter are always unlocked; others are earned (chapter 2). The saved Graphics level is applied at game start by `ApplyGraphics()` in `StartPlay` [src Source/IronwakeTest/IronwakeGameMode.cpp:69].

## 9.14 Graphics presets

`ApplyGraphics()` writes console variables with `ECVF_SetByCode` (an unknown cvar logs `IWGFX unknown cvar`). Values per level [src Source/IronwakeTest/IronwakeSettings.cpp:71-91]. Default MEDIUM targets 60 fps class on the M5 at 1080p; HIGH is "cinematic clouds, ~30 fps class" [src Source/IronwakeTest/IronwakeSettings.h:6].

| CVar | LOW (0) | MEDIUM (1) | HIGH (2) | Source |
|---|---|---|---|---|
| `t.MaxFPS` | `60` | `60` | `60` | `t.MaxFPS` [src Source/IronwakeTest/IronwakeSettings.cpp:76] |
| `r.VSync` | `1` | `1` | `1` | `r.VSync` [src Source/IronwakeTest/IronwakeSettings.cpp:77] |
| `r.DynamicRes.OperationMode` | `2` | `2` | `2` | `r.DynamicRes.OperationMode` [src Source/IronwakeTest/IronwakeSettings.cpp:78] |
| `r.DynamicRes.FrameTimeBudget` (ms) | `15.0` | `15.0` | `33.3` | `r.DynamicRes.FrameTimeBudget` [src Source/IronwakeTest/IronwakeSettings.cpp:79] |
| `r.DynamicRes.MinScreenPercentage` | `45` | `50` | `50` | `r.DynamicRes.MinScreenPercentage` [src Source/IronwakeTest/IronwakeSettings.cpp:80] |
| `r.DynamicRes.MaxScreenPercentage` | `70` | `85` | `100` | `r.DynamicRes.MaxScreenPercentage` [src Source/IronwakeTest/IronwakeSettings.cpp:81] |
| `r.VolumetricCloud.SampleMinCount` | `2` | `2` | `2` | `r.VolumetricCloud.SampleMinCount` [src Source/IronwakeTest/IronwakeSettings.cpp:82] |
| `r.VolumetricCloud.ViewRaySampleMaxCount` | `12` | `16` | `64` | `r.VolumetricCloud.ViewRaySampleMaxCount` [src Source/IronwakeTest/IronwakeSettings.cpp:83] |
| `r.VolumetricCloud.ReflectionRaySampleMaxCount` | `8` | `8` | `16` | `r.VolumetricCloud.ReflectionRaySampleMaxCount` [src Source/IronwakeTest/IronwakeSettings.cpp:84] |
| `r.VolumetricCloud.SkyAO` | `0` | `0` | `1` | `r.VolumetricCloud.SkyAO` [src Source/IronwakeTest/IronwakeSettings.cpp:85] |
| `r.VolumetricRenderTarget.Mode` | `2` | `2` | `0` | `r.VolumetricRenderTarget.Mode` [src Source/IronwakeTest/IronwakeSettings.cpp:86] |
| `r.Water.WaterMesh.TessFactorBias` | `-2` | `-1` | `0` | `r.Water.WaterMesh.TessFactorBias` [src Source/IronwakeTest/IronwakeSettings.cpp:87] |
| `r.DynamicGlobalIlluminationMethod` | `0` (none + sky light) | `1` (Lumen) | `1` | `r.DynamicGlobalIlluminationMethod` [src Source/IronwakeTest/IronwakeSettings.cpp:89] |
| `r.ReflectionMethod` | `2` (SSR) | `1` (Lumen) | `1` | `r.ReflectionMethod` [src Source/IronwakeTest/IronwakeSettings.cpp:90] |
| `r.Shadow.Virtual.Enable` | `0` | `1` | `1` | `r.Shadow.Virtual.Enable` [src Source/IronwakeTest/IronwakeSettings.cpp:91] |

Notes: every level has a `60` fps cap with vsync; dynamic resolution (TSR upscale) holds the GPU frame budget; the volumetric cloud deck measured `40` of `57` ms GPU at 1600x900 before the presets (comment cites logs/H1.md) [src Source/IronwakeTest/IronwakeSettings.cpp:75]. `FrameTimeBudget` is `33.3` only on HIGH, so HIGH allows a ~30 fps frame time; the cloud `SampleMinCount` line is written as `L == 2 ? "2" : "2"` (identical branches, no effect) [src Source/IronwakeTest/IronwakeSettings.cpp:82]. Per-preset measured fps is chapter 11.

## 9.15 Gaps / unknown

- Whether `bEnableMouseSmoothing=True` alters Mouse2D deltas (9.1): measure by logging raw `Mouse2D` per frame with the flag on/off.
- Which no-go angle (`35` or `40` deg) the sailing physics uses (9.11): measure thrust vs true-wind angle on the frigate between `30` and `45` deg.
- No gamepad route to scope, repair, steam, centre rudder, photo, help, chart, autopilot, weather, anchor (9.6): design gap, not a bug in code.
- `bEnableMotionControls=True` has no consumer in `Source/` (not grepped beyond controller/pawn files).
- Exact on-screen layout at 1280x720 / 2560x1440 is not verifiable from source alone (P11 verified help at three resolutions by capture, see logs/P11.md).
- The title menu's `STORM AT SEA` voyage behaviour lives in chapter 8/3 (`StartStormAtSea`).

## 9.x P16 CAMERA MOTION option (2026-11-11)

- Pause/title menu button `CAMERA MOTION: OFF / LOW / MEDIUM` cycles the setting; saved as `[Ironwake] CameraMotion` in GameUserSettings (default LOW, `-iwcammotion=N` overrides) [src Source/IronwakeTest/IronwakeSettings.cpp:GetCameraMotion].
- LOW: camera follows low-passed ship pitch/roll/heave, gain `0.15`, tau `2.5` s, caps roll `1.5` deg, pitch `1.0` deg, heave `0.3` m. MEDIUM: gain `0.30`, tau `2.0` s, caps `3`/`2` deg, `0.6` m. OFF: fully steady [src Source/IronwakeTest/IronwakeShipPawn.cpp:CalcCamera].
- Binoculars: zero sway. Photo mode: own camera, unaffected. Impact shake unchanged; no FOV or blur coupling.
- Help panel CAMERA section lists `CAMERA MOTION OFF / LOW / MEDIUM` [src Source/IronwakeTest/IronwakeP11.cpp:370]; the menu CONTROLS text has the same line [src Source/IronwakeTest/IronwakePlayerController.cpp:92].
- Measured (ironclad, sea 2.5, LOW): hull roll p2p `7.96` deg, camera roll p2p `0.48` deg, camera pitch sway p2p `0.48` deg, camera Z p2p `0.33` m [src logs/P16.md].

## 9.x P18 Menu that fits every screen + AUDIO tab (2026-11-11)

**Layout and scaling.** The menu root is a `UScaleBox` (`EStretch::UserSpecified`, ignores the engine DPI curve). Each frame the widget reads the viewport and sets `scale = clamp(min(w/1920, h/1080), 0.5, 1.5)`; the design area is `viewport / scale`, the card (width `min(900, area.x - 80)`, max height `area.y - 100`) sits at (60, 50) inside it, and a `UScrollBox` inside the card is the safety net when a page is taller than the card [src Source/IronwakeTest/IronwakePlayerController.cpp:FitToViewport]. Z-order stays `AddToViewport(50)`.

**Pages.** MAIN (START VOYAGE / RESUME, STORM AT SEA, SHIPYARD, SETTINGS, CONTROLS, CREDITS, QUIT), SETTINGS (tab row GAMEPLAY | GRAPHICS | VIEW | AUDIO + BACK), CREDITS, SHIPYARD. CONTROLS still opens the P11 help panel. The pause menu is the same widget with `Mode = 1` (RESUME instead of START; MODE buttons disabled; the shipyard and settings work in pause).

| Tab | Controls | Saved key |
|---|---|---|
| GAMEPLAY | MODE: PEACEFUL / CAPTAIN (title only); RUDDER: HOLDS / AUTO-CENTRE | `RudderAutoCentre` |
| GRAPHICS | PRESET: LOW / MEDIUM / HIGH (one button each, selected one tinted) | `Graphics` |
| VIEW | CAMERA MOTION: OFF / LOW / MEDIUM | `CameraMotion` |
| AUDIO | eight sliders + percent + TEST, RESET AUDIO (see chapter 10, 10.12) | `Vol_<Class>` |

**Back / navigation.** Esc, Tab (pause toggle on the main page) and gamepad B (`Gamepad_FaceButton_Right`) go back one page (`HandleBack`): help panel closes first, then SETTINGS/CREDITS/SHIPYARD -> MAIN, then on the main page Esc pauses/resumes as before (title main page: nothing, Start or Quit). Up/Down/Left/Right (arrow keys, D-pad, left stick) use Slate navigation; focus is tinted. Slider rows: `RequiresControllerLock = false`, so Left/Right changes the slider directly; Down from the AUDIO tab button lands on the MASTER slider (explicit navigation rule); Enter / gamepad A on a focused slider presses that row's TEST; PageUp/PageDown or LB/RB switch settings tabs.

**Verified (silent, offscreen `-game`, `build/tools/p18sizes.sh`, `logs/P18/`):** `CheckLayout` walks every visible button, slider and text widget, converts its rect to viewport pixels and logs `IWP18 LAYOUT ... outside=N overlaps=N clippedByCard=N PASS|FAIL`; it passed on all nine pages (title main, four settings tabs, credits, shipyard, pause main, pause AUDIO) at 1280x720, 1366x768, 1440x900, 1600x900, 1800x754, 1024x768, 800x600 and 1500x1125 (macOS caps a window at the 3024x1964 Retina screen's logical area, so 1920x1080, 2560x1440 and 3440x1440 windows cannot be opened here; windows of the same aspect give the same design area).

## 9.x P29 Harbourmaster Market, Captain's Logbook & Arrival Verb (`UIronwakeDockWidget`, 2026-11-13)
- **Arrival & Logbook Keys:** slowing `< 2.5` kn inside any port's `MoorRadiusM` or near a landmark/wreck shows `[Enter] Make Fast — Port of <Name>` / `Salvage Wreck` / `Log Discovery`; pressing `Enter` calls `TryArrivalAction()` [src Source/IronwakeTest/IronwakeVoyage.h:351], chimes two ship's bells (`SC_Ship`), switches `ViewTarget` to `DockCam`, and opens `UIronwakeDockWidget` (`AddToViewport(48)`, `UScaleBox` + `UScrollBox`). Pressing `O` opens the `CAPTAIN'S LOGBOOK` tab directly underway or in port (`Vy->OpenDockModal` [src Source/IronwakeTest/IronwakePlayerController.cpp:822]).
- **Three Tabs & Persistence:** `1. CARGO & MAIL` trades 8 commodities (`Coal`, `Pig Iron`, `Salt Fish`, `Naval Timber`, `Brasswork`, `Sailcloth`, `Whale Oil`, `Mail Packet`) within per-hull hold capacity (`18..220` tons), persisting `SilverCrowns`, `Hold_<id>`, `CargoDeliveredTons`, `NetTradeProfit`, `DockedPortMask`, and `NauticalMilesx10` in `[Ironwake]` `GameUserSettings.ini` (`10 / 10` (build report `P29.md`)); `2. CAPTAIN'S LOGBOOK` shows career milestones, rank, and timestamped entries (`9.8 / 10` (build report `P29.md`)); `3. SHIPYARD & REPAIRS` repairs hull/fires and swaps ships (`Ironclad` unlockable at `1200` Crowns or `10` arrivals). Verified `outside=0 overlaps=0 clipped=0 PASS` (build report `P29.md`) across `1280x720` and `1920x1080`.
