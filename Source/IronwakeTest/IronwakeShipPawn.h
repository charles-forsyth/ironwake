#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "IronwakeHydro.h"
#include "IronwakeCollision.h"
#include "IronwakeShipPawn.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class USpringArmComponent;
class UCameraComponent;
class UBuoyancyComponent;
class UInputAction;
class UInputMappingContext;
class UMaterialInstanceDynamic;
class UNiagaraComponent;
class UAudioComponent;
class USoundBase;
class AIronwakeShell;
class USoundAttenuation;

/**
 * Heavy steam ironclad, player-driven.
 * Root is a physics box (simple collision, mass override); the Hunyuan hull mesh rides on it.
 * Water plugin BuoyancyComponent pontoons give heave/pitch/roll on the Gerstner swell.
 * Propulsion/rudder are our own forces: engine telegraph (W/S steps that hold), rudder steps (A/D),
 * quadratic hull drag, strong keel lateral drag, yaw-rate model with a wide turning circle.
 * All input goes through Enhanced Input actions created in code (no .uasset input assets).
 */
UCLASS(Blueprintable)
class IRONWAKETEST_API AIronwakeShipPawn : public APawn
{
	GENERATED_BODY()

public:
	AIronwakeShipPawn();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PawnClientRestart() override;
	/** H1: World-of-Warships orbit camera computed after physics (smoothed pivot, level horizon, no hull pitch/roll). */
	virtual void CalcCamera(float DeltaTime, struct FMinimalViewInfo& OutResult) override;

	// --- Components ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship")
	TObjectPtr<UBoxComponent> HullBody;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship")
	TObjectPtr<UStaticMeshComponent> HullMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship")
	TObjectPtr<USceneComponent> TurretFore;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship")
	TObjectPtr<USceneComponent> TurretAft;

	// R1: library hull moving parts (props spin with speed, rudder follows the wheel).
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship")
	TObjectPtr<UStaticMeshComponent> PropPort;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship")
	TObjectPtr<UStaticMeshComponent> PropStbd;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship")
	TObjectPtr<UStaticMeshComponent> RudderMesh;
	UPROPERTY() TObjectPtr<UStaticMesh> MeshPlayerHull;   // HMS Devastation (martyn169, CC-BY)
	UPROPERTY() TObjectPtr<UStaticMesh> MeshEnemyHull;    // RSS Kracken (martyn169, CC-BY)

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship")
	TObjectPtr<USpringArmComponent> CameraArm;

	/** Cosmetic foam: stern wake strip and two bow-wave strips (absolute-rotation planes at the waterline). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship|FX")
	TObjectPtr<UStaticMeshComponent> WakeStern;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship|FX")
	TObjectPtr<UStaticMeshComponent> WakeBowL;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship|FX")
	TObjectPtr<UStaticMeshComponent> WakeBowR;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ship")
	TObjectPtr<UBuoyancyComponent> Buoyancy;

	// --- Tuning (editable in BP_PlayerShip) ---
	/** Telegraph target speeds in knots, index 0 = Full astern ... 6 = Flank. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Engine")
	TArray<float> TelegraphKnots;

	/** Design speed compression (DESIGN.md: 1.5x in combat, 3x open water). Sim speed = knots * this. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Engine")
	float SpeedCompression = 1.5f;

	/** H1: time constant (s) of the final approach to the ordered speed (below the thrust limit). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Engine")
	float SpeedTimeConstant = 3.f;

	/** H1: thrust-limited acceleration, m/s^2 of sim speed (ironclad: Stop -> Half Ahead in ~10 s, Flank ~20 s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Engine")
	float MaxAccelMS2 = 0.75f;

	/** How fast the engine room answers the telegraph, knots of order per second. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Engine")
	float EngineResponse = 6.f;

	/** Turning circle radius at hard rudder, metres. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Rudder")
	float MinTurnRadiusM = 330.f; // P8 (Chuck OK 21:25): ironclad 280 -> 330 (circle ~210 s); the ironclad takes this pawn default

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Rudder")
	float MaxRudderDeg = 35.f;

	/** Steering gear slew rate, degrees per second. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Rudder")
	float RudderSlewDegPerSec = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Rudder")
	float YawResponse = 2.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Hull")
	float MassKg = 8.0e6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Hull")
	float LateralDrag = 3.5f;

	/** Current camera distance in cm (set from ZoomStepsL * HullLengthM). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Camera")
	float ArmLength = 18200.f;

	/** H1: hull length in metres; the camera rig scales arm, height and pivot from it (S2 reuses per hull). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Camera")
	float HullLengthM = 140.f;

	/** Wheel zoom steps as multiples of the hull length (close = just behind the funnels ... far = ship + wake small). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Camera")
	TArray<float> ZoomStepsL;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Camera")
	int32 ZoomIndex = 6;
	static constexpr int32 DefaultZoomIndex = 6; // 1.5 L (Chuck: closer steps added below it)
	/** 0 at normal zoom .. 1 at the closest steps: pivot drops to the deck, pitch range opens. */
	float GetCloseZoomFrac() const;

