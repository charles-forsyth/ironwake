#pragma once
#include "CoreMinimal.h"
#include "IronwakePorts.h"
#include "GameFramework/Actor.h"
#include "IronwakeVoyage.generated.h"

class AHUD;
class UCanvas;
class UFont;
class UStaticMesh;
class UStaticMeshComponent;
class USpotLightComponent;
class UPointLightComponent;
class UDirectionalLightComponent;
class UAudioComponent;
class USoundBase;
class UNiagaraSystem;
class ACameraActor;
class AIronwakeShipPawn;
class AIronwakeMerchant;

/** S1: a point of interest you can sail to (compass marker, minimap dot, bearing text). */
USTRUCT()
struct FIronwakePOI
{
	GENERATED_BODY()
	FString Name;
	FString Tag;               // 1-2 letters for the minimap
	FVector Loc = FVector::ZeroVector;
	float ArriveRadiusCm = 50000.f;
	float HardRadiusCm = 0.f;  // rocks/pier: the ship is stopped here (no hull collision with world statics)
	bool bVisited = false;
};

/** S1: a simple flapping gull (sphere body + two cube wings on pivots) that circles a point or follows a ship. */
UCLASS()
class IRONWAKETEST_API AIronwakeGull : public AActor
{
	GENERATED_BODY()
public:
	AIronwakeGull();
	virtual void BeginPlay() override;
	virtual void Tick(float Dt) override;

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Body;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> PivotL;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> PivotR;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> WingL;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> WingR;

	FVector Centre = FVector::ZeroVector;
	TWeakObjectPtr<AActor> Follow;   // circles this actor (ship) instead of Centre
	FVector FollowOffset = FVector::ZeroVector;
	float RadiusCm = 9000.f;
	float AltCm = 4000.f;
	float SpeedCmS = 1100.f;
	float Angle = 0.f;
	float Dir = 1.f;
	float T = 0.f;
	float FlapHz = 2.6f;
	float Phase = 0.f;
};

/**
 * S1 Voyager mode director (spawned by the GameMode): points of interest (harbour with a moored steamer, a wreck,
 * sea stacks, channel buoys, plus the card-D lighthouse island), gulls, a weather/time cycle (overcast day, dusk,
 * foggy night with a lighthouse beam), photo mode, helmsman/anchor/fast-sail, the compass + minimap HUD and the
 * S1 PIE test driver (Saved/iw_s1.txt).
 */
UCLASS()
class IRONWAKETEST_API AIronwakeVoyage : public AActor
{
	GENERATED_BODY()
public:
	AIronwakeVoyage();
	virtual void BeginPlay() override;
	virtual void Tick(float Dt) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	static AIronwakeVoyage* Get(const UWorld* W);

	TArray<FIronwakePOI> POIs;
	int32 NextPOI = 0;

	// Player-facing toggles (keys bound in AIronwakePlayerController)
	void TogglePhoto();
	void CycleWeather();
	void ToggleAutoCycle();
	void NextPoint();
	void ToggleHelm();
	void ToggleAnchor();
	void ToggleFastSail();
	void SetWeather(int32 Preset, float BlendSeconds);   // legacy preset (tests, STORM AT SEA): leaves the living clock/weather
	// ---- P11 living world: continuous clock + independent weather (IronwakeP11.cpp)
	bool bLiving = false;
	float ClockHour = 10.6f;       // P34: crisp mid-morning sun altitude (~21 deg) so harbour & islands sparkle clearly at launch
	float DayLenMin = 48.f;        // real minutes per game day at x1 (-iwdaymin=)
	int32 ClockSpeedIdx = 0;       // 0 x1, 1 x10, 2 pause
	int32 LiveState = 0;           // P34: launch in Clear air (0) instead of grey haze; L cycles weather, V toggles Clear Air (haze off)
	bool bWxDrift = true;
	bool bHazeOff = false;         // P34: 'V' toggles zero-haze crystal-clear visibility on demand
	void ToggleClearHaze();
	struct FLiveWx { float Cover = 0.3f, SunMul = 1.f, SkyMul = 1.f, FogMul = 1.f, Rain = 0.f, Sea = 1.f; };
	void CycleClockSpeed();
	void JumpHours(float H);
	void SetClockHour(float H) { ClockHour = FMath::Fmod(H + 24.f, 24.f); }
	void CycleWeatherLive();
	void ForceWeatherState(int32 S, float BlendSeconds);   // blend (0.05 = snap)
	FString ClockString() const;
	FString LiveWeatherName() const;
	void StartLiving();
	void TickLiving(float Dt);
	float SeaOverride = -1.f;     // tests: force the open-sea state
	void TickP11Test(float RealDt);
	FString P11Mode; float P11T = 0.f, P11Mark = 0.f; int32 P11Step = 0; bool bP11Shot = false;

