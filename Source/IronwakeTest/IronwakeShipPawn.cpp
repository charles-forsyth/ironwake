#include "IronwakeShipPawn.h"
#include "IronwakeTest.h"
#include "IronwakeFleet.h"
#include "IronwakeHydro.h"
#include "IronwakeShipWave.h"

#include "BuoyancyComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "HAL/FileManager.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "InputTriggers.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "Engine/Engine.h"
#include "UObject/ConstructorHelpers.h"
#include "EngineUtils.h"
#include "IronwakeSettings.h"
#include "IronwakeShell.h"
#include "IronwakeCollision.h"
#include "IronwakePlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Kismet/KismetSystemLibrary.h"

namespace
{
	const TCHAR* TelegraphNames[] = { TEXT("FULL ASTERN"), TEXT("HALF ASTERN"), TEXT("STOP"), TEXT("SLOW AHEAD"), TEXT("HALF AHEAD"), TEXT("FULL AHEAD"), TEXT("FLANK") };
	constexpr float KnotToCmS = 51.4444f; // 1 knot = 0.514444 m/s
	constexpr int32 MaxRudderStep = 3;

	// Pontoon layout along the hull: (x m, |y| m). Two rows so the hull has roll stiffness.
	const FVector2D PontoonXY[] = {
		FVector2D(-56.f, 6.f), FVector2D(-36.f, 10.f), FVector2D(-14.f, 11.f),
		FVector2D(10.f, 11.f), FVector2D(32.f, 9.f), FVector2D(52.f, 5.f) };
	constexpr float PontoonRadiusCm = 800.f;
	constexpr float PontoonZCm = -150.f; // pontoon centre relative to body origin (waterline design ~0)
}