	/** Default view pitch: horizon ~ upper third, whole ship in the lower half. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Camera")
	float DefaultCamPitch = -6.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Camera")
	float CamPitchMin = -35.f;

	/** Slightly above level so the guns can still range out to the horizon (10 km). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Camera")
	float CamPitchMax = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Camera")
	float ScopeFov = 12.f;

	// --- H1 camera state ---
	bool IsScoped() const { return bScope; }
	void SetScope(bool bOn);
	float GetViewFov() const { return CurFov; }
	float ScopeRangeM = -1.f;     // rangefinder (line trace / sea plane) while scoped
	FString ScopeRangeWhat;
	bool bFreeLook = false;
	/** Turret bearing relative to the bow (deg) for the HUD arc panel. */
	float GetTurretBearingRel(int32 i) const { return TurretBaseYaw[i] + TurretOffset[i]; }
	float GetTurretRestYaw(int32 i) const { return TurretBaseYaw[i]; }
	static constexpr float TurretArcDeg = 150.f;
	/** Music on/off from IronwakeSettings (off by default). */
	void ApplyMusicSetting();
	UFUNCTION(Exec) void IWZoom(int32 Index);
	UFUNCTION(Exec) void IWScope(int32 On);
	/** H1 handling test driver (also -iwh1 on the command line / Saved/iw_h1.txt). */
	UFUNCTION(Exec) void IWH1();

	// --- State (read by the HUD) ---
	UPROPERTY(BlueprintReadOnly, Category = "Ship|State")
	int32 TelegraphIndex = 2; // Stop

	UPROPERTY(BlueprintReadOnly, Category = "Ship|State")
	int32 RudderStep = 0; // -3 hard port .. +3 hard starboard

	UPROPERTY(BlueprintReadOnly, Category = "Ship|State")
	float RudderAngleDeg = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Ship|State")
	float EngineOrderKnots = 0.f;

	UFUNCTION(BlueprintPure, Category = "Ship")
	FString GetTelegraphName() const;

	UFUNCTION(BlueprintPure, Category = "Ship")
	FString GetRudderName() const;

	/** Speed through water in nominal knots (sim speed / SpeedCompression). */
	UFUNCTION(BlueprintPure, Category = "Ship")
	float GetSpeedKnots() const;

	UFUNCTION(BlueprintPure, Category = "Ship")
	float GetHeadingDeg() const;

	UFUNCTION(BlueprintCallable, Category = "Ship")
	void StepTelegraph(int32 Delta);

	UFUNCTION(BlueprintCallable, Category = "Ship")
	void StepRudder(int32 Delta);

	UFUNCTION(BlueprintCallable, Category = "Ship")
	void CenterRudder();

	// Console helpers (work in PIE console)
	UFUNCTION(Exec)
	void IWTelegraph(int32 Index);

	UFUNCTION(Exec)
	void IWRudder(int32 Step);

	UFUNCTION(Exec)
	void IWDemo();

