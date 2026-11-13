#pragma once
// P20: a living sea. AIronwakeNpcDirector holds a LEDGER of NPC ships and airships (FNpcRec: name, role, faction, state, route, berth).
// Ledger entries near the player get an embodiment (AIronwakeNpcShip: the player's ship dynamics model driven by a steering layer =
// engine telegraph + rudder; springs for heave/pitch/roll on the sea, collision registry, wave source, wake FX); far entries advance
// along their route only. The brain is a deterministic utility/state machine in C++ (no LLM).
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IronwakeCollision.h"
#include "IronwakeHydro.h"
#include "IronwakeNpc.generated.h"

class UStaticMeshComponent;
class UNiagaraComponent;
class UPointLightComponent;
class UIronwakeSailRig;
class UIronwakeWake;
class UIronwakeBowWave;
class AIronwakeNpcDirector;
class AIronwakeNpcShip;
class AIronwakeVoyage;

enum class ENpcState : uint8 { Docked, Depart, Transit, Arrive, Moor, Fish, Patrol, Escort, Flee, Drift, Anchored, Aggressive, Work };
enum class ENpcRole : uint8 { Fisher, Taxi, Ferry, Harbour, Merchant, Cutter, Gunboat, Escort, Pilot, Tug, Airship, Pirate };
enum class ENpcFaction : uint8 { Crown, Trade, Local, Pirate };

const TCHAR* NpcStateName(ENpcState S);
const TCHAR* NpcRoleName(ENpcRole R);
const TCHAR* NpcFactionName(ENpcFaction F);

/** Hull / craft definition. Speeds are nominal knots (the sim runs them x1.5 like the player's ships). */
struct FNpcDef
{
	FName Id;
	FString ClassName;
	const TCHAR* WorldMesh = nullptr;        // /Game/Ironwake/World/<Name>/SM_<Name>: keel at z=0, bow +X
	const TCHAR* LibMesh = nullptr;          // library hull asset path, boot-top at z=0
	float LengthM = 20.f, BeamM = 5.f, DraftM = 1.f;
	float MaxKn = 8.f, CruiseKn = 6.f, AccelMS2 = 0.5f, TurnRadiusM = 60.f, RudderSlew = 25.f, YawResponse = 3.f;
	bool bAir = false, bSail = false, bSteam = false, bPaddle = false, bOars = false, bRig = false;
	float AltCm = 0.f;                       // airships: cruise altitude
	FVector SmokeCm = FVector::ZeroVector;   // funnel top relative to the waterline frame (cm); zero = none
	float SmokeScale = 0.25f;
	float LampZCm = 600.f;                   // mast lamp height over the waterline (cm)
	FVector FlagCm = FVector(0.f, 0.f, 800.f);
	float BootTopCm = 0.f;                   // library hulls: mesh z=0 height over the sea
	float HullYaw = 0.f;                     // mesh yaw correction
	float MeshScale = 1.f;
	int32 HullSize = 1;                      // 0 small, 1 medium, 2 large
	TArray<FString> Names;
	int32 NextName = 0;
};

namespace IronwakeNpc
{
	const TArray<FNpcDef>& Defs();
	FNpcDef* FindMutable(FName Id);
	const FNpcDef* Find(FName Id);
}

struct FNpcPart
{
	TWeakObjectPtr<USceneComponent> Pivot;
	float Angle = 0.f, RevPerSec = 1.f, Phase = 0.f;
	int32 Axis = 0;          // props: 0 spin about X (roll), 1 about Y (pitch)
	uint8 Kind = 0;          // 0 prop (rev ~ thrust), 1 paddle wheel (rev ~ speed), 2 oar (stroke), 3 flag (flutter)
	FVector BaseRot = FVector::ZeroVector;
	float Sign = 1.f;
	TWeakObjectPtr<USceneComponent> Blades, Disc;   // props: real blades while slow, faint translucent disc above DiscRate rev/s
	float Rate = 0.f; bool bDisc = false; int32 Idx = 0;
};

/** Ledger entry: everything about one NPC that must survive despawn. */
struct FNpcRec
{
	int32 Id = -1;
	FString Name;
	const FNpcDef* Def = nullptr;
	ENpcRole Role = ENpcRole::Harbour;
	ENpcFaction Faction = ENpcFaction::Local;
	FLinearColor FlagColor = FLinearColor::White;
	int32 Threat = 0;                 // 0 none .. 3 hostile (P2 hook; nothing fires)
	bool bCanFire = false;            // P2 hook

	ENpcState State = ENpcState::Docked;
	float StateT = 0.f, Timer = 0.f; int32 Phase = 0;
	FVector Pos = FVector::ZeroVector; float Yaw = 0.f; float SpeedKn = 0.f;