AIronwakeShipPawn::AIronwakeShipPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;

	TelegraphKnots = { -6.f, -3.f, 0.f, 5.f, 9.f, 13.f, 16.f };
	// H1 zoom steps (x hull length): 0.78 = just behind the funnels ... 4.5 = whole ship + wake small.
	ZoomStepsL = { 0.8f, 1.15f, 1.5f, 2.2f, 3.2f, 4.6f };

	HullBody = CreateDefaultSubobject<UBoxComponent>(TEXT("HullBody"));
	HullBody->SetBoxExtent(FVector(6800.f, 1500.f, 700.f));
	HullBody->SetCollisionProfileName(TEXT("PhysicsActor"));
	HullBody->SetSimulatePhysics(true);
	HullBody->SetGenerateOverlapEvents(true);
	HullBody->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);
	HullBody->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	HullBody->BodyInstance.bOverrideMass = true;
	HullBody->BodyInstance.SetMassOverride(MassKg);
	HullBody->SetLinearDamping(0.0f);
	HullBody->SetAngularDamping(0.3f);
	HullBody->SetHiddenInGame(true);
	RootComponent = HullBody;

	HullMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HullMesh"));
	HullMesh->SetupAttachment(HullBody);
	HullMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HullMesh->SetGenerateOverlapEvents(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> HullFinder(TEXT("/Game/Ironwake/Ships/Devastation/SM_Devastation_Hull.SM_Devastation_Hull"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> EnemyHullFinder(TEXT("/Game/Ironwake/Ships/Kracken/SM_Kracken_Hull.SM_Kracken_Hull"));
	MeshPlayerHull = HullFinder.Object;
	MeshEnemyHull = EnemyHullFinder.Object;
	// S2: roster hulls + sail cloth (hard references so the packaged cook includes them).
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CutterF(TEXT("/Game/Ironwake/Ships/Aglae/SM_Aglae_Hull.SM_Aglae_Hull"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> FrigateF(TEXT("/Game/Ironwake/Ships/Frigate/SM_Frigate_Hull.SM_Frigate_Hull"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ClothF(TEXT("/Game/Ironwake/Ships/Materials/M_IW_Cloth.M_IW_Cloth"));
	MeshCutterHull = CutterF.Object; MeshFrigateHull = FrigateF.Object; ClothMaterial = ClothF.Object;
	if (HullFinder.Succeeded())
	{
		HullMesh->SetStaticMesh(HullFinder.Object);
	}
	// R1: library meshes are in cm with the boot-top (waterline paint) at mesh z=0, bow +X, midships x=0.
	HullMesh->SetRelativeLocation(FVector(0.f, 0.f, HullMeshZ));
	HullMesh->SetRelativeScale3D(FVector(1.f));
	HullLengthM = 125.f;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PropF(TEXT("/Game/Ironwake/Ships/Devastation/SM_Devastation_PropPort.SM_Devastation_PropPort"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PropSF(TEXT("/Game/Ironwake/Ships/Devastation/SM_Devastation_PropStbd.SM_Devastation_PropStbd"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> RudF(TEXT("/Game/Ironwake/Ships/Devastation/SM_Devastation_Rudder.SM_Devastation_Rudder"));
	auto MakePart = [&](const TCHAR* Name, UStaticMesh* M, const FVector& Loc) -> UStaticMeshComponent*
	{
		UStaticMeshComponent* C = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		C->SetupAttachment(HullMesh);
		if (M) C->SetStaticMesh(M);
		C->SetRelativeLocation(Loc);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetGenerateOverlapEvents(false);
		return C;
	};
	// Pivots from build/assets/r1/Devastation/parts.json (metres -> cm, relative to the hull mesh origin).
	PropPort = MakePart(TEXT("PropPort"), PropF.Object, FVector(-5481.9f, -376.6f, -650.4f));
	PropStbd = MakePart(TEXT("PropStbd"), PropSF.Object, FVector(-5481.9f, 376.6f, -650.4f));
	RudderMesh = MakePart(TEXT("Rudder"), RudF.Object, FVector(-5733.7f, 0.7f, -559.7f));

	// R1: Devastation turrets + twin barrels (library meshes). Turret pivots sit on the deck ring, barrels pivot
	// at the trunnion. The aft turret mesh is exported facing +X; its pivot is yawed 180 like before.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> TFF(TEXT("/Game/Ironwake/Ships/Devastation/SM_Devastation_TurretFore.SM_Devastation_TurretFore"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> TAF(TEXT("/Game/Ironwake/Ships/Devastation/SM_Devastation_TurretAft.SM_Devastation_TurretAft"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> BFF(TEXT("/Game/Ironwake/Ships/Devastation/SM_Devastation_BarrelsFore.SM_Devastation_BarrelsFore"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> BAF(TEXT("/Game/Ironwake/Ships/Devastation/SM_Devastation_BarrelsAft.SM_Devastation_BarrelsAft"));
	auto MakeTurret = [&](const TCHAR* Name, const FVector& Loc, float Yaw, UStaticMesh* TM, UStaticMesh* BM, const FVector& BarrelRel) -> USceneComponent*
	{
		USceneComponent* Pivot = CreateDefaultSubobject<USceneComponent>(Name);
		Pivot->SetupAttachment(HullMesh);
		Pivot->SetRelativeLocation(Loc);
		Pivot->SetRelativeRotation(FRotator(0.f, Yaw, 0.f));
		const FString N(Name);
		UStaticMeshComponent* Base = CreateDefaultSubobject<UStaticMeshComponent>(*(N + TEXT("_Base")));
		Base->SetupAttachment(Pivot);
		if (TM) Base->SetStaticMesh(TM);
		Base->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		// One mesh holds both guns; it pitches about the trunnion (named "Barrel" so the combat code finds it).
		UStaticMeshComponent* Barrel = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("%s_Barrel0"), Name));
		Barrel->SetupAttachment(Pivot);
		if (BM) Barrel->SetStaticMesh(BM);
		Barrel->SetRelativeLocation(BarrelRel);
		Barrel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return Pivot;
	};
	TurretFore = MakeTurret(TEXT("TurretFore"), FVector(2427.65f, 0.7f, 451.15f), 0.f, TFF.Object, BFF.Object, FVector(386.7f, 0.f, 149.25f));
	TurretAft = MakeTurret(TEXT("TurretAft"), FVector(-1968.51f, 0.7f, 451.15f), 180.f, TAF.Object, BAF.Object, FVector(474.89f, 0.f, 149.25f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneFinder(TEXT("/Engine/BasicShapes/Plane.Plane"));
	auto MakeWake = [&](const TCHAR* Name) -> UStaticMeshComponent*
	{
		UStaticMeshComponent* W = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		W->SetupAttachment(HullBody);
		if (PlaneFinder.Succeeded()) W->SetStaticMesh(PlaneFinder.Object);
		W->SetUsingAbsoluteRotation(true);
		W->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		W->SetCastShadow(false);
		W->SetGenerateOverlapEvents(false);
		W->SetTranslucentSortPriority(10);
		return W;
	};
	WakeStern = MakeWake(TEXT("WakeStern"));
	WakeBowL = MakeWake(TEXT("WakeBowL"));
	WakeBowR = MakeWake(TEXT("WakeBowR"));

	CameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArm"));
	CameraArm->SetupAttachment(HullBody);
	CameraArm->SetRelativeLocation(FVector(0.f, 0.f, 2500.f));
	CameraArm->TargetArmLength = ArmLength;
	CameraArm->bUsePawnControlRotation = true;
	CameraArm->bInheritPitch = true;
	CameraArm->bInheritYaw = true;
	CameraArm->bInheritRoll = false;
	CameraArm->bDoCollisionTest = false;
	// H1: the spring arm is only a fallback; CalcCamera() places the view (no lag on rotation, smoothed pivot).
	CameraArm->bEnableCameraLag = false;
	CameraArm->CameraLagSpeed = 9.f;
	CameraArm->bEnableCameraRotationLag = false;
	CameraArm->CameraRotationLagSpeed = 14.f;
	CameraArm->SetUsingAbsoluteRotation(true);
	CameraArm->TargetOffset = FVector(0.f, 0.f, 5500.f); // P1: a little higher so the horizon view clears the superstructure

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraArm, USpringArmComponent::SocketName);
	Camera->FieldOfView = 70.f;

	Buoyancy = CreateDefaultSubobject<UBuoyancyComponent>(TEXT("Buoyancy"));
	for (const FVector2D& P : PontoonXY)
	{
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			FSphericalPontoon Pontoon;
			Pontoon.RelativeLocation = FVector(P.X * 100.f, Side * P.Y * 100.f, PontoonZCm);
			Pontoon.Radius = PontoonRadiusCm;
			Buoyancy->BuoyancyData.Pontoons.Add(Pontoon);
		}
	}
	Buoyancy->BuoyancyData.bCenterPontoonsOnCOM = true;
	Buoyancy->BuoyancyData.MaxBuoyantForce = 1.0e13f;
	Buoyancy->BuoyancyData.BuoyancyRampMax = 1.f;
	Buoyancy->BuoyancyData.bApplyRiverForces = false;
	Buoyancy->BuoyancyData.bApplyDragForcesInWater = false;

	AutoPossessPlayer = EAutoReceiveInput::Disabled;
	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
}

void AIronwakeShipPawn::BeginPlay()
{
	ApplyHullDef();
	// Calibrate buoyancy to the hull mass BEFORE components begin play (async aux copies BuoyancyData).
	// Pontoon coefficients sum to 1, so total lift = BC * sum(coef_i * V_i). Float at ~55% immersion.
	HullBody->BodyInstance.SetMassOverride(MassKg);
	const float R = PontoonRadiusCm;
	const float Sub = 1.1f * R; // submerged depth of each sphere at design draft
	const float VolAtDraft = (PI / 3.f) * Sub * Sub * (3.f * R - Sub);
	const float G = 980.f;
	Buoyancy->BuoyancyData.BuoyancyCoefficient = MassKg * G / VolAtDraft;
	// Heave damping (only acts on upward motion inside the plugin); our own damping does the rest.
	Buoyancy->BuoyancyData.BuoyancyDamp = 0.6f * MassKg;
	Buoyancy->BuoyancyData.BuoyancyDamp2 = 0.f;

	// Lower centre of mass a little for roll stability (ironclads are bottom-heavy).
	HullBody->SetCenterOfMass(FVector(0.f, 0.f, -300.f));

	// Engine/keel model owns horizontal drag; generic damping would cap top speed.
	HullBody->SetLinearDamping(0.0f);
	HullBody->SetAngularDamping(0.02f);

	// W1: the plugin pontoons are replaced by the waterplane hydrostatics in TickHydro (no cork bobbing).
	Buoyancy->BuoyancyData.Pontoons.Reset();
	Buoyancy->SetComponentTickEnabled(false);
	Buoyancy->Deactivate();
	HullBody->SetEnableGravity(false);

	Super::BeginPlay();
	FleetBeginPlay();

	BowWave = NewObject<UIronwakeBowWave>(this, TEXT("BowWave"));
	BowWave->SetupAttachment(HullBody);
	BowWave->RegisterComponent();
	BowWave->Init(HullProf, HullLengthM / 125.f);
	{
		float MaxB = 0.f; for (float B : HullProf.HalfB) MaxB = FMath::Max(MaxB, B);
		Wake3 = NewObject<UIronwakeWake>(this, TEXT("Wake3"));
		Wake3->SetupAttachment(HullBody);
		Wake3->RegisterComponent();
		Wake3->Init(HullLengthM, MaxB, !HasSails());
	}
	if (!bIsNPC && FParse::Param(FCommandLine::Get(), TEXT("iww1"))) { bW1Test = true; if (FParse::Param(FCommandLine::Get(), TEXT("iww1clip"))) W1Mode = TEXT("clip"); UE_LOG(LogIronwake, Display, TEXT("IWW1 test armed %s"), *W1Mode); }

	ZoomIndex = FMath::Clamp(ZoomIndex, 0, ZoomStepsL.Num() - 1);
	ArmLength = ZoomStepsL.IsValidIndex(ZoomIndex) ? ZoomStepsL[ZoomIndex] * HullLengthM * 100.f : ArmLength;
	DesiredArm = ArmLength;
	CurArm = ArmLength;
	CameraArm->TargetArmLength = ArmLength;
	if (!bIsNPC && (FParse::Param(FCommandLine::Get(), TEXT("iwh1")) || IFileManager::Get().FileExists(*(FPaths::ProjectSavedDir() / TEXT("iw_h1.txt")))))
	{
		IFileManager::Get().Delete(*(FPaths::ProjectSavedDir() / TEXT("iw_h1.txt")));
		bH1Test = true;
		bH1Quit = FParse::Param(FCommandLine::Get(), TEXT("iwh1quit"));
		UE_LOG(LogIronwake, Display, TEXT("IWH1 test armed quit=%d"), bH1Quit);
	}

	// R1: turrets now carry the library materials (the old primitive steel tint is gone).

	if (UMaterialInterface* Foam = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Ironwake/FX/M_WakeFoam.M_WakeFoam")))
	{
		for (UStaticMeshComponent* W : { WakeStern.Get(), WakeBowL.Get(), WakeBowR.Get() })
		{
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Foam, this);
			W->SetMaterial(0, MID);
			WakeMIDs.Add(MID);
		}
	}

	// Automation hook: if Saved/iw_autodemo.txt exists, run the scripted voyage (injects real input
	// actions), take screenshots and log telemetry. The file's first line (optional) is the shot prefix.
	const FString Trigger = FPaths::ProjectSavedDir() / TEXT("iw_autodemo.txt");
	if (IFileManager::Get().FileExists(*Trigger))
	{
		FString Prefix;
		FFileHelper::LoadFileToString(Prefix, *Trigger);
		Prefix.TrimStartAndEndInline();
		DemoShotPrefix = Prefix.IsEmpty() ? TEXT("B") : Prefix;
		IFileManager::Get().Delete(*Trigger);
		bDemo = true;
		if (GEngine) GEngine->Exec(GetWorld(), TEXT("DisableAllScreenMessages"));
		UE_LOG(LogIronwake, Display, TEXT("IWDEMO start prefix=%s"), *DemoShotPrefix);
	}
	CombatBeginPlay();
	RegisterCollisionHull();
	UE_LOG(LogIronwake, Display, TEXT("IWSHIP BeginPlay at %s BC=%.3f mass=%.0f"), *GetActorLocation().ToString(), Buoyancy->BuoyancyData.BuoyancyCoefficient, HullBody->GetMass());
}

void AIronwakeShipPawn::ApplyHullDef()
{
	if (!bIsNPC)
	{
		// Chuck 22:30: zoom in to ~0.15 hull length; default stays at the old 1.5 L step.
		ZoomStepsL = { 0.2f, 0.3f, 0.42f, 0.6f, 0.8f, 1.15f, 1.5f, 2.2f, 3.2f, 4.6f };
		ZoomIndex = DefaultZoomIndex;
		ShipId = IronwakeFleet::GetSelected();
		ShipDef = IronwakeFleet::Find(ShipId);
		if (ShipDef && ShipId != TEXT("ironclad")) { ApplyFleetHull(); return; }
	}
	// R1 hull table. Player: HMS Devastation scaled 87 m -> 125 m (game ironclad class), beam 25.8 m.
	// NPC: RSS Kracken at 120 m, beam ~20 m; it is a casemate/broadside ship, so its turret pivots stay
	// invisible aim points at the fore/aft gun positions (no Devastation turrets bolted on).
	float LengthM = 125.f, BeamM = 25.8f;
	if (bIsNPC && MeshEnemyHull)
	{
		HullMesh->SetStaticMesh(MeshEnemyHull);
		LengthM = 120.f; BeamM = 20.f;
		TurretFore->SetRelativeLocation(FVector(4519.f, 0.8f, 496.f));
		TurretAft->SetRelativeLocation(FVector(-3997.f, 0.8f, 496.f));
		TArray<USceneComponent*> Kids;
		TurretFore->GetChildrenComponents(true, Kids);
		TArray<USceneComponent*> Kids2;
		TurretAft->GetChildrenComponents(true, Kids2);
		Kids.Append(Kids2);
		for (USceneComponent* K : Kids) K->SetVisibility(false);
		PropPort->SetVisibility(false); PropStbd->SetVisibility(false); RudderMesh->SetVisibility(false);
		MuzzleLocal[0] = FVector(300.f, -150.f, 0.f); MuzzleLocal[1] = FVector(300.f, 150.f, 0.f); // invisible gun position (casemates)
	}
	HullProf = (bIsNPC && MeshEnemyHull) ? FIronwakeHullProfile::Kracken() : FIronwakeHullProfile::Devastation();
	HullLengthM = LengthM;
	HullBody->SetBoxExtent(FVector(LengthM * 0.47f * 100.f, BeamM * 0.5f * 100.f, 700.f));
	// Pontoons: same two-row layout as the 140 m x 22 m blockout, rescaled to this hull.
	const float SX = LengthM / 140.f, SY = BeamM / 22.f;
	Buoyancy->BuoyancyData.Pontoons.Reset();
	for (const FVector2D& P : PontoonXY)
	{
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			FSphericalPontoon Pontoon;
			Pontoon.RelativeLocation = FVector(P.X * SX * 100.f, Side * P.Y * SY * 100.f, PontoonZCm);
			Pontoon.Radius = PontoonRadiusCm;
			Buoyancy->BuoyancyData.Pontoons.Add(Pontoon);
		}
	}
	HullMeshZ = PontoonZCm + 0.1f * PontoonRadiusCm + AIronwakeShell::RenderedSeaOffsetCm - DraftBelowBootTopCm;
	HullMesh->SetRelativeLocation(FVector(0.f, 0.f, HullMeshZ));
	UE_LOG(LogIronwake, Display, TEXT("IWR1 mesh boot-top at body z %.0f (static sea %.0f)"), HullMeshZ, PontoonZCm + 0.1f * PontoonRadiusCm);
	UE_LOG(LogIronwake, Display, TEXT("IWR1 hull %s L=%.0f m B=%.1f m pontoons=%d"), bIsNPC ? TEXT("Kracken(NPC)") : TEXT("Devastation"), LengthM, BeamM, Buoyancy->BuoyancyData.Pontoons.Num());
}

void AIronwakeShipPawn::TickHullCosmetics(float Dt)
{
	// Props: rev rate follows the engine order (~2 rev/s at flank), counter-rotating twin screws.
	const float RevPerSec = EngineOrderKnots / 16.f * 2.2f;
	PropAngle = FMath::Fmod(PropAngle + RevPerSec * 360.f * Dt, 360.f);
	if (PropPort && PropPort->IsVisible())
	{
		PropPort->SetRelativeRotation(FRotator(0.f, 0.f, PropAngle));
		PropStbd->SetRelativeRotation(FRotator(0.f, 0.f, -PropAngle));
		RudderMesh->SetRelativeRotation(FRotator(0.f, -RudderAngleDeg, 0.f));
	}
	if (HelmWheelPivot) HelmWheelPivot->SetRelativeRotation(FRotator(0.f, 0.f, RudderAngleDeg * 4.5f));
	if (HelmsmanPivot) HelmsmanPivot->SetRelativeRotation(FRotator(0.f, 0.f, FMath::Clamp(RudderAngleDeg * 0.14f, -4.5f, 4.5f)));
	// R1 waterline: buoyancy floats the body with each pontoon sphere 1.1 R deep (see BeginPlay), i.e. the static sea
	// level sits at PontoonZCm + 0.1 R = -70 cm on the body. The mesh boot-top goes DraftBelowBootTopCm under that.
	// (A runtime average does not work: the hull heaves +/-4 m on this swell; W1 owns the heavier physics.)
	(void)CalibT;
}

void AIronwakeShipPawn::EnsureInputObjects()
{
	if (ShipContext)
	{
		return;
	}
	auto MakeAction = [this](const TCHAR* Name, EInputActionValueType Type) -> UInputAction*
	{
		UInputAction* A = NewObject<UInputAction>(this, Name);
		A->ValueType = Type;
		return A;
	};
	IA_Throttle = MakeAction(TEXT("IA_Throttle"), EInputActionValueType::Axis1D);
	IA_Rudder = MakeAction(TEXT("IA_Rudder"), EInputActionValueType::Axis1D);
	IA_Center = MakeAction(TEXT("IA_CenterRudder"), EInputActionValueType::Boolean);
	IA_Look = MakeAction(TEXT("IA_Look"), EInputActionValueType::Axis2D);
	IA_Zoom = MakeAction(TEXT("IA_Zoom"), EInputActionValueType::Boolean);
	IA_Wheel = MakeAction(TEXT("IA_Wheel"), EInputActionValueType::Axis1D);
	IA_Fire = MakeAction(TEXT("IA_Fire"), EInputActionValueType::Boolean);
	IA_Repair = MakeAction(TEXT("IA_Repair"), EInputActionValueType::Boolean);
	IA_Scope = MakeAction(TEXT("IA_Scope"), EInputActionValueType::Boolean);
	IA_FreeLook = MakeAction(TEXT("IA_FreeLook"), EInputActionValueType::Boolean);
	IA_Steam = MakeAction(TEXT("IA_Steam"), EInputActionValueType::Boolean);

	ShipContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Ship"));
	auto MapStep = [this](UInputAction* A, const FKey& Key, bool bNegate)
	{
		FEnhancedActionKeyMapping& M = ShipContext->MapKey(A, Key);
		M.Triggers.Add(NewObject<UInputTriggerPressed>(ShipContext));
		if (bNegate)
		{
			M.Modifiers.Add(NewObject<UInputModifierNegate>(ShipContext));
		}
	};
	MapStep(IA_Throttle, EKeys::W, false);
	MapStep(IA_Throttle, EKeys::S, true);
	MapStep(IA_Throttle, EKeys::Up, false);
	MapStep(IA_Throttle, EKeys::Down, true);
	// H1: rudder is a held axis (A/D slew the wheel smoothly while held); no Pressed trigger.
	auto MapHold = [this](UInputAction* A, const FKey& Key, bool bNegate)
	{
		FEnhancedActionKeyMapping& M = ShipContext->MapKey(A, Key);
		if (bNegate) M.Modifiers.Add(NewObject<UInputModifierNegate>(ShipContext));
	};
	MapHold(IA_Rudder, EKeys::D, false);
	MapHold(IA_Rudder, EKeys::A, true);
	MapHold(IA_Rudder, EKeys::Right, false);
	MapHold(IA_Rudder, EKeys::Left, true);
	ShipContext->MapKey(IA_Center, EKeys::Q).Triggers.Add(NewObject<UInputTriggerPressed>(ShipContext));
	ShipContext->MapKey(IA_Center, EKeys::E).Triggers.Add(NewObject<UInputTriggerPressed>(ShipContext));
	ShipContext->MapKey(IA_Look, EKeys::Mouse2D);
	// H1: right mouse no longer zooms; it exits the scope. SHIFT toggles the scope, C / middle mouse = free look.
	ShipContext->MapKey(IA_Zoom, EKeys::RightMouseButton);
	ShipContext->MapKey(IA_Scope, EKeys::LeftShift).Triggers.Add(NewObject<UInputTriggerPressed>(ShipContext));
	ShipContext->MapKey(IA_Scope, EKeys::RightShift).Triggers.Add(NewObject<UInputTriggerPressed>(ShipContext));
	ShipContext->MapKey(IA_FreeLook, EKeys::C);
	ShipContext->MapKey(IA_FreeLook, EKeys::MiddleMouseButton);
	ShipContext->MapKey(IA_Wheel, EKeys::MouseWheelAxis);
	ShipContext->MapKey(IA_Fire, EKeys::LeftMouseButton).Triggers.Add(NewObject<UInputTriggerPressed>(ShipContext));
	ShipContext->MapKey(IA_Fire, EKeys::Gamepad_RightTrigger).Triggers.Add(NewObject<UInputTriggerPressed>(ShipContext));
	ShipContext->MapKey(IA_Repair, EKeys::R).Triggers.Add(NewObject<UInputTriggerPressed>(ShipContext));
	ShipContext->MapKey(IA_Steam, EKeys::B).Triggers.Add(NewObject<UInputTriggerPressed>(ShipContext)); // S2 auxiliary steam
	// Gamepad: D-pad steps, right stick camera.
	MapStep(IA_Throttle, EKeys::Gamepad_DPad_Up, false);
	MapStep(IA_Throttle, EKeys::Gamepad_DPad_Down, true);
	MapHold(IA_Rudder, EKeys::Gamepad_DPad_Right, false);
	MapHold(IA_Rudder, EKeys::Gamepad_DPad_Left, true);
	ShipContext->MapKey(IA_Look, EKeys::Gamepad_Right2D);
	ShipContext->MapKey(IA_Zoom, EKeys::Gamepad_LeftTrigger);
}

void AIronwakeShipPawn::AddMappingToLocalPlayer()
{
	EnsureInputObjects();
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (ULocalPlayer* LP = PC->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Sub = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				Sub->RemoveMappingContext(ShipContext);
				Sub->AddMappingContext(ShipContext, 0);
			}
		}
		// P1: start the camera over the bow at the horizon (aim point ~4 km ahead), not at the deck.
		ResetCameraToHorizon();
	}
}

void AIronwakeShipPawn::PawnClientRestart()
{
	Super::PawnClientRestart();
	AddMappingToLocalPlayer();
}

void AIronwakeShipPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	EnsureInputObjects();
	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EIC->BindAction(IA_Throttle, ETriggerEvent::Triggered, this, &AIronwakeShipPawn::OnThrottle);
		EIC->BindAction(IA_Rudder, ETriggerEvent::Triggered, this, &AIronwakeShipPawn::OnRudder);
		EIC->BindAction(IA_Rudder, ETriggerEvent::Completed, this, &AIronwakeShipPawn::OnRudderRelease);
		EIC->BindAction(IA_Steam, ETriggerEvent::Triggered, this, &AIronwakeShipPawn::OnSteamAction);
		EIC->BindAction(IA_Scope, ETriggerEvent::Triggered, this, &AIronwakeShipPawn::OnScopeAction);
		EIC->BindAction(IA_FreeLook, ETriggerEvent::Started, this, &AIronwakeShipPawn::OnFreeLookStart);
		EIC->BindAction(IA_FreeLook, ETriggerEvent::Completed, this, &AIronwakeShipPawn::OnFreeLookEnd);
		EIC->BindAction(IA_Center, ETriggerEvent::Triggered, this, &AIronwakeShipPawn::OnCenter);
		EIC->BindAction(IA_Look, ETriggerEvent::Triggered, this, &AIronwakeShipPawn::OnLook);
		EIC->BindAction(IA_Zoom, ETriggerEvent::Started, this, &AIronwakeShipPawn::OnZoomStart);
		EIC->BindAction(IA_Zoom, ETriggerEvent::Completed, this, &AIronwakeShipPawn::OnZoomEnd);
		EIC->BindAction(IA_Wheel, ETriggerEvent::Triggered, this, &AIronwakeShipPawn::OnWheel);
		EIC->BindAction(IA_Fire, ETriggerEvent::Triggered, this, &AIronwakeShipPawn::OnFireAction);
		EIC->BindAction(IA_Repair, ETriggerEvent::Triggered, this, &AIronwakeShipPawn::OnRepairAction);
	}
	else
	{
		UE_LOG(LogIronwake, Warning, TEXT("IWSHIP input component is not Enhanced Input"));
	}
}

void AIronwakeShipPawn::OnThrottle(const FInputActionValue& V)
{
	const float X = V.Get<float>();
	if (FMath::Abs(X) > 0.5f) StepTelegraph(X > 0.f ? 1 : -1);
}

void AIronwakeShipPawn::OnRudder(const FInputActionValue& V)
{
	// H1: held axis; the wheel slews in ApplyShipForces while |RudderInput| > 0.
	const float NewIn = FMath::Clamp(V.Get<float>(), -1.f, 1.f);
	if (bH1Test && FMath::Abs(NewIn - RudderInput) > 0.5f) UE_LOG(LogIronwake, Display, TEXT("IWH1 dbg OnRudder input %.2f"), NewIn);
	RudderInput = NewIn;
}

void AIronwakeShipPawn::OnRudderRelease(const FInputActionValue& V)
{
	RudderInput = 0.f;
	// Auto-centre (setting): step 0 lets ApplyShipForces spring the wheel back. Otherwise the wheel holds and the
	// step mirrors the held angle so code readers (helmsman, HUD) see it.
	RudderStep = IronwakeSettings::GetRudderAutoCentre() ? 0 : FMath::RoundToInt(RudderCmdDeg / FMath::Max(MaxRudderDeg, 1.f) * MaxRudderStep);
	LastRudderStep = RudderStep;
}

void AIronwakeShipPawn::OnScopeAction(const FInputActionValue& V)
{
	if (const AIronwakePlayerController* PC = Cast<AIronwakePlayerController>(GetController()); PC && (PC->IsMenuOpen() || !PC->bStarted)) return;
	SetScope(!bScope);
}

void AIronwakeShipPawn::SetScope(bool bOn)
{
	if (bScope == bOn) return;
	bScope = bOn;
	if (bOn) ScopeFov = 12.f;
	UE_LOG(LogIronwake, Display, TEXT("IWCAM scope %d"), bScope);
}

void AIronwakeShipPawn::OnFreeLookStart(const FInputActionValue& V)
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		bFreeLook = true;
		FreeLookSaved = PC->GetControlRotation();
	}
}

void AIronwakeShipPawn::OnFreeLookEnd(const FInputActionValue& V)
{
	if (!bFreeLook) return;
	bFreeLook = false;
	if (APlayerController* PC = Cast<APlayerController>(GetController())) PC->SetControlRotation(FreeLookSaved); // snap back to the aim
}

void AIronwakeShipPawn::IWZoom(int32 Index) { ZoomIndex = FMath::Clamp(Index, 0, ZoomStepsL.Num() - 1); }
void AIronwakeShipPawn::IWScope(int32 On) { SetScope(On != 0); }
void AIronwakeShipPawn::IWH1() { bH1Test = true; H1Time = 0.f; H1Stage = 0; }

void AIronwakeShipPawn::ApplyMusicSetting()
{
	const bool bOn = IronwakeSettings::GetMusic();
	for (UAudioComponent* A : { MusicCalm.Get(), MusicBattle.Get() })
	{
		if (!A) continue;
		if (!bOn) A->Stop();
	}
	if (bOn && MusicCalm && !bInCombat && !MusicCalm->IsPlaying()) MusicCalm->FadeIn(2.f, 0.55f);
}

void AIronwakeShipPawn::OnCenter(const FInputActionValue& V) { CenterRudder(); }

void AIronwakeShipPawn::OnLook(const FInputActionValue& V)
{
	// H1: raw 1:1 mouse (no smoothing); scope look is scaled by FOV so the reticle feels the same.
	FVector2D D = V.Get<FVector2D>();
	if (D.SizeSquared() > 1e-4f && GetWorld()) LastLookTime = GetWorld()->GetTimeSeconds();
	if (bScope) D *= FMath::Clamp(CurFov / 70.f, 0.08f, 1.f);
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		FRotator R = PC->GetControlRotation();
		R.Yaw = FRotator::NormalizeAxis(R.Yaw + D.X);
		const float Cz = GetCloseZoomFrac();
		R.Pitch = FMath::Clamp(FRotator::NormalizeAxis(R.Pitch + D.Y), bScope ? -20.f : FMath::Lerp(CamPitchMin, -60.f, Cz), bScope ? CamPitchMax : FMath::Lerp(CamPitchMax, 15.f, Cz));
		R.Roll = 0.f;
		PC->SetControlRotation(R);
	}
}