	// ================= Card C: gunnery, damage, NPC AI, audio =================
	/** True for the AI-driven enemy ironclad (spawned by the GameMode). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Combat")
	bool bIsNPC = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Combat")
	float MaxHP = 100.f;

	UPROPERTY(BlueprintReadOnly, Category = "Ship|Combat")
	float HP = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Combat")
	float MuzzleVelocity = 60000.f; // cm/s (600 m/s): 4 km in ~7 s, 10 km in ~17 s

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Combat")
	float MaxGunRange = 1000000.f; // 10 km

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Combat")
	float TurretTraverseDegPerSec = 7.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Combat")
	float ReloadSeconds = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Combat")
	bool bLeadIndicator = true; // DESIGN.md: on by default

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Combat")
	bool bAimAssist = false; // DESIGN.md: optional

	bool bSinking = false;
	FTransform SinkStart;
	float SinkTime = 0.f;
	int32 NumFires() const { return Fires.Num(); }
	FString AIStateName;
	FVector AimPoint = FVector::ZeroVector;
	bool bHasLead = false;
	FVector LeadPoint = FVector::ZeroVector;
	float AimRange = 0.f;
	float LastFlightTime = 0.f;
	float TurretReload[2] = { 0.f, 0.f };
	bool TurretOnTarget[2] = { false, false };
	float RepairCooldown = 0.f;

	AIronwakeShipPawn* FindEnemy() const;
	void ApplyShellHit(const FVector& WorldPos, float Damage, AIronwakeShipPawn* From);
	void OnOwnShellImpact(bool bHit, const FVector& Where, AIronwakeShipPawn* Victim);
	FVector GetHullVelocity() const;

	/** Ballistic launch velocity (low arc) from From to To including own-ship velocity carry. */
	static bool SolveLaunch(const FVector& From, const FVector& To, float V, const FVector& Carry, FVector& OutVel, float& OutT);

	UFUNCTION(Exec) void IWFire();
	UFUNCTION(Exec) void IWAimAssist(int32 On);
	UFUNCTION(Exec) void IWLead(int32 On);
	/** IWDamage <amount> [self]: damage the enemy, or your own ship when self=1 (P1 forced-sink test). */
	UFUNCTION(Exec) void IWDamage(float Amount, int32 bSelf = 0);

	/** P1: enemy holds fire for this long after spawning, unless the range closes inside HoldFireRangeCm or it is hit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Combat")
	float HoldFireSeconds = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Combat")
	float HoldFireRangeCm = 500000.f;

	/** P1: the camera aim never lands closer than this (no shells beside your own hull). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Combat")
	float MinAimRangeCm = 150000.f;

	/** P1: point the camera over the bow so the crosshair's sea point is ~AimDistanceCm ahead. */
	void ResetCameraToHorizon(float AimDistanceCm = 400000.f);

	int32 ShellsFiredCount() const { return ShellsFired; }
	int32 HitsScoredCount() const { return HitsScored; }
	float AliveTime = 0.f;
	/** S1: world time of the last real mouse/stick look (the camera eases back over the bow after a few idle seconds). */
	float LastLookTime = -100.f;
	bool bGunsFree = false;

	/** A1 autopilot: while set, the wheel follows SetAutoRudder only (A/D input and auto-centre are ignored). */
	bool bAutoHelm = false;
	void SetAutoRudder(float Deg);
	float GetRudderInputValue() const { return RudderInput; }
	/** Test hook: same path as holding / releasing A or D. */
	void TestRudderInput(float V);

	/** Card D: start the card-C scripted gunner (used by the gameplay-clip tour). */
	void StartCombatDemo(const FString& Prefix);
	bool IsCombatDemoRunning() const { return bCombatDemo; }

protected:
	virtual void BeginPlay() override;

	void EnsureInputObjects();
	void AddMappingToLocalPlayer();
	void ApplyShipForces(float Dt);
	void TickAutoDemo(float Dt);
	void TickTelemetry(float Dt);
	void TickWake(float Dt);

	void OnThrottle(const FInputActionValue& V);
	void OnRudder(const FInputActionValue& V);
	void OnCenter(const FInputActionValue& V);
	void OnLook(const FInputActionValue& V);
	void OnZoomStart(const FInputActionValue& V);
	void OnZoomEnd(const FInputActionValue& V);
	void OnWheel(const FInputActionValue& V);
	void OnRudderRelease(const FInputActionValue& V);
	void OnScopeAction(const FInputActionValue& V);
	void OnFreeLookStart(const FInputActionValue& V);
	void OnFreeLookEnd(const FInputActionValue& V);
	void TickH1Test(float Dt);
	/** R1: pick the hull (player Devastation / NPC Kracken), size pontoons + camera to it. Before Super::BeginPlay. */
	void ApplyHullDef();
	/** R1: prop spin, rudder, waterline calibration (mesh boot-top to the mean sea surface under the hull). */
	void TickHullCosmetics(float Dt);
	float PropAngle = 0.f;
	float HullMeshZ = -110.f;
	float CalibT = 0.f, CalibSum = 0.f;
	int32 CalibN = 0;
	bool bCalibDone = false;
	/** cm the sea should sit ABOVE the mesh's boot-top line (Chuck: sitting deep). */
	float DraftBelowBootTopCm = -60.f; // A1: boot-top band ~0.6 m above the rendered sea at rest (was 50)
	/** Muzzle points in barrel-mesh space (cm): two guns per turret. */
	FVector MuzzleLocal[2] = { FVector(690.f, -185.f, 0.f), FVector(690.f, 185.f, 0.f) };