	bool bHelm = false;
	bool bAnchored = false;
	bool bFastSail = false;
	bool bPhoto = false;
	bool bAutoCycle = false;
	int32 WeatherTarget = 0;

	/** Drawn by AIronwakeHUD after its own block. */
	void DrawVoyageHUD(AHUD* H, UCanvas* C, AIronwakeShipPawn* Ship, UFont* Font, float S, bool bPeaceful);

	FString Toast;
	float ToastUntil = -1.f;
	void ShowToast(const FString& Msg, float Seconds = 4.f);

	// Photo camera (test hook)
	void SetPhotoView(const FVector& Loc, const FRotator& Rot);

	UPROPERTY() TObjectPtr<UStaticMesh> RockMesh;
	UPROPERTY() TObjectPtr<UStaticMesh> CylMesh;
	UPROPERTY() TObjectPtr<UStaticMesh> ConeMesh;
	UPROPERTY() TObjectPtr<UStaticMesh> CubeMesh;
	UPROPERTY() TObjectPtr<UStaticMesh> SphereMesh;
	UPROPERTY() TObjectPtr<UStaticMesh> HullMesh;
	UPROPERTY() TObjectPtr<USoundBase> HarbourSnd;
	UPROPERTY() TObjectPtr<UNiagaraSystem> SplashFX;
	/** E1: place a world mesh with its own materials (no tint), Nanite as imported. */
	UStaticMeshComponent* Place(USceneComponent* Parent, const TCHAR* Name, const FVector& LocCm, float Yaw, float Scale = 1.f, float Pitch = 0.f, float Roll = 0.f);
	void TickE1Test(float RealDt);
	// S2 test director (-iws2=frigate|cutter|ironclad|progress|yard [-iws2quit]), IronwakeS2Test.cpp
	void TickS2Test(float RealDt);
	// W2 storm tests / scenario (-iww2=calm|rough|storm|clip|bench [-iww2quit], -iwstorm = STORM AT SEA start), IronwakeW2Test.cpp
	void TickW2Test(float RealDt);
	void StartStormAtSea();
	FString W2Mode; float W2T = 0.f, W2Mark = 0.f; int32 W2Step = 0, W2N = 0; bool bStormStart = false;
	FString S2Mode; float S2T = 0.f, S2Mark = 0.f, S2LogAcc = 0.f; int32 S2Step = 0; int32 S2Shots = 0;
	float S2YawStart = 0.f, S2TurnT = 0.f;
	float P3Unwrap = 0.f, P3LastHdg = 0.f, P3T0 = 0.f; FVector P3Start = FVector::ZeroVector; float P3MaxD = 0.f; bool P3Done90 = false, P3Done180 = false;
	void S2Place(const FVector& LocCm, float Yaw, bool bKeepSpeed);
	void S2Shot(const TCHAR* Name, bool bPhotoView);
	FString E1Mode; float E1T = 0.f, E1Mark = 0.f; int32 E1Step = 0; bool bE1Shot = false;
	float WreckSurfT = 0.f; int32 WreckSurfN = 0;
	void BuildHarbourPieces(USceneComponent* H);
	void TickP8Test(float RealDt);
	FString P8Mode; float P8T = 0.f, P8Mark = 0.f; int32 P8Step = 0; bool bP8Shot = false;
	// P9 collision test director (-iwp9=all|beach|stack|breakwater|moored|autopilot [-iwp9quit]), IronwakeP9Test.cpp
	void TickP9Test(float RealDt);
	FString P9Mode; float P9T = 0.f, P9Mark = 0.f, P9Contact = -1.f; int32 P9Step = 0, P9Run = 0, P9Hits0 = 0, P9Ground0 = 0, P9Shots = 0;
	FVector P9Start = FVector::ZeroVector, P9BackAt = FVector::ZeroVector; TWeakObjectPtr<class AIronwakeMerchant> TestTarget; float P9Back0 = 0.f; bool bP9Auto = false;
	void BuildTownP12(USceneComponent* Hs);   // P12 hillside town
	void TickP12Test(float RealDt);
	void TickP16Test(float RealDt);
	void TickP19Test(float RealDt);
	void TickP21Test(float RealDt);
	void TickP28Test(float RealDt);
	FString P28Mode; float P28T = 0.f, P28Mark = 0.f, P28LastShotT = -10.f; int32 P28Step = 0, P28Sub = 0;
	FString P21Mode; float P21T = 0.f, P21Mark = 0.f; int32 P21Step = 0, P21Sub = 0;
	FString P19Mode; float P19T = 0.f, P19Mark = 0.f; int32 P19Step = 0; int32 P19Spd = 0;
	FString P16Mode; float P16T = 0.f, P16Mark = 0.f; int32 P16Step = 0, P16N = 0; FString P16Buf;
	FString P12Mode; float P12T = 0.f, P12Mark = 0.f; int32 P12Step = 0; bool bP12Shot = false;
	void BuildTownP8(USceneComponent* Hs);   // P8 (IronwakeP8.cpp): harbour town + NPC craft
	UPROPERTY(Transient) TObjectPtr<class AIronwakeNightSky> NightSky;
	UPROPERTY(Transient) TArray<TObjectPtr<USceneComponent>> Floaters;   // E1: debris / moored small craft that ride the swell
	TArray<FVector> FloaterBase;