void AIronwakeShipPawn::OnZoomStart(const FInputActionValue& V) { if (bScope) SetScope(false); } // H1: RMB exits the scope
void AIronwakeShipPawn::OnZoomEnd(const FInputActionValue& V) {}

void AIronwakeShipPawn::OnWheel(const FInputActionValue& V)
{
	const float W = V.Get<float>();
	if (FMath::Abs(W) < 0.1f) return;
	if (bScope)
	{
		// Magnification: 20 deg (3.5x) .. 4 deg (17x).
		ScopeFov = FMath::Clamp(ScopeFov * (W > 0.f ? 0.75f : 1.333f), 4.f, 20.f);
		return;
	}
	ZoomIndex = FMath::Clamp(ZoomIndex + (W > 0.f ? -1 : 1), 0, ZoomStepsL.Num() - 1);
}

void AIronwakeShipPawn::StepTelegraph(int32 Delta)
{
	if (Delta > 0 && ReefedTo >= 0 && TelegraphIndex >= ReefedTo) { bReefOverride = true; UE_LOG(LogIronwake, Display, TEXT("IWW2 reef override: player sets more sail")); }
	TelegraphIndex = FMath::Clamp(TelegraphIndex + Delta, 0, TelegraphKnots.Num() - 1);
	UE_LOG(LogIronwake, Display, TEXT("IWSHIP telegraph -> %s"), *GetTelegraphName());
}

