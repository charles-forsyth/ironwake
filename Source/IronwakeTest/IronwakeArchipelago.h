#pragma once
// I1: the archipelago. 14 seeded islands (build/i1/plan.py, seed 1871) of 9 kinds, each = procedural landform mesh
// (analytic radial heightfield, vertex-colour sand/grass blend) + Poly Haven CC0 rock scans (Nanite, HISM) at the
// waterline/cliffs + vegetation HISMs by height/slope with cull distances + surf ribbons that ride the analytic sea +
// Niagara spray on the windward shore + a 3D surf loop. Every island is a voyage POI, an autopilot obstacle and a
// chart outline. Deterministic (FRandomStream per island). Spawned by AIronwakeVoyage::BeginPlay.
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "IronwakeArchipelago.generated.h"

class UProceduralMeshComponent;
class UHierarchicalInstancedStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class UAudioComponent;
class UNiagaraSystem;
class AHUD;
class UCanvas;
class UFont;
class AIronwakeVoyage;

UENUM()
enum class EIsle : uint8 { Wooded, Pines, Cay, Fort, Lighthouse, Cliff, Rocky, Stacks, Reef, Graveyard };

struct FIsle
{
	FString Name;
	FString Tag;
	EIsle Kind = EIsle::Rocky;
	FVector C = FVector::ZeroVector;   // world cm, Z = sea reference
	float R = 10000.f;                 // footprint radius cm (shoreline mean)
	float Hp = 0.f;                    // peak/plateau height cm
	bool bTerrain = true;
	bool bPlateau = false;
	float CoveYaw = 0.f;               // deg, Wooded/Cay beach side
	float Harm[6] = {};                // shoreline harmonics
	float HPh[6] = {};
	float HazardR = 0.f;               // autopilot circle
	// surf
	TObjectPtr<UProceduralMeshComponent> Surf;
	TArray<FVector> SurfXY;            // per vertex: x, y (world), z = lift cm
	TArray<float> SurfA;               // base alpha
	TArray<float> SurfWind;            // outward normal yaw (deg) per vertex
	TArray<int32> SurfTri;
	TArray<FVector> SurfVtx; TArray<FVector> SurfNrm; TArray<FVector2D> SurfUV; TArray<FLinearColor> SurfCol; TArray<FProcMeshTangent> SurfTan;
	float SurfCheckT = 0.f; bool bSurfNear = true;
	TArray<FVector> SplashPts;         // world xy, z = outward yaw
	float SplashT = 0.f;
	int32 SplashN = 0;
	TObjectPtr<UAudioComponent> Audio;
	FVector LookAt = FVector::ZeroVector; // test framing target (world)
	FVector Village = FVector::ZeroVector;
	float CragTop = -1e9f;

	float RTheta(float Th) const;      // shoreline radius at angle (rad)
	float Height(float X, float Y) const; // local cm (relative to C) -> height cm above sea ref
};

UCLASS()
class IRONWAKETEST_API AIronwakeArchipelago : public AActor
{
	GENERATED_BODY()
public:
	AIronwakeArchipelago();
	virtual void BeginPlay() override;
	virtual void Tick(float Dt) override;
	static AIronwakeArchipelago* Get(const UWorld* W);

	TArray<FIsle> Isles;
	/** Sea reference offset (cm) added to the flat water-plane query; W2's gauge measured rendered == query (+0.24 m). */
	float SeaOffCm = 0.f;
	float SurfLiftCm = 70.f;

	void DrawChartLand(AHUD* H, UCanvas* Cv, TFunctionRef<FVector2D(const FVector&)> ToChart, float S, UFont* Font) const;

