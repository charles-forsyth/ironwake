#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IronwakeCollision.h"
#include "IronwakeLivingSea.generated.h"

class UStaticMeshComponent;
class UNiagaraComponent;
class UAudioComponent;
class UMaterialInstanceDynamic;
class ACameraActor;
class AIronwakeShipPawn;
class USoundBase;
class UStaticMesh;

/**
 * Card D: neutral merchant steamer. Kinematic (no physics): follows a waypoint loop at a fixed speed with a
 * limited turn rate, sits on the live Gerstner surface and takes pitch/roll from four water samples.
 * Not a combat target (shells only test AIronwakeShipPawn hulls).
 */
UCLASS()
class IRONWAKETEST_API AIronwakeMerchant : public AActor
{
	GENERATED_BODY()
public:
	AIronwakeMerchant();
	virtual void Tick(float Dt) override;
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Hull;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Funnel;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Wake;
	UPROPERTY(Transient) TObjectPtr<UNiagaraComponent> FunnelSmoke;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> WakeMID;

	/** Waypoints, world cm (Z ignored). Loops. */
	TArray<FVector> Route;
	int32 RouteIdx = 0;
	float SpeedCmS = 690.f;      // 9 kn * 1.5 speed compression
	float MaxYawRate = 1.4f;     // deg/s
	float LengthCm = 5000.f;     // R1: Goliath tug 50 m (Aglae 30 m)
	bool bAglae = false;          // R1: which library hull (set before FinishSpawning)
	UPROPERTY() TObjectPtr<UStaticMesh> MeshGoliath;
	UPROPERTY() TObjectPtr<UStaticMesh> MeshAglae;
	float Yaw = 0.f;
	float Pitch = 0.f, Roll = 0.f;
	FString ShipName;
	// P9 collision: kinematic craft hold position when their next step would overlap a solid/ship, and crawl after being rammed.
	FIwShape CollShape;
	float SlowUntil = -1.f;
	bool bCollReg = false, bRouteAudited = false;
	float BlockedS = 0.f;
};

/** Card D: the Hunyuan airship drifting on a wide circle above the sea. */
UCLASS()
class IRONWAKETEST_API AIronwakeAirship : public AActor
{
	GENERATED_BODY()
public:
	AIronwakeAirship();
	virtual void Tick(float Dt) override;

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Body;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Prop;   // R1: spinning stern propeller
	float PropAngle = 0.f;
	float PropRevPerSec = 4.f;
	FVector Centre = FVector::ZeroVector;
	float RadiusCm = 220000.f;
	float AltitudeCm = 42000.f;
	float SpeedCmS = 1400.f;
	float Angle = 0.f;        // radians on the circle
	float Clockwise = 1.f;    // +1 or -1
	float T = 0.f;
	// E1: docking at a mooring mast on its loop. The circle is re-centred so it passes through the mast; the ship eases in,
	// holds with its nose on the mast top for DockHoldS, then sails on. Starts docked.
	void SetupDock(const FVector& MastTop, const FVector& SeawardDir, float HoldS);
	bool bHasDock = false;
	FVector DockTop = FVector::ZeroVector;
	float DockAngle = 0.f;     // circle angle of the actor's docked position
	float DockHoldS = 45.f;
	float DockTimer = 0.f;
	int32 DockState = 0;      // 0 cruise/approach, 1 docked, 2 departing
	float CruiseAltCm = 26000.f;
	float DockedFor() const { return DockState == 1 ? DockTimer : -1.f; }
	static constexpr float NoseXCm = 5500.f, NoseZCm = 3130.f; // remesh nose (55 m fwd, 31 m above keel)
};

/**
 * Card D director: spawned by the GameMode. Builds a rocky island with a lighthouse, spawns the merchant
 * steamers and the airship, runs the ambience (harbour bed at the island, sea wind everywhere, a foghorn
 * every 70-140 s from a distant ship) and, in tour mode, a scripted camera for the gameplay clip.
 */
UCLASS()
class IRONWAKETEST_API AIronwakeLivingSea : public AActor
{
	GENERATED_BODY()
public:
	AIronwakeLivingSea();
	virtual void BeginPlay() override;
	virtual void Tick(float Dt) override;

	UPROPERTY(Transient) TArray<TObjectPtr<AIronwakeMerchant>> Merchants;
	UPROPERTY(Transient) TObjectPtr<AIronwakeAirship> Airship;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> HarbourAmb;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> WindAmb;
	UPROPERTY(Transient) TObjectPtr<ACameraActor> TourCam;
	UPROPERTY(Transient) TObjectPtr<class UPointLightComponent> LampLight;
	UPROPERTY() TObjectPtr<UStaticMesh> RockMesh;
	UPROPERTY() TObjectPtr<UStaticMesh> CylMesh;
	UPROPERTY() TObjectPtr<UStaticMesh> ConeMesh;
	UPROPERTY() TObjectPtr<UStaticMesh> CubeMesh;
	UPROPERTY() TObjectPtr<USoundBase> HarbourSnd;
	UPROPERTY() TObjectPtr<USoundBase> WindSnd;
	UPROPERTY() TObjectPtr<USoundBase> HornSnd;

	/** Island centre (cm, world). */
	FVector IslandCentre = FVector(-50000.f, 190000.f, 0.f); // 1.8 km off the spawn, 15 deg to starboard of its heading

	// Tour (gameplay clip / PIE captures)
	bool bTour = false;
	bool bTourQuit = false;
	bool bTourShots = false;
	float TourT = -1.5f;
	float CutUntil = -1.f;
	int32 LastHits = 0;
	float SinkShotStart = -1.f;
	TWeakObjectPtr<AIronwakeMerchant> TourMerchant;
	TSet<FString> ShotsTaken;
	FVector Anchor = FVector::ZeroVector;
	FVector Anchor2 = FVector::ZeroVector;
	float EndAt = -1.f;
	bool bDirectorCam = false;

	static AIronwakeLivingSea* Get(const UWorld* W);
	static USoundBase* LoadSnd(const TCHAR* Name);

private:
	void BuildIsland();
	void SpawnTraffic();
	void StartAmbience();
	void TickAmbience(float Dt);
	void TickTour(float Dt);
	void TourShot(const FString& Name);
	AIronwakeShipPawn* Player() const;
	// R1 capture run (-iwr1): player close, enemy at 4 km, merchant, airship props (2 frames).
	bool bR1 = false;
	float R1T = 0.f;
	int32 R1Stage = 0;
	UPROPERTY(Transient) TObjectPtr<ACameraActor> R1Cam;
	void TickR1(float Dt);
	void R1Shot(const FString& Name);
	float FoghornIn = 45.f;
	float LampPhase = 0.f;
};