	TArray<FVector> Route; int32 RouteIdx = 0;
	int32 HomePort = 0, CurPort = 0, DestPort = -1, DestKind = 0;   // kind: 0 berth, 1 road, 2 sea point
	int32 Span = -1; float SpanY = 0.f;   // reserved/occupied quay span and centre y (harbour frame, m)
	bool bFixedHome = false; FVector2D FixedHomeH = FVector2D::ZeroVector; float FixedHomePsi = 0.f;   // moored-by-habit berths (lighter, tender, pilot, gig)
	bool bAway = false;               // off the chart (ledger only)
	double FishUntilAbs = 0.0, WorkUntilAbs = 0.0;
	int32 LegSeq = 0;                 // destination cycle index
	// warp (mooring / unmooring)
	FVector WarpFrom = FVector::ZeroVector, WarpTo = FVector::ZeroVector; float WarpYawFrom = 0.f, WarpYawTo = 0.f, WarpT0 = 0.f, WarpDur = 16.f;
	FVector HoldPt = FVector::ZeroVector; float HoldYawIn = 0.f, HoldYawOut = 0.f; FVector BerthPos = FVector::ZeroVector; float BerthYaw = 0.f;
	ENpcState PendState = ENpcState::Transit; FVector PendPoint = FVector::ZeroVector; bool bPendDocking = false;
	TArray<int32> Itinerary;           // port indices (merchants, ferries)
	FVector2D FixedHome = FVector2D::ZeroVector;   // harbour frame (m) for boats that live at a fixed mooring
	float FixedPsi = 90.f; bool bFixed = false;
	int32 FixedSlotFor = -1;

	FVector Zone = FVector::ZeroVector; float ZoneR = 40000.f; FVector FishPt = FVector::ZeroVector;
	TArray<FVector> PatrolPts; int32 PatrolIdx = 0;
	int32 Leader = -1; FVector2D Station = FVector2D(-25000.f, 12000.f); bool bHelper = false; int32 Helping = -1;
	int32 BeatTack = 0; float TackT = 100.f; FVector BeatFrom = FVector::ZeroVector;
	float HailT = -1000.f;
	float LastBrainT = 0.f;
	float AvoidDelta = 0.f, AvoidHoldT = 0.f, AvoidSlow = 1.f;
	double DepartAbs = 0.0;           // game hours (absolute) of the next scheduled departure
	int32 TripCount = 0;
	bool bGaleSheltering = false;

	// brain -> body orders
	float DesYaw = 0.f, DesKn = 0.f;
	bool bWarp = false, bHold = false, bAux = false;
	bool bKnown = false;
	FString Target;
	FVector StuckPos = FVector::ZeroVector; float StuckT = 0.f; bool bStuckFlag = false;
	int32 Stucks = 0, Collisions = 0;
	TWeakObjectPtr<AIronwakeNpcShip> Body;
	// brain v2 (lane berths, timetable)
	int32 HomeLane = -1, Lane = -1, PendDest = -1, PilotId = -1, TugId = -1;
	float HomeHour = 17.f, WaitUntil = 0.f, NextActionT = 0.f;
	bool bHomePt = false, bHeld = false, bSheltering = false;
	FVector HomePt = FVector::ZeroVector, RoadPt = FVector::ZeroVector, ThreatPos = FVector::ZeroVector;
	int32 LastDay = -1; bool bBasinWait = false;
	FVector WaitPt = FVector::ZeroVector; int32 WaitIdx = -1; FVector WorkPt = FVector::ZeroVector; float BasinWaitT = 0.f;
	int32 WaitSlot = -1;
};

struct FNpcContact { FVector Pos; FVector Vel; float LenCm; int32 RecId; bool bStationary; float Yaw; };

struct FNpcSpan
{
	FString Name; float XBow = -36.f; int32 Occupant = -1, HomeOf = -1;
	float Y0 = 0.f, Y1 = 0.f, XWall = -50.f, MaxLenM = 48.f;     // quay wall span, harbour frame (m)
	float Psi = 90.f;                                                         // berthed heading (harbour frame, deg)
	bool bRoad = false;                                                       // cove mooring (small craft): XWall = water edge
	TArray<FVector> Occ;                                                      // occupied intervals (y0, y1, recId)
	int32 CorridorOwner = -1;                                                 // exclusive approach/depart lane corridor mutex
	double ReservedAt = 0.0;                                                  // reservation timestamp (180s stale release)
	FVector2D HoldPre = FVector2D::ZeroVector;                                // first approach waypoint (harbour frame, m)
};
struct FNpcPort
{
	FString Name, Tag;
	FVector Offing = FVector::ZeroVector;
	bool bHarbour = false;
	TArray<FNpcSpan> Spans;
	int32 Arrivals = 0, Departures = 0, Boarded = 0;
};