	UPROPERTY(Transient) TObjectPtr<USpotLightComponent> Beam;
	UPROPERTY(Transient) TArray<TObjectPtr<UPointLightComponent>> NightLamps;
	UPROPERTY(Transient) TObjectPtr<class UInstancedStaticMeshComponent> WinISM;      // P10 town window + lamp glow instances
	UPROPERTY(Transient) TObjectPtr<class UInstancedStaticMeshComponent> BulbISM;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> WinMID;
	UPROPERTY(Transient) TArray<TObjectPtr<USceneComponent>> Buoys;
	UPROPERTY(Transient) TArray<TObjectPtr<AIronwakeGull>> Gulls;
	UPROPERTY(Transient) TObjectPtr<AIronwakeMerchant> Moored;
	UPROPERTY(Transient) TObjectPtr<ACameraActor> PhotoCam;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> DockAmb;

	FVector HarbourC = FVector(-345000.f, -335000.f, 0.f);
	FVector WreckC = FVector(-92000.f, -150000.f, 0.f);
	FVector StacksC = FVector(110000.f, -90000.f, 0.f);

	// ------------------------------------------------------------------ A1 autopilot (IronwakeAutopilot.cpp)
	/** 0 OFF, 1 HOLD COURSE, 2 WAYPOINTS, 3 TOUR. */
	int32 ApMode = 0;
	bool bChart = false;
	bool bApLoop = false;
	TArray<FVector> Waypoints;
	void CycleAutopilot();
	void EngageAutopilot(int32 Mode);
	void DisengageAutopilot(const FString& Why);
	void ToggleChart();
	void ChartClick(const FVector2D& Screen, bool bRight);
	void ChartStart();
	void ToggleLoop();
	void ClearWaypoints();
	FVector2D WorldToChart(const FVector& W) const;
	FVector ChartToWorld(const FVector2D& S) const;
	FString AutopilotLabel() const;
	bool bRouteDone = false, bTourDone = false; // test flags

private:
	struct FObs { FString Name; FVector C; float R; };
	TArray<FObs> Obstacles;
	TArray<float> ObsMinDist;
	FVector AnchorPt = FVector::ZeroVector;
	void BuildObstacles();
	void PlanLeg(const FVector& A, const FVector& B, TArray<FVector>& Out, int32 Depth, TArray<FString>* Detours) const;
	void BuildPreview(const FVector& Orig, TArray<FVector>& Out, TArray<FString>* Detours) const;
	void TickAutopilot(float Dt);
	void SteerAuto(AIronwakeShipPawn* P, float DesiredYaw, float FFRate, float MaxRud);
	void SetApTel(AIronwakeShipPawn* P, int32 Idx);
	void StartLeg(AIronwakeShipPawn* P, const FVector& Target);
	void DrawChart(AHUD* H, UCanvas* C, AIronwakeShipPawn* Ship, UFont* Font, float S);
	float HoldYaw = 0.f;
	int32 BeatTack = 0; // S2: sailing autopilot beating to windward: +1/-1 board, 0 = free
	float ApTargetYaw = 0.f;
	float ApErr = 0.f;
	int32 CurWp = 0;
	TArray<FVector> LegPath;
	int32 LegIdx = 0;
	bool bApOwnsTel = false;
	int32 ApTelSet = -1;
	int32 CruiseTel = 4;
	int32 TourStop = 0, TourPhase = 0;
	float OrbitSwept = 0.f, OrbitPrevAng = 0.f;
	float PrevRudIn = 0.f;
	int32 LastEnemyShots = 0;
	float UnderFireUntil = -1.f;
	float GiveWayUntil = -1.f;
	FString GiveWayName;
	bool bGiveWaySlowed = false;
	int32 GiveWaySavedTel = 4;
	int32 GiveWayCount = 0;
	float GiveWayYaw = 0.f;
	float ChartCX = 0.f, ChartCY = 0.f, ChartHalfPx = 1.f;
	float ChartHalfCm = 1450000.f; // P32: 29 km x 29 km chart (34+ km diagonal across the 6-port archipelago)
	// A1 standalone test director (-iwa1=hold|route|tour|fire [-iwa1quit])
	FString A1Mode;
	float A1T = 0.f, A1Mark = 0.f, A1LogAcc = 0.f;
	int32 A1Step = 0;
	double A1ErrSq = 0.0; float A1ErrMax = 0.f; int32 A1N = 0;
	float A1Settle = -1.f, A1Overshoot = 0.f;
	int32 A1ShotMask = 0;
	void TickA1Test(float RealDt);
public:
	void DrawChartPublic(AHUD* H, UCanvas* C, AIronwakeShipPawn* Ship, UFont* Font, float S) { DrawChart(H, C, Ship, Font, S); }
	const TArray<FVector>& GetLegPath() const { return LegPath; }
	int32 GetLegIdx() const { return LegIdx; }
	float GetApTargetYaw() const { return ApTargetYaw; }
private:
	void A1Log(const TCHAR* What);

	
	void BuildHarbour();
	void BuildWreck();
	void BuildStacks();
	void BuildBuoys();
	void SpawnGulls();
	void CaptureWeatherBaseline();
	void ApplyWeather(float Dt);
	void TickSailing(float Dt);
	void TickPhoto(float RealDt);
	void TickTest(float RealDt);
	AIronwakeShipPawn* Player() const;
	class AIronwakePlayerController* PC() const;
	UStaticMeshComponent* Mesh(USceneComponent* Parent, UStaticMesh* M, const FVector& Loc, const FRotator& Rot, const FVector& Scale, const FLinearColor* Tint = nullptr);
	UPointLightComponent* Lamp(USceneComponent* Parent, const FVector& Loc, const FLinearColor& Color, float Candelas, float RadiusCm);