	UPROPERTY(Transient) TObjectPtr<UInputMappingContext> ShipContext;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Throttle;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Rudder;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Center;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Look;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Zoom;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Wheel;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Scope;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_FreeLook;
	// H1 rudder: held A/D slews the commanded angle; code (AI, helmsman) still sets RudderStep.
	float RudderInput = 0.f;
	float RudderCmdDeg = 0.f;
	int32 LastRudderStep = 0;
	// H1 camera rig
	bool bScope = false;
	bool bPivotInit = false;
	FVector PivotSmoothed = FVector::ZeroVector;
	float CamLpHeave = 0.f, CamLpPitch = 0.f, CamLpRoll = 0.f; // P16: low-passed ship motion the LOW/MEDIUM camera may follow a little
 public:
	float DbgCamSwayPitch = 0.f, DbgCamSwayRoll = 0.f, DbgCamSwayZ = 0.f;
 protected:
	float CurArm = 18200.f;
	float CurFov = 70.f;
	float HullTopAboveOrigin = -1.f; // from HullMesh bounds (cm): masts/funnel top over the body origin
	FRotator FreeLookSaved = FRotator::ZeroRotator;
	float ScopeTraceAccum = 0.f;
	// H1 test driver
	bool bH1Test = false;
	bool bH1Quit = false;
	float H1Time = 0.f;
	int32 H1Stage = 0;
	float H1Mark = 0.f;
	float H1HdgStart = 0.f;
	float H1T50 = -1.f, H1T90 = -1.f, H1TurnVisible = -1.f, H1TurnRate = -1.f;
	float H1LastHdg = 0.f;

	bool bZoomed = false;
	float DesiredArm = 26000.f;
	bool bDemo = false;
	float DemoTime = 0.f;
	int32 DemoStep = 0;
	float TelemetryAccum = 0.f;
	FString DemoShotPrefix;
	UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> WakeMIDs;
	// --- Card C internals ---
	void CombatBeginPlay();
	void TickCombat(float Dt);
	void TickTurrets(float Dt);
	void TickAim(float Dt);
	void TickAI(float Dt);
	void TickDamage(float Dt);
	void TickSinking(float Dt);
	void TickAudio(float Dt);
	void TickCombatDemo(float Dt);
	void TryFire();
	void FireTurret(int32 Index, const FVector& Target);
	void StartFire(const FVector& LocalPos);
	void SteerToHeading(float DesiredYaw);
	void PlayVoice(USoundBase* S, float Volume = 1.f);
	void PlayAt(USoundBase* S, const FVector& Where, float Volume, float Pitch = 1.f);
	void Shot(const TCHAR* Tag);
	void SetCamFocus(AActor* Focus, float Arm, float Yaw, float Pitch);
	void OnFireAction(const FInputActionValue& V);
	void OnRepairAction(const FInputActionValue& V);
	USoundBase* Snd(const TCHAR* Name);

	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Fire;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Repair;
	UPROPERTY(Transient) TArray<TObjectPtr<UNiagaraComponent>> Fires;
	UPROPERTY(Transient) TObjectPtr<UNiagaraComponent> SinkSmoke;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> MusicCalm;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> MusicBattle;
	UPROPERTY(Transient) TObjectPtr<USoundAttenuation> FarAtten;
	UPROPERTY(Transient) TMap<FName, TObjectPtr<USoundBase>> SoundCache;
	TArray<USceneComponent*> Barrels[2];
	TWeakObjectPtr<AActor> CamFocus;
	UPROPERTY(Transient) TObjectPtr<class ACameraActor> DemoCam;
	int32 CamMode = 0; // 0 own ship, 1 chase shell, 2 watch enemy
	TWeakObjectPtr<AIronwakeShell> LastShell;