/** Embodiment of a ledger entry. */
UCLASS()
class IRONWAKETEST_API AIronwakeNpcShip : public AActor
{
	GENERATED_BODY()
public:
	AIronwakeNpcShip();
	virtual void Tick(float Dt) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	void Init(AIronwakeNpcDirector* D, int32 InRecId);

	UPROPERTY(Transient) TObjectPtr<USceneComponent> Root;
	UPROPERTY(Transient) TObjectPtr<USceneComponent> HullPivot;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> Hull;
	UPROPERTY(Transient) TObjectPtr<UIronwakeSailRig> Rig;
	UPROPERTY(Transient) TObjectPtr<UNiagaraComponent> Smoke;
	UPROPERTY(Transient) TObjectPtr<UPointLightComponent> Lamp;
	UPROPERTY(Transient) TObjectPtr<UIronwakeWake> Wake3;
	UPROPERTY(Transient) TObjectPtr<UIronwakeBowWave> BowWave;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> ExtraMeshes;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> PaddleChurnPads;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> MooringLines;
	UPROPERTY(Transient) TObjectPtr<class UMaterialInstanceDynamic> PaddleChurnMID;
	UPROPERTY(Transient) TObjectPtr<USceneComponent> HelmWheelPivot;
	TArray<FNpcPart> Parts;

	int32 RecId = -1;
	TWeakObjectPtr<AIronwakeNpcDirector> Dir;
	bool bFx = false, bLampOn = false;

	float V = 0.f, YawRate = 0.f, RudderDeg = 0.f, EngineKn = 0.f, LastDesYR = 0.f;
	float HeaveZ = 0.f, HeaveVel = 0.f, PitchDeg = 0.f, PitchRate = 0.f, RollDeg = 0.f, RollRate = 0.f;
	bool bInit = false;
	FVector2D Pos2 = FVector2D::ZeroVector; float Yaw = 0.f;
	float SailKn = 0.f, TrueWindAngle = 0.f, SailFill = 0.f, SailHeel = 0.f;
	FIwShape Shape; bool bShapeReg = false;
	FIronwakeHullProfile Prof;
	float DraughtCm = 100.f;
	float LastImpactT = -100.f, LastBlockLogT = -100.f; bool bBraking = false;
	int32 MovingParts = 0;
	float BlockedT = 0.f, BackUntil = -1.f;

	void BuildVisuals();
	void SetFx(bool bOn);
	void SetLamp(bool bOn);
	void StepDynamics(float Dt);
	void StepAir(float Dt);
	void StepWarp(float Dt);
	void StepHold(float Dt);
	void StepHydro(float Dt, float DesiredYawRate);
	void StepParts(float Dt);
	FNpcRec* Rec() const;
	float ThrustFrac() const;
	static FIronwakeHullProfile GenericProfile(float LengthM, float BeamM);
	/** Waterline profile for a def, centred on the body origin; OutXcCm = the mesh shift that centred it. */
	static FIronwakeHullProfile ProfileFor(const FNpcDef& D, float& OutXcCm);
};

/** The ledger + brain + spawner. Spawned by the harbour builder. */
UCLASS()
class IRONWAKETEST_API AIronwakeNpcDirector : public AActor
{
	GENERATED_BODY()
public:
	AIronwakeNpcDirector();
	virtual void Tick(float Dt) override;
	virtual void BeginPlay() override;
	static AIronwakeNpcDirector* Get(const UWorld* W);

	TArray<FNpcRec> Recs;
	TArray<FNpcPort> Ports;
	TArray<FNpcContact> Contacts;
	bool bInit = false, bHud = false;
	float SpawnRadiusCm = 520000.f, DespawnRadiusCm = 600000.f;
	float FxRadiusCm = 110000.f; int32 MaxFx = 12, MaxLamps = 8;
	int32 MaxLive = 26;
	bool bForceLive = false, bNoPirate = false;
	double HourAbs = 8.0;

	// soak stats
	int32 StatCollisions = 0, StatStuck = 0, StatGroundings = 0;
	float LogT = 0.f; double InitTime = 0.0; float SoakScale = 1.f, QuitAfter = 0.f;
	FVector HarbourC = FVector::ZeroVector;

