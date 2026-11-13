// I1: archipelago (see IronwakeArchipelago.h). Deterministic code only; assets are Poly Haven CC0 scans imported by
// Content/Python/iw_i1_import.py into /Game/Ironwake/World/<Name>/SM_<Name> (+ E1's coast/cliff scans).
#include "IronwakeArchipelago.h"
#include "IronwakeCollision.h"
#include "IronwakeVoyage.h"
#include "IronwakeLivingSea.h"
#include "IronwakeShell.h"
#include "IronwakeHydro.h"
#include "IronwakeShipPawn.h"
#include "IronwakePlayerController.h"
#include "IronwakeWorld.h"
#include "IronwakeTest.h"

#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Canvas.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/HUD.h"
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "ProceduralMeshComponent.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "UnrealClient.h"
#include "UObject/ConstructorHelpers.h"

namespace IwArch
{
	constexpr float CM = 100.f; // metres -> cm
	struct FDef { const TCHAR* Name; const TCHAR* Tag; EIsle Kind; float X, Y, Rm; };
	// build/i1/plan.py (seed 1871): clear of the harbour channel, merchant routes, spawn and the E1 places.
	const FDef IsleDefs[] = {
		{ TEXT("Kestrel Isle"), TEXT("K"), EIsle::Wooded, 620.f, -3210.f, 520.f },
		{ TEXT("Fort Grimsby"), TEXT("F"), EIsle::Fort, 1250.f, 1950.f, 230.f },
		{ TEXT("Morrow Head"), TEXT("M"), EIsle::Lighthouse, 20.f, -910.f, 260.f },
		{ TEXT("Gannet Stacks"), TEXT("G"), EIsle::Stacks, -1950.f, -1130.f, 200.f },
		{ TEXT("Sable Cay"), TEXT("S"), EIsle::Cay, -2160.f, 140.f, 210.f },
		{ TEXT("The Teeth (reef)"), TEXT("T"), EIsle::Reef, 3420.f, 3140.f, 260.f },
		{ TEXT("Drowned Men's Shoal"), TEXT("D"), EIsle::Graveyard, -2950.f, 3030.f, 280.f },
		{ TEXT("Fir Holm"), TEXT("Fh"), EIsle::Pines, -3360.f, -1310.f, 200.f },
		{ TEXT("Seal Rocks"), TEXT("Sr"), EIsle::Rocky, 2590.f, 90.f, 120.f },
		{ TEXT("Little Sable"), TEXT("Ls"), EIsle::Cay, -570.f, 3550.f, 120.f },
		{ TEXT("Heron Holm"), TEXT("Hh"), EIsle::Pines, 1560.f, 890.f, 150.f },
		{ TEXT("The Anvil"), TEXT("A"), EIsle::Rocky, 470.f, 670.f, 140.f },
		{ TEXT("Blackscar"), TEXT("B"), EIsle::Cliff, -1240.f, -2650.f, 180.f },
		{ TEXT("The Sisters"), TEXT("Si"), EIsle::Stacks, -1480.f, 3360.f, 150.f },
	};
	const TCHAR* RockNames[] = { TEXT("CoastRocks05"), TEXT("NamaCliff01"), TEXT("RockMossSet01"), TEXT("CoastLandRocks03"), TEXT("RockFace01"), TEXT("CoastLandRocks02") };
	float Smoothx(float A, float B, float X) { const float Dn = B - A; const float T = FMath::Clamp((X - A) / (FMath::Abs(Dn) < 1e-4f ? 1e-4f : Dn), 0.f, 1.f); return T * T * (3.f - 2.f * T); }
	FVector PolarV(float ThDeg, float R) { const float T = FMath::DegreesToRadians(ThDeg); return FVector(FMath::Cos(T) * R, FMath::Sin(T) * R, 0.f); }
}

using namespace IwArch;

// ============================================================================ shape
float FIsle::RTheta(float Th) const
{
	float F = 1.f;
	for (int32 k = 0; k < 6; ++k) F += Harm[k] * FMath::Cos((k == 5 ? 11 : k + 2) * Th + HPh[k]);
	// beach cove: a shallow bay on the cove side (Wooded only)
	if (Kind == EIsle::Wooded)
	{
		const float D = FMath::Abs(FMath::FindDeltaAngleRadians(Th, FMath::DegreesToRadians(CoveYaw)));
		F -= 0.28f * FMath::Exp(-D * D / 0.09f);
	}
	return R * F;
}

float FIsle::Height(float X, float Y) const
{
	const float Th = FMath::Atan2(Y, X);
	const float Rs = RTheta(Th);
	const float Rho = FMath::Sqrt(X * X + Y * Y) / FMath::Max(Rs, 1.f);
	const float Under = -900.f; // seabed shelf under the shore (hidden by water)
	if (Rho >= 1.25f) return Under;
	if (Rho >= 1.f) return FMath::Lerp(0.f, Under, Smoothx(1.f, 1.25f, Rho));
	// beach band 1.0 -> 0.82: rises to ~2.2 m
	const float Beach = 220.f * Smoothx(1.f, 0.80f, Rho);
	float Body;
	if (bPlateau) Body = Hp * Smoothx(0.92f, 0.62f, Rho); // steep-sided table (fort / cliff isle)
	else Body = Hp * FMath::Pow(Smoothx(0.92f, 0.05f, Rho), 1.6f) * (0.75f + 0.25f * FMath::Cos(Th * 2.f + HPh[4])); // lopsided ridge
	// gentle hummocks
	const float N = (0.12f * Hp * FMath::Sin(X / 5300.f + HPh[0]) * FMath::Cos(Y / 4100.f + HPh[1])
		+ 0.06f * Hp * (1.f - FMath::Abs(FMath::Sin(X / 1900.f + Y / 2600.f + HPh[2]))) + 0.03f * Hp * FMath::Sin(X / 830.f - Y / 1100.f + HPh[3])) * Smoothx(0.95f, 0.5f, Rho);
	float Hh = FMath::Max(Beach, Body + N + Beach * 0.4f);
	if (Kind == EIsle::Cay) Hh += 0.35f * Hp * FMath::Max(0.f, FMath::Sin(X / 2600.f + HPh[2]) * FMath::Sin(Y / 3100.f + HPh[3])) * Smoothx(0.85f, 0.4f, Rho);
	if (Kind == EIsle::Wooded || Kind == EIsle::Cay)
	{
		// cove side: lower the hill so the beach reads from the water
		const float D = FMath::Abs(FMath::FindDeltaAngleRadians(Th, FMath::DegreesToRadians(CoveYaw)));
		if (D < 0.9f) Hh = FMath::Lerp(Beach + 0.15f * Body, Hh, Smoothx(0.f, 0.9f, D) * Smoothx(0.85f, 0.45f, Rho) + Smoothx(0.45f, 0.2f, Rho) * (1.f - Smoothx(0.f, 0.9f, D)));
	}
	return Hh;
}