	float TurretBaseYaw[2] = { 0.f, 180.f };
	float TurretOffset[2] = { 0.f, 0.f };
	float TurretElev = 0.f;
	float VoiceBusyUntil = 0.f;
	bool bInCombat = false;
	float CombatCheckAccum = 0.f;
	int32 MissLines = 0;
	float LastMissLine = -100.f;
	float LastDamageLine = -100.f;
	bool bCaptainLoadPending = false;
	float CaptainLoadAt = 0.f;

	float CamFocusArm = 25000.f;
	float CamFocusYaw = 0.f, CamFocusPitch = -8.f;
	bool bFreezeAim = false;
	// AI
	float AIThink = 0.f;
	int32 AIState = 0; // 0 patrol, 1 engage, 2 flee
	int32 PatrolIdx = 0;
	FVector PatrolCentre = FVector::ZeroVector;
	float AIDesiredYaw = 0.f;
	float AIFireAccum = 0.f;
	// combat demo
	bool bCombatDemo = false;
	float CDTime = 0.f;
	int32 CDStage = 0;
	float CDNext = 0.f;
	bool bFireHeld = false;
	FString CDPrefix;
	bool CDShotDone[6] = { false, false, false, false, false, false };
	float CDShellLaunch = -1.f, CDShellFlight = 0.f;
	float CDSinkSeen = -1.f;
	// pending demo capture
	bool bPend = false;
	FString PendTag;
	float PendAt = 0.f;
	float PendRestoreAt = -1.f;
	bool bPendFollowShell = false;
	FVector PendCamLoc = FVector::ZeroVector;
	FVector PendLook = FVector::ZeroVector;
	bool bContact = false;
	float SpotTimer = 0.f;
	float CalmTimer = 0.f;
	int32 HitsTaken = 0;
	int32 HitsScored = 0;
	int32 ShellsFired = 0;

