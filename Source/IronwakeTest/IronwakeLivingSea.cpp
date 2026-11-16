// Card D: living sea (merchant steamers, airship, harbour island, ambience) and the tour camera used for
// the gameplay clip. Everything is deterministic code: no LLM, no assets beyond the project/engine.
#include "IronwakeLivingSea.h"
#include "IronwakeCollision.h"
#include "IronwakeWorld.h"
#include "IronwakeVoyage.h"
#include "IronwakeShell.h"
#include "IronwakeShipPawn.h"
#include "IronwakeGameMode.h"
#include "IronwakePlayerController.h"
#include "IronwakeTest.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "UnrealClient.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	UMaterialInstanceDynamic* Tint(UObject* Outer, const FLinearColor& C)
	{
		// No function-local static raw pointer: it dangled after a map reload (shipyard LAUNCH crash in SetParentInternal).
		static TWeakObjectPtr<UMaterialInterface> Cached;
		UMaterialInterface* Base = Cached.Get();
		if (!Base) { Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")); Cached = Base; }
		if (!Base) return nullptr;
		UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Base, Outer);
		M->SetVectorParameterValue(TEXT("Color"), C);
		return M;
	}

	UStaticMeshComponent* AddMesh(AActor* Owner, USceneComponent* Parent, UStaticMesh* Mesh, const FVector& Loc, const FRotator& Rot, const FVector& Scale)
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(Owner);
		C->SetStaticMesh(Mesh);
		C->SetupAttachment(Parent);
		C->SetRelativeLocation(Loc);
		C->SetRelativeRotation(Rot);
		C->SetRelativeScale3D(Scale);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->RegisterComponent();
		Owner->AddInstanceComponent(C);
		return C;
	}

	USoundAttenuation* MakeAtten(UObject* Outer, float InnerCm, float FalloffCm)
	{
		USoundAttenuation* A = NewObject<USoundAttenuation>(Outer);
		A->Attenuation.bAttenuate = true;
		A->Attenuation.bSpatialize = true;
		A->Attenuation.AttenuationShapeExtents = FVector(InnerCm, 0.f, 0.f);
		A->Attenuation.FalloffDistance = FalloffCm;
		return A;
	}

	constexpr float AirshipNoseYaw = 90.f; // SM_Airship's long axis is mesh Y (nose at -Y); this turns its nose to actor +X
}