void AIronwakeShipPawn::StepRudder(int32 Delta)
{
	RudderStep = FMath::Clamp(RudderStep + Delta, -MaxRudderStep, MaxRudderStep);
	LastRudderStep = RudderStep;
	RudderCmdDeg = MaxRudderDeg * (float)RudderStep / (float)MaxRudderStep;
	UE_LOG(LogIronwake, Display, TEXT("IWSHIP rudder -> %s"), *GetRudderName());
}

void AIronwakeShipPawn::CenterRudder()
{
	RudderStep = 0;
	LastRudderStep = 0;
	RudderCmdDeg = 0.f;
	UE_LOG(LogIronwake, Display, TEXT("IWSHIP rudder -> MIDSHIPS"));
}

void AIronwakeShipPawn::IWTelegraph(int32 Index) { TelegraphIndex = FMath::Clamp(Index, 0, TelegraphKnots.Num() - 1); }
void AIronwakeShipPawn::SetAutoRudder(float Deg)
{
	RudderCmdDeg = FMath::Clamp(Deg, -MaxRudderDeg, MaxRudderDeg);
	RudderStep = FMath::Clamp(FMath::RoundToInt(RudderCmdDeg / FMath::Max(MaxRudderDeg, 1.f) * MaxRudderStep), -MaxRudderStep, MaxRudderStep);
	LastRudderStep = RudderStep;
}
void AIronwakeShipPawn::TestRudderInput(float V)
{
	if (FMath::Abs(V) > 0.01f) OnRudder(FInputActionValue(V));
	else OnRudderRelease(FInputActionValue(0.f));
}
void AIronwakeShipPawn::IWRudder(int32 Step) { RudderStep = FMath::Clamp(Step, -MaxRudderStep, MaxRudderStep); LastRudderStep = RudderStep; RudderCmdDeg = MaxRudderDeg * (float)RudderStep / (float)MaxRudderStep; }
void AIronwakeShipPawn::IWDemo() { bDemo = true; DemoTime = 0.f; DemoStep = 0; if (DemoShotPrefix.IsEmpty()) DemoShotPrefix = TEXT("B"); }

FString AIronwakeShipPawn::GetTelegraphName() const
{
	if (SailRig)
	{
		static const TCHAR* SailNames[] = { TEXT("FULL ASTERN (steam)"), TEXT("HALF ASTERN (steam)"), TEXT("FURLED"), TEXT("REEFED TOPSAILS"), TEXT("HALF SAIL"), TEXT("FULL SAIL"), TEXT("ALL SAIL + STAYSAILS") };
		if (ReefedTo >= 0 && !bReefOverride && TelegraphIndex > ReefedTo) return FString::Printf(TEXT("%s (AUTO-REEFED, W = override)"), SailNames[FMath::Clamp(ReefedTo, 0, 6)]);
		return SailNames[FMath::Clamp(TelegraphIndex, 0, 6)];
	}
	return TelegraphNames[FMath::Clamp(TelegraphIndex, 0, 6)];
}

FString AIronwakeShipPawn::GetRudderName() const
{
	// H1: continuous wheel; name the commanded angle.
	const float C = RudderCmdDeg;
	if (FMath::Abs(C) < 1.5f) return TEXT("MIDSHIPS");
	if (FMath::Abs(C) >= MaxRudderDeg - 0.5f) return FString::Printf(TEXT("HARD %s"), C < 0 ? TEXT("PORT") : TEXT("STBD"));
	return FString::Printf(TEXT("%s %.0f"), C < 0 ? TEXT("PORT") : TEXT("STBD"), FMath::Abs(C));
}

float AIronwakeShipPawn::GetSpeedKnots() const
{
	const FVector V = HullBody->GetPhysicsLinearVelocity();
	const float Fwd = FVector::DotProduct(V, GetActorForwardVector().GetSafeNormal2D());
	return Fwd / (KnotToCmS * FMath::Max(SpeedCompression, 0.01f));
}