	// ================= W1: heavy hull hydrostatics, bow wave, water audio =================
	/** Natural periods (s) and damping ratios of the hull on the sea (ironclad: slow heave/pitch, lightly damped roll). */
	/** Body-frame height of the static (query) sea surface: the body origin rides this far above the sea fit. */
	static constexpr float StaticSeaBodyZ = -70.f;
	float HeavePeriod = 6.5f, HeaveZeta = 0.5f;
	float PitchPeriod = 6.8f, PitchZeta = 0.55f;
	float RollPeriod = 10.f, RollZeta = 0.4f;   // P16: livelier (was 13 s / 0.35)
	float SeaFollowHeave = 0.7f, SeaFollowPitch = 0.7f, SeaFollowRoll = 0.6f; // P16: fraction of the fitted plane the hull follows (ironclad heavy ~60 % of the cutter)
	float FitSpan = 0.6f; // P16: waterplane fit uses the central fraction of the hull length so 60-100 m waves register
	FIronwakeHullProfile HullProf;
	float HeaveVel = 0.f, PitchRate = 0.f, RollRate = 0.f;  // cm/s, deg/s
	float SeaFitZ = 0.f, SeaFitPitch = 0.f, SeaFitRoll = 0.f; // waterplane fit (query frame)
	float BowSubmergeM = 0.f;  // rendered sea over the forecastle deck at the stem (m, >0 = green water)
	float SlamCooldown = 0.f;
	bool bHydroInit = false;
	UPROPERTY(Transient) TObjectPtr<UIronwakeBowWave> BowWave;
	UPROPERTY(Transient) TObjectPtr<class UNiagaraSystem> BowSprayFX;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> SndBow;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> SndWake;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> SndLap;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> SndRush;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> SndHiss;
	float AudDrift = 0.f;
	void TickHydro(float Dt, float DesiredYawRateDeg);
	void TickBowFX(float Dt);
	void TickWaterAudio(float Dt);
	// W1 test director (-iww1): band tests (chop only / swell only), settle test, captures.
	bool bW1Test = false; float W1T = 0.f; int32 W1Stage = 0; float W1Mark = 0.f;
	double W1Acc[8] = { 0, 0, 0, 0, 0, 0, 0, 0 }; int32 W1N = 0;
	float W1Peak = 0.f, W1SettleStart = -1.f, W1LastOut = 0.f;
	FString W1Mode;
	void TickW1Test(float Dt);
	void W1Shot(const TCHAR* Name);
	float LastDesiredYawRate = 0.f;
public:
	/** A1: wave-induced yaw rate (deg/s) added to the rudder's; gain in deg/s per unit bow-stern lateral slope difference. */
	float WaveYawRate = 0.f;
	float WaveYawGain = 3.f;
	float GetBowSubmergeM() const { return BowSubmergeM; }
	const FIronwakeHullProfile& GetHullProf() const { return HullProf; }
	float GetHullX0() const { return HullProf.X0; }
	float GetHullX1() const { return HullProf.X1; }
protected:
	void BeginCapture(const FString& Tag, const FVector& CamLoc, const FVector& Look, float Delay, bool bFollowShell);
	void TickCapture(float Dt);
public:
	// ================= S2: fleet roster (IronwakeShipFleet.cpp) =================
	/** Roster id of this hull (player: the shipyard selection; NPC: ironclad). */
	FName ShipId = TEXT("ironclad");
	// ---- P9 collision + grounding (IronwakeShipCollision.cpp)
	/** Keel depth (cm): cutter 200, frigate 500, ironclad 700. */
	float DraughtCm = 700.f;
	bool bAground = false, bShoal = false;
	float MinDepthCm = 1e7f;           // shallowest water under the hull (cm), last tick
	float ShakeAmpCm = 0.f, ShakeT = 0.f;  // camera jolt after an impact
	int32 CollisionHits = 0;           // impacts since start (tests)
	float LastImpactCmS = 0.f;         // last impact speed
	FString LastHitTag;
	bool bNoCollision = false;         // -iwnocoll: debug / old tours
	/** Called from ApplyShipForces with the planned world velocity (cm/s) and yaw rate (deg/s); edits both. */
	void TickCollision(float Dt, FVector& InOutVel, float& InOutYawRate);
	void RegisterCollisionHull();
	FIwShape CollShape;         // waterline outline (padded), see IronwakeCollision.h
	TArray<FVector2D> CollKeel;        // keel line + narrowed outline (depth check)
	double LastImpactWall = -100.0;
	int32 GroundingCount = 0;
	/** Steer a desired heading away from land/solids within LookAheadCm (returns it unchanged if clear). */
	float AvoidLand(float DesiredYaw, float LookAheadCm = 150000.f) const;
	float GroundMsgT = -100.f, ShoalMsgT = -100.f, ScrapeSndT = -100.f;
	bool bCollInit = false;
	const struct FIronwakeShipDef* ShipDef = nullptr;
	bool HasSails() const { return SailRig != nullptr; }
	float ReefWindKn = 0.f; // W3: slow wind for the auto-reef
	UPROPERTY(Transient) TObjectPtr<class UIronwakeWake> Wake3; // W3 Kelvin wake trail
	float SprayAcc = 0.f;
	FString GetShipName() const;
	UPROPERTY(Transient) TObjectPtr<class UIronwakeSailRig> SailRig;
	UPROPERTY() TObjectPtr<UStaticMesh> MeshCutterHull;
	UPROPERTY() TObjectPtr<UStaticMesh> MeshFrigateHull;
	UPROPERTY() TObjectPtr<class UMaterialInterface> ClothMaterial;
	UPROPERTY(Transient) TObjectPtr<UNiagaraComponent> FunnelSmoke;
	UPROPERTY(Transient) TObjectPtr<USceneComponent> HelmWheelPivot;
	UPROPERTY(Transient) TObjectPtr<USceneComponent> HelmsmanPivot;
	/** Sailing ship: auxiliary steam engine (B). Astern orders always use steam. */
	bool bAuxSteam = false;
	float SailKn = 0.f;         // sail drive target (kn) for the current wind / heading / sail set
	float TrueWindAngle = 0.f;  // deg off the bow, signed (+ = wind from starboard)
	float SailFill = 0.f;
	float SailHeelDeg = 0.f;
	/** W2 auto-reef state: plan the sails were reefed to (-1 none) and the player override (more sail ordered after a reef). */
	int32 ReefedTo = -1; float ReefAt = -100.f; bool bReefOverride = false; float SailPlanFrac = 0.f;
	/** Apparent wind (deg off the bow, signed) and speed (kn), for the HUD. */
	float ApparentAngle = 0.f, ApparentKn = 0.f;
	UFUNCTION(Exec) void IWSteam(int32 On);
	void ToggleSteam();
protected:
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Steam;
	void OnSteamAction(const FInputActionValue& V);
	void ApplyFleetHull();
	void FleetBeginPlay();
	void TickSails(float Dt);
};