// ============================================================================ actor
AIronwakeArchipelago::AIronwakeArchipelago()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GroundF(TEXT("/Game/Ironwake/Islands/M_IW_IslandGround.M_IW_IslandGround"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> SurfF(TEXT("/Game/Ironwake/Islands/M_IW_Surf2.M_IW_Surf2"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShelfF(TEXT("/Game/Ironwake/Islands/M_IW_Shelf.M_IW_Shelf"));
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> SplashF(TEXT("/Game/Ironwake/FX/NS_IW_Splash.NS_IW_Splash"));
	static ConstructorHelpers::FObjectFinder<USoundBase> SurfSndF(TEXT("/Game/Ironwake/Audio/surf_proc.surf_proc"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylF(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeF(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeF(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	GroundMat = GroundF.Object; SurfMat = SurfF.Object; ShelfMat = ShelfF.Object; SplashFX = SplashF.Object; SurfSnd = SurfSndF.Object;
	CylMesh = CylF.Object; ConeMesh = ConeF.Object; ShapeMat = ShapeF.Object;
}

AIronwakeArchipelago* AIronwakeArchipelago::Get(const UWorld* W)
{
	if (!W) return nullptr;
	TActorIterator<AIronwakeArchipelago> It(const_cast<UWorld*>(W));
	return It ? *It : nullptr;
}

float AIronwakeArchipelago::SeaZ(const FVector& P) const
{
	// Analytic Gerstner (FastZ) is calibrated per frame against the plugin query + RenderedSeaOffsetCm (-4.7 m), but the
	// W2 gauge test showed the render sits on the raw query in today's sea, so undo that offset.
	return IronwakeSea::FastZ(GetWorld(), P) - AIronwakeShell::RenderedSeaOffsetCm + SeaOffCm;
}

UHierarchicalInstancedStaticMeshComponent* AIronwakeArchipelago::Hism(const TCHAR* MeshName, float CullStartM, float CullEndM, bool bShadow, const FString& Group)
{
	const FString Key = FString::Printf(TEXT("%s|%s"), MeshName, *Group);
	if (TObjectPtr<UHierarchicalInstancedStaticMeshComponent>* F = HismByKey.Find(Key)) return F->Get();
	UStaticMesh* Me = IronwakeWorld::Mesh(MeshName);
	if (!Me) return nullptr;
	UHierarchicalInstancedStaticMeshComponent* H = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
	H->SetStaticMesh(Me);
	H->SetupAttachment(RootComponent);
	H->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	H->SetCastShadow(bShadow);
	H->SetMobility(EComponentMobility::Static);
	if (CullEndM > 0.f) { H->InstanceStartCullDistance = (int32)(CullStartM * CM); H->InstanceEndCullDistance = (int32)(CullEndM * CM); }
	H->RegisterComponent();
	AddInstanceComponent(H);
	// Leaves read near-black against the overcast sky: lift foliage (M_IW_Foliage children) albedo.
	for (int32 mi = 0; mi < H->GetNumMaterials(); ++mi)
		if (UMaterialInterface* Mt = H->GetMaterial(mi))
			if (Mt->GetBaseMaterial() && Mt->GetBaseMaterial()->GetName() == TEXT("M_IW_Foliage"))
				if (UMaterialInstanceDynamic* D = H->CreateAndSetMaterialInstanceDynamic(mi)) D->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.9f, 2.1f, 1.7f));
	HismByKey.Add(Key, H);
	HISMs.Add(H);
	return H;
}

void AIronwakeArchipelago::Inst(const TCHAR* MeshName, const FVector& W, const FRotator& R, const FVector& S, float CullStartM, float CullEndM, bool bShadow, const FString& Group)
{
	if (UHierarchicalInstancedStaticMeshComponent* H = Hism(MeshName, CullStartM, CullEndM, bShadow, Group))
	{
		H->AddInstance(FTransform(R, W, S), true);
		++NumInstances;
		// P9: solid rocks/cliffs become ship-collision ellipses (bounds of the scaled scan), vegetation and pebbles do not.
		const FString N(MeshName);
		if (N.StartsWith(TEXT("Boulder")) || N.StartsWith(TEXT("CoastRocks")) || N.StartsWith(TEXT("CoastLandRocks")) || N.StartsWith(TEXT("Nama")) || N.StartsWith(TEXT("RockMoss")) || N.StartsWith(TEXT("RockFace")) || N.StartsWith(TEXT("CoastalCliff")))
			if (UIronwakeColl* Co = UIronwakeColl::Get(GetWorld())) Co->AddMeshBounds(H->GetStaticMesh(), FTransform(R, W, S), N, true, 250.f);
	}
}

USceneComponent* AIronwakeArchipelago::Piece(USceneComponent* Root, const TCHAR* MeshName, const FVector& W, const FRotator& R, const FVector& S)
{
	UStaticMeshComponent* C = IronwakeWorld::Place(this, Root, MeshName, FVector::ZeroVector, FRotator::ZeroRotator, S);
	if (C) { C->SetWorldLocationAndRotation(W, R); ++NumPieces; }
	return C;
}

UStaticMeshComponent* AIronwakeArchipelago::Shape(USceneComponent* Root, UStaticMesh* Me, const FVector& W, const FRotator& R, const FVector& S, const FLinearColor& Col)
{
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
	C->SetStaticMesh(Me);
	C->SetupAttachment(Root);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->RegisterComponent();
	AddInstanceComponent(C);
	C->SetWorldLocationAndRotation(W, R);
	C->SetWorldScale3D(S);
	if (ShapeMat) { UMaterialInstanceDynamic* D = UMaterialInstanceDynamic::Create(ShapeMat, this); D->SetVectorParameterValue(TEXT("Color"), Col); C->SetMaterial(0, D); }
	return C;
}

void AIronwakeArchipelago::BeginPlay()
{
	Super::BeginPlay();
	UWorld* W = GetWorld();
	const double T0 = FPlatformTime::Seconds();
	FParse::Value(FCommandLine::Get(), TEXT("iwi1seaoff="), SeaOffCm);
	AIronwakeVoyage* Vy = AIronwakeVoyage::Get(W);
	HostedPortIsles.Reset();
	if (Vy)
	{
		for (const FIwPortSpec& Ps : Vy->WorldPorts)
			if (!Ps.bMainHarbour && Ps.HostIsleIdx >= 0) HostedPortIsles.Add(Ps.HostIsleIdx);
	}
	for (int32 i = 0; i < UE_ARRAY_COUNT(IsleDefs); ++i)
	{
		const FDef& D = IsleDefs[i];
		FIsle I;
		I.Name = D.Name; I.Tag = D.Tag; I.Kind = D.Kind;
		int32 PortSpecIdx = -1;
		if (Vy && IwPorts::IsHostedPortIsle(i, Vy->WorldPorts, &PortSpecIdx))
		{
			const FIwPortSpec& Ps = Vy->WorldPorts[PortSpecIdx];
			I.Name = Ps.Name;
			I.Tag = Ps.Tag;
		}
		const FVector C(D.X * CM, D.Y * CM, 0.f);
		I.C = FVector(C.X, C.Y, AIronwakeShell::WaterZAt(W, C) + SeaOffCm);
		I.R = D.Rm * CM;
		FRandomStream Rng(9100 + i * 37);
		for (int32 k = 0; k < 6; ++k) { I.Harm[k] = Rng.FRandRange(0.04f, 0.16f) / (1.f + 0.35f * k); I.HPh[k] = Rng.FRandRange(0.f, 2.f * PI); }
		I.CoveYaw = Rng.FRandRange(0.f, 360.f);
		switch (I.Kind)
		{
		case EIsle::Wooded: I.Hp = 48.f * CM; break;
		case EIsle::Pines: I.Hp = 12.f * CM; break; // low base; the silhouette comes from rock-scan landforms
		case EIsle::Cay: I.Hp = 7.f * CM; I.R *= 0.95f; break;
		case EIsle::Fort: I.Hp = 14.f * CM; I.bPlateau = true; break;
		case EIsle::Lighthouse: I.Hp = 9.f * CM; break;   // body = boulder-scan crag (St Brannoc recipe)
		case EIsle::Cliff: I.Hp = 8.f * CM; break;
		case EIsle::Rocky: I.Hp = 5.f * CM; I.R *= 1.1f; break;
		default: I.bTerrain = false; break; // Stacks / Reef / Graveyard: rocks only
		}
		Isles.Add(MoveTemp(I));
	}
	for (int32 i = 0; i < Isles.Num(); ++i) BuildIsle(Isles[i], i);
	if (FParse::Value(FCommandLine::Get(), TEXT("iwi1="), TestMode)) UE_LOG(LogIronwake, Display, TEXT("IWI1 test armed '%s'"), *TestMode);
	UE_LOG(LogIronwake, Display, TEXT("IWI1 archipelago: %d islands, %d HISMs, %d instances, %d pieces, %d surf rings, %d audio, %.2f s"),
		Isles.Num(), HISMs.Num(), NumInstances, NumPieces, Surfs.Num(), Audios.Num(), FPlatformTime::Seconds() - T0);
}

// ---------------------------------------------------------------------------- per island
void AIronwakeArchipelago::BuildIsle(FIsle& I, int32 Idx)
{
	FRandomStream Rng(5000 + Idx * 101);
	USceneComponent* Root = NewObject<USceneComponent>(this, *FString::Printf(TEXT("Isle_%d"), Idx));
	Root->SetupAttachment(RootComponent);
	Root->RegisterComponent();
	AddInstanceComponent(Root);
	Root->SetWorldLocation(I.C);
	TArray<FVector> SurfRocks; // world xy + radius (cm) for rock surf rings
	I.LookAt = I.C + FVector(0, 0, FMath::Max(I.Hp * 0.4f, 1500.f));
	{
		// P9: the autopilot circle must clear the whole shelf a 7 m draught can ground on (rho ~1.12 of the shoreline at its widest bearing).
		float MaxRs = I.R;
		for (int32 k = 0; k < 72; ++k) MaxRs = FMath::Max(MaxRs, I.RTheta(2.f * PI * k / 72.f));
		I.HazardR = FMath::Max(I.R * 1.35f + 6000.f, MaxRs * 1.25f + 15000.f);
	}

	if (HostedPortIsles.Contains(Idx))
	{
		if (AIronwakeVoyage* Vy = AIronwakeVoyage::Get(GetWorld()))
		{
			int32 PIdx = -1;
			if (IwPorts::IsHostedPortIsle(Idx, Vy->WorldPorts, &PIdx))
			{
				const FIwPortSpec& Ps = Vy->WorldPorts[PIdx];
				I.LookAt = Ps.H2W(-85.f, 0.f, 14.f);
				for (int32 Side = -1; Side <= 1; Side += 2)
				{
					TArray<FVector> Arc;
					for (int32 k = 0; k <= 14; ++k)
					{
						const float t = (float)k / 14.f;
						const float Lx = FMath::Lerp(108.f, -155.f * Ps.HillScale - 40.f, t);
						const float Ly = Side * FMath::Lerp(96.f, 215.f * Ps.HillScale, FMath::Sin(t * PI * 0.65f));
						Arc.Add(Ps.H2W(Lx, Ly, 0.f));
					}
					BuildSurfRing(I, Arc, 1300.f, false);
				}
				FinishSurf(I);
			}
		}
		if (SurfSnd)
		{
			USoundAttenuation* At = NewObject<USoundAttenuation>(this);
			At->Attenuation.bAttenuate = true;
			At->Attenuation.bSpatialize = true;
			At->Attenuation.AttenuationShapeExtents = FVector(10000.f, 0.f, 0.f);
			At->Attenuation.FalloffDistance = 150000.f;
			I.Audio = UGameplayStatics::SpawnSoundAtLocation(this, SurfSnd, I.C + FVector(0, 0, 300.f), FRotator::ZeroRotator, 1.2f, 1.f, Rng.FRandRange(0.f, 25.f), At, nullptr, false);
			if (I.Audio) Audios.Add(I.Audio);
		}
		UE_LOG(LogIronwake, Display, TEXT("IWI1 port-hosted isle %s idx=%d at (%.0f, %.0f) m"), *I.Name, Idx, I.C.X / CM, I.C.Y / CM);
		return;
	}

	if (I.bTerrain) BuildLand(I);

	switch (I.Kind)
	{
	case EIsle::Wooded:
		RockRing(I, Rng, 0.97f, 1.08f, 2600.f, 3.0f, 7.0f, 0.8f);
		Outcrops(I, Rng, 6);
		ScatterVeg(I, Rng);
		Beaches(I, Rng);
		BuildVillage(I, Rng);
		break;
	case EIsle::Pines:
		RockLandform(I, Rng, 5, 26.f);
		RockRing(I, Rng, 0.95f, 1.06f, 2000.f, 2.5f, 6.0f, 0.f);
		ScatterVeg(I, Rng);
		Beaches(I, Rng);
		break;
	case EIsle::Cay:
		RockRing(I, Rng, 1.0f, 1.12f, 5200.f, 1.2f, 2.6f, 2.0f);
		ScatterVeg(I, Rng);
		Beaches(I, Rng);
		break;
	case EIsle::Fort:
		CliffRing(I, Rng, 9, 0.9f);
		RockRing(I, Rng, 0.95f, 1.08f, 2200.f, 3.f, 6.f, 0.f);
		ScatterVeg(I, Rng);
		BuildFort(I, Rng);
		break;
	case EIsle::Lighthouse:
		Crag(I, Rng, 40.f);
		CliffRing(I, Rng, 6, 0.95f);
		RockRing(I, Rng, 0.95f, 1.1f, 2000.f, 3.f, 7.f, 0.f);
		ScatterVeg(I, Rng);
		BuildLighthouse(I);
		break;
	case EIsle::Cliff:
		Crag(I, Rng, 50.f);
		CliffRing(I, Rng, 6, 0.95f);
		RockRing(I, Rng, 0.95f, 1.1f, 1800.f, 3.f, 8.f, 0.f);
		ScatterVeg(I, Rng);
		break;
	case EIsle::Rocky:
	{
		RockRing(I, Rng, 0.6f, 1.1f, 1500.f, 4.f, 9.f, 0.f);
		// break the silhouette: 3 tall stacks (20-60 m) around a low rock platform + an arch, surf round each
		for (int32 k = 0; k < 3; ++k)
		{
			const FVector2D L = FVector2D(PolarV(k * 120.f + Rng.FRandRange(-25.f, 25.f), I.R * Rng.FRandRange(0.55f, 0.95f)));
			Stack(I, Rng, L, k == 0 ? 58.f : Rng.FRandRange(22.f, 40.f), k == 0 ? 0.9f : 0.7f, &SurfRocks);
		}
		const FVector2D A = FVector2D(PolarV(Rng.FRandRange(0.f, 360.f), I.R * 0.3f));
		const float Ay = Rng.FRandRange(0.f, 180.f);
		const FVector Ax = FRotator(0.f, Ay, 0.f).Vector();
		for (int32 s2 = -1; s2 <= 1; s2 += 2)
		{
			const FVector P = I.C + FVector(A.X, A.Y, 0.f) + Ax * s2 * 1300.f;
			Inst(TEXT("Boulder01"), P + FVector(0, 0, -800.f), FRotator(-s2 * 8.f, Ay + 90.f, 0.f), FVector(8.f, 6.f, 26.f));
			SurfRocks.Add(FVector(P.X, P.Y, 1000.f));
		}
		Inst(TEXT("Boulder01"), I.C + FVector(A.X, A.Y, 1800.f), FRotator(4.f, Ay, 0.f), FVector(28.f, 7.f, 6.f));
		I.LookAt = I.C + FVector(0, 0, 2000.f);
		I.HazardR = I.R * 1.5f + 6000.f;
		break;
	}
	case EIsle::Stacks:
	{
		const int32 N = I.R > 18000.f ? 5 : 3;
		for (int32 k = 0; k < N; ++k)
		{
			const FVector2D L = k == 0 ? FVector2D::ZeroVector : FVector2D(PolarV(Rng.FRandRange(0.f, 360.f), Rng.FRandRange(0.35f, 0.85f) * I.R));
			Stack(I, Rng, L, k == 0 ? Rng.FRandRange(55.f, 80.f) : Rng.FRandRange(20.f, 48.f), k == 0 ? 1.15f : Rng.FRandRange(0.7f, 1.0f), &SurfRocks);
		}
		// natural arch: two leaning boulder columns + a lintel boulder across
		const FVector2D A = FVector2D(PolarV(Rng.FRandRange(0.f, 360.f), 0.55f * I.R));
		const float Ay = Rng.FRandRange(0.f, 180.f);
		const FVector Ax = FRotator(0.f, Ay, 0.f).Vector();
		for (int32 s = -1; s <= 1; s += 2)
		{
			const FVector P = I.C + FVector(A.X, A.Y, 0.f) + Ax * s * 1500.f;
			Inst(TEXT("Boulder01"), P + FVector(0, 0, -800.f), FRotator(-s * 9.f, Ay + 90.f, 0.f), FVector(9.f, 7.f, 30.f));
			SurfRocks.Add(FVector(P.X, P.Y, 1100.f));
		}
		Inst(TEXT("Boulder01"), I.C + FVector(A.X, A.Y, 2200.f), FRotator(4.f, Ay, 0.f), FVector(32.f, 8.f, 7.f));
		Inst(TEXT("Boulder01"), I.C + FVector(A.X, A.Y, 2600.f) + Ax * 300.f, FRotator(-6.f, Ay + 12.f, 3.f), FVector(20.f, 7.f, 5.f));
		I.LookAt = I.C + FVector(A.X, A.Y, 1500.f);
		break;
	}
	case EIsle::Reef:
	{
		// a crescent of low rocks awash: breaking surf all along it
		const float A0 = Rng.FRandRange(0.f, 360.f);
		for (int32 k = 0; k < 26; ++k)
		{
			const float Th = A0 + (k - 13) * 7.5f + Rng.FRandRange(-3.f, 3.f);
			const float Rr = I.R * (1.f + Rng.FRandRange(-0.18f, 0.18f));
			const FVector P = I.C + PolarV(Th, Rr);
			const float S = Rng.FRandRange(2.2f, 4.5f);
			const TCHAR* Mn = RockNames[Rng.RandRange(0, 5)];
			Inst(Mn, P + FVector(0, 0, -S * 70.f), FRotator(Rng.FRandRange(-8.f, 8.f), Rng.FRandRange(0.f, 360.f), Rng.FRandRange(-8.f, 8.f)), FVector(S, S, S * 0.7f));
			SurfRocks.Add(FVector(P.X, P.Y, S * 260.f + 600.f));
			if (k % 3 == 0) I.SplashPts.Add(FVector(P.X, P.Y, Th));
		}
		I.LookAt = I.C + PolarV(A0, I.R);
		I.HazardR = I.R * 1.4f + 5000.f;
		break;
	}
	case EIsle::Graveyard:
		BuildGraveyard(I, Rng, SurfRocks);
		break;
	}

	// Surf: shoreline ring for land; individual rings around rocks awash/stacks.
	if (I.bTerrain)
	{
		TArray<FVector> Ring;
		const int32 N = 160;
		for (int32 k = 0; k < N; ++k)
		{
			const float Th = 2.f * PI * k / N;
			const float Rs = I.RTheta(Th) * 1.005f;
			Ring.Add(FVector(I.C.X + FMath::Cos(Th) * Rs, I.C.Y + FMath::Sin(Th) * Rs, 0.f));
			if (k % 12 == 0) I.SplashPts.Add(FVector(Ring.Last().X, Ring.Last().Y, FMath::RadiansToDegrees(Th)));
		}
		BuildSurfRing(I, Ring, I.Kind == EIsle::Cay || I.Kind == EIsle::Wooded ? 1800.f : 1300.f, true);
	}
	SurfRocks.Append(PendingRockSurf); PendingRockSurf.Reset();
	for (const FVector& Rk : SurfRocks)
	{
		TArray<FVector> Ring;
		const int32 N = 20;
		const float P1 = FMath::FRandRange(0.f, 6.28f), P2 = FMath::FRandRange(0.f, 6.28f);
		for (int32 k = 0; k < N; ++k)
		{
			const float Th = 2.f * PI * k / N;
			const float Rr = Rk.Z * (1.f + 0.22f * FMath::Sin(2.f * Th + P1) + 0.14f * FMath::Sin(3.f * Th + P2));
			Ring.Add(FVector(Rk.X + FMath::Cos(Th) * Rr, Rk.Y + FMath::Sin(Th) * Rr, 0.f));
		}
		BuildSurfRing(I, Ring, FMath::Clamp(Rk.Z * 0.9f, 500.f, 1600.f), true);
	}
	FinishSurf(I);
	// Shallow shelf: turquoise/sand tint just under the sea surface 0-60 m out from every shore / rock group.
	{
		TArray<FVector> Cs;
		if (I.bTerrain) Cs.Add(FVector(I.C.X, I.C.Y, 0.f));
		else for (const FVector& Rk : SurfRocks) Cs.Add(FVector(Rk.X, Rk.Y, 0.f));
		if (I.Kind == EIsle::Reef || I.Kind == EIsle::Graveyard) { Cs.Reset(); Cs.Add(FVector(I.C.X, I.C.Y, 0.f)); }
		BuildShelf(I, Cs, 0.f, 0.f);
	}

	// 3D surf audio (attenuated ~1.5 km)
	if (SurfSnd)
	{
		USoundAttenuation* At = NewObject<USoundAttenuation>(this);
		At->Attenuation.bAttenuate = true;
		At->Attenuation.bSpatialize = true;
		At->Attenuation.AttenuationShapeExtents = FVector(FMath::Max(I.R * 0.8f, 6000.f), 0.f, 0.f);
		At->Attenuation.FalloffDistance = 150000.f;
		I.Audio = UGameplayStatics::SpawnSoundAtLocation(this, SurfSnd, I.C + FVector(0, 0, 300.f), FRotator::ZeroRotator, 1.4f, 1.f, Rng.FRandRange(0.f, 25.f), At, nullptr, false);
		if (I.Audio) Audios.Add(I.Audio);
	}
	UE_LOG(LogIronwake, Display, TEXT("IWI1 isle %s kind=%d at (%.0f, %.0f) m R=%.0f m peak=%.0f m surfVerts=%d splash=%d hazard=%.0f m"),
		*I.Name, (int32)I.Kind, I.C.X / CM, I.C.Y / CM, I.R / CM, I.Hp / CM, I.SurfXY.Num(), I.SplashPts.Num(), I.HazardR / CM);
}

void AIronwakeArchipelago::BuildLand(FIsle& I)
{
	// Polar grid mesh: rings out to rho 1.3 (seabed), vertex colour R = sand, G = macro variation, B = wet.
	const int32 NT = 128, NR = 44;
	TArray<FVector> V; TArray<int32> Tri; TArray<FVector> N; TArray<FVector2D> UV; TArray<FLinearColor> Col; TArray<FProcMeshTangent> Tan;
	auto RhoAt = [&](int32 j) { const float t = (float)j / NR; return 1.3f * FMath::Pow(t, 0.75f); };
	V.Add(FVector(0, 0, I.Height(0, 0)));
	for (int32 j = 1; j <= NR; ++j)
		for (int32 i = 0; i < NT; ++i)
		{
			const float Th = 2.f * PI * i / NT;
			const float Rr = RhoAt(j) * I.RTheta(Th);
			const float X = FMath::Cos(Th) * Rr, Y = FMath::Sin(Th) * Rr;
			V.Add(FVector(X, Y, I.Height(X, Y)));
		}
	auto Idx = [&](int32 j, int32 i) { return j == 0 ? 0 : 1 + (j - 1) * NT + (i % NT); };
	for (int32 i = 0; i < NT; ++i) Tri.Append({ 0, Idx(1, i + 1), Idx(1, i) });
	for (int32 j = 1; j < NR; ++j)
		for (int32 i = 0; i < NT; ++i)
		{
			const int32 A = Idx(j, i), B = Idx(j, i + 1), C = Idx(j + 1, i), D = Idx(j + 1, i + 1);
			Tri.Append({ A, B, C, B, D, C });
		}
	N.SetNum(V.Num()); UV.SetNum(V.Num()); Col.SetNum(V.Num()); Tan.SetNum(V.Num());
	for (int32 k = 0; k < V.Num(); ++k)
	{
		const FVector& P = V[k];
		const float E = 150.f;
		const float Dx = I.Height(P.X + E, P.Y) - I.Height(P.X - E, P.Y), Dy = I.Height(P.X, P.Y + E) - I.Height(P.X, P.Y - E);
		N[k] = FVector(-Dx, -Dy, 2.f * E).GetSafeNormal();
		UV[k] = FVector2D((I.C.X + P.X) / 2400.f, (I.C.Y + P.Y) / 2400.f);
		const float Slope = 1.f - N[k].Z;
		const float Th = FMath::Atan2(P.Y, P.X);
		const float Rho = FVector2D(P.X, P.Y).Size() / FMath::Max(I.RTheta(Th), 1.f);
		float Sand = Smoothx(0.72f, 0.88f, Rho);
		if (I.Kind == EIsle::Cay) Sand = FMath::Max(Sand, 0.75f + 0.25f * Smoothx(0.4f, 0.8f, Rho));
		if (I.Kind == EIsle::Wooded)
		{
			const float D = FMath::Abs(FMath::FindDeltaAngleRadians(Th, FMath::DegreesToRadians(I.CoveYaw)));
			Sand = FMath::Max(Sand, Smoothx(0.62f, 0.75f, Rho) * (1.f - Smoothx(0.5f, 0.9f, D)));
		}
		if (I.bPlateau && I.Kind != EIsle::Fort) Sand *= 0.6f; // shingle not sand under cliffs
		Sand = FMath::Max(Sand, Smoothx(0.25f, 0.6f, Slope) * 0.0f);
		const float Var = 0.5f + 0.5f * FMath::Sin((I.C.X + P.X) / 2300.f) * FMath::Cos((I.C.Y + P.Y) / 1900.f);
		const float Wet = Smoothx(260.f, 40.f, P.Z);
		Col[k] = FLinearColor(Sand, Var, Wet, 1.f);
		Tan[k] = FProcMeshTangent(FVector::CrossProduct(N[k], FVector(0, 1, 0)).GetSafeNormal(), false);
	}
	UProceduralMeshComponent* PM = NewObject<UProceduralMeshComponent>(this);
	PM->SetupAttachment(RootComponent);
	PM->RegisterComponent();
	AddInstanceComponent(PM);
	PM->SetWorldLocation(I.C);
	PM->bUseAsyncCooking = true;
	PM->CreateMeshSection_LinearColor(0, V, Tri, N, UV, Col, Tan, true, false);
	if (GroundMat) PM->SetMaterial(0, GroundMat);
	PM->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PM->SetCollisionResponseToAllChannels(ECR_Block);
	Lands.Add(PM);
}

void AIronwakeArchipelago::RockRing(FIsle& I, FRandomStream& Rng, float RhoIn, float RhoOut, float Every, float MinS, float MaxS, float SkipCove)
{
	// Scan rocks along the waterline (rho ~1): sunk so they rise from the sea, never float.
	const float Perim = 2.f * PI * I.R;
	const int32 N = FMath::Max(8, (int32)(Perim / Every));
	for (int32 k = 0; k < N; ++k)
	{
		const float Th = 2.f * PI * (k + Rng.FRandRange(-0.3f, 0.3f)) / N;
		if (SkipCove > 0.f && FMath::Abs(FMath::FindDeltaAngleRadians(Th, FMath::DegreesToRadians(I.CoveYaw))) < 0.55f * SkipCove) continue;
		const float Rho = Rng.FRandRange(RhoIn, RhoOut);
		const float Rr = I.RTheta(Th) * Rho;
		const FVector L(FMath::Cos(Th) * Rr, FMath::Sin(Th) * Rr, 0.f);
		const float S = Rng.FRandRange(MinS, MaxS);
		const TCHAR* Mn = RockNames[Rng.RandRange(0, 5)];
		const float Ground = I.bTerrain ? FMath::Max(I.Height(L.X, L.Y), -250.f) : -150.f;
		Inst(Mn, I.C + L + FVector(0, 0, Ground - S * 55.f), FRotator(Rng.FRandRange(-10.f, 10.f), Rng.FRandRange(0.f, 360.f), Rng.FRandRange(-10.f, 10.f)), FVector(S, S, S * Rng.FRandRange(0.6f, 1.0f)));
		if (Rho > 0.99f && (k % 2 == 0)) PendingRockSurf.Add(FVector(I.C.X + L.X, I.C.Y + L.Y, S * 160.f + 300.f));
	}
}

void AIronwakeArchipelago::CliffRing(FIsle& I, FRandomStream& Rng, int32 N, float Rho)
{
	// Foot cliffs: E1 coastal-cliff / namaqualand scans facing outward (sheer face = UE +Y, so yaw = outward - 90).
	// Round 1 stretched them up to 8x vertically into grey walls (vision 1-2.5/10): now near-uniform scale, <= ~22 m tall.
	const TCHAR* Cl[] = { TEXT("CoastalCliff04"), TEXT("CoastalCliff01"), TEXT("NamaCliff02"), TEXT("CoastalCliff02") };
	const float Off = Rng.FRandRange(0.f, 360.f);
	for (int32 k = 0; k < N; ++k)
	{
		const float Ph = Off + 360.f * k / N + Rng.FRandRange(-8.f, 8.f);
		const float Th = FMath::DegreesToRadians(Ph);
		const float Rr = I.RTheta(Th) * Rho;
		const TCHAR* Mn = Cl[(k + (int32)Off) % 4];
		const bool bNama = FCString::Strcmp(Mn, TEXT("NamaCliff02")) == 0;
		const float NatH = bNama ? 718.f : 1050.f, NatW = bNama ? 2022.f : 7000.f;
		const float Want = FMath::Clamp(FMath::Max(I.Height(FMath::Cos(Th) * Rr * 0.85f, FMath::Sin(Th) * Rr * 0.85f), 600.f) + 700.f, 900.f, 2400.f);
		const float Zs = FMath::Clamp(Want / NatH, 0.8f, bNama ? 3.2f : 2.3f);
		const float Ws = FMath::Clamp((2.f * PI * Rr / N) / NatW * 1.2f, Zs * 0.7f, Zs * 1.6f);
		Inst(Mn, I.C + FVector(FMath::Cos(Th) * Rr, FMath::Sin(Th) * Rr, -600.f), FRotator(0.f, Ph - 90.f, Rng.FRandRange(-3.f, 3.f)), FVector(Ws, Ws * 0.9f, Zs));
	}
}

void AIronwakeArchipelago::Crag(FIsle& I, FRandomStream& Rng, float TopM)
{
	// St Brannoc recipe (E1, 7.5/10): big boulder_01 scans (1.27 x 1.83 x 1.0 m) scaled to the island and stacked with
	// varied yaw; the main one has query collision so the lighthouse can find the rock top by trace.
	const float D = I.R * 1.45f / 183.f; // across
	UStaticMeshComponent* Main = IronwakeWorld::Place(this, RootComponent, TEXT("Boulder01"), FVector::ZeroVector, FRotator::ZeroRotator, FVector(D * 1.35f, D, (TopM * CM + 1500.f) / 100.f), true);
	if (Main) { Main->SetWorldLocationAndRotation(I.C + FVector(0, 0, -1500.f), FRotator(3.f, Rng.FRandRange(0.f, 360.f), -2.f)); ++NumPieces; }
	for (int32 k = 0; k < 3; ++k)
	{
		const FVector L = PolarV(Rng.FRandRange(0.f, 360.f), I.R * Rng.FRandRange(0.25f, 0.45f));
		Inst(TEXT("Boulder01"), I.C + L + FVector(0, 0, -600.f), FRotator(Rng.FRandRange(-6.f, 6.f), Rng.FRandRange(0.f, 360.f), Rng.FRandRange(-5.f, 5.f)),
			FVector(D * Rng.FRandRange(0.45f, 0.65f), D * Rng.FRandRange(0.35f, 0.5f), (TopM * Rng.FRandRange(0.5f, 0.8f) * CM + 600.f) / 100.f));
	}
	// rock-top: trace for the lighthouse / framing
	float Top = I.C.Z + TopM * CM * 0.8f;
	if (Main)
	{
		FHitResult Hit; FCollisionQueryParams Q(TEXT("IWI1Crag"), true);
		const FVector From = I.C + FVector(0, 0, 30000.f);
		if (Main->LineTraceComponent(Hit, From, From - FVector(0, 0, 40000.f), Q)) Top = Hit.ImpactPoint.Z;
	}
	I.Hp = FMath::Max(I.Hp, Top - I.C.Z);
	I.CragTop = Top;
	I.LookAt = FVector(I.C.X, I.C.Y, Top * 0.6f + I.C.Z * 0.4f);
}

void AIronwakeArchipelago::Outcrops(FIsle& I, FRandomStream& Rng, int32 N)
{
	// Rock outcrops breaking the wooded hillside silhouette (boulder scans, uniform-ish scale).
	for (int32 k = 0; k < N; ++k)
	{
		const float Th = Rng.FRandRange(0.f, 2.f * PI);
		if (FMath::Abs(FMath::FindDeltaAngleRadians(Th, FMath::DegreesToRadians(I.CoveYaw))) < 0.9f) continue;
		const float Rr = I.RTheta(Th) * Rng.FRandRange(0.25f, 0.75f);
		const float X = FMath::Cos(Th) * Rr, Y = FMath::Sin(Th) * Rr;
		const float S = Rng.FRandRange(9.f, 18.f);
		Inst(TEXT("Boulder01"), I.C + FVector(X, Y, I.Height(X, Y) - S * 40.f), FRotator(Rng.FRandRange(-10.f, 10.f), Rng.FRandRange(0.f, 360.f), Rng.FRandRange(-10.f, 10.f)), FVector(S, S * 0.8f, S * 0.7f));
	}
}

void AIronwakeArchipelago::Stack(FIsle& I, FRandomStream& Rng, const FVector2D& L, float Hm, float Wd, TArray<FVector>* SurfRocks)
{
	// E1 'Needles' recipe: 3 boulder_01 scans stretched tall and leaning (7.5/10), awash shelf at the foot.
	const FVector B = I.C + FVector(L.X, L.Y, 0.f);
	const float Hcm = Hm * CM, A0 = Rng.FRandRange(0.f, 360.f);
	Inst(TEXT("Boulder01"), B + FVector(0, 0, -800.f), FRotator(Rng.FRandRange(-4.f, 4.f), A0, Rng.FRandRange(-4.f, 4.f)), FVector(20.f * Wd, 15.f * Wd, (0.45f * Hcm + 800.f) / 100.f));
	Inst(TEXT("Boulder01"), B + FVector(Rng.FRandRange(-300.f, 300.f), Rng.FRandRange(-300.f, 300.f), 0.28f * Hcm), FRotator(Rng.FRandRange(-6.f, 6.f), A0 + 70.f, Rng.FRandRange(-6.f, 6.f)), FVector(14.f * Wd, 11.f * Wd, 0.5f * Hcm / 100.f));
	Inst(TEXT("Boulder01"), B + FVector(Rng.FRandRange(-400.f, 400.f), Rng.FRandRange(-400.f, 400.f), 0.62f * Hcm), FRotator(Rng.FRandRange(-9.f, 9.f), A0 + 150.f, Rng.FRandRange(-9.f, 9.f)), FVector(8.f * Wd, 6.5f * Wd, 0.4f * Hcm / 100.f));
	Inst(TEXT("CoastLandRocks03"), B + FVector(900.f, -600.f, -150.f), FRotator(0.f, A0 * 2.f, 0.f), FVector(3.5f * Wd, 3.5f * Wd, 4.f));
	Inst(TEXT("CoastLandRocks02"), B + FVector(-800.f, 700.f, -150.f), FRotator(0.f, A0 * 3.f, 0.f), FVector(3.0f * Wd, 2.5f * Wd, 4.f));
	if (SurfRocks) SurfRocks->Add(FVector(B.X, B.Y, 1500.f * Wd));
	I.SplashPts.Add(FVector(B.X + 1500.f * Wd, B.Y, 0.f));
}

void AIronwakeArchipelago::ScatterVeg(FIsle& I, FRandomStream& Rng)
{
	// Vegetation by height and slope: palms/olive (island_tree) on cays and low ground, firs on high ground, shrubs/grass
	// everywhere above the beach. Cull distances keep the far islands silhouettes only.
	const float Area = PI * I.R * I.R;
	const int32 Tries = FMath::Clamp((int32)(Area / (I.Kind == EIsle::Wooded ? 900000.f : 1300000.f)), 30, I.Kind == EIsle::Wooded ? 1000 : 420);
	int32 Placed = 0;
	for (int32 t = 0; t < Tries; ++t)
	{
		const float Th = Rng.FRandRange(0.f, 2.f * PI);
		const float Rho = FMath::Sqrt(Rng.FRand()) * 0.86f;
		const float Rr = Rho * I.RTheta(Th);
		const float X = FMath::Cos(Th) * Rr, Y = FMath::Sin(Th) * Rr;
		const float Z = I.Height(X, Y);
		if (Z < 220.f) continue; // above the beach
		const float E = 300.f;
		const float Slope = FVector2D(I.Height(X + E, Y) - I.Height(X - E, Y), I.Height(X, Y + E) - I.Height(X, Y - E)).Size() / (2.f * E);
		if (Slope > 0.75f) continue;
		const FVector P = I.C + FVector(X, Y, Z - 30.f);
		const float Yaw = Rng.FRandRange(0.f, 360.f);
		const float Hn = Z / FMath::Max(I.Hp, 1.f);
		// clumping: woods where a low-frequency field is high, clearings (grass/shrub) where it is low
		const float Field = 0.5f + 0.5f * FMath::Sin((I.C.X + X) / 4200.f + I.HPh[2]) * FMath::Cos((I.C.Y + Y) / 3300.f + I.HPh[3]);
		const float Rv = Rng.FRand() * (I.Kind == EIsle::Cay ? 1.f : FMath::Lerp(1.6f, 0.55f, Field));
		const bool bLow = I.Kind == EIsle::Cay || (I.Kind == EIsle::Wooded && Hn < 0.25f);
		const bool bConifer = I.Kind == EIsle::Pines || (I.Kind == EIsle::Wooded && Hn > 0.3f) || I.Kind == EIsle::Cliff;
		if (Slope < 0.45f && Rv < (bLow ? 0.35f : 0.42f))
		{
			if (bConifer && !bLow && (I.Kind != EIsle::Pines || Rng.FRand() < 0.75f))
				Inst(TEXT("FirTree01"), P, FRotator(Rng.FRandRange(-3.f, 3.f), Yaw, 0.f), FVector(Rng.FRandRange(0.55f, 1.3f)), 900.f, 1400.f, true, TEXT("tree"));
			else
			{
				const TCHAR* Tn[] = { TEXT("IslandTree01"), TEXT("IslandTree02"), TEXT("IslandTree03") };
				Inst(Tn[Rng.RandRange(0, 2)], P, FRotator(0.f, Yaw, 0.f), FVector(Rng.FRandRange(1.6f, 2.6f)), 800.f, 1200.f, true, TEXT("tree"));
			}
		}
		else if (Rv < 0.75f)
			Inst(Rng.FRand() < 0.5f ? TEXT("Shrub02") : TEXT("Shrub03"), P, FRotator(0.f, Yaw, 0.f), FVector(Rng.FRandRange(1.4f, 3.0f)), 300.f, 450.f, false, TEXT("shrub"));
		else if (Rv < 0.9f)
			Inst(TEXT("GrassMedium01"), P, FRotator(0.f, Yaw, 0.f), FVector(Rng.FRandRange(2.f, 3.5f)), 180.f, 280.f, false, TEXT("grass"));
		else
		{
			const TCHAR* Dn[] = { TEXT("DeadTreeTrunk02"), TEXT("TreeStump01"), TEXT("RockMossSet01"), TEXT("SandRocksSmall01") };
			const int32 Di = Rng.RandRange(0, 3);
			Inst(Dn[Di], P + FVector(0, 0, -30.f), FRotator(0.f, Yaw, 0.f), FVector(Di >= 2 ? Rng.FRandRange(1.f, 2.f) : Rng.FRandRange(1.3f, 2.2f)), 600.f, 1000.f, Di >= 2, TEXT("debris"));
		}
		++Placed;
	}
	// beach driftwood + pebble patches on cays / coves
	if (I.Kind == EIsle::Cay || I.Kind == EIsle::Wooded)
		for (int32 k = 0; k < (I.Kind == EIsle::Cay ? 14 : 10); ++k)
		{
			const float Th = I.Kind == EIsle::Cay ? Rng.FRandRange(0.f, 2.f * PI) : FMath::DegreesToRadians(I.CoveYaw + Rng.FRandRange(-35.f, 35.f));
			const float Rr = I.RTheta(Th) * Rng.FRandRange(0.84f, 0.95f);
			const float X = FMath::Cos(Th) * Rr, Y = FMath::Sin(Th) * Rr;
			Inst(k % 2 ? TEXT("SandRocksSmall01") : TEXT("DeadTreeTrunk02"), I.C + FVector(X, Y, I.Height(X, Y) - 15.f), FRotator(0.f, Rng.FRandRange(0.f, 360.f), 0.f), FVector(Rng.FRandRange(1.2f, 2.f)), 500.f, 900.f, false, TEXT("debris"));
		}
	UE_LOG(LogIronwake, Display, TEXT("IWI1 veg %s: %d placed of %d tries"), *I.Name, Placed, Tries);
}

void AIronwakeArchipelago::BuildVillage(FIsle& I, FRandomStream& Rng)
{
	// Small fishing village at the head of the cove: E1 harbour buildings (scaled down), a short timber pier, boats.
	USceneComponent* Root = RootComponent;
	const float Cy = I.CoveYaw;
	const FVector Out = FRotator(0.f, Cy, 0.f).Vector(), Side(-Out.Y, Out.X, 0.f);
	const float Th = FMath::DegreesToRadians(Cy);
	const float Rs = I.RTheta(Th);
	auto At = [&](float Rho, float Lat) { const FVector L = Out * Rs * Rho + Side * Lat; return FVector(L.X, L.Y, I.Height(L.X, L.Y)); };
	// buildings on the low ground behind the beach
	struct FB { const TCHAR* N; float Rho; float Lat; float Yaw; float S; };
	const FB Bs[] = { { TEXT("WharfBuilding"), 0.66f, -4500.f, 0.f, 0.42f }, { TEXT("DockWarehouse"), 0.64f, 3800.f, 90.f, 0.45f }, { TEXT("WharfBuilding"), 0.58f, 500.f, 180.f, 0.36f } };
	for (const FB& B : Bs)
	{
		const FVector L = At(B.Rho, B.Lat);
		Piece(Root, B.N, I.C + L + FVector(0, 0, -60.f), FRotator(0.f, Cy + 180.f + B.Yaw, 0.f), FVector(B.S));
	}
	I.Village = I.C + At(0.62f, 0.f);
	// pier from the beach into the cove: modular pier (22.9 m sections at scale 1.5 in E1 -> here 1.0 = 15.3 m)
	const FVector P0 = At(0.86f, 0.f);
	for (int32 k = 0; k < 4; ++k)
	{
		const FVector L = I.C + FVector(P0.X, P0.Y, 0.f) + Out * (k * 1525.f);
		Piece(Root, TEXT("ModularPier"), FVector(L.X, L.Y, I.C.Z - 362.f), FRotator(0.f, Cy + 90.f, 0.f), FVector(1.0f));
	}
	// boats moored off the pier + hauled up on the beach
	auto Float = [&](const TCHAR* N, const FVector& W, float Yaw, float Keel, float S)
	{
		if (USceneComponent* C = Piece(Root, N, W, FRotator(0.f, Yaw, 0.f), FVector(S))) { Floats.Add(C); FloatBase.Add(FVector(Keel, Yaw, Floats.Num() * 1.3f)); }
	};
	const FVector Pe = I.C + FVector(P0.X, P0.Y, 0.f) + Out * 5000.f;
	Float(TEXT("Rowboat"), Pe + Side * 700.f, Cy + 80.f, -35.f, 1.f);
	Float(TEXT("Fourareen"), Pe - Side * 750.f + Out * 600.f, Cy + 100.f, -40.f, 1.f);
	Float(TEXT("Rowboat"), Pe + Side * 1500.f - Out * 900.f, Cy + 60.f, -35.f, 1.f);
	Float(TEXT("SteamPinnace"), Pe + Out * 2400.f + Side * 300.f, Cy + 90.f, -100.f, 1.f);
	{
		const FVector Bb = At(0.9f, -2500.f);
		Piece(Root, TEXT("Rowboat"), I.C + Bb + FVector(0, 0, 10.f), FRotator(0.f, Cy + 20.f, 8.f), FVector(1.f));
		const FVector Bc = At(0.89f, 2200.f);
		Piece(Root, TEXT("Fourareen"), I.C + Bc + FVector(0, 0, 5.f), FRotator(0.f, Cy - 30.f, -6.f), FVector(1.f));
	}
	// props + lamps
	for (int32 k = 0; k < 6; ++k)
	{
		const FVector L = At(0.78f, Rng.FRandRange(-3500.f, 3500.f));
		Piece(Root, k % 2 ? TEXT("Barrel03") : TEXT("MilitaryCrate"), I.C + L, FRotator(0.f, Rng.FRandRange(0.f, 360.f), 0.f), FVector(k % 2 ? 1.f : 1.4f));
	}
	if (AIronwakeVoyage* Vy = AIronwakeVoyage::Get(GetWorld()))
		for (int32 k = 0; k < 3; ++k)
		{
			const FVector L = I.C + At(0.74f, -3000.f + 3000.f * k);
			Piece(Root, TEXT("GasLamp"), L, FRotator::ZeroRotator, FVector(1.f));
			UPointLightComponent* Lp = NewObject<UPointLightComponent>(Vy);
			Lp->SetupAttachment(Vy->GetRootComponent());
			Lp->SetIntensityUnits(ELightUnits::Candelas); Lp->SetIntensity(0.f);
			Lp->SetAttenuationRadius(9000.f); Lp->SetLightColor(FLinearColor(1.f, 0.72f, 0.4f)); Lp->SetCastShadows(false);
			Lp->RegisterComponent(); Vy->AddInstanceComponent(Lp);
			Lp->SetWorldLocation(L + FVector(0, 0, 420.f));
			Lp->ComponentTags.Add(TEXT("cd=9000"));
			Vy->NightLamps.Add(Lp);
		}
	I.LookAt = I.C + At(0.75f, 0.f) + FVector(0, 0, 600.f);
}

void AIronwakeArchipelago::BuildFort(FIsle& I, FRandomStream& Rng)
{
	// Ruined star-ish fort on the table top: Poly Haven modular fort pieces (CC0), some sections missing/tumbled.
	USceneComponent* Root = RootComponent;
	const float Top = I.Hp;
	const float A0 = Rng.FRandRange(0.f, 90.f);
	const FRotator Ry(0.f, A0, 0.f);
	auto W = [&](float X, float Y, float Z) { return I.C + Ry.RotateVector(FVector(X * CM, Y * CM, 0.f)) + FVector(0, 0, Z); };
	// square curtain 44 m: 3 wall sections of 14.6 m per side, corner pieces, one gate, two round towers
	const float H = 22.f;
	int32 Missing = 0;
	for (int32 s = 0; s < 4; ++s)
	{
		const float Ys = A0 + 90.f * s;
		for (int32 k = -1; k <= 1; ++k)
		{
			const FVector L = FRotator(0.f, 90.f * s, 0.f).RotateVector(FVector(H, k * 14.6f, 0.f));
			const bool bGate = (s == 0 && k == 0);
			const bool bGone = !bGate && Rng.FRand() < 0.22f;
			if (bGone)
			{
				// tumbled section: rubble boulders + a fallen piece tilted
				++Missing;
				Piece(Root, TEXT("FortWallThin"), W(L.X, L.Y, 0) + FVector(0, 0, Top - 350.f), FRotator(Rng.FRandRange(-25.f, 25.f), Ys, Rng.FRandRange(30.f, 70.f)), FVector(1.f, 0.5f, 0.6f));
				for (int32 r = 0; r < 3; ++r) Inst(TEXT("RockMossSet01"), W(L.X, L.Y, 0) + Rng.VRand() * FVector(400, 400, 0) + FVector(0, 0, Top - 60.f), FRotator(0, Rng.FRandRange(0, 360), 0), FVector(0.8f));
				continue;
			}
			Piece(Root, bGate ? TEXT("FortGate") : (k == 0 ? TEXT("FortWallThick") : TEXT("FortWallThin")), W(L.X, L.Y, 0) + FVector(0, 0, Top - 80.f), FRotator(0.f, Ys, 0.f), FVector(1.f, 1.f, bGate ? 1.f : Rng.FRandRange(0.75f, 1.0f)));
			if (bGate)
				for (int32 g = -1; g <= 1; g += 2)
					Piece(Root, TEXT("FortWallThin"), W(L.X, L.Y, 0) + FRotator(0.f, Ys, 0.f).RotateVector(FVector(0, g * 5.5f * CM, 0)) + FVector(0, 0, Top - 80.f), FRotator(0.f, Ys, 0.f), FVector(1.f, 0.25f, 1.f));
		}
		Piece(Root, s % 2 ? TEXT("FortTower") : TEXT("FortCorner"), W(FRotator(0.f, 90.f * s, 0.f).RotateVector(FVector(H, H, 0)).X, FRotator(0.f, 90.f * s, 0.f).RotateVector(FVector(H, H, 0)).Y, 0) + FVector(0, 0, Top - 100.f),
			FRotator(0.f, Ys, 0.f), FVector(s % 2 ? 0.85f : 1.2f, s % 2 ? 0.85f : 1.2f, s % 2 ? 1.f : 1.f));
	}
	// guns on the seaward wall (Poly Haven cannon is not imported: use the E1 sea mine as a 'shot pile' prop + crates)
	for (int32 k = 0; k < 4; ++k)
		Piece(Root, k % 2 ? TEXT("WoodenBarrels") : TEXT("MilitaryCrate"), W(-12.f + 7.f * k, -14.f, 0) + FVector(0, 0, Top - 20.f), FRotator(0.f, Rng.FRandRange(0, 360), 0.f), FVector(1.f));
	I.LookAt = W(0, 0, 0) + FVector(0, 0, Top + 400.f);
	UE_LOG(LogIronwake, Display, TEXT("IWI1 fort on %s: %d sections tumbled, yaw %.0f"), *I.Name, Missing, A0);
}

void AIronwakeArchipelago::BuildLighthouse(FIsle& I)
{
	// Same S1 primitive tower as St Brannoc (lime/oxblood), smaller lamp; its beacon pulses at night via NightLamps.
	USceneComponent* Root = RootComponent;
	const FVector B = FVector(I.C.X, I.C.Y, (I.CragTop > -1e8f ? I.CragTop : I.C.Z + I.Hp) - 150.f);
	const FLinearColor Lime(0.55f, 0.55f, 0.52f), Ox(0.17f, 0.03f, 0.03f), Iron(0.03f, 0.033f, 0.036f);
	if (CylMesh && ConeMesh)
	{
		if (UStaticMeshComponent* Tw = Shape(Root, CylMesh, B + FVector(0, 0, 1600.f), FRotator::ZeroRotator, FVector(6.5f, 6.5f, 32.f), Lime))
			if (UIronwakeColl* Co = UIronwakeColl::Get(GetWorld())) Co->AddComponent(Tw, TEXT("lighthouse"), true, 100.f);
		Shape(Root, CylMesh, B + FVector(0, 0, 2100.f), FRotator::ZeroRotator, FVector(6.7f, 6.7f, 4.f), Ox);
		Shape(Root, CylMesh, B + FVector(0, 0, 3230.f), FRotator::ZeroRotator, FVector(9.f, 9.f, 0.6f), Iron);
		Shape(Root, CylMesh, B + FVector(0, 0, 3450.f), FRotator::ZeroRotator, FVector(4.5f, 4.5f, 3.6f), FLinearColor(3.f, 2.4f, 1.4f));
		Shape(Root, ConeMesh, B + FVector(0, 0, 3800.f), FRotator::ZeroRotator, FVector(6.f, 6.f, 3.f), Ox);
		Piece(Root, TEXT("WharfBuilding"), B + FVector(1800.f, 900.f, -40.f), FRotator(0.f, 30.f, 0.f), FVector(0.22f));
	}
	if (AIronwakeVoyage* Vy = AIronwakeVoyage::Get(GetWorld()))
	{
		UPointLightComponent* Lp = NewObject<UPointLightComponent>(Vy);
		Lp->SetupAttachment(Vy->GetRootComponent());
		Lp->SetIntensityUnits(ELightUnits::Candelas); Lp->SetIntensity(0.f);
		Lp->SetAttenuationRadius(60000.f); Lp->SetLightColor(FLinearColor(1.f, 0.85f, 0.6f)); Lp->SetCastShadows(false);
		Lp->RegisterComponent(); Vy->AddInstanceComponent(Lp);
		Lp->SetWorldLocation(B + FVector(0, 0, 3450.f));
		Lp->ComponentTags.Add(TEXT("cd=80000"));
		Vy->NightLamps.Add(Lp);
	}
	I.LookAt = B + FVector(0, 0, 500.f);
}

void AIronwakeArchipelago::BuildGraveyard(FIsle& I, FRandomStream& Rng, TArray<FVector>& SurfRocks)
{
	// Wreck graveyard on a rocky shoal: E1 wreck stern x3 (scaled/tilted variously), the torpedo boat on its beam ends,
	// broken masts and debris riding the swell.
	USceneComponent* Root = RootComponent;
	for (int32 k = 0; k < 34; ++k)
	{
		const FVector P = I.C + PolarV(Rng.FRandRange(0.f, 360.f), FMath::Sqrt(Rng.FRand()) * I.R);
		const float S = Rng.FRandRange(2.5f, 5.f);
		if (k % 4 == 0) I.SplashPts.Add(FVector(P.X, P.Y, 0.f));
		Inst(RockNames[Rng.RandRange(0, 5)], P + FVector(0, 0, -S * 80.f), FRotator(Rng.FRandRange(-8.f, 8.f), Rng.FRandRange(0.f, 360.f), 0.f), FVector(S, S, S * 0.6f));
		SurfRocks.Add(FVector(P.X, P.Y, S * 250.f + 500.f));
	}
	struct FW { FVector2D L; float Yaw, Pitch, Roll, S, Z; };
	const FW Ws[] = { { FVector2D(0.f, 0.f), 30.f, 22.f, -18.f, 7.5f, -700.f }, { FVector2D(0.55f, 0.3f), 200.f, -14.f, 28.f, 6.f, -600.f }, { FVector2D(-0.45f, 0.5f), 120.f, 30.f, 8.f, 5.f, -450.f } };
	for (const FW& Wd : Ws)
	{
		const FVector P = I.C + FVector(Wd.L.X * I.R, Wd.L.Y * I.R, Wd.Z);
		Piece(Root, TEXT("WreckStern"), P, FRotator(Wd.Pitch, Wd.Yaw, Wd.Roll), FVector(Wd.S));
		SurfRocks.Add(FVector(P.X, P.Y, Wd.S * 320.f));
		I.SplashPts.Add(FVector(P.X, P.Y, Wd.Yaw));
	}
	Piece(Root, TEXT("TorpedoBoat"), I.C + FVector(-0.3f * I.R, -0.55f * I.R, -650.f), FRotator(8.f, 75.f, 62.f), FVector(1.f));
	SurfRocks.Add(FVector(I.C.X - 0.3f * I.R, I.C.Y - 0.55f * I.R, 2200.f));
	for (int32 k = 0; k < 14; ++k)
	{
		const FVector P = I.C + PolarV(Rng.FRandRange(0.f, 360.f), Rng.FRandRange(0.4f, 1.3f) * I.R);
		const bool bB = k % 3 != 0;
		if (USceneComponent* C = Piece(Root, bB ? TEXT("Barrel03") : TEXT("MilitaryCrate"), P, FRotator(bB ? 90.f : 0.f, Rng.FRandRange(0.f, 360.f), 0.f), FVector(2.3f)))
		{ Floats.Add(C); FloatBase.Add(FVector(bB ? 30.f : 15.f, Rng.FRandRange(0.f, 360.f), k * 1.7f)); }
	}
	I.LookAt = I.C + FVector(0, 0, 900.f);
	I.HazardR = I.R * 1.3f + 6000.f;
}

void AIronwakeArchipelago::BuildShelf(FIsle& I, const TArray<FVector>& Centres, float, float)
{
	// One flat disc-ring per centre: inner radius = shore (alpha 0.9), outer = shore + 60 m (alpha 0); colour turquoise near,
	// fading to the sea. Sits 30 cm under the mean surface so the swell covers/uncovers it; translucent unlit tint.
	if (!ShelfMat || Centres.Num() == 0) return;
	TArray<FVector> V; TArray<int32> Tri; TArray<FVector> N; TArray<FVector2D> UV; TArray<FLinearColor> Col; TArray<FProcMeshTangent> Tan;
	const int32 NT = 64;
	for (const FVector& Cc : Centres)
	{
		const bool bMain = I.bTerrain || Centres.Num() == 1;
		const float Base = V.Num();
		for (int32 r = 0; r < 3; ++r)
			for (int32 k = 0; k < NT; ++k)
			{
				const float Th = 2.f * PI * k / NT;
				float R0 = bMain ? (I.bTerrain ? I.RTheta(Th) * 0.97f : I.R * 1.05f) : 1500.f;
				const float Rr = r == 0 ? R0 * (bMain && !I.bTerrain ? 0.f : 1.f) : (r == 1 ? R0 + 2500.f : R0 + 6500.f);
				V.Add(FVector(Cc.X + FMath::Cos(Th) * Rr, Cc.Y + FMath::Sin(Th) * Rr, I.C.Z - 150.f)); // W3 fix: below the troughs, never a flat layer over the swell
				const float A = r == 0 ? 0.85f : (r == 1 ? 0.55f : 0.f);
				Col.Add(FLinearColor(r == 0 ? 1.25f : 1.f, r == 0 ? 1.2f : 1.f, r == 0 ? 1.0f : 1.f, A));
				N.Add(FVector::UpVector); UV.Add(FVector2D(0, 0)); Tan.Add(FProcMeshTangent(1, 0, 0));
			}
		for (int32 r = 0; r < 2; ++r)
			for (int32 k = 0; k < NT; ++k)
			{
				const int32 A = Base + r * NT + k, B = Base + r * NT + (k + 1) % NT, C = A + NT, D = B + NT;
				Tri.Append({ A, C, B, B, C, D });
			}
	}
	UProceduralMeshComponent* PM = NewObject<UProceduralMeshComponent>(this);
	PM->SetupAttachment(RootComponent);
	PM->RegisterComponent();
	AddInstanceComponent(PM);
	PM->SetWorldLocation(FVector::ZeroVector);
	PM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PM->SetCastShadow(false);
	PM->CreateMeshSection_LinearColor(0, V, Tri, N, UV, Col, Tan, false, false);
	PM->SetMaterial(0, ShelfMat);
	PM->SetTranslucentSortPriority(1);
	PM->SetCullDistance(300000.f);
}

void AIronwakeArchipelago::RockLandform(FIsle& I, FRandomStream& Rng, int32 N, float TopM)
{
	// Irregular landform from several overlapping rock scans scaled up and rotated (coastal cliffs / namaqualand cliff /
	// boulders), heights varied, so the silhouette is never a dome.
	const TCHAR* Ls[] = { TEXT("NamaCliff01"), TEXT("Boulder01"), TEXT("NamaCliff02"), TEXT("CoastalCliff04"), TEXT("Boulder01") };
	for (int32 k = 0; k < N; ++k)
	{
		const FVector2D L = k == 0 ? FVector2D(PolarV(Rng.FRandRange(0.f, 360.f), I.R * 0.15f)) : FVector2D(PolarV(k * 360.f / N + Rng.FRandRange(-20.f, 20.f), I.R * Rng.FRandRange(0.3f, 0.6f)));
		const TCHAR* Mn = Ls[k % 5];
		const float H = TopM * CM * Rng.FRandRange(0.45f, 1.0f) * (k == 0 ? 1.f : 0.8f);
		float Sx, Sz;
		if (FCString::Strcmp(Mn, TEXT("Boulder01")) == 0) { Sx = I.R * Rng.FRandRange(0.35f, 0.6f) / 130.f; Sz = (H + 800.f) / 100.f; }
		else if (FCString::Strcmp(Mn, TEXT("NamaCliff01")) == 0) { Sx = I.R * Rng.FRandRange(0.6f, 0.9f) / 830.f; Sz = (H + 800.f) / 496.f; }
		else if (FCString::Strcmp(Mn, TEXT("NamaCliff02")) == 0) { Sx = I.R * Rng.FRandRange(0.6f, 0.9f) / 2022.f; Sz = (H + 800.f) / 718.f; }
		else { Sx = I.R * Rng.FRandRange(0.5f, 0.8f) / 8700.f; Sz = (H + 800.f) / 1100.f; }
		const float Ground = I.Height(L.X, L.Y);
		Inst(Mn, I.C + FVector(L.X, L.Y, Ground - 800.f), FRotator(Rng.FRandRange(-5.f, 5.f), Rng.FRandRange(0.f, 360.f), Rng.FRandRange(-5.f, 5.f)),
			FVector(Sx, Sx * Rng.FRandRange(0.7f, 1.1f), Sz));
	}
	I.LookAt = I.C + FVector(0, 0, TopM * CM * 0.5f);
}

void AIronwakeArchipelago::Beaches(FIsle& I, FRandomStream& Rng)
{
	// Pebble/sand beach patches (sand_rocks_small scans) on the low coast just above the waterline.
	const int32 N = I.Kind == EIsle::Wooded ? 26 : 16;
	for (int32 k = 0; k < N; ++k)
	{
		float Th = Rng.FRandRange(0.f, 2.f * PI);
		if (I.Kind == EIsle::Wooded) Th = FMath::DegreesToRadians(I.CoveYaw + Rng.FRandRange(-50.f, 50.f));
		const float Rr = I.RTheta(Th) * Rng.FRandRange(0.9f, 1.0f);
		const float X = FMath::Cos(Th) * Rr, Y = FMath::Sin(Th) * Rr;
		const float S = Rng.FRandRange(3.f, 5.5f);
		Inst(TEXT("SandRocksSmall01"), I.C + FVector(X, Y, FMath::Max(I.Height(X, Y), -20.f) - 20.f), FRotator(0.f, Rng.FRandRange(0.f, 360.f), 0.f), FVector(S, S, 1.f), 700.f, 1200.f, false, TEXT("beach"));
	}
}

// ---------------------------------------------------------------------------- surf ribbons
void AIronwakeArchipelago::BuildSurfRing(FIsle& I, const TArray<FVector>& Ring, float Width, bool bClosed)
{
	// Each ring = 3 rows: inner edge (just inside the rock/shore, dense white), middle, outer (fades to 0).
	// Vertex colour R = density (1 at the rock), A = alpha (base x per-frame surge set in UpdateSurf).
	const int32 N = Ring.Num();
	FVector2D Ctr(0, 0);
	for (const FVector& P : Ring) Ctr += FVector2D(P.X, P.Y);
	Ctr /= N;
	const int32 Base = I.SurfXY.Num();
	const float Off[] = { -0.30f, 0.30f, 1.0f };
	const float Alpha[] = { 0.85f, 0.65f, 0.f };
	const float Dens[] = { 0.8f, 0.25f, 0.f };
	const float Ph1 = FMath::FRandRange(0.f, 6.28f), Ph2 = FMath::FRandRange(0.f, 6.28f);
	for (int32 r = 0; r < 3; ++r)
		for (int32 k = 0; k < N; ++k)
		{
			const FVector2D P(Ring[k].X, Ring[k].Y);
			const FVector2D O = (P - Ctr).GetSafeNormal();
			const float Tk = 2.f * PI * k / N;
			const float Wv = Width * (0.75f + 0.35f * FMath::Sin(5.f * Tk + Ph1) + 0.2f * FMath::Sin(11.f * Tk + Ph2));
			const FVector2D Q = P + O * Wv * Off[r];
			I.SurfXY.Add(FVector(Q.X, Q.Y, SurfLiftCm + (r == 0 ? 15.f : 0.f)));
			I.SurfA.Add(Alpha[r] * (0.55f + 0.45f * FMath::Abs(FMath::Sin(3.f * Tk + Ph2 + 0.7f * r))));
			I.SurfWind.Add(FMath::RadiansToDegrees(FMath::Atan2(O.Y, O.X)));
			I.SurfUV.Add(FVector2D(Q.X / 1800.f, Q.Y / 1800.f));
			I.SurfCol.Add(FLinearColor(Dens[r], 0.f, 0.f, Alpha[r]));
		}
	const int32 Segs = bClosed ? N : N - 1;
	for (int32 r = 0; r < 2; ++r)
		for (int32 k = 0; k < Segs; ++k)
		{
			const int32 A = Base + r * N + k, B = Base + r * N + (k + 1) % N, C = A + N, D = B + N;
			I.SurfTri.Append({ A, C, B, B, C, D });
		}
}

void AIronwakeArchipelago::FinishSurf(FIsle& I)
{
	if (I.SurfXY.Num() == 0) return;
	UProceduralMeshComponent* PM = NewObject<UProceduralMeshComponent>(this);
	PM->SetupAttachment(RootComponent);
	PM->RegisterComponent();
	AddInstanceComponent(PM);
	PM->SetWorldLocation(FVector::ZeroVector);
	PM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PM->SetCastShadow(false);
	PM->bUseAsyncCooking = true;
	const int32 Nv = I.SurfXY.Num();
	I.SurfVtx.SetNum(Nv); I.SurfNrm.Init(FVector::UpVector, Nv); I.SurfTan.Init(FProcMeshTangent(1, 0, 0), Nv);
	for (int32 k = 0; k < Nv; ++k) I.SurfVtx[k] = FVector(I.SurfXY[k].X, I.SurfXY[k].Y, I.C.Z + I.SurfXY[k].Z);
	PM->CreateMeshSection_LinearColor(0, I.SurfVtx, I.SurfTri, I.SurfNrm, I.SurfUV, I.SurfCol, I.SurfTan, false, false);
	if (SurfMat) PM->SetMaterial(0, SurfMat);
	PM->SetTranslucentSortPriority(2);
	I.Surf = PM;
	Surfs.Add(PM);
}

void AIronwakeArchipelago::UpdateSurf(FIsle& I, float Now)
{
	// Ride the analytic sea and surge: the swell travels along WaveTravelYaw; vertices facing into it get more white
	// water, pulsing with a ~9 s breaker cycle phased by position along the swell.
	if (!I.Surf) return;
	const float Tw = FMath::DegreesToRadians(IronwakeSea::WaveTravelYaw());
	const FVector2D Wd(FMath::Cos(Tw), FMath::Sin(Tw));
	const float Sea = FMath::Clamp(IronwakeSea::SigWaveHeightM() / 3.f, 0.35f, 1.6f);
	const float Z0 = SeaZ(I.C); (void)Z0;
	float PrevSea = Z0;
	const int32 Nv = I.SurfXY.Num();
	for (int32 k = 0; k < Nv; ++k)
	{
		const FVector& Q = I.SurfXY[k];
		// height: cheap = island-centre analytic + a local term from the dominant swell
		const float Ph = (Q.X * Wd.X + Q.Y * Wd.Y) / 12000.f * 2.f * PI - Now * 0.7f;
		const float Zl = (k % 2 == 0) ? SeaZ(FVector(Q.X, Q.Y, 0.f)) : PrevSea; // per-vertex analytic sea (odd verts reuse the neighbour's)
		PrevSea = Zl;
		I.SurfVtx[k].Z = Zl + Q.Z;
		const float Face = FMath::Cos(FMath::DegreesToRadians(I.SurfWind[k]) - Tw + PI); // +1 = faces the incoming swell
		const float Expo = 0.45f + 0.55f * FMath::Clamp(Face * 0.5f + 0.5f, 0.f, 1.f);
		const float Surge = 0.7f + 0.3f * FMath::Sin(Ph * 0.5f + Now * 0.35f);
		I.SurfCol[k].A = I.SurfA[k] * FMath::Clamp(Expo * Surge * (0.8f + 0.25f * Sea) * 1.25f, 0.f, 1.f);
	}
	I.Surf->UpdateMeshSection_LinearColor(0, I.SurfVtx, I.SurfNrm, I.SurfUV, I.SurfCol, I.SurfTan, false);
}

// ---------------------------------------------------------------------------- tick
void AIronwakeArchipelago::Tick(float Dt)
{
	Super::Tick(Dt);
	UWorld* W = GetWorld();
	const float Now = W->GetTimeSeconds();
	Dt = FMath::Min(Dt, 0.1f);
	const APlayerController* PCtl = W->GetFirstPlayerController();
	const FVector Cam = (PCtl && PCtl->PlayerCameraManager) ? PCtl->PlayerCameraManager->GetCameraLocation() : FVector::ZeroVector;
	const float Tw = IronwakeSea::WaveTravelYaw();
	for (FIsle& I : Isles)
	{
		const float D = FVector::Dist2D(Cam, I.C) - I.R;
		// Surf ribbons only within 2.5 km (beyond that they are sub-pixel); update at 30 Hz-ish budget by distance.
		const bool bNear = D < 180000.f;
		if (I.Surf && I.Surf->IsVisible() != bNear) I.Surf->SetVisibility(bNear);
		if (bNear) UpdateSurf(I, Now);
		// Splashes where the swell hits the windward shore (within 1.6 km of the camera).
		I.SplashT -= Dt;
		if (SplashFX && I.SplashPts.Num() > 0 && I.SplashT <= 0.f && D < 160000.f)
		{
			I.SplashT = FMath::FRandRange(0.8f, 1.8f) * (I.Kind == EIsle::Reef ? 0.5f : 1.f);
			for (int32 tries = 0; tries < 6; ++tries)
			{
				const FVector& Sp = I.SplashPts[(I.SplashN++) % I.SplashPts.Num()];
				const FVector2D O = FVector2D(Sp.X - I.C.X, Sp.Y - I.C.Y).GetSafeNormal();
				const float Face = FMath::Cos(FMath::Atan2(O.Y, O.X) - FMath::DegreesToRadians(Tw) + PI);
				if (Face < -0.2f && I.Kind != EIsle::Reef) continue;
				const FVector L(Sp.X, Sp.Y, SeaZ(Sp));
				if (UNiagaraComponent* Nc = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, SplashFX, L))
					Nc->SetWorldScale3D(FVector(I.Kind == EIsle::Reef ? 0.45f : 0.35f));
				break;
			}
		}
	}
	for (int32 i = 0; i < Floats.Num(); ++i)
	{
		USceneComponent* F = Floats[i];
		if (!F || !FloatBase.IsValidIndex(i)) continue;
		const FVector L = F->GetComponentLocation();
		if (FVector::Dist2D(L, Cam) > 200000.f) continue;
		const FVector& FB = FloatBase[i];
		const float Z = SeaZ(L) + FB.X;
		F->SetWorldLocation(FVector(L.X, L.Y, FMath::FInterpTo(L.Z, Z, Dt, 1.5f)));
		F->SetWorldRotation(FRotator(2.f * FMath::Sin(Now * 0.5f + FB.Z), FB.Y + 3.f * FMath::Sin(Now * 0.11f + FB.Z), 3.f * FMath::Sin(Now * 0.6f + FB.Z)));
	}
	if (!TestMode.IsEmpty()) TickTest(FMath::Min((float)FApp::GetDeltaTime(), 0.1f));
}

// ---------------------------------------------------------------------------- chart
void AIronwakeArchipelago::DrawChartLand(AHUD* H, UCanvas* Cv, TFunctionRef<FVector2D(const FVector&)> ToChart, float S, UFont* Font) const
{
	const FLinearColor Land(0.36f, 0.31f, 0.21f, 1.f), Sand(0.55f, 0.50f, 0.36f, 1.f), Shore(0.80f, 0.74f, 0.55f, 1.f), Reef(0.70f, 0.45f, 0.30f, 1.f), Ink(0.86f, 0.74f, 0.47f, 1.f);
	const AIronwakeVoyage* Vy = AIronwakeVoyage::Get(GetWorld());
	for (int32 IsleIdx = 0; IsleIdx < Isles.Num(); ++IsleIdx)
	{
		const FIsle& I = Isles[IsleIdx];
		if (HostedPortIsles.Contains(IsleIdx) && Vy)
		{
			int32 PIdx = -1;
			if (IwPorts::IsHostedPortIsle(IsleIdx, Vy->WorldPorts, &PIdx))
			{
				const FIwPortSpec& Ps = Vy->WorldPorts[PIdx];
				const float Hs = Ps.HillScale;
				const FVector2D Poly[6] = {
					ToChart(Ps.H2W(-48.f, -150.f * Hs, 0.f)),
					ToChart(Ps.H2W(-48.f,  150.f * Hs, 0.f)),
					ToChart(Ps.H2W(-140.f * Hs - 40.f,  195.f * Hs, 0.f)),
					ToChart(Ps.H2W(-225.f * Hs - 45.f,  110.f * Hs, 0.f)),
					ToChart(Ps.H2W(-225.f * Hs - 45.f, -110.f * Hs, 0.f)),
					ToChart(Ps.H2W(-140.f * Hs - 40.f, -195.f * Hs, 0.f)),
				};
				const FVector2D Ctr = ToChart(Ps.H2W(-125.f * Hs - 35.f, 0.f, 0.f));
				TArray<FCanvasUVTri> Tris;
				for (int32 k = 0; k < 6; ++k)
				{
					const FVector2D& Pa = Poly[k];
					const FVector2D& Pb = Poly[(k + 1) % 6];
					FCanvasUVTri T; T.V0_Pos = Ctr; T.V1_Pos = Pa; T.V2_Pos = Pb;
					T.V0_Color = T.V1_Color = T.V2_Color = Land;
					Tris.Add(T);
					H->DrawLine(Pa.X, Pa.Y, Pb.X, Pb.Y, Shore, 1.6f);
				}
				Cv->K2_DrawTriangle(nullptr, Tris);
				continue;
			}
		}
		const int32 N = 48;
		const FVector2D Cc = ToChart(I.C);
		if (I.bTerrain)
		{
			TArray<FCanvasUVTri> Tris;
			for (int32 k = 0; k < N; ++k)
			{
				const float A = 2.f * PI * k / N, B = 2.f * PI * (k + 1) / N;
				const FVector2D Pa = ToChart(I.C + FVector(FMath::Cos(A) * I.RTheta(A), FMath::Sin(A) * I.RTheta(A), 0.f));
				const FVector2D Pb = ToChart(I.C + FVector(FMath::Cos(B) * I.RTheta(B), FMath::Sin(B) * I.RTheta(B), 0.f));
				FCanvasUVTri T; T.V0_Pos = Cc; T.V1_Pos = Pa; T.V2_Pos = Pb;
				T.V0_Color = T.V1_Color = T.V2_Color = (I.Kind == EIsle::Cay) ? Sand : Land;
				Tris.Add(T);
				H->DrawLine(Pa.X, Pa.Y, Pb.X, Pb.Y, Shore, 1.5f);
			}
			Cv->K2_DrawTriangle(nullptr, Tris);
		}
		else
		{
			// rocks / reef / wrecks: dotted symbols
			for (int32 k = 0; k < 10; ++k)
			{
				const FVector2D P = ToChart(I.C + PolarV(k * 36.f + 11.f * (k % 3), I.R * (0.35f + 0.6f * ((k * 7) % 5) / 4.f)));
				H->DrawRect(I.Kind == EIsle::Reef ? Reef : Shore, P.X - 1.5f * S, P.Y - 1.5f * S, 3.f * S, 3.f * S);
			}
			if (I.Kind == EIsle::Graveyard) { H->DrawLine(Cc.X - 5 * S, Cc.Y, Cc.X + 5 * S, Cc.Y, Shore, 1.5f); H->DrawLine(Cc.X, Cc.Y - 4 * S, Cc.X, Cc.Y + 4 * S, Shore, 1.5f); }
		}
	}
	(void)Font; (void)Ink;
}

// ---------------------------------------------------------------------------- I1 test director
void AIronwakeArchipelago::FindBenchView()
{
	// A spot ~1.2 km from the nearest island with the most islands within 3.5 km in a 70 deg cone: '3+ islands in view'.
	int32 Best = -1; FVector BP = FVector::ZeroVector; float BY = 0.f;
	for (int32 a = 0; a < 36; ++a)
		for (const FIsle& I : Isles)
		{
			if (!I.bTerrain) continue;
			const FVector P = I.C + PolarV(a * 10.f, I.R + 120000.f);
			bool bClear = true;
			for (const FIsle& J : Isles) if (FVector::Dist2D(P, J.C) < J.HazardR) bClear = false;
			if (!bClear || FMath::Abs(P.X) > 360000.f || FMath::Abs(P.Y) > 360000.f) continue;
			for (int32 y = 0; y < 36; ++y)
			{
				const float Yaw = y * 10.f;
				const FVector F = FRotator(0.f, Yaw, 0.f).Vector();
				int32 Cnt = 0;
				for (const FIsle& J : Isles)
				{
					const FVector D = J.C - P; const float Dd = D.Size2D();
					if (Dd < 350000.f && FVector::DotProduct(D.GetSafeNormal2D(), F) > FMath::Cos(FMath::DegreesToRadians(35.f))) ++Cnt;
				}
				if (Cnt > Best) { Best = Cnt; BP = P; BY = Yaw; }
			}
		}
	BenchPos = BP; BenchYaw = BY; BenchCount = Best;
	UE_LOG(LogIronwake, Display, TEXT("IWI1 bench view at (%.0f, %.0f) m yaw %.0f: %d islands within 3.5 km in a 70 deg cone"), BP.X / CM, BP.Y / CM, BY, Best);
}

void AIronwakeArchipelago::TickTest(float RealDt)
{
	AIronwakeVoyage* Vy = AIronwakeVoyage::Get(GetWorld());
	AIronwakePlayerController* C = Vy ? Cast<AIronwakePlayerController>(GetWorld()->GetFirstPlayerController()) : nullptr;
	AIronwakeShipPawn* P = C ? Cast<AIronwakeShipPawn>(C->GetPawn()) : nullptr;
	if (!Vy || !C || !P) return;
	TT += RealDt;
	const float Since = TT - TMark;
	auto Place = [&](const FVector& L, float Yaw, int32 Tel)
	{
		P->SetActorLocationAndRotation(FVector(L.X, L.Y, P->GetActorLocation().Z), FRotator(0.f, Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
		if (P->HullBody) { P->HullBody->SetPhysicsLinearVelocity(FRotator(0.f, Yaw, 0.f).Vector() * (Tel > 2 ? 400.f : 0.f)); P->HullBody->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector); }
		P->IWTelegraph(Tel);
		P->IWRudder(0);
		P->ResetCameraToHorizon();
	};
	auto IsleOf = [&](EIsle K, int32 Nth = 0) -> const FIsle* { int32 n = 0; for (const FIsle& I : Isles) if (I.Kind == K && n++ == Nth) return &I; return nullptr; };
	if (TestMode.StartsWith(TEXT("bench")))
	{
		if (TStep == 0 && TT >= 3.f)
		{
			if (!C->bStarted) C->StartGame(true);
			FindBenchView();
			Place(BenchPos, BenchYaw, 2);
			TStep = 1; TMark = TT;
		}
		if (TStep == 1 && Since > 2.f && Since < 2.3f) P->ResetCameraToHorizon();
		if (TStep == 1 && Since > 6.f && !bShot) { C->IWShot(TEXT("I_bench_view")); bShot = true; }
		return;
	}
	// shots: list of (name, island, stand-off m, from-yaw offset, telegraph, photo?)
	struct FV { const TCHAR* Shot; const FIsle* I; float StandM; float FromYaw; int32 Tel; int32 Mode; };
	// Mode 0 = default chase camera on the ship placed StandM from LookAt; 1 = photo cam low over the sea toward LookAt;
	// 2 = chart overlay screenshot; 3 = photo cam at StandM, 60 m up
	static TArray<FV> Views;
	if (Views.Num() == 0)
	{
		const FIsle* W = IsleOf(EIsle::Wooded); const FIsle* St = IsleOf(EIsle::Stacks); const FIsle* Cy = IsleOf(EIsle::Cay);
		const FIsle* Ft = IsleOf(EIsle::Fort); const FIsle* Rf = IsleOf(EIsle::Reef); const FIsle* Lh = IsleOf(EIsle::Lighthouse);
		const FIsle* Gy = IsleOf(EIsle::Graveyard); const FIsle* Pn = IsleOf(EIsle::Pines); const FIsle* Cl = IsleOf(EIsle::Cliff); const FIsle* Rk = IsleOf(EIsle::Rocky);
		Views.Add({ TEXT("I_archipelago_chart"), nullptr, 0.f, 0.f, 2, 2 });
		Views.Add({ TEXT("I_cove"), W, 520.f, 0.f, 2, 0 });
		Views.Add({ TEXT("I_cove_photo"), W, 380.f, 0.f, 2, 1 });
		Views.Add({ TEXT("I_stacks"), St, 420.f, 40.f, 2, 0 });
		Views.Add({ TEXT("I_cay"), Cy, 330.f, 30.f, 2, 0 });
		Views.Add({ TEXT("I_fort"), Ft, 420.f, 200.f, 2, 0 });
		Views.Add({ TEXT("I_fort_photo"), Ft, 300.f, 200.f, 2, 3 });
		Views.Add({ TEXT("I_surf"), Rf, 260.f, 0.f, 5, 0 });
		Views.Add({ TEXT("I_lighthouse"), Lh, 700.f, 120.f, 2, 0 });
		Views.Add({ TEXT("I_graveyard"), Gy, 420.f, 300.f, 2, 0 });
		Views.Add({ TEXT("I_pines"), Pn, 450.f, 90.f, 2, 0 });
		Views.Add({ TEXT("I_cliff"), Cl, 550.f, 250.f, 2, 0 });
		Views.Add({ TEXT("I_rocky"), Rk, 350.f, 60.f, 2, 0 });
		Views.Add({ TEXT("I_cove_close"), W, 230.f, 0.f, 2, 1 });
		Views.Add({ TEXT("I_far_silhouettes"), nullptr, 0.f, 0.f, 2, 4 });
		Views.Add({ TEXT("I_cay_close"), Cy, 300.f, 30.f, 2, 1 });
	}
	auto Setup = [&](const FV& V)
	{
		if (Vy->bPhoto) Vy->TogglePhoto();
		if (Vy->bChart) Vy->ToggleChart();
		if (V.Mode == 2) { Place(FVector(-300000.f, -270000.f, 0.f), 45.f, 2); Vy->ToggleChart(); return; }
		if (V.Mode == 4) { FindBenchView(); Place(BenchPos, BenchYaw, 2); return; }
		const FIsle& I = *V.I;
		// stand on the cove side for wooded/cay (CoveYaw), else FromYaw from +X
		const float From = (I.Kind == EIsle::Wooded || I.Kind == EIsle::Cay) ? I.CoveYaw + V.FromYaw : V.FromYaw;
		const FVector Tgt = FVector(I.LookAt.X, I.LookAt.Y, 0.f);
		const FVector L = Tgt + FRotator(0.f, From, 0.f).Vector() * V.StandM * CM;
		float Yaw = From + 180.f;
		if (V.Tel > 2) Yaw += 70.f; // I_surf: sail past the reef, broadside to the camera
		Place(L, Yaw, V.Tel);
		if (V.Mode == 1 || V.Mode == 3)
		{
			Vy->TogglePhoto();
			const float Up = V.Mode == 1 ? 900.f : 6000.f;
			const FVector Eye = FVector(L.X, L.Y, I.C.Z + Up) + FRotator(0.f, From, 0.f).Vector() * 3000.f;
			Vy->SetPhotoView(Eye, (I.LookAt - Eye).Rotation());
		}
		else C->SetControlRotation(FRotator(-4.f, (I.LookAt - L).Rotation().Yaw + (V.Tel > 2 ? -25.f : 0.f), 0.f));
	};
	if (TStep == 0)
	{
		if (TT < 3.f) return;
		if (!C->bStarted) C->StartGame(true);
		Setup(Views[0]); TStep = 1; TMark = TT; return;
	}
	if (TStep >= 1 && TStep <= Views.Num())
	{
		const FV& V = Views[TStep - 1];
		if (V.Mode == 0 && Since > 2.f && Since < 2.3f && V.I) C->SetControlRotation(FRotator(-4.f, (V.I->LookAt - P->GetActorLocation()).Rotation().Yaw + (V.Tel > 2 ? -25.f : 0.f), 0.f));
		const float ShootAt = V.Tel > 2 ? 14.f : 9.f;
		if (Since >= ShootAt && !bShot)
		{
			C->IWShot(V.Shot); bShot = true;
			UE_LOG(LogIronwake, Display, TEXT("IWI1 shot %s ship=%s"), V.Shot, *P->GetActorLocation().ToString());
		}
		if (Since >= ShootAt + 1.5f)
		{
			bShot = false; ++TStep; TMark = TT;
			if (TStep <= Views.Num()) Setup(Views[TStep - 1]);
		}
		return;
	}
	if (TStep == Views.Num() + 1)
	{
		if (Vy->bPhoto) Vy->TogglePhoto();
		if (Vy->bChart) Vy->ToggleChart();
		UE_LOG(LogIronwake, Display, TEXT("IWI1 DONE"));
		TStep++; TMark = TT;
	}
	if (TStep == Views.Num() + 2 && Since > 2.f)
	{
		TestMode.Empty();
		if (FParse::Param(FCommandLine::Get(), TEXT("iwi1quit"))) UKismetSystemLibrary::QuitGame(this, C, EQuitPreference::Quit, false);
	}
}