float AIronwakeShipPawn::GetHeadingDeg() const
{
	float Yaw = GetActorRotation().Yaw;
	// Compass: 0 = +X (north in our world frame), clockwise positive, 0..360.
	return FMath::Fmod(Yaw + 360.f, 360.f);
}

void AIronwakeShipPawn::ApplyShipForces(float Dt)
{
	if (!HullBody->IsSimulatingPhysics()) return;

	// Engine room answers the telegraph (H1: ~1-2 s, the hull's inertia does the rest).
	float Ordered = TelegraphKnots.IsValidIndex(TelegraphIndex) ? TelegraphKnots[TelegraphIndex] : 0.f;
	if (SailRig)
	{
		// S2: ahead orders set sail (telegraph = sail plan); the auxiliary engine adds steam when raised (B). Astern = steam.
		const float Steam = (bAuxSteam || Ordered < 0.f) ? Ordered : 0.f;
		Ordered = Ordered < 0.f ? Steam : FMath::Max(SailKn, Steam) + 0.3f * FMath::Min(SailKn, FMath::Max(Steam, 0.f));
	}
	EngineOrderKnots = FMath::FInterpConstantTo(EngineOrderKnots, Ordered, Dt, EngineResponse);

	// H1 wheel: A/D held slews the commanded angle (35 deg in ~1.2 s); code that sets RudderStep (AI, helmsman,
	// console) takes over whenever the step changes. Steering gear then slews the blade to the command.
	if (bAutoHelm)
	{
		LastRudderStep = RudderStep; // A1: the autopilot owns RudderCmdDeg (SetAutoRudder)
	}
	else
	{
	if (RudderStep != LastRudderStep)
	{
		if (bH1Test) UE_LOG(LogIronwake, Display, TEXT("IWH1 dbg RudderStep changed by code %d -> %d"), LastRudderStep, RudderStep);
		LastRudderStep = RudderStep;
		RudderCmdDeg = MaxRudderDeg * (float)RudderStep / (float)MaxRudderStep;
	}
	if (FMath::Abs(RudderInput) > 0.1f)
	{
		RudderCmdDeg = FMath::Clamp(RudderCmdDeg + RudderInput * 30.f * Dt, -MaxRudderDeg, MaxRudderDeg);
	}
	else if (!bIsNPC && IronwakeSettings::GetRudderAutoCentre() && RudderStep == 0)
	{
		RudderCmdDeg = FMath::FInterpConstantTo(RudderCmdDeg, 0.f, Dt, 45.f);
	}
	}
	RudderAngleDeg = FMath::FInterpConstantTo(RudderAngleDeg, RudderCmdDeg, Dt, RudderSlewDegPerSec);

	const FVector Fwd = GetActorForwardVector().GetSafeNormal2D();
	const FVector Right = FVector(-Fwd.Y, Fwd.X, 0.f);
	const FVector Vel = HullBody->GetPhysicsLinearVelocity();
	const float VFwd = FVector::DotProduct(Vel, Fwd);
	const float VLat = FVector::DotProduct(Vel, Right);

	// Only push while the hull is actually in the water.
	const bool bInWater = true; // W1: hydrostatics keep the hull on the sea
	const float WaterFactor = bInWater ? 1.f : 0.f;

	// Horizontal motion and yaw are driven as velocities (deterministic, no async-substep force dilution);
	// buoyancy physics keeps heave, pitch and roll.
	const float TargetV = EngineOrderKnots * KnotToCmS * SpeedCompression;
	// H1: thrust-limited acceleration (scaled with speed compression) with an exponential final approach.
	const float MaxDv = MaxAccelMS2 * 100.f * FMath::Max(SpeedCompression / 1.5f, 1.f) * Dt;
	const float Want = (TargetV - VFwd) * FMath::Clamp(Dt / FMath::Max(SpeedTimeConstant, 0.2f), 0.f, 1.f);
	const float NewVFwd = VFwd + FMath::Clamp(Want, -MaxDv * 1.3f, MaxDv) * WaterFactor; // astern/braking a bit stronger
	// Keel grip: sideslip decays at LateralDrag/s (exp); some slip survives only in hard turns.
	float NewVLat = VLat * FMath::Exp(-LateralDrag * Dt * WaterFactor);
	if (SailRig && SailFill > 0.01f)
	{
		// S2 leeway: the sails push the hull sideways to leeward, most close-hauled (~6 deg slip), little running.
		const float Twa = FMath::Abs(TrueWindAngle);
		const float LeewayDeg = 6.f * FMath::Clamp((110.f - Twa) / 70.f, 0.f, 1.f) * FMath::Clamp(SailFill * 1.5f, 0.f, 1.f);
		const float WantLat = -FMath::Sign(TrueWindAngle) * FMath::Tan(FMath::DegreesToRadians(LeewayDeg)) * FMath::Max(NewVFwd, 0.f);
		NewVLat += (WantLat - NewVLat) * (1.f - FMath::Exp(-0.6f * Dt));
	}
	FVector PlanVel = Fwd * NewVFwd + Right * NewVLat;

	// Yaw: rudder authority scales with speed through water; turning radius at hard rudder = MinTurnRadiusM.
	const float RudderFrac = RudderAngleDeg / FMath::Max(MaxRudderDeg, 1.f);
	const float SpeedMS = NewVFwd / 100.f;
	float DesiredYawRateDeg = FMath::RadiansToDegrees(SpeedMS / FMath::Max(MinTurnRadiusM, 1.f)) * RudderFrac;
	if (SailRig && FMath::Abs(TrueWindAngle) < 40.f && FMath::Abs(SpeedMS) < 2.5f)
	{
		// In irons: the backed sails pay her head off the wind (~0.8 deg/s), the rudder still has a little bite.
		DesiredYawRateDeg += -FMath::Sign(TrueWindAngle == 0.f ? 1.f : TrueWindAngle) * 0.8f * (1.f - FMath::Abs(SpeedMS) / 2.5f) + 0.6f * RudderFrac;
	}
	FVector AngVelDeg = HullBody->GetPhysicsAngularVelocityInDegrees();
	// H1: yaw rate builds with time constant 1/YawResponse (~0.35 s) after the blade reaches its angle.
	AngVelDeg.Z = AngVelDeg.Z + ((DesiredYawRateDeg + (bIsNPC ? 0.f : WaveYawRate)) * WaterFactor - AngVelDeg.Z) * (1.f - FMath::Exp(-YawResponse * Dt));
	// P9: real collision + grounding edit the planned velocity and yaw rate (no-op in open water).
	{ float YR = (float)AngVelDeg.Z; TickCollision(Dt, PlanVel, YR); AngVelDeg.Z = YR; }
	HullBody->SetPhysicsLinearVelocity(PlanVel + FVector(0.f, 0.f, Vel.Z));
	HullBody->SetPhysicsAngularVelocityInDegrees(AngVelDeg);

	// W1: heave/pitch/roll (incl. outward heel in a turn) come from the waterplane hydrostatics.
	LastDesiredYawRate = DesiredYawRateDeg;
	TickHydro(Dt, DesiredYawRateDeg);
}

void AIronwakeShipPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float Dt = FMath::Min(DeltaSeconds, 0.1f);
	TickSails(Dt);
	ApplyShipForces(Dt);

	// H1: zoom target from the wheel step (eased in CalcCamera).
	if (ZoomStepsL.IsValidIndex(ZoomIndex)) ArmLength = ZoomStepsL[ZoomIndex] * HullLengthM * 100.f;
	CameraArm->TargetArmLength = ArmLength;
	if (bH1Test) TickH1Test(Dt);

	TickWake(Dt);
	TickHullCosmetics(Dt);
	if (!bSinking)
	{
		TickBowFX(Dt);
		TickWaterAudio(Dt);
	}
	if (bW1Test) TickW1Test(Dt);
	TickCombat(Dt);
	if (bDemo) TickAutoDemo(DeltaSeconds);
	TickTelemetry(Dt);
}

void AIronwakeShipPawn::TickTelemetry(float Dt)
{
	TelemetryAccum += Dt;
	if (TelemetryAccum < 2.f) return;
	TelemetryAccum = 0.f;
	const FRotator R = GetActorRotation();
	int32 InWater = 0;
	for (const FSphericalPontoon& P : Buoyancy->BuoyancyData.Pontoons) InWater += P.bIsInWater ? 1 : 0;
	UE_LOG(LogIronwake, Display, TEXT("IWTEL t=%.1f pos=(%.0f,%.0f,%.0f) hdg=%.1f pitch=%.2f roll=%.2f kn=%.2f tel=%s rud=%s(%.1f) eng=%.1f pontoonsWet=%d/%d"),
		GetWorld()->GetTimeSeconds(), GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z, GetHeadingDeg(), R.Pitch, R.Roll,
		GetSpeedKnots(), *GetTelegraphName(), *GetRudderName(), RudderAngleDeg, EngineOrderKnots, InWater, Buoyancy->BuoyancyData.Pontoons.Num());
}