	static TArray<TWeakObjectPtr<USceneComponent>> Windmills;   // P21: sail-cross pivots, turned in Tick with the wind
	float WindAngle = 0.f;
	int32 LiveCount() const;
	bool InHarbour(const FVector& P, float Margin = 0.f) const;
	FVector H2W(float XM, float YM) const;      // harbour frame metres -> world cm
	FVector2D W2H(const FVector& W) const;      // world cm -> harbour frame metres
	FNpcRec* RecById(int32 Id) { return Recs.IsValidIndex(Id) ? &Recs[Id] : nullptr; }
	FNpcRec* FindByDef(FName DefId, int32 Nth = 0);
	FString HudLine(const FNpcRec& R, const FVector& Player) const;
	void DrawNpcHud(class AHUD* H, class UCanvas* C, class UFont* Font, float S, const FVector& Player);
	const FNpcRec* ScopeTarget(const FVector& Eye, float AimYaw, float MaxRangeCm, float& OutRangeCm);
	void SpawnBody(FNpcRec& R);
	void DespawnBody(FNpcRec& R);
	void Log(const FString& S);
	float HourNow() const;
	bool IsGale() const;
	// test hooks
	void TeleportRec(FNpcRec& R, const FVector& P, float Yaw, ENpcState S, float Kn);
	void SendToPort(FNpcRec& R, int32 PortIdx, int32 Kind) {}
	void MarkAllKnown();
	void TestRoute(FNpcRec& R, bool bHome);   // P21 test hook: plan a trip to Greyhaven (berth) from wherever the rec is now
	void ForceLive(int32 Max) { bForceLive = true; MaxLive = Max; }
	FString SummaryLine() const;
	/** Moving parts count over live bodies (for the IWNPC moving_parts log). */
	int32 CountMovingParts() const;

private:
	void BuildPorts(); void BuildRoster(); void BuildObstacles(); void BuildContacts();
	void TickBrain(FNpcRec& R, float Dt); void FarStep(FNpcRec& R, float Dt);
	void SetState(FNpcRec& R, ENpcState S);
	void PlanRoute(const FVector& A, const FVector& B, TArray<FVector>& Out, int32 Depth = 0) const;
	void StartTrip(FNpcRec& R, int32 DestPort, ENpcState Then);
	bool FollowRoute(FNpcRec& R, float Kn, float FinalSlowM);
	void Avoid(FNpcRec& R, float Dt, bool bLand, bool bHarb);
	float LandAvoidYaw(const FNpcRec& R, float Des, float LookCm) const;
	void ArriveDone(FNpcRec& R);
	bool PickLane(FNpcRec& R); void ReleaseLane(FNpcRec& R);
	void BerthPose(const FNpcRec& R, int32 Lane, FVector& OutPos, float& OutYaw) const;
	void BeginDepartFromLane(FNpcRec& R); void DepartPhase1(FNpcRec& R);
	bool ShouldShelter(const FNpcRec& R) const;
	bool BasinFree(int32 Me); void BasinTake(int32 Me); void BasinDrop(int32 Me);
	int32 BasinOwner = -1; double BasinT = 0.0;
	struct FWaitSlot { FVector2D H; int32 Owner = -1; double ReservedAt = 0.0; };
	TArray<FWaitSlot> WaitSlots;
	int32 MouthOwner = -1; int32 MouthDir = 0; double MouthReservedAt = 0.0;
	void InitWaitSlots();
	int32 ReserveWaitSlot(FNpcRec& R);
	void ReleaseWaitSlot(FNpcRec& R);
	void ExpireStaleReservations(double Now);
	bool ReserveBerthAndCorridor(FNpcRec& R, bool bNeedCorridor);
	void ReleaseCorridorOnly(FNpcRec& R);
	bool MouthFreeFor(const FNpcRec& R, int32 Dir) const;
	void MouthTake(int32 Me, int32 Dir);
	void MouthDrop(int32 Me);
	void BuildInboundBasinRoute(FNpcRec& R);
	float Dwell(const FNpcRec& R) const;
	void Decide(FNpcRec& R); void GoHome(FNpcRec& R); void AirBrain(FNpcRec& R, float Dt);
	void DoSailing(FNpcRec& R, const FVector& Target, float& Yaw);
	int32 NearestPort(const FVector& P) const;
	void UpdateEmbodiment(float Dt); void SoakStats(float Dt);
	bool ValidPose(const FNpcDef& D, const FVector& P, float Yaw, bool bShoalCheck = true) const;
	void Advance(FNpcRec& R, float DistCm); void AdvanceAir(FNpcRec& R, float Dt);
	float MyLenCm(const FNpcRec& R) const; float HourNowF() const;
	FNpcRec* RecAt(int32 Id) { return Recs.IsValidIndex(Id) ? &Recs[Id] : nullptr; }
	const FNpcRec* RecAt(int32 Id) const { return Recs.IsValidIndex(Id) ? &Recs[Id] : nullptr; }
	int32 Add(FName DefId, ENpcRole Role, ENpcFaction F, const FLinearColor& Col);
	FVector PlayerPos() const;
	FVector ViewPos() const;
	FVector PlayerVel() const;
	float PlayerLenCm = 12000.f;

	struct FObs { FVector C; float R; };
	TArray<FObs> Obs;
	float EmbT = 0.f, ContactT = 0.f, ClockPrev = -1.f;
	int32 BrainCursor = 0;
	FRandomStream Rng;
	TWeakObjectPtr<AIronwakeVoyage> Voy;
	double OwnHour = 8.0;
};