// ============================================================================ Merchant
AIronwakeMerchant::AIronwakeMerchant()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> HullFinder(TEXT("/Game/Ironwake/Ships/Goliath/SM_Goliath_Hull.SM_Goliath_Hull"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> AglaeFinder(TEXT("/Game/Ironwake/Ships/Aglae/SM_Aglae_Hull.SM_Aglae_Hull"));
	MeshGoliath = HullFinder.Object;
	MeshAglae = AglaeFinder.Object;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneFinder(TEXT("/Engine/BasicShapes/Plane.Plane"));

	Hull = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Hull"));
	Hull->SetupAttachment(Root);
	if (HullFinder.Succeeded()) Hull->SetStaticMesh(HullFinder.Object);
	// R1: Goliath tug (50 m, library mesh in cm, boot-top at z=0); sits 0.4 m deep.
	Hull->SetRelativeScale3D(FVector(1.f));
	Hull->SetRelativeLocation(FVector(0.f, 0.f, AIronwakeShell::RenderedSeaOffsetCm - 40.f)); // actor rides the queried wave surface; boot-top 0.4 m under the RENDERED sea
	Hull->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// R1: the library hulls have their own funnels; this component is now only the smoke emitter anchor (hidden).
	Funnel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Funnel"));
	Funnel->SetupAttachment(Root);
	if (CylFinder.Succeeded()) Funnel->SetStaticMesh(CylFinder.Object);
	Funnel->SetRelativeLocation(FVector(330.f, 0.f, 1140.f + AIronwakeShell::RenderedSeaOffsetCm)); // Goliath funnel top
	Funnel->SetRelativeScale3D(FVector(0.01f));
	Funnel->SetHiddenInGame(true);
	Funnel->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Wake = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Wake"));
	Wake->SetupAttachment(Root);
	if (PlaneFinder.Succeeded()) Wake->SetStaticMesh(PlaneFinder.Object);
	Wake->SetUsingAbsoluteRotation(true);
	Wake->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Wake->SetCastShadow(false);
	Wake->SetTranslucentSortPriority(10);
}

void AIronwakeMerchant::BeginPlay()
{
	Super::BeginPlay();
	Yaw = GetActorRotation().Yaw;
	if (bAglae && MeshAglae)
	{
		// Aglae 1874 steam launch: 30 m, tall thin funnel amidships.
		Hull->SetStaticMesh(MeshAglae);
		Hull->SetRelativeLocation(FVector(0.f, 0.f, AIronwakeShell::RenderedSeaOffsetCm - 25.f));
		Funnel->SetRelativeLocation(FVector(60.f, 0.f, 940.f + AIronwakeShell::RenderedSeaOffsetCm));
		LengthCm = 3000.f;
	}
	else
	{
		LengthCm = 5000.f;
	}
	if (UIronwakeColl* Co = UIronwakeColl::Get(GetWorld()))
	{
		const FIronwakeHullProfile Pr = bAglae ? FIronwakeHullProfile::Aglae() : FIronwakeHullProfile::Goliath();
		TArray<FVector2D> Poly; UIronwakeColl::OutlineFromProfile(Pr, Poly, 50.f);
		CollShape = UIronwakeColl::MakeShape(Poly, bAglae ? 200.f : 250.f);
		Co->RegisterHull(this, CollShape, false);
		bCollReg = true;
	}
	if (UMaterialInstanceDynamic* M = Tint(this, FLinearColor(0.11f, 0.035f, 0.02f))) Funnel->SetMaterial(0, M);
	if (UMaterialInterface* Foam = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Ironwake/FX/M_WakeFoam.M_WakeFoam")))
	{
		WakeMID = UMaterialInstanceDynamic::Create(Foam, this);
		WakeMID->SetScalarParameterValue(TEXT("Opacity"), 0.75f);
		Wake->SetMaterial(0, WakeMID);
	}
	if (UNiagaraSystem* Smoke = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/Ironwake/FX/NS_IW_FunnelSmoke.NS_IW_FunnelSmoke")))
	{
		FunnelSmoke = UNiagaraFunctionLibrary::SpawnSystemAttached(Smoke, Funnel, NAME_None, FVector(0.f, 0.f, 55.f), FRotator::ZeroRotator,
			EAttachLocation::KeepRelativeOffset, false);
		if (FunnelSmoke) FunnelSmoke->SetWorldScale3D(FVector(bAglae ? 0.2f : 0.3f));
	}
}

void AIronwakeMerchant::Tick(float Dt)
{
	Super::Tick(Dt);
	Dt = FMath::Min(Dt, 0.1f);
	if (Route.Num() < 2) return;
	UIronwakeColl* Co = UIronwakeColl::Get(GetWorld());
	if (Co && !bRouteAudited) { bRouteAudited = true; Co->AuditPath(ShipName, Route, true, bAglae ? 250.f : 500.f, bAglae ? 200.f : 250.f); }
	FVector P = GetActorLocation();
	const FVector Target = Route[RouteIdx];
	FVector To = Target - P;
	To.Z = 0.f;
	if (To.Size() < 25000.f) RouteIdx = (RouteIdx + 1) % Route.Num();
	float Desired = To.Rotation().Yaw;
	bool bYieldSlow = false;
	if (Co && bCollReg)
	{
		const FVector CurFwd = FRotator(0.f, Yaw, 0.f).Vector();
		const FVector2D Probe(P.X + CurFwd.X * SpeedCmS * 6.f, P.Y + CurFwd.Y * SpeedCmS * 6.f);
		const FIwHit Ha = Co->TestPose(this, CollShape, Probe, Yaw, UIronwakeColl::MaskPawns | UIronwakeColl::MaskOthers);
		if (Ha.bHit && Ha.Other.IsValid()) { Desired = FRotator::NormalizeAxis(Desired + 30.f); bYieldSlow = true; }
	}
	const float Delta = FMath::FindDeltaAngleDegrees(Yaw, Desired);
	Yaw = FRotator::NormalizeAxis(Yaw + FMath::Clamp(Delta, -MaxYawRate * Dt, MaxYawRate * Dt));
	const FVector Fwd = FRotator(0.f, Yaw, 0.f).Vector();
	const FVector Side(-Fwd.Y, Fwd.X, 0.f);
	const bool bSlow = GetWorld()->GetTimeSeconds() < SlowUntil || bYieldSlow;
	const FVector PBefore = P;
	P += Fwd * SpeedCmS * (bSlow ? 0.3f : 1.f) * Dt;
	if (Co && bCollReg)
	{
		// Don't step into a solid, the shallows or another hull: hold (and let the player's ship push clear).
		const FVector2D PN(P.X, P.Y);
		const FIwHit H = Co->TestPose(this, CollShape, PN, Yaw);
		const bool bShallow = Co->MinDepthAt(CollShape, PN, Yaw) < CollShape.DraughtCm;
		if (H.bHit || bShallow)
		{
			P = PBefore; BlockedS += Dt;
			if (BlockedS > 0.5f && BlockedS - Dt <= 0.5f) UE_LOG(LogIronwake, Display, TEXT("IWP9 merchant %s holds: blocked by %s%s"), *ShipName, bShallow ? TEXT("shoal") : *H.Tag, H.bHit && bShallow ? TEXT(" +shoal") : TEXT(""));
		}
		else BlockedS = 0.f;
	}

	// Ride the live swell: centre height plus pitch/roll from bow/stern/beam samples.
	UWorld* W = GetWorld();
	const float HalfL = LengthCm * 0.42f, HalfB = bAglae ? 190.f : 440.f;
	const float ZB = AIronwakeShell::WaterZAtWaves(W, P + Fwd * HalfL);
	const float ZS = AIronwakeShell::WaterZAtWaves(W, P - Fwd * HalfL);
	const float ZP = AIronwakeShell::WaterZAtWaves(W, P - Side * HalfB);
	const float ZT = AIronwakeShell::WaterZAtWaves(W, P + Side * HalfB);
	const float ZC = 0.25f * (ZB + ZS + ZP + ZT);
	const float TargetPitch = FMath::RadiansToDegrees(FMath::Atan2(ZB - ZS, 2.f * HalfL)) * 0.8f;
	const float TargetRoll = FMath::RadiansToDegrees(FMath::Atan2(ZP - ZT, 2.f * HalfB)) * 0.35f + 0.6f * Delta / FMath::Max(FMath::Abs(Delta), 1.f) * FMath::Min(FMath::Abs(Delta), 30.f) / 30.f;
	Pitch = FMath::FInterpTo(Pitch, TargetPitch, Dt, 1.2f);
	Roll = FMath::FInterpTo(Roll, FMath::Clamp(TargetRoll, -6.f, 6.f), Dt, 0.8f);
	P.Z = FMath::FInterpTo(GetActorLocation().Z, ZC, Dt, 1.5f);
	SetActorLocationAndRotation(P, FRotator(Pitch, Yaw, Roll));

	// Stern wake strip, 160 m long.
	const float Len = 16000.f;
	Wake->SetWorldRotation(FRotator(0.f, Yaw + 180.f, 0.f));
	Wake->SetWorldScale3D(FVector(Len / 100.f, bAglae ? 7.f : 12.f, 1.f));
	Wake->SetWorldLocation(FVector(P.X, P.Y, ZC + 20.f) - Fwd * (LengthCm * 0.45f + Len * 0.5f));
}

// ============================================================================ Airship
AIronwakeAirship::AIronwakeAirship()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ShipFinder(TEXT("/Game/Ironwake/World/BrassAirshipRemesh/SM_BrassAirshipRemesh.SM_BrassAirshipRemesh"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PropFinder(TEXT("/Game/Ironwake/Ships/BrassAirship/SM_BrassAirship_PropStern.SM_BrassAirship_PropStern"));
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(Root);
	if (ShipFinder.Succeeded()) Body->SetStaticMesh(ShipFinder.Object);
	// R1: brass airship (Pigcraft, CC-BY), 110 m, nose +X, keel at z=0; hull centre ~46 m above the actor's keel.
	Body->SetRelativeScale3D(FVector(1.f));
	// E1: watertight remesh + 4K bake (Ursa job 2162). Its bbox includes the stern prop, so shift it 1.53 m aft onto R1's frame.
	Body->SetRelativeLocation(FVector(-153.f, 0.f, 0.f));
	Body->SetRelativeRotation(FRotator::ZeroRotator);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Prop = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PropStern"));
	Prop->SetupAttachment(Body);
	if (PropFinder.Succeeded()) Prop->SetStaticMesh(PropFinder.Object);
	Prop->SetRelativeLocation(FVector(-5600.04f + 153.f, 34.66f, 2784.41f)); // parts.json pivot (m -> cm), relative to the shifted body
	Prop->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	(void)AirshipNoseYaw;
}

void AIronwakeAirship::Tick(float Dt)
{
	Super::Tick(Dt);
	Dt = FMath::Min(Dt, 0.1f);
	T += Dt;
	float SpeedK = 1.f, Bob = 1.f;
	if (bHasDock)
	{
		const float Ahead = FMath::Fmod(Clockwise * (DockAngle - Angle) + 4.f * PI, 2.f * PI) * RadiusCm;   // cm to the dock along the loop
		const float Behind = 2.f * PI * RadiusCm - Ahead;
		if (DockState == 1)
		{
			SpeedK = 0.f;
			DockTimer += Dt;
			if (DockTimer >= DockHoldS) { DockState = 2; UE_LOG(LogIronwake, Display, TEXT("IWE1 airship casts off the mast")); }
		}
		else if (DockState == 2) { SpeedK = FMath::Clamp(Behind / 40000.f, 0.08f, 1.f); if (Behind > 150000.f) DockState = 0; }
		else if (Ahead < 40000.f)
		{
			SpeedK = FMath::Clamp(Ahead / 40000.f, 0.04f, 1.f);
			if (Ahead < 150.f) { DockState = 1; DockTimer = 0.f; Angle = DockAngle; UE_LOG(LogIronwake, Display, TEXT("IWE1 airship docked at the mooring mast")); }
		}
		const float D = (DockState == 2) ? Behind : FMath::Min(Ahead, Behind);
		const float B = FMath::SmoothStep(0.f, 160000.f, D);
		AltitudeCm = FMath::Lerp(DockTop.Z - NoseZCm, CruiseAltCm, B);
		Bob = B;
	}
	Angle += Clockwise * SpeedCmS * SpeedK * Dt / RadiusCm;
	const FVector P = Centre + FVector(FMath::Cos(Angle) * RadiusCm, FMath::Sin(Angle) * RadiusCm, AltitudeCm + 600.f * Bob * FMath::Sin(T * 0.21f));
	// Tangent of the circle, plus a few degrees of crab into the wind.
	const FVector Tan = FVector(-FMath::Sin(Angle), FMath::Cos(Angle), 0.f) * Clockwise;
	float Yaw = Tan.Rotation().Yaw + 6.f;
	if (bHasDock && DockState == 1) Yaw = (DockTop - P).GetSafeNormal2D().Rotation().Yaw + 2.f * FMath::Sin(T * 0.07f); // weathervane on the mast
	SetActorLocationAndRotation(P, FRotator(1.2f * FMath::Sin(T * 0.33f), Yaw, -2.5f * Clockwise + 1.0f * FMath::Sin(T * 0.27f)));
	// R1: stern prop spins about the ship's long axis (~4 rev/s: reads as a blur with motion blur on).
	PropAngle = FMath::Fmod(PropAngle + PropRevPerSec * 360.f * Dt, 360.f);
	if (Prop) Prop->SetRelativeRotation(FRotator(0.f, 0.f, PropAngle));
}

void AIronwakeAirship::SetupDock(const FVector& MastTop, const FVector& SeawardDir, float HoldS)
{
	// The actor docks where its nose (NoseXCm ahead) touches the mast top: re-centre the loop on the sea side so the
	// docked actor position lies on the circle and the tangent there points at the mast.
	CruiseAltCm = AltitudeCm;
	DockTop = MastTop;
	DockHoldS = HoldS;
	const FVector S = SeawardDir.GetSafeNormal2D();
	const FVector Tangent(-S.Y, S.X, 0.f);           // flying along the coast, nose to the mast
	const FVector DockPos = MastTop - Tangent * NoseXCm;
	Centre = FVector(DockPos.X, DockPos.Y, 0.f) + S * RadiusCm;
	// Tangent of the circle at angle a (Clockwise=+1) is (-sin a, cos a). Pick the sign so it matches Tangent.
	const FVector R = (FVector(DockPos.X, DockPos.Y, 0.f) - Centre).GetSafeNormal2D();
	Clockwise = (FVector(-R.Y, R.X, 0.f) | Tangent) >= 0.f ? 1.f : -1.f;
	DockAngle = FMath::Atan2(R.Y, R.X);
	Angle = DockAngle;
	DockState = 1; DockTimer = 0.f; bHasDock = true;
	AltitudeCm = DockTop.Z - NoseZCm;
	UE_LOG(LogIronwake, Display, TEXT("IWE1 airship dock: mast top %s, loop centre %s r=%.0f m cw=%.0f"), *MastTop.ToString(), *Centre.ToString(), RadiusCm / 100.f, Clockwise);
}

// ============================================================================ Director
AIronwakeLivingSea::AIronwakeLivingSea()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	// Hard references from the CDO so the packaged cook includes these (LoadObject by path alone is invisible to the cooker).
	static ConstructorHelpers::FObjectFinder<UStaticMesh> RockF(TEXT("/PCG/SampleContent/SimpleForest/Meshes/PCG_Boulder_02.PCG_Boulder_02"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylF(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeF(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeF(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<USoundBase> HarbourF(TEXT("/Game/Ironwake/Audio/harbour_lap_proc.harbour_lap_proc"));
	// S2 (Chuck 23:50): sea_wind.wav was a tonal synth pad ("angelic noise"): removed, no hard ref so it is not cooked.
	static ConstructorHelpers::FObjectFinder<USoundBase> HornF(TEXT("/Game/Ironwake/Audio/foghorn_sfx.foghorn_sfx"));
	RockMesh = RockF.Object; CylMesh = CylF.Object; ConeMesh = ConeF.Object; CubeMesh = CubeF.Object;
	HarbourSnd = HarbourF.Object; WindSnd = nullptr; HornSnd = HornF.Object;
}

AIronwakeLivingSea* AIronwakeLivingSea::Get(const UWorld* W)
{
	if (!W) return nullptr;
	TActorIterator<AIronwakeLivingSea> It(const_cast<UWorld*>(W));
	return It ? *It : nullptr;
}

USoundBase* AIronwakeLivingSea::LoadSnd(const TCHAR* Name)
{
	return LoadObject<USoundBase>(nullptr, *FString::Printf(TEXT("/Game/Ironwake/Audio/%s.%s"), Name, Name));
}

AIronwakeShipPawn* AIronwakeLivingSea::Player() const
{
	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	return PC ? Cast<AIronwakeShipPawn>(PC->GetPawn()) : nullptr;
}

void AIronwakeLivingSea::BeginPlay()
{
	Super::BeginPlay();
	BuildIsland();
	SpawnTraffic();
	StartAmbience();

	const FString Trigger = FPaths::ProjectSavedDir() / TEXT("iw_tour.txt");
	FString TourArg;
	if (IFileManager::Get().FileExists(*Trigger))
	{
		FFileHelper::LoadFileToString(TourArg, *Trigger);
		IFileManager::Get().Delete(*Trigger);
		bTour = true;
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("iwtour"))) { bTour = true; TourArg += TEXT(" quit"); }
	if (FParse::Param(FCommandLine::Get(), TEXT("iwtourshots"))) TourArg += TEXT(" shots");
	if (bTour)
	{
		bTourQuit = TourArg.Contains(TEXT("quit"));
		bTourShots = TourArg.Contains(TEXT("shots"));
		if (GEngine) GEngine->Exec(GetWorld(), TEXT("DisableAllScreenMessages"));
		UE_LOG(LogIronwake, Display, TEXT("IWTOUR start quit=%d shots=%d"), bTourQuit, bTourShots);
	}
	bR1 = FParse::Param(FCommandLine::Get(), TEXT("iwr1"));
	if (bR1 && GEngine) GEngine->Exec(GetWorld(), TEXT("DisableAllScreenMessages"));
}

void AIronwakeLivingSea::R1Shot(const FString& Name)
{
	const FString File = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("shots") / (Name + TEXT(".png")));
	FScreenshotRequest::RequestScreenshot(File, false, false);
	UE_LOG(LogIronwake, Display, TEXT("IWR1 shot %s t=%.1f"), *File, R1T);
	if (R1Cam) UE_LOG(LogIronwake, Display, TEXT("IWR1 cam %s"), *R1Cam->GetActorLocation().ToString());
	for (AIronwakeMerchant* M : Merchants) if (M)
		UE_LOG(LogIronwake, Display, TEXT("IWR1 merchant %s aglae=%d actor=%s hullZ=%.0f sea=%.0f"), *M->ShipName, M->bAglae, *M->GetActorLocation().ToString(),
			M->Hull->GetComponentLocation().Z, AIronwakeShell::WaterZAt(GetWorld(), M->GetActorLocation()));
	if (Airship) UE_LOG(LogIronwake, Display, TEXT("IWR1 airship %s"), *Airship->GetActorLocation().ToString());
}

void AIronwakeLivingSea::TickR1(float Dt)
{
	// R1 capture director (deterministic): settle -> player close (default rig) -> enemy at 4 km from the player's
	// deck -> merchant broadside -> airship stern prop, two frames 0.06 s apart (prop at ~4 rev/s moves ~86 deg).
	UWorld* W = GetWorld();
	APlayerController* PC = W->GetFirstPlayerController();
	AIronwakeShipPawn* P = Player();
	if (!PC || !P) return;
	R1T += Dt;
	auto Cam = [&](const FVector& Loc, const FVector& Look, float Fov)
	{
		if (!R1Cam)
		{
			FActorSpawnParameters SP; SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			R1Cam = W->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), Loc, FRotator::ZeroRotator, SP);
			R1Cam->GetCameraComponent()->bConstrainAspectRatio = false;
		}
		R1Cam->SetActorLocationAndRotation(Loc, (Look - Loc).Rotation());
		R1Cam->GetCameraComponent()->SetFieldOfView(Fov);
		PC->SetViewTargetWithBlend(R1Cam, 0.f);
	};
	AIronwakeShipPawn* E = nullptr;
	for (TActorIterator<AIronwakeShipPawn> It(W); It; ++It) if (It->bIsNPC) E = *It;
	if (R1Stage == 0 && R1T > 2.f)
	{
		if (AIronwakeGameMode* GM = W->GetAuthGameMode<AIronwakeGameMode>()) GM->SpawnEnemy();
		P->IWTelegraph(4); // half ahead so wakes/props run
		R1Stage = 1;
	}
	// Keep the enemy at exactly 4 km off the player's starboard bow for the range shot (no AI drift).
	if (E && R1Stage >= 1 && R1Stage < 4)
	{
		const FVector Fwd = P->GetActorForwardVector().GetSafeNormal2D();
		const FVector Want = P->GetActorLocation() + FRotator(0.f, 25.f, 0.f).RotateVector(Fwd) * 400000.f;
		E->HullBody->SetWorldLocation(FVector(Want.X, Want.Y, E->GetActorLocation().Z), false, nullptr, ETeleportType::TeleportPhysics);
		E->HullBody->SetWorldRotation(FRotator(0.f, P->GetActorRotation().Yaw + 70.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
	}
	if (R1Stage == 1 && R1T > 24.f) { PC->SetViewTargetWithBlend(P, 0.f); R1Stage = 2; }
	if (R1Stage == 2 && R1T > 27.f && R1T < 27.5f) { R1Shot(TEXT("R_player_close")); R1T = 27.5f; }
	if (R1Stage == 2 && R1T > 27.5f)
	{
		const FVector F = P->GetActorForwardVector().GetSafeNormal2D(), R(-F.Y, F.X, 0.f);
		const float Sea = AIronwakeShell::WaterZAtWaves(GetWorld(), P->GetActorLocation() + R * 12000.f);
		Cam(FVector(P->GetActorLocation().X, P->GetActorLocation().Y, Sea + 200.f) + R * 12000.f, FVector(P->GetActorLocation().X, P->GetActorLocation().Y, Sea + 200.f), 40.f);
		static TWeakObjectPtr<AActor> Gauge;
		if (!Gauge.IsValid() && CubeMesh)
		{
			// Gauge post 30 m off the beam: white 10 m above WaterZAt, black 10 m below (1 m ticks via a 2nd thin post).
			const FVector G = P->GetActorLocation() + R * 3000.f + F * 2000.f;
			const float GS = AIronwakeShell::WaterZAtWaves(GetWorld(), G);
			FActorSpawnParameters SP2; SP2.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AActor* A = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), FTransform(FVector(G.X, G.Y, GS)), SP2);
			USceneComponent* GR = NewObject<USceneComponent>(A); A->SetRootComponent(GR); GR->RegisterComponent();
			A->SetActorLocation(FVector(G.X, G.Y, GS));
			for (int32 k = -10; k < 10; ++k)
			{
				UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(A); C->SetStaticMesh(CubeMesh); C->SetupAttachment(GR);
				C->SetRelativeLocation(FVector(0, 0, k * 100.f + 50.f)); C->SetRelativeScale3D(FVector(2.f, 2.f, 0.98f));
				C->SetCollisionEnabled(ECollisionEnabled::NoCollision); C->RegisterComponent();
				if (UMaterialInstanceDynamic* MI = Tint(A, k >= 0 ? (k % 2 ? FLinearColor(0.9f, 0.9f, 0.9f) : FLinearColor(0.9f, 0.6f, 0.1f)) : (k % 2 ? FLinearColor(0.02f, 0.02f, 0.02f) : FLinearColor(0.1f, 0.02f, 0.4f)))) C->SetMaterial(0, MI);
			}
			Gauge = A;
			UE_LOG(LogIronwake, Display, TEXT("IWR1 gauge at sea=%.0f (white/orange 1 m bands above WaterZAt, black/purple below)"), GS);
		}
		if (Gauge.IsValid())
		{
			const FVector GL = Gauge->GetActorLocation();
			Gauge->SetActorLocation(FVector(GL.X, GL.Y, AIronwakeShell::WaterZAtWaves(GetWorld(), FVector(GL.X, GL.Y, 0.f)) + AIronwakeShell::RenderedSeaOffsetCm));
		}
		static int32 GShots = 0;
		if (R1T > 27.8f + GShots * 0.6f && GShots < 3 && R1T < 28.55f) { R1Shot(FString::Printf(TEXT("R_gauge_%d"), GShots)); ++GShots; }
		if (R1T > 28.6f) { R1Shot(TEXT("R_player_waterline")); R1Stage = 3; UE_LOG(LogIronwake, Display, TEXT("IWR1 player actorZ=%.0f meshZ=%.0f sea=%.0f seaWaves(player)=%.0f seaWaves(gauge)=%.0f"), P->GetActorLocation().Z, P->HullMesh->GetComponentLocation().Z, Sea,
			AIronwakeShell::WaterZAtWaves(GetWorld(), P->GetActorLocation()), Gauge.IsValid() ? AIronwakeShell::WaterZAtWaves(GetWorld(), Gauge->GetActorLocation()) : 0.f); }
	}
	if (R1Stage == 3 && R1T > 29.f)
	{
		// Enemy at 4 km from the player's foredeck, 12 m eye height, 35 deg FOV (binocular-ish view of the horizon).
		const FVector Eye = P->GetActorLocation() + P->GetActorForwardVector() * 3000.f + FVector(0, 0, 1200.f);
		if (E) Cam(Eye, E->GetActorLocation() + FVector(0, 0, 1000.f), 35.f);
		if (R1T > 31.f) { R1Shot(TEXT("R_enemy_4km")); R1Stage = 4; }
	}
	if (R1Stage == 4 && R1T > 33.f)
	{
		AIronwakeMerchant* M = Merchants.Num() ? Merchants[0].Get() : nullptr;
		for (AIronwakeMerchant* X : Merchants) if (X && !X->bAglae) { M = X; break; }
		if (M)
		{
			const FVector F = M->GetActorForwardVector().GetSafeNormal2D(), R(-F.Y, F.X, 0.f);
			Cam(M->GetActorLocation() + R * 9000.f + F * 3000.f + FVector(0, 0, 2200.f), M->GetActorLocation() + FVector(0, 0, 500.f), 50.f);
		}
		if (R1T > 35.f && R1T < 35.5f) { R1Shot(TEXT("R_merchant")); R1T = 35.5f; }
		if (M && R1T > 35.5f)
		{
			// Waterline check: eye 1.5 m above the local sea, 60 m abeam.
			const FVector F = M->GetActorForwardVector().GetSafeNormal2D(), R(-F.Y, F.X, 0.f);
			const float Sea = AIronwakeShell::WaterZAtWaves(GetWorld(), M->GetActorLocation() + R * 6000.f);
			Cam(FVector(M->GetActorLocation().X, M->GetActorLocation().Y, Sea + 150.f) + R * 6000.f, FVector(M->GetActorLocation().X, M->GetActorLocation().Y, Sea + 150.f), 40.f);
		}
		if (R1T > 36.5f) { R1Shot(TEXT("R_merchant_waterline")); R1Stage = 5; }
	}
	if (R1Stage == 5 && R1T > 37.f)
	{
		AIronwakeMerchant* M = nullptr;
		for (AIronwakeMerchant* X : Merchants) if (X && X->bAglae) { M = X; break; }
		if (M)
		{
			const FVector F = M->GetActorForwardVector().GetSafeNormal2D(), R(-F.Y, F.X, 0.f);
			// Hesper circles the lighthouse island: shoot from the seaward side so the rocks are not in front of the lens.
			FVector Out = (M->GetActorLocation() - IslandCentre).GetSafeNormal2D();
			if (Out.IsNearlyZero()) Out = R;
			Cam(M->GetActorLocation() + Out * 6000.f + F * 2500.f + FVector(0, 0, 1500.f), M->GetActorLocation() + FVector(0, 0, 300.f), 45.f);
		}
		if (R1T > 39.f) { R1Shot(TEXT("R_merchant_aglae")); R1Stage = 6; }
	}
	if (R1Stage >= 6 && R1Stage < 9 && Airship)
	{
		const FVector F = Airship->GetActorForwardVector(), R = Airship->GetActorRightVector();
		const FVector PropW = Airship->Prop->GetComponentLocation();
		Cam(PropW - F * 4500.f + R * 3500.f + FVector(0, 0, 800.f), PropW + F * 1500.f, 50.f);
		static bool bPropOnly = false;
		if (R1Stage == 6 && R1T > 40.f && !bPropOnly) { R1Shot(TEXT("R_airship_propdbg_prop_hidden")); Airship->Prop->SetVisibility(false); bPropOnly = true; }
		if (R1Stage == 6 && R1T > 40.4f && bPropOnly && Airship->Prop && !Airship->Prop->IsVisible()) { R1Shot(TEXT("R_airship_propdbg_hull_only")); Airship->Prop->SetVisibility(true); }
		if (R1Stage == 6 && R1T > 41.f) { R1Shot(TEXT("R_airship_props_a")); R1Stage = 7; }
		else if (R1Stage == 7 && R1T > 41.06f) { R1Shot(TEXT("R_airship_props_b")); R1Stage = 8; }
		else if (R1Stage == 8 && R1T > 43.f)
		{
			Cam(Airship->GetActorLocation() - F * 9000.f + R * 14000.f + FVector(0, 0, 2500.f), Airship->GetActorLocation() + FVector(0, 0, 4000.f), 50.f);
			if (R1T > 44.f) { R1Shot(TEXT("R_airship_wide")); R1Stage = 9; }
		}
	}
	if (R1Stage == 9 && R1T > 47.f)
	{
		UE_LOG(LogIronwake, Display, TEXT("IWR1 capture done"));
		R1Stage = 10;
		UKismetSystemLibrary::QuitGame(W, PC, EQuitPreference::Quit, false);
	}
}

void AIronwakeLivingSea::BuildIsland()
{
	UStaticMesh* Rock = RockMesh;
	UStaticMesh* Cyl = CylMesh;
	UStaticMesh* Cone = ConeMesh;
	UStaticMesh* Cube = CubeMesh;
	(void)Rock;

	USceneComponent* IslandRoot = NewObject<USceneComponent>(this, TEXT("Island"));
	IslandRoot->SetupAttachment(RootComponent);
	IslandRoot->RegisterComponent();
	IslandRoot->SetWorldLocation(IslandCentre);
	AddInstanceComponent(IslandRoot);

	// E1: St Brannoc island from Poly Haven coastal-cliff photoscans: a ring of cliff faces looking out to sea (scaled up,
	// sunk 6 m into the water), a coast-rock plateau capping the top (the lighthouse stands on it), and scan boulders
	// and rock shelves round the foot. Varied yaw so no two faces read alike.
	UStaticMeshComponent* Main = nullptr;
	const float SeaR = AIronwakeShell::WaterZAt(GetWorld(), IslandCentre) + AIronwakeShell::RenderedSeaOffsetCm - IslandCentre.Z;
	// v2: the coast-rock "plateau" read as a flat cantilevered slab (vision 3.5/10), so the island body is now stacked
	// boulder_01 scans (irregular crag silhouette, like the Needles) ringed by cliff faces at the foot; the old cap is gone.
	struct FC { const TCHAR* N; float Phi; float Rm; FVector S; };
	const FC Ring[] = {
		{ TEXT("CoastalCliff04"), 0.f, 85.f, FVector(1.5f, 2.0f, 3.0f) },
		{ TEXT("CoastalCliff01"), 62.f, 92.f, FVector(1.5f, 2.4f, 2.8f) },
		{ TEXT("CoastalCliff04"), 128.f, 88.f, FVector(1.4f, 2.0f, 2.7f) },
		{ TEXT("CoastalCliff02"), 180.f, 82.f, FVector(2.6f, 2.4f, 3.0f) },
		{ TEXT("CoastalCliff01"), 236.f, 90.f, FVector(1.5f, 2.4f, 3.0f) },
		{ TEXT("CoastalCliff02"), 298.f, 84.f, FVector(2.8f, 2.4f, 3.2f) },
	};
	for (const FC& C : Ring)
	{
		const FVector D(FMath::Cos(FMath::DegreesToRadians(C.Phi)), FMath::Sin(FMath::DegreesToRadians(C.Phi)), 0.f);
		// Scan's sheer face looks toward UE +Y (Blender -Y, FBX flips Y); yaw Phi-90 turns +Y to the outward direction D.
		IronwakeWorld::Place(this, IslandRoot, C.N, D * C.Rm * 100.f + FVector(0, 0, SeaR - 600.f), FRotator(0.f, C.Phi - 90.f, 0.f), C.S, true);
	}
	// Crag body: big boulder scans (1.27 x 1.83 x 1.0 m) scaled to ~150 m across and stacked, each rotated.
	Main = IronwakeWorld::Place(this, IslandRoot, TEXT("Boulder01"), FVector(0, 0, SeaR - 1500.f), FRotator(3.f, 20.f, -2.f), FVector(115.f, 85.f, 52.f), true);
	IronwakeWorld::Place(this, IslandRoot, TEXT("Boulder01"), FVector(3500, 4000, SeaR + 600.f), FRotator(-4.f, 115.f, 5.f), FVector(60.f, 45.f, 40.f), true);
	IronwakeWorld::Place(this, IslandRoot, TEXT("Boulder01"), FVector(4500, -2500, SeaR - 500.f), FRotator(6.f, 250.f, 0.f), FVector(55.f, 40.f, 30.f), true);
	// Foot: rock shelves awash and scan boulders.
	struct FR { const TCHAR* N; FVector L; float Yaw; FVector S; };
	const FR Foot[] = {
		{ TEXT("CoastRocks03"), FVector(14500, 1500, -150), 40.f, FVector(2.2f) },
		{ TEXT("CoastRocks03"), FVector(-13000, -6000, -150), 200.f, FVector(2.0f) },
		{ TEXT("CoastRocks02"), FVector(4000, -14500, -200), 110.f, FVector(1.6f) },
		{ TEXT("Boulder01"), FVector(17000, 11500, -300), 140.f, FVector(14.f) },
		{ TEXT("Boulder01"), FVector(-19000, 6500, -300), 250.f, FVector(18.f, 16.f, 15.f) },
		{ TEXT("Boulder01"), FVector(-7000, -18000, -300), 75.f, FVector(12.f) },
		{ TEXT("Boulder01"), FVector(22500, -11000, -300), 310.f, FVector(9.f) },
		{ TEXT("CoastLandRocks03"), FVector(-27000, -11000, -150), 190.f, FVector(4.f, 4.f, 6.f) },
	};
	for (const FR& R : Foot) IronwakeWorld::Place(this, IslandRoot, R.N, R.L + FVector(0, 0, SeaR), FRotator(0.f, R.Yaw, 0.f), R.S, true);

	// Lighthouse on the main crag: trace down to find the rock top.
	FVector Base = IslandCentre + FVector(0, 0, SeaR + 4000.f); // E1: crag top if the trace misses
	FHitResult Hit;
	FCollisionQueryParams Q(TEXT("IWIsland"), true);
	const FVector From = IslandCentre + FVector(-2500.f, 800.f, 30000.f);
	if (Main && Main->LineTraceComponent(Hit, From, From - FVector(0, 0, 40000.f), Q)) Base = Hit.ImpactPoint - FVector(0, 0, 150.f);
	UE_LOG(LogIronwake, Display, TEXT("IWSEA island at %s, lighthouse base z=%.0f m"), *IslandCentre.ToString(), Base.Z / 100.f);
	if (Cyl && Cone && Cube)
	{
		const FVector L = Base - IslandCentre;
		UStaticMeshComponent* Tower = AddMesh(this, IslandRoot, Cyl, L + FVector(0, 0, 2200.f), FRotator::ZeroRotator, FVector(9.f, 9.f, 44.f));
		UStaticMeshComponent* Band = AddMesh(this, IslandRoot, Cyl, L + FVector(0, 0, 2700.f), FRotator::ZeroRotator, FVector(9.2f, 9.2f, 6.f));
		UStaticMeshComponent* Gallery = AddMesh(this, IslandRoot, Cyl, L + FVector(0, 0, 4430.f), FRotator::ZeroRotator, FVector(12.5f, 12.5f, 0.8f));
		UStaticMeshComponent* Lamp = AddMesh(this, IslandRoot, Cyl, L + FVector(0, 0, 4730.f), FRotator::ZeroRotator, FVector(6.2f, 6.2f, 5.f));
		UStaticMeshComponent* Roof = AddMesh(this, IslandRoot, Cone, L + FVector(0, 0, 5200.f), FRotator::ZeroRotator, FVector(8.f, 8.f, 4.f));
		UStaticMeshComponent* Hut = AddMesh(this, IslandRoot, Cube, L + FVector(1500, 500, 300.f), FRotator(0, 18, 0), FVector(12.f, 8.f, 6.f));
		const FLinearColor Lime(0.55f, 0.55f, 0.52f), Oxblood(0.17f, 0.03f, 0.03f), Iron(0.03f, 0.033f, 0.036f);
		Tower->SetMaterial(0, Tint(this, Lime));
		Band->SetMaterial(0, Tint(this, Oxblood));
		Gallery->SetMaterial(0, Tint(this, Iron));
		Roof->SetMaterial(0, Tint(this, Iron));
		Hut->SetMaterial(0, Tint(this, FLinearColor(0.25f, 0.24f, 0.22f)));
		if (UMaterialInstanceDynamic* Glow = Tint(this, FLinearColor(4.f, 2.6f, 1.2f))) Lamp->SetMaterial(0, Glow);

		LampLight = NewObject<UPointLightComponent>(this, TEXT("LighthouseLamp"));
		LampLight->SetupAttachment(IslandRoot);
		LampLight->SetRelativeLocation(L + FVector(0, 0, 4730.f));
		LampLight->SetIntensityUnits(ELightUnits::Candelas);
		LampLight->SetIntensity(60000.f);
		LampLight->SetAttenuationRadius(60000.f);
		LampLight->SetLightColor(FLinearColor(1.f, 0.78f, 0.45f));
		LampLight->SetCastShadows(false);
		LampLight->RegisterComponent();
		AddInstanceComponent(LampLight);
	}
}

void AIronwakeLivingSea::SpawnTraffic()
{
	UWorld* W = GetWorld();
	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SP.Owner = this;

	struct FRouteDef { const TCHAR* Name; TArray<FVector> Pts; float Knots; int32 Start; };
	const auto M = [](float X, float Y) { return FVector(X * 100.f, Y * 100.f, 0.f); }; // metres -> cm
	const FRouteDef Defs[] = {
		// Coastal packet: separated offshore corridor >= 1.8 km east of Greyhaven outer roads (Calloway x Hodman zero impacts).
		{ TEXT("SS Calloway"), { M(-2100, 1200), M(1500, 3300), M(3200, 1200), M(2500, -2500), M(-1600, -1900) }, 8.f, 0 },
		// Collier: circles the lighthouse island (harbour traffic).
		{ TEXT("SS Hesper"), { M(450, 1900), M(170, 2570), M(-500, 2850), M(-1170, 2570), M(-1450, 1900), M(-1170, 1230), M(-500, 950), M(170, 1230) }, 7.f, 7 },
		// Grain steamer: eastern loop.
		{ TEXT("SS Marrow Queen"), { M(3300, -3200), M(3500, 2600), M(2100, 2500), M(1800, -2900) }, 9.f, 1 },
	};
	const bool bLegacy = FParse::Param(FCommandLine::Get(), TEXT("iwoldnpc"));   // P21: the NPC director owns merchant traffic; the scripted loops (no avoidance, duplicate names) only run with -iwoldnpc
	for (const FRouteDef& D : Defs)
	{
		if (!bLegacy) break;
		const FVector P0 = D.Pts[D.Start];
		const FVector P1 = D.Pts[(D.Start + 1) % D.Pts.Num()];
		const FRotator R(0.f, (P1 - P0).Rotation().Yaw, 0.f);
		AIronwakeMerchant* Mc = W->SpawnActorDeferred<AIronwakeMerchant>(AIronwakeMerchant::StaticClass(), FTransform(R, P0), this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Mc) continue;
		Mc->Route = D.Pts;
		Mc->RouteIdx = (D.Start + 1) % D.Pts.Num();
		Mc->SpeedCmS = D.Knots * 51.44f * 1.5f;
		Mc->ShipName = D.Name;
		Mc->bAglae = (Merchants.Num() == 1); // R1: one Aglae launch among the Goliath tugs
		Mc->Tags.Add(TEXT("IronwakeMerchant"));
		Mc->FinishSpawning(FTransform(R, P0));
		Merchants.Add(Mc);
	}

	Airship = W->SpawnActor<AIronwakeAirship>(AIronwakeAirship::StaticClass(), FTransform(FVector(0, 0, 40000.f)), SP);
	if (Airship)
	{
		Airship->Centre = FVector(60000.f, 160000.f, 0.f);
		Airship->RadiusCm = 190000.f;
		Airship->AltitudeCm = 26000.f;
		Airship->SpeedCmS = 1500.f;
		Airship->Angle = FMath::DegreesToRadians(-35.f);
		Airship->Clockwise = 1.f;
	}
	UE_LOG(LogIronwake, Display, TEXT("IWSEA spawned %d merchants + airship=%d"), Merchants.Num(), Airship != nullptr);
}

void AIronwakeLivingSea::StartAmbience()
{
	if (USoundBase* H = HarbourSnd.Get())
	{
		HarbourAmb = UGameplayStatics::SpawnSoundAtLocation(this, H, IslandCentre + FVector(0, 0, 1500.f), FRotator::ZeroRotator, 1.4f, 1.f, 0.f,
			MakeAtten(this, 35000.f, 280000.f), nullptr, false);
	}
	// No wind bed: ambience is only water loops (ship), harbour ambience and the foghorn.
	UE_LOG(LogIronwake, Display, TEXT("IWAMB harbour=%d wind=%d"), HarbourAmb != nullptr, WindAmb != nullptr);
}

void AIronwakeLivingSea::TickAmbience(float Dt)
{
	if (HarbourAmb && !HarbourAmb->IsPlaying()) HarbourAmb->Play();
	if (WindAmb && !WindAmb->IsPlaying()) WindAmb->Play();
	FoghornIn -= Dt;
	if (FoghornIn <= 0.f && Merchants.Num() > 0)
	{
		FoghornIn = FMath::FRandRange(70.f, 140.f);
		AIronwakeMerchant* Mc = Merchants[FMath::RandRange(0, Merchants.Num() - 1)];
		if (IsValid(Mc))
		{
			UGameplayStatics::PlaySoundAtLocation(this, HornSnd.Get(), Mc->GetActorLocation() + FVector(0, 0, 2500.f), 1.6f, FMath::FRandRange(0.85f, 1.0f), 0.f,
				MakeAtten(this, 40000.f, 900000.f));
			UE_LOG(LogIronwake, Display, TEXT("IWAMB foghorn from %s"), *Mc->ShipName);
		}
	}
	// Lighthouse lamp: slow sweep pulse (rotating lens seen side-on).
	LampPhase += Dt;
	if (LampLight) LampLight->SetIntensity(15000.f + 160000.f * FMath::Pow(FMath::Max(0.f, FMath::Sin(LampPhase * 0.9f)), 8.f));

	// P9: the old lighthouse-circle grounding hack is gone; real collision + depth grounding live in IronwakeShipCollision.cpp.
}

void AIronwakeLivingSea::Tick(float Dt)
{
	Super::Tick(Dt);
	if (!GetWorld()->IsPaused()) TickAmbience(FMath::Min(Dt, 0.1f));
	if (bTour) TickTour(FMath::Min(Dt, 0.1f));
	if (bR1) TickR1(FMath::Min(Dt, 0.1f));
}

void AIronwakeLivingSea::TourShot(const FString& Name)
{
	if (!bTourShots || GIsDumpingMovie || ShotsTaken.Contains(Name)) return;
	ShotsTaken.Add(Name);
	const FString File = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("shots") / (Name + TEXT(".png")));
	FScreenshotRequest::RequestScreenshot(File, true, false);
	UE_LOG(LogIronwake, Display, TEXT("IWTOUR screenshot %s t=%.1f"), *File, TourT);
}

void AIronwakeLivingSea::TickTour(float Dt)
{
	UWorld* W = GetWorld();
	APlayerController* PC = W->GetFirstPlayerController();
	AIronwakePlayerController* IPC = Cast<AIronwakePlayerController>(PC);
	AIronwakeShipPawn* P = Player();
	if (!PC || !P) return;
	const float Prev = TourT;
	TourT += Dt;
	auto Crossed = [&](float T) { return Prev < T && TourT >= T; };
	if (!TourCam)
	{
		FActorSpawnParameters SP;
		SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		TourCam = W->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), P->GetActorLocation(), FRotator::ZeroRotator, SP);
		TourCam->GetCameraComponent()->SetFieldOfView(50.f);
		TourCam->GetCameraComponent()->bConstrainAspectRatio = false;
	}
	auto Aim = [&](const FVector& From, const FVector& At)
	{
		TourCam->SetActorLocationAndRotation(From, (At - From).Rotation());
		if (PC->GetViewTarget() != TourCam) PC->SetViewTarget(TourCam);
	};
	const FVector PL = P->GetActorLocation();
	const FVector PF = P->GetActorForwardVector().GetSafeNormal2D();
	const FVector PS(-PF.Y, PF.X, 0.f);

	// 0-3 s: title menu over the sea.
	if (Crossed(1.6f)) TourShot(TEXT("D_1_title"));
	if (Crossed(3.0f) && IPC) IPC->StartGame(false, true); // peaceful start; the enemy joins at 24 s
	if (Crossed(3.2f)) P->IWTelegraph(4);                    // HALF AHEAD
	// 3-10 s: crane down the player's quarter toward the lighthouse island.
	if (TourT >= 3.f && TourT < 10.f)
	{
		const float A = (TourT - 3.f) / 7.f;
		const FVector Cam = PL - PF * FMath::Lerp(32000.f, 24000.f, A) + PS * FMath::Lerp(16000.f, 9000.f, A) + FVector(0, 0, FMath::Lerp(9000.f, 3500.f, A));
		Aim(Cam, FMath::Lerp(PL + PF * 20000.f, IslandCentre + FVector(0, 0, 3000.f), 0.6f));
		if (Crossed(8.5f)) TourShot(TEXT("D_2_island"));
	}
	// 10-17 s: a merchant steamer passes a low camera.
	if (TourT >= 10.f && TourT < 17.f)
	{
		if (Crossed(10.f) || !TourMerchant.IsValid())
		{
			float Best = 1e20f;
			for (AIronwakeMerchant* Mc : Merchants)
			{
				const float D = FVector::Dist2D(Mc->GetActorLocation(), PL);
				if (IsValid(Mc) && D < Best) { Best = D; TourMerchant = Mc; }
			}
		}
		if (AIronwakeMerchant* Mc = TourMerchant.Get())
		{
			const FVector MF = Mc->GetActorForwardVector().GetSafeNormal2D();
			const FVector MS(-MF.Y, MF.X, 0.f);
			if (Crossed(10.f) || Anchor.IsZero()) Anchor = Mc->GetActorLocation() + MF * 9000.f + MS * 15000.f + FVector(0, 0, 1800.f);
			Aim(Anchor, Mc->GetActorLocation() + FVector(0, 0, 900.f));
			if (Crossed(13.5f)) TourShot(TEXT("D_3_merchant"));
		}
	}
	// 17-24 s: the airship passes overhead.
	if (TourT >= 17.f && TourT < 24.f && Airship)
	{
		const FVector AL = Airship->GetActorLocation();
		const FVector AF = Airship->GetActorForwardVector().GetSafeNormal2D();
		const FVector AS(-AF.Y, AF.X, 0.f);
		if (Crossed(17.f) || Anchor2.IsZero()) Anchor2 = AL + AF * 16000.f + AS * 9000.f + FVector(0, 0, -16000.f);
		Aim(Anchor2, AL);
		if (Crossed(21.f)) TourShot(TEXT("D_4_airship"));
	}
	// 24 s: back aboard; the enemy ironclad appears and the scripted gunner (card C) fights it.
	if (Crossed(24.f))
	{
		PC->SetViewTarget(P);
		if (AIronwakeGameMode* GM = W->GetAuthGameMode<AIronwakeGameMode>()) GM->SpawnEnemy();
		// Demo only: a tougher hero hull so the recorded fight shows the full loop (the RNG duel can go either way).
		P->MaxHP = P->HP = 400.f;
		P->StartCombatDemo(TEXT("D_combat"));
	}
	if (Crossed(27.f)) TourShot(TEXT("D_5_hud_sea"));
	// Fight director (clip only): alternate the gunner's view with cameras beside the enemy so hits,
	// fires and the sinking read on video. Card C's own capture cameras still run in between.
	if (TourT > 30.f && P->IsCombatDemoRunning())
	{
		AIronwakeShipPawn* E = P->FindEnemy();
		const float Cycle = FMath::Fmod(TourT - 30.f, 14.f);
		const bool bEnemyCam = E && (E->bSinking || Cycle > 8.f);
		if (bEnemyCam)
		{
			const FVector EL = E->bSinking ? FVector(E->SinkStart.GetLocation().X, E->SinkStart.GetLocation().Y, 0.f) : E->GetActorLocation();
			const FVector EF = (E->bSinking ? E->SinkStart.GetRotation().GetForwardVector() : E->GetActorForwardVector()).GetSafeNormal2D();
			const FVector ES(-EF.Y, EF.X, 0.f);
			const FVector Side = FVector::DotProduct(PL - EL, ES) > 0.f ? ES : -ES; // the side facing the player, so incoming shells are in frame
			const float Orbit = (TourT - 30.f) * 0.6f;
			const FVector Cam = EL + Side * 26000.f + EF * (6000.f + 3000.f * FMath::Sin(Orbit * 0.2f)) + FVector(0, 0, E->bSinking ? 3500.f : 2600.f);
			Aim(Cam, EL + FVector(0, 0, E->bSinking ? 0.f : 900.f));
			bDirectorCam = true;
		}
		else if (bDirectorCam)
		{
			PC->SetViewTarget(P);
			bDirectorCam = false;
		}
	}
	// After the fight (or 200 s), pause menu, then quit.
	if (TourT > 30.f && EndAt < 0.f && (!P->IsCombatDemoRunning() || TourT > 140.f || P->bSinking)) EndAt = TourT + 2.f;
	if (EndAt > 0.f && TourT >= EndAt && IPC && !IPC->IsMenuOpen())
	{
		IPC->ShowMenu(1);
		UE_LOG(LogIronwake, Display, TEXT("IWTOUR pause menu t=%.1f"), TourT);
	}
	if (EndAt > 0.f && TourT >= EndAt + 1.5f) TourShot(TEXT("D_6_pause"));
	if (EndAt > 0.f && TourT >= EndAt + 3.f)
	{
		UE_LOG(LogIronwake, Display, TEXT("IWTOUR done t=%.1f"), TourT);
		bTour = false;
		EndAt = -1.f;
		if (bTourQuit) UKismetSystemLibrary::QuitGame(this, PC, EQuitPreference::Quit, false);
	}
}