	// Weather blend state: 0 overcast day, 1 dusk, 2 foggy night
	struct FWx { float SunPitch, SunI, SkyI, Fog, Bias, Night; FLinearColor SunC, FogC; float Sea = 1.f; float Clear = 0.f; };
	static constexpr int32 NumWx = 6;
	FWx Presets[6];
	FWx Cur;
	FWx From;
	float BlendT = 1.f, BlendDur = 1.f;
	bool bWxReady = false;
	float SunYaw = -150.f;
	float LiveSunYaw = -150.f;
	float LiveLamp = 0.f;   // lamps/windows/beam 0..1 from sun altitude
	FLiveWx LiveFrom, LiveTo, LiveCur;
	UPROPERTY(Transient) TObjectPtr<UDirectionalLightComponent> MoonLight;   // P13: bluish moon light (no atmosphere), phase/altitude driven
	float LiveMoonI = 0.f;
	float LiveBlendT = 1.f, LiveBlendDur = 1.f, WxHoldT = 0.f, WxNextIn = 300.f;
	FRandomStream WxRng;
	float AutoCycleT = 0.f;
	float BeamYaw = 0.f;
	float LampBlink = 0.f;
	TWeakObjectPtr<class ADirectionalLight> Sun;
	TWeakObjectPtr<class ASkyLight> Sky;
	TWeakObjectPtr<class AExponentialHeightFog> Fog;
	TWeakObjectPtr<class APostProcessVolume> PPV;