void AIronwakeShipPawn::TickAutoDemo(float Dt)
{
	DemoTime += Dt;
	APlayerController* PC = Cast<APlayerController>(GetController());
	UEnhancedInputLocalPlayerSubsystem* Sub = (PC && PC->GetLocalPlayer()) ? PC->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	auto Inject = [&](UInputAction* A, float X)
	{
		if (Sub && A)
		{
			Sub->InjectInputForAction(A, FInputActionValue(X), {}, {});
		}
		else
		{
			UE_LOG(LogIronwake, Warning, TEXT("IWDEMO no input subsystem; stepping directly"));
			if (A == IA_Throttle) StepTelegraph(X > 0 ? 1 : -1); else if (A == IA_Rudder) StepRudder(X > 0 ? 1 : -1);
		}
	};
	auto Shot = [&](const TCHAR* Tag, float CamYaw, float CamPitch)
	{
		if (PC)
		{
			PC->SetControlRotation(FRotator(CamPitch, GetActorRotation().Yaw + CamYaw, 0.f));
		}
		const FString File = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("shots") / FString::Printf(TEXT("%s_%s.png"), *DemoShotPrefix, Tag));
		FScreenshotRequest::RequestScreenshot(File, true, false);
		UE_LOG(LogIronwake, Display, TEXT("IWDEMO screenshot %s t=%.1f kn=%.2f hdg=%.1f pitch=%.2f roll=%.2f"), *File, DemoTime, GetSpeedKnots(), GetHeadingDeg(), GetActorRotation().Pitch, GetActorRotation().Roll);
	};

	// Script (sim seconds since demo start). Camera framing is set ~2 s before each shot for the lag to settle.
	struct FStep { float T; int32 Kind; float A; float B; const TCHAR* Tag; };
	// Kind: 0 throttle step, 1 rudder step, 2 camera pre-frame, 3 screenshot, 4 end
	static const FStep Steps[] = {
		{ 3.f, 0, 1, 0, nullptr }, { 3.5f, 0, 1, 0, nullptr }, { 4.f, 0, 1, 0, nullptr }, { 4.5f, 0, 1, 0, nullptr }, // Stop -> Flank via W x4
		{ 30.f, 2, 140, -10, nullptr }, { 34.f, 3, 140, -10, TEXT("1_ahead") },
		{ 36.f, 1, 1, 0, nullptr }, { 36.5f, 1, 1, 0, nullptr }, { 37.f, 1, 1, 0, nullptr }, // D x3 -> hard starboard
		{ 62.f, 2, 60, -8, nullptr }, { 66.f, 3, 60, -8, TEXT("2_turning") },
		{ 70.f, 1, -1, 0, nullptr }, { 70.5f, 1, -1, 0, nullptr }, { 71.f, 1, -1, 0, nullptr }, // A x3 -> midships
		{ 86.f, 2, -150, -6, nullptr }, { 90.f, 3, -150, -6, TEXT("3_wake") },
		{ 92.f, 4, 0, 0, nullptr },
	};
	constexpr int32 NumSteps = UE_ARRAY_COUNT(Steps);
	while (DemoStep < NumSteps && DemoTime >= Steps[DemoStep].T)
	{
		const FStep& S = Steps[DemoStep++];
		switch (S.Kind)
		{
		case 0: Inject(IA_Throttle, S.A); break;
		case 1: Inject(IA_Rudder, S.A); break;
		case 2: if (PC) PC->SetControlRotation(FRotator(S.B, GetActorRotation().Yaw + S.A, 0.f)); break;
		case 3: Shot(S.Tag, S.A, S.B); break;
		case 4: bDemo = false; UE_LOG(LogIronwake, Display, TEXT("IWDEMO done")); break;
		}
	}
}

void AIronwakeShipPawn::TickWake(float Dt)
{
	// Planes are 100x100 cm; texcoord U runs along the strip (U=0 at the hull, fading to 1 at the far end).
	const float Kn = FMath::Max(GetSpeedKnots(), 0.f);
	const float Frac = FMath::Clamp(Kn / 16.f, 0.f, 1.f);
	const FVector Fwd = GetActorForwardVector().GetSafeNormal2D();
	const float Yaw = Fwd.Rotation().Yaw;
	// Sea level under the ship: body origin rides at the float line, so use its Z minus draft offset.
	const float SeaZ = IronwakeSea::RenderedZ(GetWorld(), GetActorLocation() - GetActorForwardVector().GetSafeNormal2D() * HullLengthM * 60.f) + 15.f;
	const FVector Base = GetActorLocation();
	const bool bShow = Kn > 0.5f;
	// W3: the flat stern plane is replaced by the Kelvin wake trail (UIronwakeWake) that rides the waves for ~60 s.
	WakeStern->SetVisibility(false);
	WakeBowL->SetVisibility(false); WakeBowR->SetVisibility(false); // W1: replaced by UIronwakeBowWave
	static const bool bNoWake = FParse::Param(FCommandLine::Get(), TEXT("iwnowake"));
	{   // P19 + P27: this hull is a Havelock LUT wave + turbulent prop-wash source (CPU + M_IW_WaterRoot synchronized)
		FIwShipWaveSrc Sw; const FVector Lc = GetActorLocation(); const FVector Fw2 = GetActorForwardVector().GetSafeNormal2D();
		Sw.PosCm = FVector2D(Lc.X, Lc.Y); Sw.CosH = Fw2.X; Sw.SinH = Fw2.Y;
		Sw.V = FMath::Max(Kn, 0.f) * 0.514444f * SpeedCompression; Sw.L = HullProf.Length(); Sw.X1 = HullProf.X1;
		float MaxB = 0.f; for (float B : HullProf.HalfB) MaxB = FMath::Max(MaxB, B);
		Sw.HalfBeam = MaxB;
		if (ShipDef) { Sw.Draft = ShipDef->DraftM; Sw.Freeboard = ShipDef->FreeboardM; Sw.CrestK = ShipDef->WaveCrestK; Sw.TroughK = ShipDef->WaveTroughK; Sw.QuarterK = ShipDef->WaveQuarterK; Sw.WedgeK = ShipDef->WaveWedgeK; Sw.Bluff = ShipDef->WaveBluff; }
		else { Sw.Draft = HullProf.KeelM; Sw.Freeboard = HullProf.BowDeckM; Sw.Bluff = 0.7f; }
		const bool bSteamActive = !HasSails() || bAuxSteam;
		const float SteamKn = !HasSails()
			? FMath::Abs(EngineOrderKnots)
			: (bAuxSteam && TelegraphKnots.IsValidIndex(TelegraphIndex) ? FMath::Abs(TelegraphKnots[TelegraphIndex]) : 0.f);
		Sw.bPlayer = !bIsNPC;
		Sw.bTwinScrew = (ShipId == TEXT("ironclad"));
		Sw.ThrustSteam = bSteamActive ? FMath::Clamp(FMath::Max(SteamKn, Kn * 0.85f) / 13.f, 0.f, 1.15f) : 0.f;
		if (bSteamActive && TelegraphIndex == 2 && SteamKn < 0.25f) Sw.ThrustSteam = 0.f; // STOP telegraph stops screw cavitation
		Sw.RollOutM = FMath::Sin(FMath::DegreesToRadians(FMath::Abs(GetActorRotation().Roll))) * Sw.Freeboard * 0.65f;
		if (!bSinking) IronwakeSea::RegisterShipWave(this, Sw);
		if (Wake3) Wake3->SetPropWash(bSteamActive, Sw.bTwinScrew, Sw.ThrustSteam);
	}
	if (Wake3 && !bNoWake) Wake3->UpdateWake(Dt, GetActorTransform(), HullProf.X0 + 2.f, HullProf.X1 - 2.f, Kn * 51.4444f * SpeedCompression / 100.f, FMath::Clamp(Kn / 13.f, 0.f, 1.2f), IronwakeSea::GetSeaState());
	if (!bShow) return;

	// Stern wake: starts at the stern (-70 m) and trails 120..450 m behind; 26..40 m wide.
	const float WakeSc = FMath::Clamp(HullLengthM / 125.f, 0.3f, 1.f); // S2: smaller hulls, narrower/shorter wake
	const float SternLen = FMath::Lerp(15000.f, 90000.f, Frac) * FMath::Sqrt(WakeSc); // W2: long wake (~40 s of track at speed)
	const float SternW = FMath::Lerp(2400.f, 3400.f, Frac) * WakeSc;
	// Plane UV: U along local X (0 at -X edge). Yaw +180 puts U=0 at the stern end.
	WakeStern->SetWorldRotation(FRotator(0.f, Yaw + 180.f, 0.f));
	WakeStern->SetWorldScale3D(FVector(SternLen / 100.f, SternW / 100.f, 1.f));
	WakeStern->SetWorldLocation(FVector(Base.X, Base.Y, SeaZ) - Fwd * (HullLengthM * 43.f + SternLen * 0.5f));

	// Bow waves: from the bow, sweeping back and out at ~20 deg each side, 90 m long.
	const float BowLen = FMath::Lerp(4000.f, 11000.f, Frac);
	const float BowW = FMath::Lerp(500.f, 1100.f, Frac);
	const FVector BowPt = FVector(Base.X, Base.Y, SeaZ + 5.f) + Fwd * HullLengthM * 46.f;
	for (int32 Side = -1; Side <= 1; Side += 2)
	{
		UStaticMeshComponent* W = Side < 0 ? WakeBowL : WakeBowR;
		const float A = Yaw + 180.f - Side * 19.f;
		const FVector Dir = FRotator(0.f, A, 0.f).Vector();
		W->SetWorldRotation(FRotator(0.f, A, 0.f));
		W->SetWorldScale3D(FVector(BowLen / 100.f, BowW / 100.f, 1.f));
		W->SetWorldLocation(BowPt + Dir * (BowLen * 0.5f) + FVector(-Dir.Y, Dir.X, 0.f) * 0.f);
	}
	for (int32 i = 0; i < WakeMIDs.Num(); ++i)
	{
		WakeMIDs[i]->SetScalarParameterValue(TEXT("Opacity"), (i == 0 ? 1.0f : 1.0f) * FMath::Clamp(Frac * 1.3f, 0.f, 1.f));
	}
}