	UPROPERTY(Transient) TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> HISMs;
	UPROPERTY(Transient) TArray<TObjectPtr<UProceduralMeshComponent>> Lands;
	UPROPERTY(Transient) TArray<TObjectPtr<UProceduralMeshComponent>> Surfs;
	UPROPERTY(Transient) TArray<TObjectPtr<UAudioComponent>> Audios;
	UPROPERTY(Transient) TArray<TObjectPtr<USceneComponent>> Floats;
	TArray<FVector> FloatBase; // x = keel offset cm, y = base yaw, z = phase
	UPROPERTY() TObjectPtr<UMaterialInterface> GroundMat;
	UPROPERTY() TObjectPtr<UMaterialInterface> SurfMat;
	UPROPERTY() TObjectPtr<UMaterialInterface> ShelfMat;
	UPROPERTY() TObjectPtr<UNiagaraSystem> SplashFX;
	UPROPERTY() TObjectPtr<USoundBase> SurfSnd;
	UPROPERTY() TObjectPtr<UStaticMesh> CylMesh;
	UPROPERTY() TObjectPtr<UStaticMesh> ConeMesh;
	UPROPERTY() TObjectPtr<UMaterialInterface> ShapeMat;

	int32 NumInstances = 0, NumPieces = 0;
	TArray<FVector> PendingRockSurf;

private:
	TMap<FString, TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> HismByKey;
	UHierarchicalInstancedStaticMeshComponent* Hism(const TCHAR* Mesh, float CullStartM, float CullEndM, bool bShadow, const FString& Group);
	void Inst(const TCHAR* Mesh, const FVector& W, const FRotator& R, const FVector& S, float CullStartM = 0.f, float CullEndM = 0.f, bool bShadow = true, const FString& Group = TEXT("all"));
	USceneComponent* Piece(USceneComponent* Root, const TCHAR* Mesh, const FVector& W, const FRotator& R, const FVector& S);
	void BuildIsle(FIsle& I, int32 Idx);
	void BuildLand(FIsle& I);
	void BuildShelf(FIsle& I, const TArray<FVector>& Centres, float InnerR, float OuterR);
	void RockLandform(FIsle& I, FRandomStream& Rng, int32 N, float TopM);
	void Beaches(FIsle& I, FRandomStream& Rng);
	void BuildSurfRing(FIsle& I, const TArray<FVector>& Ring, float Width, bool bClosed);
	void FinishSurf(FIsle& I);
	void ScatterVeg(FIsle& I, FRandomStream& Rng);
	void RockRing(FIsle& I, FRandomStream& Rng, float RhoIn, float RhoOut, float Every, float MinS, float MaxS, float SkipCove);
	void CliffRing(FIsle& I, FRandomStream& Rng, int32 N, float Rho);
	void Crag(FIsle& I, FRandomStream& Rng, float TopM);
	void Outcrops(FIsle& I, FRandomStream& Rng, int32 N);
	void Stack(FIsle& I, FRandomStream& Rng, const FVector2D& L, float Hm, float Wd, TArray<FVector>* SurfRocks);
	void BuildVillage(FIsle& I, FRandomStream& Rng);
	void BuildFort(FIsle& I, FRandomStream& Rng);
	void BuildLighthouse(FIsle& I);
	void BuildGraveyard(FIsle& I, FRandomStream& Rng, TArray<FVector>& SurfRocks);
	UStaticMeshComponent* Shape(USceneComponent* Root, UStaticMesh* M, const FVector& W, const FRotator& R, const FVector& S, const FLinearColor& Col);
	void UpdateSurf(FIsle& I, float Now);
	float SeaZ(const FVector& P) const;   // rendered surface incl. waves (cm), analytic

	// I1 test director (-iwi1=shots|bench [-iwi1quit])
	FString TestMode;
	float TT = 0.f, TMark = 0.f;
	int32 TStep = 0;
	bool bShot = false;
	void TickTest(float RealDt);
	FVector BenchPos = FVector::ZeroVector; float BenchYaw = 0.f; int32 BenchCount = 0;
	void FindBenchView();
public:
	TSet<int32> HostedPortIsles;
};