	// Sailing helpers
	int32 HelmStep = 0;
	float LastBowZ = 0.f;
	float SplashCooldown = 0.f;
	float IdleLook = 0.f;
	float HorizonPitch = -1.2f;
	bool bHorizonKnown = false;
	float GroundLogCooldown = 0.f;
	float FastSailMulOn = 4.f;

	// Photo mode
	FRotator PhotoRot = FRotator::ZeroRotator;
	float PrevDilation = 1.f;
	int32 PhotoTel = 2, PhotoRud = 0;

	// Test driver (Saved/iw_s1.txt)
	FString TestMode;
	float TestT = 0.f;
	int32 TestStep = 0;
	float TestMark = 0.f;
	int32 TestLeg = 0;
	bool bTestCamHold = false;
	float TestLogAccum = 0.f;
	void TestLog(const TCHAR* What);
	void AimControlAt(const FVector& Target, float Pitch);

public:
	// ---- P29: Data-driven multi-port world, arrival harbourmaster / discovery modal & trade logbook (IronwakePorts.cpp)
	TArray<struct FIwGoodDef> Goods;
	TArray<struct FIwPortSpec> WorldPorts;
	UPROPERTY(Transient) TArray<TObjectPtr<class UInstancedStaticMeshComponent>> PortGlowISMs;
	UPROPERTY(Transient) TObjectPtr<ACameraActor> DockCam;
	UPROPERTY(Transient) TObjectPtr<class UIronwakeDockWidget> DockWidget;
	int32 CanDockPortIdx = -1;
	int32 CanLogPoiIdx = -1;
	int32 ActiveDockPortIdx = -1;
	bool bDockOpen = false;
	float DistMetersAccum = 0.f;
	FString P29Mode;
	float P29T = 0.f, P29Mark = 0.f;
	int32 P29Step = 0, P29Sub = 0;
	bool bP29Shot = false;

	void InitWorldPorts();
	void BuildSecondaryPorts();
	bool TryArrivalAction();
	void OpenDockModal(int32 InPortIdx, int32 StartTab = 0);
	void OpenLandmarkModal(int32 PoiIdx);
	void CloseDockModal();
	void TickP29Test(float RealDt);
	UStaticMeshComponent* PortMesh(USceneComponent* Parent, UStaticMesh* M, const FVector& Loc, const FRotator& Rot, const FVector& Scale, const FLinearColor* Tint = nullptr) { return Mesh(Parent, M, Loc, Rot, Scale, Tint); }
	UPointLightComponent* PortLamp(USceneComponent* Parent, const FVector& Loc, const FLinearColor& Color, float Candelas, float RadiusCm) { return Lamp(Parent, Loc, Color, Candelas, RadiusCm); }
};