// ============================================================================ H1: World-of-Warships camera
void AIronwakeShipPawn::CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (bIsNPC || !PC)
	{
		Super::CalcCamera(DeltaTime, OutResult);
		return;
	}
	const float Dt = FMath::Clamp(DeltaTime, 0.f, 0.1f);
	UWorld* W = GetWorld();
	// Pivot: hull position, horizontally tight (no visible lag) and vertically heavily smoothed, so the hull's
	// heave/pitch/roll never shake the view. Rotation comes only from the control rotation (always level).
	const FVector Hull = GetActorLocation();
	const float SeaHere = AIronwakeShell::WaterZAt(W, Hull);
	if (HullTopAboveOrigin < 0.f && HullMesh && HullMesh->GetStaticMesh())
	{
		// Measured once in the hull's own frame (unaffected by heel): top of masts/funnels over the body origin.
		const FBox LB = HullMesh->GetStaticMesh()->GetBoundingBox();
		const FTransform Rel = HullMesh->GetRelativeTransform();
		HullTopAboveOrigin = Rel.TransformPosition(FVector(0.f, 0.f, LB.Max.Z)).Z;
		UE_LOG(LogIronwake, Display, TEXT("IWCAM hull top %.1f m above body origin (hull %.0f m)"), HullTopAboveOrigin / 100.f, HullLengthM);
	}
	// Pivot just above the highest mast/funnel so the crosshair ray clears the ship (WoWS: ship below the aim).
	const float PivotFar = FMath::Max(HullTopAboveOrigin, 0.2f * HullLengthM * 100.f) + 0.06f * HullLengthM * 100.f; // ~45 m above the waterline on the 140 m ironclad: ship sits below the screen centre
	// Chuck 22:30: at the closest zoom steps the pivot drops to deck level amidships so you look AT the ship.
	const float DeckPivot = (SailRig ? (ShipId == TEXT("brig") ? 5.2f : 7.f) : FMath::Clamp(0.11f * HullLengthM, 3.f, 12.f)) * 100.f; // sail ship: under the courses
	// W3: in a GALE/STORM the chase camera drops to ~0.6x height and closes in (crests stand above the horizon from the troughs).
	const float StormCam = FMath::Clamp((IronwakeSea::GetSeaState() - 2.5f) / 1.5f, 0.f, 1.f);
	const float PivotH = FMath::Lerp(PivotFar, DeckPivot, GetCloseZoomFrac()) * FMath::Lerp(1.f, 0.6f, StormCam);
	const FVector Target(Hull.X, Hull.Y, (FMath::IsFinite(SeaHere) && FMath::Abs(SeaHere - Hull.Z) < 5000.f ? SeaHere : Hull.Z) + PivotH);
	if (!bPivotInit || FVector::DistSquared(PivotSmoothed, Target) > FMath::Square(50000.f))
	{
		PivotSmoothed = Target;
		bPivotInit = true;
	}
	else
	{
		const float AXY = 1.f - FMath::Exp(-14.f * Dt);
		const float AZ = 1.f - FMath::Exp(-1.2f * Dt);
		PivotSmoothed.X += (Target.X - PivotSmoothed.X) * AXY;
		PivotSmoothed.Y += (Target.Y - PivotSmoothed.Y) * AXY;
		PivotSmoothed.Z += (Target.Z - PivotSmoothed.Z) * AZ;
	}
	FRotator R = PC->GetControlRotation();
	R.Roll = 0.f;
	R.Pitch = FRotator::NormalizeAxis(R.Pitch);
	// Quick eased zoom (wheel steps) and FOV (scope).
	CurArm += (ArmLength - CurArm) * (1.f - FMath::Exp(-7.f * Dt));
	const float WantFov = bScope ? ScopeFov : 70.f;
	CurFov += (WantFov - CurFov) * (1.f - FMath::Exp(-14.f * Dt));
	FVector Loc;
	if (bScope)
	{
		// Binoculars: eye half a hull length out along the aim (clear of the funnels/superstructure), above deck.
		Loc = PivotSmoothed + R.Vector().GetSafeNormal2D() * 0.55f * HullLengthM * 100.f;
	}
	else
	{
		Loc = PivotSmoothed - R.Vector() * CurArm * FMath::Lerp(1.f, 0.78f, StormCam);
	}
	// Close range: push the camera out of the ship's own hull + superstructure (hull frame box, yaw only).
	if (!bScope)
	{
		const FTransform HX(FRotator(0.f, GetActorRotation().Yaw, 0.f), Hull);
		FVector Lc = HX.InverseTransformPosition(Loc);
		const float Xc = HullMesh ? -HullMesh->GetRelativeLocation().X : 0.f; (void)Xc;
		const float X0 = (HullProf.X0 - 2.f) * 100.f, X1 = (HullProf.X1 + 2.f) * 100.f;
		float MaxB = 0.f; for (float B : HullProf.HalfB) MaxB = FMath::Max(MaxB, B);
		const float Yh = (MaxB + 1.5f) * 100.f;
		const float Zt = FMath::Max(HullProf.BowDeckM + 3.5f, 0.09f * HullLengthM) * 100.f;
		// Sailing rig: yards span ~+/-12 m up to ~48 m; keep the camera outside that box (no sails across the lens).
		if (SailRig)
		{
			const float RigY = (MaxB + (ShipId == TEXT("brig") ? 6.5f : 10.f)) * 100.f, RigZ0 = Zt, RigZ1 = (ShipId == TEXT("brig") ? 28.f : 52.f) * 100.f;
			const float RX0 = X0 + 600.f, RX1 = X1 - 900.f;
			if (Lc.X > RX0 && Lc.X < RX1 && FMath::Abs(Lc.Y) < RigY && Lc.Z > RigZ0 && Lc.Z < RigZ1)
			{
				const float DY = RigY - FMath::Abs(Lc.Y), DZ = RigZ1 - Lc.Z;
				Lc.Y = FMath::Sign(Lc.Y == 0.f ? 1.f : Lc.Y) * RigY; (void)DY; (void)DZ; // always step out sideways (above the rig = sails across the lens)
			}
		}
		if (Lc.X > X0 && Lc.X < X1 && FMath::Abs(Lc.Y) < Yh && Lc.Z < Zt)
		{
			const float DX = FMath::Min(Lc.X - X0, X1 - Lc.X), DY = Yh - FMath::Abs(Lc.Y), DZ = Zt - Lc.Z;
			if (DZ <= DY && DZ <= DX) Lc.Z = Zt;
			else if (DY <= DX) Lc.Y = FMath::Sign(Lc.Y == 0.f ? 1.f : Lc.Y) * Yh;
			else Lc.X = (Lc.X - X0 < X1 - Lc.X) ? X0 : X1;
			Loc = HX.TransformPosition(Lc);
		}
	}
	// P16 CAMERA MOTION. The view is level and steady by default; LOW/MEDIUM let it follow the ship's own heave/pitch/roll by a
	// small fraction, heavily low-passed (tau >= 2 s), hard-capped. Never in binoculars (photo mode uses its own camera).
	{
		const int32 CM = IronwakeSettings::GetCameraMotion();
		float TgtH = 0.f, TgtP = 0.f, TgtR = 0.f;
		if (CM > 0 && !bScope)
		{
			const FRotator SR = GetActorRotation();
			TgtP = SR.Pitch; TgtR = SR.Roll;
			TgtH = (FMath::IsFinite(SeaHere) ? (Hull.Z - SeaHere - 70.f) : 0.f);   // heave about the flat plane (cm)
		}
		const float Tau = CM >= 2 ? 2.0f : 2.5f;
		const float Al = 1.f - FMath::Exp(-Dt / Tau);
		CamLpHeave += (TgtH - CamLpHeave) * Al; CamLpPitch += (TgtP - CamLpPitch) * Al; CamLpRoll += (TgtR - CamLpRoll) * Al;
		const float Gain = CM >= 2 ? 0.30f : 0.15f;
		const float CapR = CM >= 2 ? 3.0f : 1.5f, CapP = CM >= 2 ? 2.0f : 1.0f, CapZ = (CM >= 2 ? 60.f : 30.f);
		const float Rel = FMath::DegreesToRadians(R.Yaw - GetActorRotation().Yaw);
		const float Pw = CamLpPitch * FMath::Cos(Rel) - CamLpRoll * FMath::Sin(Rel);   // tilt about the camera's right axis
		const float Rw = CamLpRoll * FMath::Cos(Rel) + CamLpPitch * FMath::Sin(Rel);   // tilt about the view axis
		DbgCamSwayPitch = (CM > 0 && !bScope) ? FMath::Clamp(Pw * Gain, -CapP, CapP) : 0.f;
		DbgCamSwayRoll = (CM > 0 && !bScope) ? FMath::Clamp(Rw * Gain, -CapR, CapR) : 0.f;
		DbgCamSwayZ = (CM > 0 && !bScope) ? FMath::Clamp(CamLpHeave * Gain, -CapZ, CapZ) : 0.f;
		R.Pitch += DbgCamSwayPitch; R.Roll = DbgCamSwayRoll; Loc.Z += DbgCamSwayZ;
	}
	// P9: impact jolt (decaying shake, scaled by hull size).
	if (ShakeAmpCm > 1.f)
	{
		ShakeT += Dt;
		ShakeAmpCm *= FMath::Exp(-3.2f * Dt);
		Loc += FVector(FMath::Sin(ShakeT * 41.f), FMath::Cos(ShakeT * 53.f), 0.6f * FMath::Sin(ShakeT * 67.f)) * ShakeAmpCm;
	}
	// Never under (or skimming) the rendered water: >= 2 m above it.
	const float SeaCam = IronwakeSea::RenderedZ(W, Loc);
	const float MinZ = (FMath::IsFinite(SeaCam) && FMath::Abs(SeaCam - Hull.Z) < 5000.f ? SeaCam : Hull.Z - 470.f) + 200.f;
	if (Loc.Z < MinZ) Loc.Z = MinZ;
	OutResult.Location = Loc;
	OutResult.Rotation = R;
	OutResult.FOV = CurFov;
	OutResult.AspectRatio = 16.f / 9.f;
	OutResult.bConstrainAspectRatio = false;
	OutResult.ProjectionMode = ECameraProjectionMode::Perspective;
	OutResult.PostProcessBlendWeight = 0.f;
	if (Camera) { Camera->SetWorldLocationAndRotation(Loc, R); }
}

// ============================================================================ H1 handling test driver
// -iwh1 [-iwh1quit] or Saved/iw_h1.txt. Measures: time to half speed, time for the bow to start swinging after A,
// sideslip after a hard turn; then takes the three camera captures. All via the same functions as the keys.
void AIronwakeShipPawn::TickH1Test(float Dt)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;
	H1Time += Dt;
	const float T = H1Time;
	const float Kn = GetSpeedKnots();
	const float Hdg = GetActorRotation().Yaw;
	const FVector Fwd = GetActorForwardVector().GetSafeNormal2D();
	const FVector Vel = HullBody->GetPhysicsLinearVelocity();
	const float VLat = FVector::DotProduct(Vel, FVector(-Fwd.Y, Fwd.X, 0.f)) / 100.f;
	const float VF = FVector::DotProduct(Vel, Fwd) / 100.f;
	const float YawRate = HullBody->GetPhysicsAngularVelocityInDegrees().Z;
	auto Shot = [&](const TCHAR* Name)
	{
		const FString File = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("shots") / (FString(Name) + TEXT(".png")));
		FScreenshotRequest::RequestScreenshot(File, true, false);
		UE_LOG(LogIronwake, Display, TEXT("IWH1 shot %s t=%.1f kn=%.1f zoom=%d arm=%.0fm fov=%.1f"), *File, T, Kn, ZoomIndex, CurArm / 100.f, CurFov);
	};
	auto Next = [&](int32 S) { H1Stage = S; H1Mark = T; };
	const float Since = T - H1Mark;
	if (FMath::Fmod(T, 1.f) < Dt)
	{
		UE_LOG(LogIronwake, Display, TEXT("IWH1 tel t=%.1f stage=%d kn=%.2f hdg=%.1f yawRate=%.2f rud=%.1f cmd=%.1f vlat=%.2f m/s drift=%.1f deg"),
			T, H1Stage, Kn, Hdg, YawRate, RudderAngleDeg, RudderCmdDeg, VLat, FMath::RadiansToDegrees(FMath::Atan2(VLat, FMath::Max(FMath::Abs(VF), 0.1f))));
	}
	switch (H1Stage)
	{
	case 0: // settle at stop
		if (T > 4.f) { IWTelegraph(2); CenterRudder(); Next(1); }
		break;
	case 1: // ring HALF AHEAD from STOP
		if (Since > 2.f)
		{
			StepTelegraph(+1); StepTelegraph(+1); // STOP -> SLOW -> HALF, as two W presses
			UE_LOG(LogIronwake, Display, TEXT("IWH1 telegraph HALF AHEAD at t=%.1f (target %.1f kn)"), T, TelegraphKnots[4]);
			Next(2);
		}
		break;
	case 2:
		if (H1T50 < 0.f && Kn >= 0.5f * TelegraphKnots[4]) { H1T50 = Since; UE_LOG(LogIronwake, Display, TEXT("IWH1 RESULT time_to_50pct_half=%.1f s"), Since); }
		if (H1T90 < 0.f && Kn >= 0.9f * TelegraphKnots[4]) { H1T90 = Since; UE_LOG(LogIronwake, Display, TEXT("IWH1 RESULT time_to_90pct_half=%.1f s"), Since); }
		if (Since == Dt) {}
		if (H1T90 >= 0.f && Since > H1T90 + 4.f) { StepTelegraph(+1); Next(3); } // FULL for the turn test
		if (Since > 60.f) { UE_LOG(LogIronwake, Warning, TEXT("IWH1 half speed not reached in 60 s (kn=%.1f)"), Kn); Next(3); }
		break;
	case 3: // settle at FULL, then hold A
		if (Since > 14.f)
		{
			H1HdgStart = Hdg; H1LastHdg = Hdg; H1TurnVisible = -1.f;
			UE_LOG(LogIronwake, Display, TEXT("IWH1 press A (hold) at t=%.1f kn=%.1f hdg=%.1f"), T, Kn, Hdg);
			Next(4);
		}
		break;
	case 4: // A held for 1.5 s (commanded wheel -> ~ -35 deg), then released (wheel holds)
		RudderInput = Since < 1.5f ? -1.f : 0.f;
		if (Since >= 1.5f && Since - Dt < 1.5f) OnRudderRelease(FInputActionValue(0.f));
		{
			const float Turned = FMath::Abs(FMath::FindDeltaAngleDegrees(H1HdgStart, Hdg));
			if (H1TurnVisible < 0.f && (FMath::Abs(YawRate) >= 0.5f || Turned >= 0.5f))
			{
				H1TurnVisible = Since;
				UE_LOG(LogIronwake, Display, TEXT("IWH1 RESULT bow_starts_swinging=%.2f s after A (yawRate %.2f deg/s, turned %.2f deg)"), Since, YawRate, Turned);
			}
			if (Since >= 1.f && Since - Dt < 1.f) UE_LOG(LogIronwake, Display, TEXT("IWH1 RESULT at 1.0 s: turned %.2f deg, yawRate %.2f deg/s, rudder %.1f"), Turned, YawRate, RudderAngleDeg);
			if (Since >= 3.f && Since - Dt < 3.f) UE_LOG(LogIronwake, Display, TEXT("IWH1 RESULT at 3.0 s: turned %.2f deg, yawRate %.2f deg/s"), Turned, YawRate);
			if (Since >= 20.f && Since - Dt < 20.f)
			{
				UE_LOG(LogIronwake, Display, TEXT("IWH1 RESULT steady hard turn: yawRate %.2f deg/s kn=%.1f vlat=%.2f m/s drift=%.1f deg; centring (Q)"), YawRate, Kn, VLat, FMath::RadiansToDegrees(FMath::Atan2(VLat, FMath::Max(FMath::Abs(VF), 0.1f))));
				CenterRudder();
				Next(5);
			}
		}
		break;
	case 5: // sideslip decay after the hard turn
		if ((Since >= 1.f && Since - Dt < 1.f) || (Since >= 2.f && Since - Dt < 2.f) || (Since >= 4.f && Since - Dt < 4.f))
			UE_LOG(LogIronwake, Display, TEXT("IWH1 RESULT after centring +%.0f s: vlat=%.2f m/s yawRate=%.2f deg/s"), Since, VLat, YawRate);
		if (Since >= 6.f) { IWTelegraph(4); Next(6); }
		break;
	case 6: // camera captures: default tight view astern, zoomed out, scope
		if (Since < Dt * 1.5f) { ZoomIndex = 2; PC->SetControlRotation(FRotator(DefaultCamPitch, Hdg, 0.f)); }
		if (Since >= 3.f) { Shot(TEXT("H1_cam_default")); Next(7); }
		break;
	case 7:
		if (Since < Dt * 1.5f) { ZoomIndex = ZoomStepsL.Num() - 1; PC->SetControlRotation(FRotator(-14.f, Hdg + 25.f, 0.f)); }
		if (Since >= 3.f) { Shot(TEXT("H1_cam_zoomed_out")); Next(8); }
		break;
	case 8:
		if (Since < Dt * 1.5f)
		{
			ZoomIndex = 2;
			// Scope on the nearest point of interest / other ship ahead if any: look at the nearest actor tagged by the voyage.
			FVector Look = GetActorLocation() + Fwd * 400000.f;
			float Best = 1e12f;
			for (TActorIterator<AActor> It(GetWorld()); It; ++It)
			{
				const FString N = It->GetName();
				if (!(N.Contains(TEXT("Merchant")) || N.Contains(TEXT("Lighthouse")) || N.Contains(TEXT("Moored")) || N.Contains(TEXT("Airship")))) continue;
				const float D = FVector::Dist2D(It->GetActorLocation(), GetActorLocation());
				if (D > 150000.f && D < 900000.f && D < Best) { Best = D; Look = It->GetActorLocation(); }
			}
			const FVector Eye = PivotSmoothed;
			FRotator LR = (Look - Eye).Rotation(); LR.Roll = 0.f; LR.Pitch = FMath::Clamp(LR.Pitch, -10.f, 3.f);
			PC->SetControlRotation(LR);
			SetScope(true);
			ScopeFov = 8.f;
			UE_LOG(LogIronwake, Display, TEXT("IWH1 scope target at %.1f km"), Best < 1e11f ? Best / 100000.f : -1.f);
		}
		if (Since >= 3.f) { Shot(TEXT("H1_scope")); Next(9); }
		break;
	case 9:
		if (Since >= 2.f)
		{
			SetScope(false);
			UE_LOG(LogIronwake, Display, TEXT("IWH1 done half50=%.1f half90=%.1f swing=%.2f"), H1T50, H1T90, H1TurnVisible);
			bH1Test = false;
			if (bH1Quit) UKismetSystemLibrary::QuitGame(this, PC, EQuitPreference::Quit, false);
		}
		break;
	default: break;
	}
}

float AIronwakeShipPawn::GetCloseZoomFrac() const
{
	const float Lm = FMath::Max(HullLengthM, 1.f) * 100.f;
	const float ZL = CurArm / Lm;                  // eased arm in hull lengths
	const float Far = ZoomStepsL.IsValidIndex(5) ? ZoomStepsL[5] : 1.15f, Near = ZoomStepsL.IsValidIndex(1) ? ZoomStepsL[1] : 0.25f;
	return FMath::SmoothStep(0.f, 1.f, FMath::Clamp((Far - ZL) / FMath::Max(Far - Near, 0.01f), 0.f, 1.f));
}
