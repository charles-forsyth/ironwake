// S1: Voyager (peaceful) mode. Points of interest, gulls, weather/time cycle, photo mode, helmsman/anchor/fast-sail,
// compass + minimap HUD and the S1 PIE test driver. Deterministic code only (no LLM, no new assets).
#include "IronwakeVoyage.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "IronwakeLivingSea.h"
#include "IronwakeShell.h"
#include "IronwakeHydro.h"
#include "IronwakeShipPawn.h"
#include "IronwakePlayerController.h"
#include "IronwakeTest.h"
#include "IronwakeWorld.h"
#include "IronwakeFleet.h"
#include "IronwakeNpc.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/SkyLight.h"
#include "WaterZoneActor.h"
#include "WaterMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/HUD.h"
#include "GameFramework/SpringArmComponent.h"
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"
#include "IronwakeStorm.h"
#include "IronwakeArchipelago.h"
#include "IronwakeP8.h"

namespace
{
	UMaterialInstanceDynamic* VTint(UObject* Outer, const FLinearColor& C)
	{
		static TWeakObjectPtr<UMaterialInterface> Base;
		if (!Base.IsValid()) Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		if (!Base.IsValid()) return nullptr;
		UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Base.Get(), Outer);
		M->SetVectorParameterValue(TEXT("Color"), C);
		return M;
	}

	float BearingDeg(const FVector& From, const FVector& To)
	{
		const FVector D = To - From;
		return FMath::Fmod(FMath::RadiansToDegrees(FMath::Atan2(D.Y, D.X)) + 360.f, 360.f);
	}

	const TCHAR* CompassName(float Deg)
	{
		static const TCHAR* N[] = { TEXT("N"), TEXT("NE"), TEXT("E"), TEXT("SE"), TEXT("S"), TEXT("SW"), TEXT("W"), TEXT("NW") };
		return N[((int32)FMath::RoundToInt(FMath::Fmod(Deg + 360.f, 360.f) / 45.f)) % 8];
	}

	const TCHAR* WeatherNames[] = { TEXT("OVERCAST DAY (moderate)"), TEXT("DUSK (calm)"), TEXT("FOGGY NIGHT (rough)"), TEXT("GALE"), TEXT("STORM"), TEXT("CLEAR NIGHT (calm)") };

	constexpr float GullScale = 2.2f;
}

// ============================================================================ Gull (P28: SeaGullV4 + hinged wing_L / wing_R)
AIronwakeGull::AIronwakeGull()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereF(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeF(TEXT("/Engine/BasicShapes/Cube.Cube"));
	const float G = GullScale;
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(Root);
	Body->SetStaticMesh(SphereF.Object);
	Body->SetRelativeScale3D(FVector(0.55f, 0.17f, 0.15f) * G);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetCastShadow(false);
	Body->SetCullDistance(45000.f);
	auto MakeWing = [&](const TCHAR* PN, const TCHAR* WN, float Side, TObjectPtr<USceneComponent>& Pivot, TObjectPtr<UStaticMeshComponent>& Wing)
	{
		Pivot = CreateDefaultSubobject<USceneComponent>(PN);
		Pivot->SetupAttachment(Root);
		Pivot->SetRelativeLocation(FVector(4.f * G, Side * 6.f * G, 2.f * G));
		Wing = CreateDefaultSubobject<UStaticMeshComponent>(WN);
		Wing->SetupAttachment(Pivot);
		Wing->SetStaticMesh(CubeF.Object);
		Wing->SetRelativeScale3D(FVector(0.26f, 0.72f, 0.018f) * G);
		Wing->SetRelativeLocation(FVector(-3.f * G, Side * 36.f * G, 0.f));
		Wing->SetRelativeRotation(FRotator(0.f, -Side * 8.f, 0.f));
		Wing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Wing->SetCastShadow(false);
		Wing->SetCullDistance(45000.f);
	};
	MakeWing(TEXT("PivotL"), TEXT("WingL"), -1.f, PivotL, WingL);
	MakeWing(TEXT("PivotR"), TEXT("WingR"), 1.f, PivotR, WingR);
}

void AIronwakeGull::BeginPlay()
{
	Super::BeginPlay();
	UStaticMesh* Gm = IronwakeWorld::Mesh(TEXT("SeaGullV4"));
	UStaticMesh* Wl = IronwakeWorld::Mesh(TEXT("SeaGullV4_wing_L"));
	UStaticMesh* Wr = IronwakeWorld::Mesh(TEXT("SeaGullV4_wing_R"));
	if (Gm && Wl && Wr)
	{
		// P28: textured Atlantic gull (1.30 m native wingspan * 1.65 = 2.15 m readable sky silhouette); pivots from SeaGullV4/parts.json (Y_ue = -100*Y_b)
		constexpr float Gs = 1.65f;
		Body->SetStaticMesh(Gm);
		Body->SetRelativeLocation(FVector::ZeroVector);
		Body->SetRelativeRotation(FRotator::ZeroRotator);
		Body->SetRelativeScale3D(FVector(Gs));

		PivotL->SetRelativeLocation(FVector(6.35f, -10.40f, 11.64f) * Gs);
		WingL->SetStaticMesh(Wl);
		WingL->SetRelativeLocation(FVector::ZeroVector);
		WingL->SetRelativeRotation(FRotator::ZeroRotator);
		WingL->SetRelativeScale3D(FVector(Gs));

		PivotR->SetRelativeLocation(FVector(6.07f,  10.40f, 11.61f) * Gs);
		WingR->SetStaticMesh(Wr);
		WingR->SetRelativeLocation(FVector::ZeroVector);
		WingR->SetRelativeRotation(FRotator::ZeroRotator);
		WingR->SetRelativeScale3D(FVector(Gs));
	}
	else
	{
		if (UMaterialInstanceDynamic* W = VTint(this, FLinearColor(0.80f, 0.80f, 0.78f))) Body->SetMaterial(0, W);
		if (UMaterialInstanceDynamic* Wg = VTint(this, FLinearColor(0.55f, 0.57f, 0.60f))) { WingL->SetMaterial(0, Wg); WingR->SetMaterial(0, Wg); }
	}
	Phase = FMath::FRand() * 10.f;
	FlapHz = FMath::FRandRange(2.45f, 2.95f);
}

void AIronwakeGull::Tick(float Dt)
{
	Super::Tick(Dt);
	Dt = FMath::Min(Dt, 0.1f);
	T += Dt;
	FVector C = Centre;
	if (AActor* F = Follow.Get()) C = F->GetActorLocation() + F->GetActorRotation().RotateVector(FollowOffset);
	Angle += Dir * SpeedCmS * Dt / FMath::Max(RadiusCm, 100.f);
	const float R = RadiusCm * (1.f + 0.18f * FMath::Sin(T * 0.23f + Phase));
	const FVector P = FVector(C.X, C.Y, Follow.IsValid() ? C.Z : 0.f) + FVector(FMath::Cos(Angle) * R, FMath::Sin(Angle) * R, AltCm + 900.f * FMath::Sin(T * 0.4f + Phase));
	const FVector Tan = FVector(-FMath::Sin(Angle), FMath::Cos(Angle), 0.f) * Dir;
	// Smooth flap (3.1 s) <-> soaring dihedral glide (2.1 s) blend with crisp downstroke harmonics
	const float Cycle = FMath::Fmod(T + Phase, 5.2f);
	const float FlapEnv = FMath::SmoothStep(0.0f, 0.35f, Cycle) * (1.f - FMath::SmoothStep(2.75f, 3.20f, Cycle));
	const float Om = 2.f * PI * FlapHz * (T + Phase);
	const float FlapWave = 31.f * FMath::Sin(Om) + 6.5f * FMath::Cos(2.f * Om);
	const float GlideArch = 8.5f + 2.2f * FMath::Sin(T * 1.1f + Phase);
	const float UpDeg = FMath::Lerp(GlideArch, FlapWave, FlapEnv);
	const float BodyBob = FlapEnv * -2.2f * FMath::Cos(Om);
	SetActorLocationAndRotation(P + FVector(0.f, 0.f, BodyBob), FRotator(3.5f * FMath::Sin(T * 0.7f) + FlapEnv * 1.8f * FMath::Sin(Om), Tan.Rotation().Yaw, -Dir * 19.f));
	// In UE (+X fwd, +Y right, +Z up), +Roll raises -Y (WingL) and lowers +Y (WingR), so WingL=+UpDeg and WingR=-UpDeg flap symmetrically!
	PivotL->SetRelativeRotation(FRotator(0.f, 0.f,  UpDeg));
	PivotR->SetRelativeRotation(FRotator(0.f, 0.f, -UpDeg));
}

// ============================================================================ Voyage director
AIronwakeVoyage::AIronwakeVoyage()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> RockF(TEXT("/PCG/SampleContent/SimpleForest/Meshes/PCG_Boulder_02.PCG_Boulder_02"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylF(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeF(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeF(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereF(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> HullF(TEXT("/Game/Ironwake/Ships/Kracken/SM_Kracken_Hull.SM_Kracken_Hull"));
	static ConstructorHelpers::FObjectFinder<USoundBase> HarbourF(TEXT("/Game/Ironwake/Audio/harbour_lap_proc.harbour_lap_proc"));
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> SplashF(TEXT("/Game/Ironwake/FX/NS_IW_Splash.NS_IW_Splash"));
	RockMesh = RockF.Object; CylMesh = CylF.Object; ConeMesh = ConeF.Object; CubeMesh = CubeF.Object; SphereMesh = SphereF.Object;
	HullMesh = HullF.Object; HarbourSnd = HarbourF.Object; SplashFX = SplashF.Object;
}

AIronwakeVoyage* AIronwakeVoyage::Get(const UWorld* W)
{
	if (!W) return nullptr;
	TActorIterator<AIronwakeVoyage> It(const_cast<UWorld*>(W));
	return It ? *It : nullptr;
}

AIronwakeShipPawn* AIronwakeVoyage::Player() const
{
	APlayerController* P = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	return P ? Cast<AIronwakeShipPawn>(P->GetPawn()) : nullptr;
}

AIronwakePlayerController* AIronwakeVoyage::PC() const
{
	return GetWorld() ? Cast<AIronwakePlayerController>(GetWorld()->GetFirstPlayerController()) : nullptr;
}

UStaticMeshComponent* AIronwakeVoyage::Mesh(USceneComponent* Parent, UStaticMesh* M, const FVector& Loc, const FRotator& Rot, const FVector& Scale, const FLinearColor* Tint)
{
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
	C->SetStaticMesh(M);
	C->SetupAttachment(Parent ? Parent : RootComponent.Get());
	C->SetRelativeLocation(Loc);
	C->SetRelativeRotation(Rot);
	C->SetRelativeScale3D(Scale);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->RegisterComponent();
	AddInstanceComponent(C);
	if (Tint) if (UMaterialInstanceDynamic* MI = VTint(this, *Tint)) C->SetMaterial(0, MI);
	return C;
}

UPointLightComponent* AIronwakeVoyage::Lamp(USceneComponent* Parent, const FVector& Loc, const FLinearColor& Color, float Candelas, float RadiusCm)
{
	UPointLightComponent* L = NewObject<UPointLightComponent>(this);
	L->SetupAttachment(Parent ? Parent : RootComponent.Get());
	L->SetRelativeLocation(Loc);
	L->SetIntensityUnits(ELightUnits::Candelas);
	L->SetIntensity(0.f);
	L->SetAttenuationRadius(RadiusCm);
	L->SetLightColor(Color);
	L->SetCastShadows(false);
	L->RegisterComponent();
	AddInstanceComponent(L);
	L->ComponentTags.Add(*FString::Printf(TEXT("cd=%.0f"), Candelas));
	NightLamps.Add(L);
	return L;
}

void AIronwakeVoyage::BeginPlay()
{
	Super::BeginPlay();
	if (GEngine) GEngine->Exec(GetWorld(), TEXT("DisableAllScreenMessages")); // E1: no dev "REFLECTION CAPTURES NEED TO BE REBUILT" text in the game
	// P34: ensure WaterZone covers the full 36 km archipelago AND WaterMesh uses 1600 cm LOD0 tiles (~12.5 cm vertex spacing) so hull-waterline contacts are smooth everywhere!
	for (TActorIterator<AWaterZone> WzIt(GetWorld()); WzIt; ++WzIt)
	{
		AWaterZone* Wz = *WzIt;
		if (!Wz) continue;
		if (Wz->GetZoneExtent().X < 3500000.f) Wz->SetZoneExtent(FVector2D(3600000.f, 3600000.f));
		if (UWaterMeshComponent* Wm = Wz->GetWaterMeshComponent())
		{
			Wm->SetTileSize(1600.f);
		}
		Wz->MarkForRebuild(EWaterZoneRebuildFlags::All, this);
	}
	InitWorldPorts();
	BuildHarbour();
	BuildWreck();
	BuildStacks();
	BuildBuoys();
	// I1: the archipelago (14 seeded islands) - spawned before the POI list so its islands append after the 4 core points.
	{
		FActorSpawnParameters ASP;
		ASP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ASP.Owner = this;
		GetWorld()->SpawnActor<AIronwakeArchipelago>(AIronwakeArchipelago::StaticClass(), FTransform::Identity, ASP);
	}
	BuildSecondaryPorts();
	CaptureWeatherBaseline();
	{
		FActorSpawnParameters NSP; NSP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn; NSP.Owner = this;
		NightSky = GetWorld()->SpawnActor<AIronwakeNightSky>(AIronwakeNightSky::StaticClass(), FTransform::Identity, NSP);
	}

	// POIs in sailing order from the spawn (just outside the harbour): wreck -> stacks -> lighthouse -> home.
	auto Add = [&](const TCHAR* N, const TCHAR* Tg, const FVector& L, float Arrive, float Hard)
	{
		FIronwakePOI P; P.Name = N; P.Tag = Tg; P.Loc = FVector(L.X, L.Y, 0.f); P.ArriveRadiusCm = Arrive; P.HardRadiusCm = Hard; POIs.Add(P);
	};
	Add(TEXT("Wreck of the Valiant"), TEXT("W"), WreckC, 60000.f, 9000.f);
	Add(TEXT("The Needles (sea stacks)"), TEXT("N"), StacksC, 70000.f, 26000.f);
	FVector Island(-50000.f, 190000.f, 0.f);
	if (AIronwakeLivingSea* LS = AIronwakeLivingSea::Get(GetWorld())) Island = LS->IslandCentre;
	Add(TEXT("St Brannoc Light"), TEXT("L"), Island, 80000.f, 0.f); // card D already stops ships at 300 m
	Add(TEXT("Greyhaven Harbour"), TEXT("H"), HarbourC, 60000.f, 0.f);
	// I1: every archipelago island is a point of interest (indices 4+; progression still counts the first 4).
	if (AIronwakeArchipelago* Ar = AIronwakeArchipelago::Get(GetWorld()))
	{
		for (int32 i = 0; i < Ar->Isles.Num(); ++i)
		{
			const FIsle& I = Ar->Isles[i];
			int32 PortSpecIdx = -1;
			if (IwPorts::IsHostedPortIsle(i, WorldPorts, &PortSpecIdx))
				Add(*WorldPorts[PortSpecIdx].Name, *WorldPorts[PortSpecIdx].Tag, WorldPorts[PortSpecIdx].MoorWorldCm, WorldPorts[PortSpecIdx].MoorRadiusM * 100.f, 0.f);
			else
				Add(*I.Name, *I.Tag, I.C, I.HazardR + 25000.f, 0.f);
		}
	}
	SpawnGulls();

	// Lighthouse beam: a rotating narrow spot from the card-D lamp, only lit at dusk/night.
	FVector LampPos = Island + FVector(-2500.f, 800.f, 7000.f);
	if (AIronwakeLivingSea* LS = AIronwakeLivingSea::Get(GetWorld())) if (LS->LampLight) LampPos = LS->LampLight->GetComponentLocation();
	Beam = NewObject<USpotLightComponent>(this, TEXT("LighthouseBeam"));
	Beam->SetupAttachment(RootComponent);
	Beam->SetWorldLocation(LampPos);
	Beam->SetIntensityUnits(ELightUnits::Candelas);
	Beam->SetIntensity(0.f);
	Beam->SetAttenuationRadius(450000.f);
	Beam->SetInnerConeAngle(1.5f);
	Beam->SetOuterConeAngle(4.0f);
	Beam->SetLightColor(FLinearColor(1.f, 0.85f, 0.6f));
	Beam->SetCastShadows(false);
	Beam->SetVolumetricScatteringIntensity(4.f);
	Beam->RegisterComponent();
	AddInstanceComponent(Beam);

	const FString Trig = FPaths::ProjectSavedDir() / TEXT("iw_s1.txt");
	if (IFileManager::Get().FileExists(*Trig))
	{
		FFileHelper::LoadFileToString(TestMode, *Trig);
		TestMode.TrimStartAndEndInline();
		IFileManager::Get().Delete(*Trig);
	}
	BuildObstacles();
	{
		FString A1;
		if (FParse::Value(FCommandLine::Get(), TEXT("iwa1="), A1)) { A1Mode = A1; UE_LOG(LogIronwake, Display, TEXT("IWA1 test armed '%s'"), *A1Mode); }
		{ FString P9; if (FParse::Value(FCommandLine::Get(), TEXT("iwp9="), P9)) { P9Mode = P9; UE_LOG(LogIronwake, Display, TEXT("IWP9 test armed '%s'"), *P9Mode); } }
		{ FString P8; if (FParse::Value(FCommandLine::Get(), TEXT("iwp8="), P8)) { P8Mode = P8; UE_LOG(LogIronwake, Display, TEXT("IWP8 test armed '%s'"), *P8Mode); } }
		FString E1;
		if (FParse::Value(FCommandLine::Get(), TEXT("iwe1="), E1)) { E1Mode = E1; UE_LOG(LogIronwake, Display, TEXT("IWE1 test armed '%s'"), *E1Mode); }
		FString S2;
		{ FString W2; if (FParse::Value(FCommandLine::Get(), TEXT("iww2="), W2)) { W2Mode = W2; UE_LOG(LogIronwake, Display, TEXT("IWW2 test armed '%s'"), *W2Mode); } }
		if (FParse::Param(FCommandLine::Get(), TEXT("iwstorm"))) bStormStart = true;
		if (FParse::Value(FCommandLine::Get(), TEXT("iws2="), S2)) { S2Mode = S2; if (S2Mode == TEXT("progress")) SetTickableWhenPaused(true); UE_LOG(LogIronwake, Display, TEXT("IWS2 test armed '%s'"), *S2Mode); }
	}
	{
		{ FString P12; if (FParse::Value(FCommandLine::Get(), TEXT("iwp12="), P12)) { P12Mode = P12; UE_LOG(LogIronwake, Display, TEXT("IWP12 test armed '%s'"), *P12Mode); } }
		{ FString P29; if (FParse::Value(FCommandLine::Get(), TEXT("iwp29="), P29)) { P29Mode = P29; UE_LOG(LogIronwake, Display, TEXT("IWP29 test armed '%s'"), *P29Mode); } }
		{ FString P28; if (FParse::Value(FCommandLine::Get(), TEXT("iwp28="), P28)) { P28Mode = P28; UE_LOG(LogIronwake, Display, TEXT("IWP28 test armed '%s'"), *P28Mode); } }
		{ FString P21; if (FParse::Value(FCommandLine::Get(), TEXT("iwp21="), P21)) { P21Mode = P21; UE_LOG(LogIronwake, Display, TEXT("IWP21 test armed '%s'"), *P21Mode); } }
		{ FString P19; if (FParse::Value(FCommandLine::Get(), TEXT("iwp19="), P19)) { P19Mode = P19; UE_LOG(LogIronwake, Display, TEXT("IWP19 test armed '%s'"), *P19Mode); } }
		{ FString P16; if (FParse::Value(FCommandLine::Get(), TEXT("iwp16="), P16)) { P16Mode = P16; if (P16Mode == TEXT("menu")) SetTickableWhenPaused(true); UE_LOG(LogIronwake, Display, TEXT("IWP16 test armed '%s'"), *P16Mode); } }
		FString P11; if (FParse::Value(FCommandLine::Get(), TEXT("iwp11="), P11)) { P11Mode = P11; UE_LOG(LogIronwake, Display, TEXT("IWP11 test armed '%s'"), *P11Mode); }
		const bool bOtherTest = !A1Mode.IsEmpty() || !E1Mode.IsEmpty() || !P8Mode.IsEmpty() || !P9Mode.IsEmpty() || !P16Mode.IsEmpty() || !P19Mode.IsEmpty() || !P21Mode.IsEmpty() || !P28Mode.IsEmpty() || !P29Mode.IsEmpty() || !S2Mode.IsEmpty() || !W2Mode.IsEmpty() || bStormStart || !TestMode.IsEmpty();
		if (!bOtherTest && !FParse::Param(FCommandLine::Get(), TEXT("iwlegacywx"))) StartLiving();
	}
	UE_LOG(LogIronwake, Display, TEXT("IWS1 voyage up: %d POIs, %d gulls, %d buoys, %d night lamps, beam at %s, test='%s'"), POIs.Num(), Gulls.Num(), Buoys.Num(), NightLamps.Num(), *LampPos.ToString(), *TestMode);
}

void AIronwakeVoyage::EndPlay(const EEndPlayReason::Type Reason)
{
	if (bPhoto && GetWorld()) UGameplayStatics::SetGlobalTimeDilation(this, 1.f);
	Super::EndPlay(Reason);
}

// ---------------------------------------------------------------------------- builders
void AIronwakeVoyage::BuildHarbour()
{
	// E1: Greyhaven rebuilt from real assets (IronwakeWorld.h). Layout in metres in the harbour frame: +X seaward (NE),
	// +Y along the quay, z relative to the RENDERED sea (query plane + RenderedSeaOffsetCm). Quay top 2 m above the sea.
	UWorld* W = GetWorld();
	float SeaZ = 0.f;
	for (int32 i = 0; i < 9; ++i) SeaZ += AIronwakeShell::WaterZAt(W, HarbourC + FVector((i % 3 - 1) * 5000.f, (i / 3 - 1) * 5000.f, 0.f));
	SeaZ /= 9.f;
	USceneComponent* H = NewObject<USceneComponent>(this, TEXT("Harbour"));
	H->SetupAttachment(RootComponent);
	H->RegisterComponent();
	AddInstanceComponent(H);
	H->SetWorldLocationAndRotation(FVector(HarbourC.X, HarbourC.Y, SeaZ), FRotator(0.f, 45.f, 0.f));
	USceneComponent* Hs = NewObject<USceneComponent>(this, TEXT("HarbourSea"));
	Hs->SetupAttachment(H);
	Hs->SetRelativeLocation(FVector(0.f, 0.f, AIronwakeShell::RenderedSeaOffsetCm));
	Hs->RegisterComponent();
	AddInstanceComponent(Hs);
	const FLinearColor Stone(0.085f, 0.085f, 0.08f), Glow(5.f, 3.4f, 1.6f);
	const auto M = [](float X, float Y, float Z) { return FVector(X * 100.f, Y * 100.f, Z * 100.f); };
	const float Q = 2.0f; // quay top (m above the rendered sea)
	auto P = [&](const TCHAR* N, float X, float Y, float Z, float Yaw, float S = 1.f, float Pitch = 0.f, float Roll = 0.f)
	{ return IronwakeWorld::Place(this, Hs, N, M(X, Y, Z), FRotator(Pitch, Yaw, Roll), FVector(S)); };
	auto P3 = [&](const TCHAR* N, float X, float Y, float Z, float Yaw, const FVector& S)
	{ return IronwakeWorld::Place(this, Hs, N, M(X, Y, Z), FRotator(0.f, Yaw, 0.f), S); };
	FRandomStream Rng(1798);

	// Quay ground: Poly Haven coast-rock slabs (60 x 42 m scans, x2) instead of the old untextured grey block; tops ~Q.
	{
		const float Ys[] = { -128.f, -43.f, 43.f, 128.f };
		for (int32 i = 0; i < 4; ++i)
			P3(i % 2 ? TEXT("CoastRocks02") : TEXT("CoastRocks01"), -112.f + (i % 2) * 6.f, Ys[i], Q - 5.4f, (i % 2) ? 90.f : 180.f * (i / 2), FVector(2.05f, 2.05f, 1.6f));
	}
	// Headland cliffs behind the town (Poly Haven coastal cliffs; the sheer face is UE +Y, so yaw -90 turns it seaward to +X).
	// P15: the two inner headland cliffs (X -235/-245, ~40 m tall) stood inside the new hillside as a dark wall behind the town; the heightfield replaces them.
	if (FParse::Param(FCommandLine::Get(), TEXT("iwoldcliffs")))
	{
		P(TEXT("CoastalCliff04"), -235, -120, -6, -90, 4.0f);
		P(TEXT("CoastalCliff01"), -245, 175, -6, -88, 3.6f);
	}
	P(TEXT("CoastalCliff02"), -200, -335, -6, -122, 4.0f);
	P(TEXT("CoastalCliff02"), -190, 335, -6, -58, 4.0f);
	P(TEXT("CoastRocks02"), -175, -290, -2.2f, 20, 2.2f);
	P(TEXT("CoastRocks01"), -165, 300, -2.2f, 75, 2.0f);
	// Town: 1798 wharf building (its log wharf meets the quay edge at x = -50), brick factory with a smoking chimney,
	// steampunk dock warehouse, coaling station and steam crane on the quay.
	P(TEXT("WharfBuilding"), -69.6f, -95, Q + 0.15f, 0);
	UStaticMeshComponent* Factory = P(TEXT("VictorianFactory"), -128, -102, Q, 90);   // P10: industry moved behind the wharf building, town gets the middle
	P(TEXT("DockWarehouse"), -112, 146, Q, 0);
	P(TEXT("CoalingStation"), -64, 140, Q, 90);
	P(TEXT("SteamCrane"), -51.5f, -30, Q, 0);
	if (UNiagaraSystem* Smoke = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/Ironwake/FX/NS_IW_Smoke.NS_IW_Smoke")))
	{
		if (Factory)
			if (UNiagaraComponent* N = UNiagaraFunctionLibrary::SpawnSystemAttached(Smoke, Factory, NAME_None, FVector(-1092.f, 472.f, 2800.f), FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, false))
				{ N->SetWorldScale3D(FVector(0.045f)); N->Deactivate(); N->SetVisibility(false); }
	}
	// Timber pier on piles: Poly Haven modular pier, chained so each copy's pile-only tail sits under the next deck.
	for (int32 i = 0; i < 6; ++i) P(TEXT("ModularPier"), -35.7f + 22.9f * i, 0, Q - 5.43f, 90);
	// Rubble breakwaters (coast_land_rocks scans) leaving the mouth on the seaward side.
	auto Rubble = [&](float CX, float CY, float YawDeg, float LenM)
	{
		const float Yr = FMath::DegreesToRadians(YawDeg);
		int32 k = 0;
		for (float t = -LenM * 0.5f; t <= LenM * 0.5f + 1.f; t += 25.f, ++k)
		{
			const float X = CX + FMath::Cos(Yr) * t, Y = CY + FMath::Sin(Yr) * t;
			const float J = Rng.FRandRange(-12.f, 12.f), S = Rng.FRandRange(0.9f, 1.15f);
			if (k % 2 == 0) P3(TEXT("CoastLandRocks02"), X, Y, -4.f, YawDeg - 90.f + J, FVector(4.5f, 3.0f, 5.5f) * S);
			else P3(TEXT("CoastLandRocks03"), X, Y, -4.f, YawDeg + J, FVector(3.0f, 4.0f, 5.0f) * S);
		}
	};
	Rubble(60, -190, 12, 300);
	Rubble(10, 200, -18, 200);
	// Harbour mouth: steampunk signal tower on a rock base at the south tip (red light), stone beacon at the north tip (green).
	P(TEXT("CoastRocks03"), 205, -160, -1.5f, 30, 1.2f);
	P(TEXT("SignalTower"), 205, -160, 2.0f, 200);
	const FLinearColor RedGlow(6.f, 0.4f, 0.2f);
	Mesh(Hs, SphereMesh, M(205, -160, 31.5f), FRotator::ZeroRotator, FVector(2.2f), &RedGlow);
	Lamp(Hs, M(205, -160, 31.5f), FLinearColor(1.f, 0.15f, 0.1f), 60000.f, 60000.f);
	{
		const FVector Tip = M(105, 170, 0);
		const FLinearColor LC(0.3f, 5.f, 0.6f);
		P(TEXT("CoastRocks03"), 105, 170, -1.5f, 120, 0.9f);
		P(TEXT("GasLamp"), 105, 170, 1.4f, 0, 2.6f); // tall iron beacon post (lamp post x2.6 = 12 m)
		Mesh(Hs, SphereMesh, Tip + M(0, 0, 13.6f), FRotator::ZeroRotator, FVector(1.4f), &LC);
		Lamp(Hs, Tip + M(0, 0, 13.6f), FLinearColor(0.2f, 1.f, 0.3f), 20000.f, 40000.f);
	}
	// Gas lamps along the quay edge and the pier (Hunyuan lamp post; warm point light at the lantern).
	const float LampY[] = { -140.f, -100.f, -60.f, -25.f, 32.f, 70.f, 110.f };
	for (float Y : LampY) { P(TEXT("GasLamp"), -52.5f, Y, Q, 0); Lamp(Hs, M(-52.5f, Y, Q + 4.2f), FLinearColor(1.f, 0.72f, 0.4f), 3000.f, 6000.f); }
	const float PierX[] = { -20.f, 20.f, 60.f, 84.f };
	for (float X : PierX) { P(TEXT("GasLamp"), X, 1.7f, Q, 0, 0.85f); Lamp(Hs, M(X, 1.7f, Q + 3.6f), FLinearColor(1.f, 0.72f, 0.4f), 3000.f, 6000.f); }
	// Bollards/capstans on the quay edge, cargo, and a recovered sea mine on show by the pier root.
	P(TEXT("BollardCapstan"), -51.4f, -32, Q, 90);
	P(TEXT("BollardCapstan"), -51.4f, 45, Q, 90);
	P(TEXT("BollardCapstan"), -51.4f, 95, Q, 90);
	P(TEXT("WoodenBarrels"), -56, -19.5f, Q, 15, 1.35f);
	P(TEXT("TownCrateStack"), -56.6f, -18.2f, Q, 40, 1.0f);
	P(TEXT("WoodenBarrels"), -58, 78, Q, 30);
	P(TEXT("MilitaryCrate"), -57, 86, Q, 5); P(TEXT("MilitaryCrate"), -57.3f, 86.2f, Q + 0.42f, 12); P(TEXT("MilitaryCrate"), -60, 90, Q, 80);
	P(TEXT("SeaMine"), -55.5f, -7, Q, 0, 1.15f);
	P(TEXT("WallLamp"), -78.6f, 48, Q + 4.5f, 0); P(TEXT("WallLamp"), -78.6f, 62, Q + 4.5f, 0);
	Lamp(Hs, M(-78.2f, 48, Q + 4.3f), FLinearColor(1.f, 0.72f, 0.4f), 4000.f, 5000.f);
	Lamp(Hs, M(-78.2f, 62, Q + 4.3f), FLinearColor(1.f, 0.72f, 0.4f), 4000.f, 5000.f);

	// Airship mooring mast on the north headland (behind the coaling station); the brass airship's loop docks here.
	P(TEXT("CoastRocks03"), -150, 262, -1.0f, 15, 1.4f);
	P(TEXT("MooringMast"), -150, 262, Q + 1.2f, 135);
	if (AIronwakeLivingSea* LS = AIronwakeLivingSea::Get(W))
		if (AIronwakeAirship* As = LS->Airship)
		{
			const FVector Top = Hs->GetComponentTransform().TransformPosition(M(-150, 262, Q + 1.2f + 53.f));
			As->SetupDock(Top, FVector(0.7071f, 0.7071f, 0.f), 150.f);
		}

	// Moored craft that ride the swell: torpedo boat + steam pinnace on the pier's south side, small boats at the quay.
	auto Float = [&](const TCHAR* N, float X, float Y, float KeelZ, float Yaw, float S = 1.f)
	{
		if (UStaticMeshComponent* C = P(N, X, Y, KeelZ, Yaw, S)) { Floaters.Add(C); FloaterBase.Add(FVector(KeelZ * 100.f, Yaw, Floaters.Num() * 1.7f)); }
	};
	Float(TEXT("TorpedoBoat"), 35, -11, -2.8f, 0);
	Float(TEXT("SteamPinnace"), 75, -7.5f, -1.0f, 0);
	Float(TEXT("Rowboat"), -46, -60, -0.35f, 0);
	Float(TEXT("Fourareen"), -46.5f, -42, -0.4f, 90);

	// Moored steamer alongside the pier (static merchant: empty route; the director bobs it on the swell).
	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SP.Owner = this;
	const FTransform MT(FRotator(0.f, 45.f, 0.f), H->GetComponentTransform().TransformPosition(M(40, 10, 0)));
	Moored = W->SpawnActor<AIronwakeMerchant>(AIronwakeMerchant::StaticClass(), MT, SP);
	if (Moored) { Moored->ShipName = TEXT("SS Wren (moored)"); Moored->Tags.Add(TEXT("IronwakeMoored")); if (Moored->FunnelSmoke) Moored->FunnelSmoke->SetActive(false, true); }  // P12: banked fires, no white pillar over the quay

	// Harbour ambience at the dock.
	if (HarbourSnd)
	{
		USoundAttenuation* A = NewObject<USoundAttenuation>(this);
		A->Attenuation.bAttenuate = true;
		A->Attenuation.bSpatialize = true;
		A->Attenuation.AttenuationShapeExtents = FVector(20000.f, 0.f, 0.f);
		A->Attenuation.FalloffDistance = 160000.f;
		DockAmb = UGameplayStatics::SpawnSoundAtLocation(this, HarbourSnd, HarbourC + FVector(0, 0, 800.f), FRotator::ZeroRotator, 1.6f, 0.95f, 0.f, A, nullptr, false);
	}
	BuildTownP8(Hs);
	(void)Glow; (void)Stone;
	UE_LOG(LogIronwake, Display, TEXT("IWS1 harbour at %s seaZ=%.0f moored=%d amb=%d (E1 real assets, %d floaters)"), *HarbourC.ToString(), SeaZ, Moored != nullptr, DockAmb != nullptr, Floaters.Num());
}

void AIronwakeVoyage::BuildWreck()
{
	UWorld* W = GetWorld();
	const float SeaZ = AIronwakeShell::WaterZAt(W, WreckC);
	USceneComponent* R = NewObject<USceneComponent>(this, TEXT("Wreck"));
	R->SetupAttachment(RootComponent);
	R->RegisterComponent();
	AddInstanceComponent(R);
	R->SetWorldLocationAndRotation(FVector(WreckC.X, WreckC.Y, SeaZ + AIronwakeShell::RenderedSeaOffsetCm), FRotator(0.f, 115.f, 0.f));
	const FLinearColor Wood(0.10f, 0.07f, 0.045f);
	// E1: the real SS Rifle stern scan (Scottish Maritime Museum, CC0), display cradle removed, scaled x8 (60 m):
	// stern (+Y) cocked up out of the water, the torn-off forward end down, heeled over.
	IronwakeWorld::Place(this, R, TEXT("WreckStern"), FVector(0.f, 0.f, -700.f), FRotator(24.f, 0.f, -20.f), FVector(8.f));
	// Foam/white water ring where the hull breaks the surface (wake-foam planes that ride the swell) + a slow splash.
	// W1's bow-foam material is tuned for the rendered surface (DepthFade ~8 cm, lifted 30-40 cm); M_WakeFoam faded out here.
	if (UMaterialInterface* Foam = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Ironwake/FX/M_IW_BowFoam.M_IW_BowFoam")))
	{
		if (UStaticMesh* PlaneM = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")))
		{
			const FVector FL[] = { FVector(0, -900, 0), FVector(300, 900, 0), FVector(-500, 0, 0), FVector(700, -2200, 0) };
			const FVector FS[] = { FVector(40, 60, 1), FVector(34, 46, 1), FVector(50, 30, 1), FVector(26, 26, 1) };
			for (int32 i = 0; i < 4; ++i)
			{
				UStaticMeshComponent* Fm = Mesh(R, PlaneM, FL[i] + FVector(0, 0, 40.f), FRotator(0.f, 25.f * i, 0.f), FS[i]);
				UMaterialInstanceDynamic* D = UMaterialInstanceDynamic::Create(Foam, this);
				D->SetScalarParameterValue(TEXT("Opacity"), 1.0f);
				D->SetScalarParameterValue(TEXT("Foam Boost"), 5.f);
				Fm->SetMaterial(0, D);
				Fm->SetCastShadow(false);
				Floaters.Add(Fm); FloaterBase.Add(FVector(40.f, 25.f * i, 2.2f + i));
			}
		}
	}
	// A broken mast standing out of the sunken end, plus floating debris that rides the swell.
	Mesh(R, CylMesh, FVector(400.f, -1700.f, 500.f), FRotator(0.f, 0.f, -35.f), FVector(0.6f, 0.6f, 18.f), &Wood);
	auto Float = [&](const TCHAR* N, float X, float Y, float Z, float Yaw, float Pitch, float S)
	{
		if (UStaticMeshComponent* C = IronwakeWorld::Place(this, R, N, FVector(X, Y, Z), FRotator(Pitch, Yaw, 0.f), FVector(S)))
		{ Floaters.Add(C); FloaterBase.Add(FVector(Z, Yaw, Floaters.Num() * 1.3f)); }
	};
	// Debris field: ~20 pieces within 15-45 m of the hull, floating proud of the rendered surface (keel offset > 0).
	FRandomStream DR(1880);
	const TCHAR* DN[] = { TEXT("Barrel03"), TEXT("Barrel03"), TEXT("WoodenBarrels"), TEXT("MilitaryCrate"), TEXT("Barrel03") };
	for (int32 i = 0; i < 20; ++i)
	{
		const float A = DR.FRandRange(0.f, 2.f * PI), Rr = DR.FRandRange(1500.f, 4500.f);
		const TCHAR* N = DN[i % 5];
		const bool bBarrel = N == DN[0];
		Float(N, FMath::Cos(A) * Rr, FMath::Sin(A) * Rr, bBarrel ? 30.f : 15.f, DR.FRandRange(0.f, 360.f), bBarrel ? 90.f : 0.f, bBarrel ? 2.4f : 2.2f);
	}
	Float(TEXT("Fourareen"), -4200.f, -600.f, 0.f, 70.f, 0.f, 1.f); // salvage boat
	for (int32 i = 0; i < 7; ++i)
	{
		const float A = i * 0.9f;
		Mesh(R, CubeMesh, FVector(FMath::Cos(A) * (3000.f + 700.f * i), FMath::Sin(A) * (2400.f + 450.f * i), 20.f), FRotator(0.f, 40.f * i, 4.f * i), FVector(3.f + i % 3, 0.6f, 0.2f), &Wood);
	}
	// Wreck buoy: steam buoy tinted green, lamp on top.
	if (UStaticMeshComponent* B = IronwakeWorld::Place(this, R, TEXT("SteamBuoy"), FVector(-9000.f, -2500.f, -90.f), FRotator::ZeroRotator, FVector(1.f)))
	{
		IronwakeWorld::TintAll(B, FLinearColor(0.30f, 0.95f, 0.40f));
		Floaters.Add(B); FloaterBase.Add(FVector(-90.f, 0.f, 4.1f));
		Lamp(B, FVector(0.f, 0.f, 560.f), FLinearColor(0.2f, 1.f, 0.3f), 20000.f, 25000.f);
	}
}

void AIronwakeVoyage::BuildStacks()
{
	// E1: The Needles as layered photoscan cliff tiers (three coastal_cliff_02 faces looking outward per tier,
	// each tier rotated and narrower, sunk into the sea), grass ledges between tiers like real chalk stacks.
	const float SeaZ = AIronwakeShell::WaterZAt(GetWorld(), StacksC);
	USceneComponent* R = NewObject<USceneComponent>(this, TEXT("Stacks"));
	R->SetupAttachment(RootComponent);
	R->RegisterComponent();
	AddInstanceComponent(R);
	R->SetWorldLocation(FVector(StacksC.X, StacksC.Y, SeaZ + AIronwakeShell::RenderedSeaOffsetCm));
	// E1 v2: each stack = 3 boulder_01 photoscan pieces (stretched tall, each with its own yaw and a few degrees of lean so the
	// silhouette tapers irregularly) on a shelf of coast-rock scans awash at the foot. Boulder01 is 1.27 x 1.83 x 1.0 m.
	struct FS { FVector2D L; float H; float Wd; float A0; };
	const FS Defs[] = {
		{ FVector2D(0, 0), 95.f, 1.3f, 10.f },
		{ FVector2D(9000, 6000), 74.f, 1.1f, 50.f },
		{ FVector2D(-7000, 9500), 55.f, 1.0f, 200.f },
		{ FVector2D(4000, -9000), 36.f, 1.1f, 300.f },
		{ FVector2D(-11000, -4000), 22.f, 0.8f, 70.f },
	};
	int32 N = 0;
	FRandomStream Rng(1871);
	for (const FS& D : Defs)
	{
		const float Hcm = D.H * 100.f;
		// base: wide, ~45 % of the height, sunk 8 m
		IronwakeWorld::Place(this, R, TEXT("Boulder01"), FVector(D.L.X, D.L.Y, -800.f), FRotator(Rng.FRandRange(-4.f, 4.f), D.A0, Rng.FRandRange(-4.f, 4.f)),
			FVector(20.f * D.Wd, 15.f * D.Wd, (0.45f * Hcm + 800.f) / 100.f));
		// mid: narrower, overlapping the base by a third, rotated
		IronwakeWorld::Place(this, R, TEXT("Boulder01"), FVector(D.L.X + Rng.FRandRange(-300.f, 300.f), D.L.Y + Rng.FRandRange(-300.f, 300.f), 0.28f * Hcm),
			FRotator(Rng.FRandRange(-6.f, 6.f), D.A0 + 70.f, Rng.FRandRange(-6.f, 6.f)), FVector(14.f * D.Wd, 11.f * D.Wd, 0.5f * Hcm / 100.f));
		// top: a leaning needle
		IronwakeWorld::Place(this, R, TEXT("Boulder01"), FVector(D.L.X + Rng.FRandRange(-400.f, 400.f), D.L.Y + Rng.FRandRange(-400.f, 400.f), 0.62f * Hcm),
			FRotator(Rng.FRandRange(-9.f, 9.f), D.A0 + 150.f, Rng.FRandRange(-9.f, 9.f)), FVector(8.f * D.Wd, 6.5f * D.Wd, 0.4f * Hcm / 100.f));
		N += 3;
		// awash shelf
		IronwakeWorld::Place(this, R, TEXT("CoastLandRocks03"), FVector(D.L.X + 900.f, D.L.Y - 600.f, -150.f), FRotator(0.f, D.A0 * 2.f, 0.f), FVector(3.5f * D.Wd, 3.5f * D.Wd, 4.f));
		IronwakeWorld::Place(this, R, TEXT("CoastLandRocks02"), FVector(D.L.X - 800.f, D.L.Y + 700.f, -150.f), FRotator(0.f, D.A0 * 3.f, 0.f), FVector(3.0f * D.Wd, 2.5f * D.Wd, 4.f));
		N += 2;
	}
	UE_LOG(LogIronwake, Display, TEXT("IWE1 needles: %d scan pieces"), N);
}

void AIronwakeVoyage::BuildBuoys()
{
	// Channel buoys from the harbour mouth toward the wreck: red to port, green to starboard (outbound).
	// E1: Hunyuan3D steam buoys (5.5 m), tinted per side, lamp on the lantern.
	const FVector A = HarbourC + FVector(30000.f, 30000.f, 0.f);
	const FVector B = WreckC;
	const FVector Dir = (B - A).GetSafeNormal2D();
	const FVector Side(-Dir.Y, Dir.X, 0.f);
	const FLinearColor RedL(6.f, 0.5f, 0.2f), GreenL(0.3f, 5.f, 0.6f);
	for (int32 i = 0; i < 4; ++i)
	{
		for (int32 s = -1; s <= 1; s += 2)
		{
			const FVector P = A + Dir * (40000.f * i) + Side * (s * 16000.f);
			USceneComponent* Bu = NewObject<USceneComponent>(this);
			Bu->SetupAttachment(RootComponent);
			Bu->RegisterComponent();
			AddInstanceComponent(Bu);
			Bu->SetWorldLocation(P);
			const bool bPort = s < 0;
			if (UStaticMeshComponent* C = IronwakeWorld::Place(this, Bu, TEXT("SteamBuoy"), FVector::ZeroVector, FRotator(0.f, 37.f * i, 0.f), FVector(1.f)))
				IronwakeWorld::TintAll(C, bPort ? FLinearColor(1.0f, 0.30f, 0.25f) : FLinearColor(0.30f, 0.95f, 0.40f));
			Mesh(Bu, SphereMesh, FVector(0, 0, 545.f), FRotator::ZeroRotator, FVector(0.35f), bPort ? &RedL : &GreenL);
			Lamp(Bu, FVector(0, 0, 545.f), bPort ? FLinearColor(1.f, 0.12f, 0.08f) : FLinearColor(0.2f, 1.f, 0.3f), 15000.f, 20000.f);
			Buoys.Add(Bu);
		}
	}
}

void AIronwakeVoyage::SpawnGulls()
{
	UWorld* W = GetWorld();
	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SP.Owner = this;
	auto Flock = [&](const FVector& C, int32 N, float R, float Alt)
	{
		for (int32 i = 0; i < N; ++i)
		{
			AIronwakeGull* G = W->SpawnActor<AIronwakeGull>(AIronwakeGull::StaticClass(), FTransform(C), SP);
			if (!G) continue;
			G->Centre = C;
			G->RadiusCm = R * FMath::FRandRange(0.6f, 1.3f);
			G->AltCm = Alt + FMath::FRandRange(-1500.f, 2500.f);
			G->Angle = FMath::FRand() * 2.f * PI;
			G->Dir = (i % 3 == 0) ? -1.f : 1.f;
			G->SpeedCmS = FMath::FRandRange(900.f, 1400.f);
			Gulls.Add(G);
		}
	};
	if (POIs.Num() >= 4)
	{
		Flock(POIs[2].Loc + FVector(-2500.f, 800.f, 0.f), 7, 9000.f, 6500.f);   // lighthouse
		Flock(HarbourC + FVector(4000.f, 4000.f, 0.f), 6, 7000.f, 2500.f);      // harbour
		Flock(WreckC, 5, 6000.f, 2200.f);                                        // wreck
		Flock(StacksC, 4, 8000.f, 9000.f);                                       // stacks
	}
	// Three gulls follow the player's ship (picked up in TickSailing once the pawn exists).
}

// ---------------------------------------------------------------------------- weather / time of day
void AIronwakeVoyage::CaptureWeatherBaseline()
{
	{ TActorIterator<ADirectionalLight> It(GetWorld()); if (It) Sun = *It; }
	{ TActorIterator<ASkyLight> It(GetWorld()); if (It) Sky = *It; }
	{ TActorIterator<AExponentialHeightFog> It(GetWorld()); if (It) Fog = *It; }
	for (TActorIterator<APostProcessVolume> It(GetWorld()); It; ++It) { if (!PPV.IsValid() || It->bUnbound) PPV = *It; }
	FWx Day;
	Day.SunPitch = -22.f; Day.SunI = 3.f; Day.SkyI = 1.5f; Day.Fog = 0.02f; Day.Bias = -0.5f; Day.Night = 0.f;
	Day.SunC = FLinearColor(0.8f, 0.84f, 0.9f); Day.FogC = FLinearColor(0.35f, 0.38f, 0.41f);
	if (ADirectionalLight* S = Sun.Get())
	{
		Day.SunPitch = S->GetActorRotation().Pitch; SunYaw = S->GetActorRotation().Yaw;
		if (ULightComponent* L = S->GetLightComponent()) { Day.SunI = L->Intensity; Day.SunC = L->GetLightColor(); }
	}
	if (ASkyLight* K = Sky.Get()) if (USkyLightComponent* L = K->GetLightComponent()) Day.SkyI = L->Intensity;
	if (AExponentialHeightFog* F = Fog.Get()) if (UExponentialHeightFogComponent* C = F->GetComponent())
	{
		// P34: thin exponential height fog ~4.5x and push StartDistance out to 1.8 km with max opacity 0.42 so the sea & harbour
		// are crystal-clear (15-25 km maritime visibility, zero grey "fog of war" curtain!).
		C->SetFogDensity(C->FogDensity * 0.16f);
		C->SetStartDistance(180000.f);
		C->SetFogMaxOpacity(0.42f);
		Day.Fog = C->FogDensity; Day.FogC = FLinearColor(0.42f, 0.56f, 0.78f);
	}
	if (APostProcessVolume* P = PPV.Get()) Day.Bias = P->Settings.AutoExposureBias;
	Presets[0] = Day;
	FWx Dusk = Day;
	Dusk.SunPitch = -2.5f; Dusk.SunI = Day.SunI * 0.7f; Dusk.SkyI = Day.SkyI * 0.5f; Dusk.Fog = Day.Fog * 0.8f; Dusk.Bias = Day.Bias - 0.3f; Dusk.Night = 0.45f;
	Dusk.SunC = FLinearColor(1.f, 0.42f, 0.16f); Dusk.FogC = FLinearColor(0.32f, 0.16f, 0.08f);
	Presets[1] = Dusk;
	FWx Night = Day;
	Night.SunPitch = 6.f; Night.SunI = Day.SunI * 0.03f; Night.SkyI = Day.SkyI * 0.35f; Night.Fog = Day.Fog * 1.6f; Night.Bias = Day.Bias + 1.0f; Night.Night = 1.f;
	Night.SunC = FLinearColor(0.55f, 0.66f, 1.f); Night.FogC = FLinearColor(0.012f, 0.016f, 0.026f);
	Presets[2] = Night;
	// W2: one weather -> one sea state (0 CALM .. 4 STORM). Gale/storm: dark low day, thick fog, rain.
	FWx Gale = Day;
	Gale.SunPitch = -14.f; Gale.SunI = Day.SunI * 0.35f; Gale.SkyI = Day.SkyI * 0.55f; Gale.Fog = Day.Fog * 4.f; Gale.Bias = Day.Bias - 0.5f; Gale.Night = 0.15f;
	Gale.SunC = FLinearColor(0.62f, 0.68f, 0.76f); Gale.FogC = FLinearColor(0.16f, 0.18f, 0.2f);
	Presets[3] = Gale;
	FWx Storm = Day;
	Storm.SunPitch = -40.f; Storm.SunI = Day.SunI * 0.1f; Storm.SkyI = Day.SkyI * 0.55f; Storm.Fog = Day.Fog * 9.f; // W3: visibility ~1-2 km Storm.Bias = Day.Bias - 0.9f; Storm.Night = 0.3f;
	Storm.SunC = FLinearColor(0.52f, 0.58f, 0.68f); Storm.FogC = FLinearColor(0.11f, 0.12f, 0.13f);
	Presets[4] = Storm;
	FWx Clr = Night;   // P8: clear moonlit night, thin air, the stars and Milky Way show
	Clr.Fog = Day.Fog * 0.45f; Clr.SkyI = Day.SkyI * 0.5f; Clr.Bias = Day.Bias + 1.2f; Clr.Clear = 1.f; Clr.Sea = 0.5f;
	Clr.FogC = FLinearColor(0.010f, 0.014f, 0.030f);
	Presets[5] = Clr;
	Presets[0].Sea = 1.f; Presets[1].Sea = 0.f; Presets[2].Sea = 2.f; Presets[3].Sea = 3.f; Presets[4].Sea = 4.f;
	Cur = From = Presets[0];
	bWxReady = true;
	if (AIronwakeStorm* St = AIronwakeStorm::Get(GetWorld())) St->SetTarget(Cur.Sea, 0.1f); else IronwakeSea::SetSeaState(GetWorld(), Cur.Sea);
	UE_LOG(LogIronwake, Display, TEXT("IWS1 weather baseline sun=%d sky=%d fog=%d ppv=%d pitch=%.1f sunI=%.2f skyI=%.2f fog=%.3f bias=%.2f"),
		Sun.IsValid(), Sky.IsValid(), Fog.IsValid(), PPV.IsValid(), Day.SunPitch, Day.SunI, Day.SkyI, Day.Fog, Day.Bias);
}

void AIronwakeVoyage::SetWeather(int32 Preset, float BlendSeconds)
{
	WeatherTarget = ((Preset % NumWx) + NumWx) % NumWx;
	bLiving = false;
	if (AIronwakeStorm* St = AIronwakeStorm::Get(GetWorld())) { St->CloudCover = -1.f; St->RainAmount = 0.f; St->SetTarget(Presets[WeatherTarget].Sea, FMath::Max(BlendSeconds * 2.f, 8.f)); }
	From = Cur;
	BlendT = 0.f;
	BlendDur = FMath::Max(BlendSeconds, 0.05f);
	ShowToast(FString::Printf(TEXT("Weather: %s"), WeatherNames[WeatherTarget]));
	UE_LOG(LogIronwake, Display, TEXT("IWS1 weather -> %s over %.0f s"), WeatherNames[WeatherTarget], BlendDur);
}

void AIronwakeVoyage::CycleWeather() { if (bLiving) CycleWeatherLive(); else SetWeather(WeatherTarget + 1, 8.f); }

void AIronwakeVoyage::ToggleAutoCycle()
{
	if (bLiving) { bWxDrift = !bWxDrift; ShowToast(bWxDrift ? TEXT("Weather drifts on its own") : TEXT("Weather held (L changes it)")); return; }
	bAutoCycle = !bAutoCycle;
	AutoCycleT = 0.f;
	ShowToast(bAutoCycle ? TEXT("Day/night cycle ON (slow)") : TEXT("Day/night cycle OFF"));
}

void AIronwakeVoyage::ApplyWeather(float Dt)
{
	if (!bWxReady) return;
	if (!bLiving && bAutoCycle)
	{
		AutoCycleT += Dt;
		if (AutoCycleT >= 120.f) { AutoCycleT = 0.f; static const int32 Seq[] = { 1, 0, 2, 3, 4, 3, 2, 0 }; static int32 Si = 0; Si = (Si + 1) % 8; SetWeather(Seq[Si], 45.f); } // W2: calm -> moderate -> rough -> gale -> storm -> back; hold ~75 s, blend 45 s
	}
	bool bApply = bLiving;
	if (bLiving) TickLiving(Dt);
	else if (BlendT < 1.f)
	{
		BlendT = FMath::Min(1.f, BlendT + Dt / BlendDur);
		const float A = FMath::SmoothStep(0.f, 1.f, BlendT);
		const FWx& To = Presets[WeatherTarget];
		Cur.SunPitch = FMath::Lerp(From.SunPitch, To.SunPitch, A);
		Cur.SunI = FMath::Lerp(From.SunI, To.SunI, A);
		Cur.SkyI = FMath::Lerp(From.SkyI, To.SkyI, A);
		Cur.Fog = FMath::Lerp(From.Fog, To.Fog, A);
		Cur.Bias = FMath::Lerp(From.Bias, To.Bias, A);
		Cur.Night = FMath::Lerp(From.Night, To.Night, A);
		Cur.Clear = FMath::Lerp(From.Clear, To.Clear, A);
		Cur.SunC = FMath::Lerp(From.SunC, To.SunC, A);
		Cur.FogC = FMath::Lerp(From.FogC, To.FogC, A);
		Cur.Sea = FMath::Lerp(From.Sea, To.Sea, A);
		bApply = true;
	}
	if (bApply)
	{
		// W2: the sea state is owned by AIronwakeStorm (open-sea target + land fade), set in SetWeather.
		if (ADirectionalLight* S = Sun.Get())
		{
			S->SetActorRotation(FRotator(Cur.SunPitch, bLiving ? LiveSunYaw : SunYaw, 0.f));
			if (ULightComponent* L = S->GetLightComponent()) { L->SetIntensity(Cur.SunI); L->SetLightColor(Cur.SunC); }
		}
		if (ASkyLight* K = Sky.Get()) if (USkyLightComponent* L = K->GetLightComponent()) L->SetIntensity(Cur.SkyI);
		if (AExponentialHeightFog* F = Fog.Get()) if (UExponentialHeightFogComponent* C = F->GetComponent())
		{
			const float EffFog = bHazeOff ? 0.00015f : Cur.Fog;
			C->SetFogDensity(EffFog);
			C->SetStartDistance(bHazeOff ? 900000.f : (LiveState >= 3 ? 35000.f : 180000.f));
			C->SetFogMaxOpacity(bHazeOff ? 0.08f : (LiveState >= 3 ? 0.78f : 0.42f));
			C->SetFogInscatteringColor(Cur.FogC);
			const bool bVol = Cur.Night > 0.3f && !bHazeOff;
			if (C->bEnableVolumetricFog != bVol)
			{
				C->SetVolumetricFog(bVol);
				C->SetVolumetricFogDistance(250000.f);
				C->SetVolumetricFogExtinctionScale(0.4f);
			}
		}
		if (APostProcessVolume* P = PPV.Get())
		{
			P->Settings.bOverride_AutoExposureBias = true; P->Settings.AutoExposureBias = Cur.Bias;
			// P10: clear night = locked exposure (no auto lift of the black sky) and low bloom so stars stay points.
			static bool bInit = false; static float DMin = 0.03f, DMax = 8.f, DBloom = 0.675f; static bool oMin = false, oMax = false, oBl = false;
			if (!bInit) { bInit = true; DMin = P->Settings.AutoExposureMinBrightness; DMax = P->Settings.AutoExposureMaxBrightness; DBloom = P->Settings.BloomIntensity; oMin = P->Settings.bOverride_AutoExposureMinBrightness; oMax = P->Settings.bOverride_AutoExposureMaxBrightness; oBl = P->Settings.bOverride_BloomIntensity; }
			static float NightEV = -999.f; if (NightEV < -900.f) { float V = 0.f; NightEV = FParse::Value(FCommandLine::Get(), TEXT("iwev="), V) ? V : 0.4f; }
			const float K = FMath::Clamp((Cur.Night - 0.5f) / 0.4f, 0.f, 1.f) * Cur.Clear;
			if (K > 0.01f)
			{
				P->Settings.bOverride_AutoExposureMinBrightness = P->Settings.bOverride_AutoExposureMaxBrightness = P->Settings.bOverride_BloomIntensity = true;
				P->Settings.AutoExposureMinBrightness = NightEV; P->Settings.AutoExposureMaxBrightness = NightEV; P->Settings.BloomIntensity = 0.08f;
			}
			else
			{
				P->Settings.bOverride_AutoExposureMinBrightness = oMin; P->Settings.bOverride_AutoExposureMaxBrightness = oMax; P->Settings.bOverride_BloomIntensity = oBl;
				P->Settings.AutoExposureMinBrightness = DMin; P->Settings.AutoExposureMaxBrightness = DMax; P->Settings.BloomIntensity = DBloom;
			}
		}
	}
	if (NightSky) NightSky->SetState(Cur.Night, Cur.Clear);
	if (AIronwakeStorm* St = AIronwakeStorm::Get(GetWorld())) St->ClearSky = Cur.Clear;
	// Lamps and beam follow the night factor; buoy lamps blink.
	LampBlink += Dt;
	const float N = bLiving ? LiveLamp : FMath::Clamp((Cur.Night - 0.2f) / 0.8f, 0.f, 1.f);
	if (WinMID) WinMID->SetScalarParameterValue(TEXT("On"), N * 0.12f);
	{
		const bool bOn = N > 0.02f;
		if (WinISM && WinISM->IsVisible() != bOn) WinISM->SetVisibility(bOn);
		if (BulbISM && BulbISM->IsVisible() != bOn) BulbISM->SetVisibility(bOn);
		for (UInstancedStaticMeshComponent* Gi : PortGlowISMs)
			if (Gi && Gi->IsVisible() != bOn) Gi->SetVisibility(bOn);
	}   // P24 + P29: no dark window boxes by day across all ports
	static const bool bNoLamps = FParse::Param(FCommandLine::Get(), TEXT("iwnolamps"));
	for (UPointLightComponent* L : NightLamps)
	{
		if (!L) continue;
		float Cd = 10000.f;
		if (L->ComponentTags.Num() > 0) LexFromString(Cd, *L->ComponentTags[0].ToString().RightChop(3));
		const bool bBuoy = L->GetAttachParent() && Buoys.Contains(L->GetAttachParent());
		const float Blink = bBuoy ? (FMath::Fmod(LampBlink + L->GetComponentLocation().X * 0.0001f, 3.f) < 1.f ? 1.f : 0.08f) : 1.f;
		L->SetIntensity(Cd * N * 0.18f * Blink * (bNoLamps ? 0.f : 1.f));
	}
	if (Beam)
	{
		BeamYaw = FMath::Fmod(BeamYaw + Dt * 30.f, 360.f); // one turn per 12 s
		Beam->SetWorldRotation(FRotator(-1.5f, BeamYaw, 0.f));
		Beam->SetIntensity(N * 2.0e6f);
	}
}

// ---------------------------------------------------------------------------- toggles
void AIronwakeVoyage::ShowToast(const FString& Msg, float Seconds)
{
	Toast = Msg;
	ToastUntil = GetWorld()->GetRealTimeSeconds() + Seconds;
}

void AIronwakeVoyage::NextPoint()
{
	if (POIs.Num() == 0) return;
	NextPOI = (NextPOI + 1) % POIs.Num();
	ShowToast(FString::Printf(TEXT("Course for %s"), *POIs[NextPOI].Name));
	UE_LOG(LogIronwake, Display, TEXT("IWS1 next point -> %s"), *POIs[NextPOI].Name);
}

void AIronwakeVoyage::ToggleHelm()
{
	if (!bHelm && ApMode != 0) DisengageAutopilot(TEXT("Autopilot off: helmsman has the wheel"));
	bHelm = !bHelm;
	AIronwakeShipPawn* P = Player();
	if (!bHelm && P) P->IWRudder(0);
	if (bHelm && bAnchored) ToggleAnchor();
	if (bHelm && P && P->TelegraphIndex <= 2) P->IWTelegraph(4); // helmsman rings HALF AHEAD if stopped
	ShowToast(bHelm && POIs.IsValidIndex(NextPOI) ? FString::Printf(TEXT("Helmsman: steering for %s"), *POIs[NextPOI].Name) : FString(TEXT("Helmsman stood down: you have the wheel")));
	UE_LOG(LogIronwake, Display, TEXT("IWS1 helm %d"), bHelm);
}

void AIronwakeVoyage::ToggleAnchor()
{
	AIronwakeShipPawn* P = Player();
	if (!P) return;
	bAnchored = !bAnchored;
	if (bAnchored)
	{
		bHelm = false;
		P->IWTelegraph(2);
		P->IWRudder(0);
		if (SplashFX)
		{
			const FVector Bow = P->GetActorLocation() + P->GetActorForwardVector().GetSafeNormal2D() * 6000.f;
			if (UNiagaraComponent* N = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, SplashFX, FVector(Bow.X, Bow.Y, AIronwakeShell::WaterZAt(GetWorld(), Bow))))
				N->SetWorldScale3D(FVector(0.25f));
		}
		ShowToast(TEXT("Let go the anchor!"));
	}
	else ShowToast(TEXT("Anchor aweigh"));
	UE_LOG(LogIronwake, Display, TEXT("IWS1 anchor %d at %.1f kn"), bAnchored, P->GetSpeedKnots());
}

void AIronwakeVoyage::ToggleFastSail()
{
	AIronwakeShipPawn* P = Player();
	if (!P) return;
	AIronwakePlayerController* C = PC();
	if (C && !C->bPeaceful && P->FindEnemy()) { ShowToast(TEXT("Fast sail unavailable: enemy in these waters")); return; }
	bFastSail = !bFastSail;
	P->SpeedCompression = bFastSail ? 1.5f * FastSailMulOn : 1.5f;
	ShowToast(bFastSail ? TEXT("Fast sail: x4 passage speed") : TEXT("Fast sail off"));
	UE_LOG(LogIronwake, Display, TEXT("IWS1 fastsail %d compression=%.1f"), bFastSail, P->SpeedCompression);
}

// ---------------------------------------------------------------------------- photo mode
void AIronwakeVoyage::TogglePhoto()
{
	AIronwakePlayerController* C = PC();
	AIronwakeShipPawn* P = Player();
	if (!C || !P) return;
	bPhoto = !bPhoto;
	if (bPhoto)
	{
		if (!PhotoCam)
		{
			FActorSpawnParameters SP;
			SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			PhotoCam = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), FTransform::Identity, SP);
			PhotoCam->GetCameraComponent()->bConstrainAspectRatio = false;
		}
		const FVector L = C->PlayerCameraManager ? C->PlayerCameraManager->GetCameraLocation() : P->GetActorLocation();
		PhotoRot = C->PlayerCameraManager ? C->PlayerCameraManager->GetCameraRotation() : C->GetControlRotation();
		PhotoRot.Roll = 0.f;
		PhotoCam->SetActorLocationAndRotation(L, PhotoRot);
		PhotoCam->GetCameraComponent()->SetFieldOfView(55.f);
		PhotoCam->GetCameraComponent()->PostProcessSettings.bOverride_MotionBlurAmount = true;
		PhotoCam->GetCameraComponent()->PostProcessSettings.MotionBlurAmount = 0.f;
		C->SetViewTarget(PhotoCam);
		PrevDilation = UGameplayStatics::GetGlobalTimeDilation(this);
		UGameplayStatics::SetGlobalTimeDilation(this, 0.25f);
		C->SetPhotoInput(true);
		if (GEngine) GEngine->Exec(GetWorld(), TEXT("DisableAllScreenMessages"));
		UE_LOG(LogIronwake, Display, TEXT("IWS1 photo ON at %s"), *L.ToString());
	}
	else
	{
		UGameplayStatics::SetGlobalTimeDilation(this, 1.f);
		C->SetViewTarget(P);
		C->SetPhotoInput(false);
		UE_LOG(LogIronwake, Display, TEXT("IWS1 photo OFF"));
	}
}

void AIronwakeVoyage::SetPhotoView(const FVector& Loc, const FRotator& Rot)
{
	PhotoRot = Rot;
	if (PhotoCam) PhotoCam->SetActorLocationAndRotation(Loc, Rot);
}

void AIronwakeVoyage::TickPhoto(float RealDt)
{
	AIronwakePlayerController* C = PC();
	if (!bPhoto || !PhotoCam || !C) return;
	const FVector Move = C->PhotoMove;     // x fwd, y right, z up (WASD QE)
	const FVector2D Look = C->ConsumePhotoLook();
	PhotoRot.Yaw += Look.X;
	PhotoRot.Pitch = FMath::Clamp(PhotoRot.Pitch + Look.Y, -85.f, 85.f);
	const float Speed = 3000.f * (C->bPhotoFast ? 6.f : 1.f); // 30 m/s, x6 with shift
	FVector L = PhotoCam->GetActorLocation();
	const FRotationMatrix RM(PhotoRot);
	L += (RM.GetScaledAxis(EAxis::X) * Move.X + RM.GetScaledAxis(EAxis::Y) * Move.Y + FVector::UpVector * Move.Z) * Speed * RealDt;
	// Keep the camera above the waves.
	L.Z = FMath::Max(L.Z, AIronwakeShell::WaterZAt(GetWorld(), L) + 150.f);
	PhotoCam->SetActorLocationAndRotation(L, PhotoRot);
	if (FMath::Abs(C->PhotoFovDelta) > 0.f)
	{
		UCameraComponent* Cam = PhotoCam->GetCameraComponent();
		Cam->SetFieldOfView(FMath::Clamp(Cam->FieldOfView - C->PhotoFovDelta * 3.f, 15.f, 100.f));
		C->PhotoFovDelta = 0.f;
	}
	if (C->bPhotoTimeCycle)
	{
		C->bPhotoTimeCycle = false;
		const float D = UGameplayStatics::GetGlobalTimeDilation(this);
		const float NewD = D > 0.2f ? 0.03f : (D > 0.02f ? 1.f : 0.25f);
		UGameplayStatics::SetGlobalTimeDilation(this, NewD > 0.9f ? 1.f : NewD);
		UE_LOG(LogIronwake, Display, TEXT("IWS1 photo time dilation %.2f"), UGameplayStatics::GetGlobalTimeDilation(this));
	}
}

// ---------------------------------------------------------------------------- sailing feel
void AIronwakeVoyage::TickSailing(float Dt)
{
	AIronwakeShipPawn* P = Player();
	AIronwakePlayerController* C = PC();
	if (!P || !C) return;
	UWorld* W = GetWorld();
	const float Now = W->GetTimeSeconds();
	static TWeakObjectPtr<AIronwakeShipPawn> Tuned;
	if (Tuned.Get() != P)
	{
		Tuned = P;
		// Smoother chase camera: softer position lag (rides over the swell) and slower rotation catch-up.
		P->CameraArm->CameraLagSpeed = 1.6f;
		P->CameraArm->CameraRotationLagSpeed = 3.2f;
		P->CameraArm->CameraLagMaxDistance = 6000.f;
		// Running lights (only lit at dusk/night): port red, starboard green, masthead white.
		const float Ls = P->HullLengthM / 125.f; // S2: scale to the hull (cutter 30 m .. ironclad 125 m)
		const struct { FVector L; FLinearColor C; float Cd; } RL[] = {
			// W3: the starboard green lit the sea + sails into a green rim (vision). Running lights are markers, not floodlights:
			// 250 cd, 30 m reach, softer blue-green.
			{ FVector(1500.f * Ls, -1550.f * Ls, 1500.f * FMath::Sqrt(Ls)), FLinearColor(1.f, 0.15f, 0.1f), 250.f },
			{ FVector(1500.f * Ls, 1550.f * Ls, 1500.f * FMath::Sqrt(Ls)), FLinearColor(0.2f, 0.75f, 0.55f), 250.f },
			{ FVector(2600.f * Ls, 0.f, 3600.f * FMath::Sqrt(Ls)), FLinearColor(1.f, 0.9f, 0.75f), 2500.f } };
		for (const auto& R : RL)
		{
			UPointLightComponent* L = Lamp(nullptr, FVector::ZeroVector, R.C, R.Cd, R.Cd < 1000.f ? 3000.f : 12000.f);
			L->AttachToComponent(P->HullBody, FAttachmentTransformRules::KeepRelativeTransform);
			L->SetRelativeLocation(R.L);
		}
		// Three gulls follow the ship.
		FActorSpawnParameters SP;
		SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SP.Owner = this;
		for (int32 i = 0; i < 3; ++i)
		{
			if (AIronwakeGull* G = W->SpawnActor<AIronwakeGull>(AIronwakeGull::StaticClass(), FTransform(P->GetActorLocation()), SP))
			{
				G->Follow = P;
				G->FollowOffset = FVector(-2500.f - 1500.f * i, 0.f, 0.f);
				G->RadiusCm = 2500.f + 1200.f * i;
				G->AltCm = 3800.f + 700.f * i;
				G->Angle = i * 2.1f;
				G->Dir = i == 1 ? -1.f : 1.f;
				G->SpeedCmS = 1600.f;
				Gulls.Add(G);
			}
		}
		UE_LOG(LogIronwake, Display, TEXT("IWS1 ship tuned: camera lag 1.6/3.2, running lights, 3 follower gulls"));
	}
	if (!C->bStarted) return;

	const float Kn = P->GetSpeedKnots();
	const FVector Fwd = P->GetActorForwardVector().GetSafeNormal2D();

	// Helmsman: steer for the next point of interest (rudder steps, damped by yaw rate).
	if (bHelm && POIs.IsValidIndex(NextPOI))
	{
		float Brg = BearingDeg(P->GetActorLocation(), POIs[NextPOI].Loc);
		if (P->HasSails())
		{
			// S2 helmsman on the sailing ship: bear away to close-hauled if the point lies upwind.
			const float WindFrom = IronwakeFleet::WindFromYaw(GetWorld());
			const float Off = FMath::FindDeltaAngleDegrees(WindFrom, Brg);
			if (FMath::Abs(Off) < 50.f) Brg = WindFrom + (Off >= 0.f ? 50.f : -50.f);
		}
		const float Err = FMath::FindDeltaAngleDegrees(P->GetHeadingDeg(), Brg);
		const float YawRate = P->HullBody->GetPhysicsAngularVelocityInDegrees().Z;
		const float E = Err - 6.f * YawRate;
		const int32 Step = FMath::Clamp(FMath::RoundToInt(E / 9.f), -3, 3);
		if (Step != P->RudderStep) P->IWRudder(Step);
	}
	// Arrival at the next point.
	if (POIs.IsValidIndex(NextPOI))
	{
		FIronwakePOI& Pt = POIs[NextPOI];
		if (FVector::Dist2D(P->GetActorLocation(), Pt.Loc) < Pt.ArriveRadiusCm)
		{
			Pt.bVisited = true;
			// S2: progression (saved): all four points -> frigate, 10 arrivals -> ironclad.
			const TArray<FString> NewShips = IronwakeFleet::RecordArrival(NextPOI, POIs.Num());
			if (NewShips.Num() > 0) ShowToast(FString::Printf(TEXT("Arrived: %s.   NEW SHIP UNLOCKED: %s  (Tab > SHIPYARD)"), *Pt.Name, *FString::Join(NewShips, TEXT(", "))), 10.f);
			else ShowToast(FString::Printf(TEXT("Arrived: %s"), *Pt.Name), 6.f);
			UE_LOG(LogIronwake, Display, TEXT("IWS1 ARRIVED %s at t=%.1f pos=%s kn=%.1f"), *Pt.Name, Now, *P->GetActorLocation().ToString(), Kn);
			if (bHelm) { bHelm = false; P->IWRudder(0); P->IWTelegraph(3); } // helmsman eases to SLOW AHEAD and hands back the wheel
			NextPOI = (NextPOI + 1) % POIs.Num();
		}
	}
	// P9: the old POI grounding guard is gone (real collision, IronwakeShipCollision.cpp).
	// P29: accumulate nautical miles sailed & detect Port Mooring (< 2.5 kn inside mooring zone) or Landmark Salvage / Survey (anchored / < 2.0 kn near POI).
	{
		const float StepM = FMath::Abs(Kn) * 0.51444f * P->SpeedCompression * Dt;
		DistMetersAccum += StepM;
		if (DistMetersAccum >= 185.2f) { IwPorts::AddDistanceMeters(DistMetersAccum); DistMetersAccum = 0.f; }

		CanDockPortIdx = -1;
		CanLogPoiIdx = -1;
		if (!bDockOpen)
		{
			const FVector ShipPos = P->GetActorLocation();
			const float AbsKn = FMath::Abs(Kn);
			if (AbsKn < 2.5f)
			{
				float BestD = 1e12f;
				for (int32 p = 0; p < WorldPorts.Num(); ++p)
				{
					const float D = FVector::Dist2D(ShipPos, WorldPorts[p].MoorWorldCm);
					if (D <= WorldPorts[p].MoorRadiusM * 100.f && D < BestD) { BestD = D; CanDockPortIdx = p; }
				}
			}
			if (CanDockPortIdx < 0 && (bAnchored || AbsKn < 2.0f))
			{
				float BestD = 1e12f;
				for (int32 i = 0; i < POIs.Num(); ++i)
				{
					if (POIs[i].Tag == TEXT("H") || POIs[i].Tag == TEXT("Ic") || POIs[i].Tag == TEXT("St")) continue;
					const float D = FVector::Dist2D(ShipPos, POIs[i].Loc);
					if (D <= FMath::Max(POIs[i].ArriveRadiusCm, 65000.f) && D < BestD) { BestD = D; CanLogPoiIdx = i; }
				}
			}
		}
	}
	// Anchor: hold the ship; ringing ahead/astern weighs it.
	if (bAnchored)
	{
		if (P->TelegraphIndex != 2) { bAnchored = false; ShowToast(TEXT("Anchor aweigh")); }
		else
		{
			const FVector V = P->HullBody->GetPhysicsLinearVelocity();
			const float K = FMath::Exp(-Dt / 5.f);
			P->HullBody->SetPhysicsLinearVelocity(FVector(V.X * K, V.Y * K, V.Z));
			FVector Av = P->HullBody->GetPhysicsAngularVelocityInDegrees();
			Av.Z *= FMath::Exp(-Dt / 3.f);
			P->HullBody->SetPhysicsAngularVelocityInDegrees(Av);
		}
	}
	// W1: the old bow NS_IW_Splash spawn (sprites inside the hull) and the bow-lift torque are gone; the ship pawn owns
	// the bow wave, spray and squat (IronwakeShipHydro.cpp).
	// Camera: after 5 s without mouse look, ease back over the bow toward the horizon (no snap).
	if (!bPhoto && !bTestCamHold && P->GetController() == C && C->GetViewTarget() == P)
	{
		if (!bHorizonKnown && Now > 1.f) { HorizonPitch = FMath::Clamp(FRotator::NormalizeAxis(C->GetControlRotation().Pitch), -8.f, 0.f); bHorizonKnown = true; }
		if (false && Now - P->LastLookTime > 5.f) // H1: WoWS camera stays where the player leaves it
		{
			FRotator R = C->GetControlRotation();
			const float Hd = P->GetActorRotation().Yaw;
			R.Yaw = R.Yaw + FMath::FindDeltaAngleDegrees(R.Yaw, Hd) * FMath::Clamp(Dt * 0.35f, 0.f, 1.f);
			R.Pitch = FMath::FInterpTo(FRotator::NormalizeAxis(R.Pitch), HorizonPitch, Dt, 0.35f);
			C->SetControlRotation(R);
		}
	}
	// E1: surf breaking on the wreck: a splash at one of 4 waterline points every ~2.5 s.
	WreckSurfT -= Dt;
	if (WreckSurfT <= 0.f && SplashFX)
	{
		WreckSurfT = 2.5f;
		const FTransform WX(FRotator(0.f, 115.f, 0.f), FVector(WreckC.X, WreckC.Y, 0.f));
		const FVector Pts[] = { FVector(0, -1600, 0), FVector(900, 400, 0), FVector(-900, -300, 0), FVector(300, 1500, 0) };
		const FVector L = WX.TransformPosition(Pts[(WreckSurfN++) % 4]);
		if (FVector::Dist2D(L, Player() ? Player()->GetActorLocation() : L) < 300000.f)
			if (UNiagaraComponent* N = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, SplashFX, FVector(L.X, L.Y, AIronwakeShell::WaterZAtWaves(W, L) + AIronwakeShell::RenderedSeaOffsetCm)))
				N->SetWorldScale3D(FVector(0.35f));
	}
	// Moored steamer and buoys ride the swell.
	if (Moored)
	{
		const FVector L = Moored->GetActorLocation();
		const float Z = AIronwakeShell::WaterZAt(W, L);
		Moored->SetActorLocationAndRotation(FVector(L.X, L.Y, FMath::FInterpTo(L.Z, Z, Dt, 1.2f)), FRotator(0.4f * FMath::Sin(Now * 0.5f), 45.f, 1.2f * FMath::Sin(Now * 0.37f)));
	}
	for (int32 i = 0; i < Buoys.Num(); ++i)
	{
		USceneComponent* B = Buoys[i];
		const FVector L = B->GetComponentLocation();
		B->SetWorldLocationAndRotation(FVector(L.X, L.Y, AIronwakeShell::WaterZAtWaves(W, L) + AIronwakeShell::RenderedSeaOffsetCm - 90.f), FRotator(6.f * FMath::Sin(Now * 0.9f + i), 0.f, 7.f * FMath::Sin(Now * 0.7f + i * 1.3f)));
	}
	// E1: moored small craft and wreck debris ride the rendered swell (FloaterBase = keel z offset cm, base yaw, phase).
	for (int32 i = 0; i < Floaters.Num(); ++i)
	{
		USceneComponent* F = Floaters[i];
		if (!F || !FloaterBase.IsValidIndex(i)) continue;
		const FVector L = F->GetComponentLocation();
		const FVector& FB = FloaterBase[i];
		const float Z = AIronwakeShell::WaterZAtWaves(W, L) + AIronwakeShell::RenderedSeaOffsetCm + FB.X;
		F->SetWorldLocation(FVector(L.X, L.Y, FMath::FInterpTo(L.Z, Z, Dt, 1.5f)));
		const FRotator Rr = F->GetRelativeRotation();
		F->SetRelativeRotation(FRotator(Rr.Pitch, FB.Y + 3.f * FMath::Sin(Now * 0.11f + FB.Z), 2.5f * FMath::Sin(Now * 0.6f + FB.Z)));
	}
}

// ---------------------------------------------------------------------------- tick
void AIronwakeVoyage::Tick(float Dt)
{
	Super::Tick(Dt);
	const float RealDt = FMath::Min((float)FApp::GetDeltaTime(), 0.1f);
	Dt = FMath::Min(Dt, 0.1f);
	if (GetWorld()->IsPaused()) { if (!S2Mode.IsEmpty()) TickS2Test(RealDt); if (!P16Mode.IsEmpty()) TickP16Test(RealDt); return; } // only the S2 shipyard test ticks through the menu pause
	ApplyWeather(RealDt);
	TickSailing(Dt);
	TickAutopilot(Dt);
	TickPhoto(RealDt);
	if (DockWidget && DockWidget->IsInViewport()) DockWidget->TickDock(RealDt);
	if (!A1Mode.IsEmpty()) TickA1Test(RealDt);
	if (!E1Mode.IsEmpty()) TickE1Test(RealDt);
	if (!P8Mode.IsEmpty()) TickP8Test(RealDt);
	if (!P9Mode.IsEmpty()) TickP9Test(RealDt);
	if (!P11Mode.IsEmpty()) TickP11Test(RealDt);
	if (!P16Mode.IsEmpty()) TickP16Test(RealDt);
	if (!P19Mode.IsEmpty()) TickP19Test(RealDt);
	if (!P21Mode.IsEmpty()) TickP21Test(RealDt);
	if (!P28Mode.IsEmpty()) TickP28Test(RealDt);
	if (!P29Mode.IsEmpty()) TickP29Test(RealDt);
	if (!P12Mode.IsEmpty()) TickP12Test(RealDt);
	if (!S2Mode.IsEmpty()) TickS2Test(RealDt);
	if (!W2Mode.IsEmpty() || bStormStart) TickW2Test(RealDt);
	if (!TestMode.IsEmpty()) TickTest(RealDt);
}

// ---------------------------------------------------------------------------- HUD (compass tape, bearing, minimap, toasts)
void AIronwakeVoyage::DrawVoyageHUD(AHUD* H, UCanvas* Cv, AIronwakeShipPawn* Ship, UFont* Font, float S, bool bPeaceful)
{
	if (!H || !Cv || !Ship) return;
	if (bChart) { DrawChart(H, Cv, Ship, Font, S); if (GetWorld()->GetRealTimeSeconds() < ToastUntil && !Toast.IsEmpty()) { float W2, H2; H->GetTextSize(Toast, W2, H2, Font, 1.0f * S); H->DrawText(Toast, FLinearColor(0.86f, 0.74f, 0.47f, 1.f), Cv->ClipX * 0.40f - W2 * 0.5f, Cv->ClipY - 40.f * S, Font, 1.0f * S); } return; }
	const FLinearColor Brass(0.86f, 0.74f, 0.47f, 1.f), Dim(0.62f, 0.66f, 0.70f, 1.f), Bg(0.02f, 0.025f, 0.03f, 0.55f), Hi(1.f, 0.85f, 0.45f, 1.f);
	APlayerController* PCtl = H->GetOwningPlayerController();
	const float ViewYaw = PCtl && PCtl->PlayerCameraManager ? PCtl->PlayerCameraManager->GetCameraRotation().Yaw : Ship->GetActorRotation().Yaw;
	const float CX = Cv->ClipX * 0.5f;

	// Compass tape: +/-60 deg around the view direction, ship heading caret, POI diamonds.
	const float TapeW = 640.f * S, TapeH = 34.f * S, TY = 18.f * S;
	H->DrawRect(Bg, CX - TapeW * 0.5f - 8.f * S, TY - 4.f * S, TapeW + 16.f * S, TapeH + 30.f * S);
	const float PxPerDeg = TapeW / 120.f;
	auto XFor = [&](float Brg) { return CX + FMath::FindDeltaAngleDegrees(ViewYaw, Brg) * PxPerDeg; };
	for (int32 d = 0; d < 360; d += 5)
	{
		const float Dl = FMath::FindDeltaAngleDegrees(ViewYaw, (float)d);
		if (FMath::Abs(Dl) > 60.f) continue;
		const float X = CX + Dl * PxPerDeg;
		const bool bMajor = d % 45 == 0;
		H->DrawLine(X, TY + TapeH - (bMajor ? 14.f : (d % 15 == 0 ? 9.f : 5.f)) * S, X, TY + TapeH, bMajor ? Brass : Dim, bMajor ? 2.f : 1.f);
		if (bMajor || d % 15 == 0)
		{
			const FString L = bMajor ? FString(CompassName((float)d)) : FString::Printf(TEXT("%03d"), d);
			float W2, H2; H->GetTextSize(L, W2, H2, Font, (bMajor ? 0.95f : 0.65f) * S);
			H->DrawText(L, bMajor ? Brass : Dim, X - W2 * 0.5f, TY + (bMajor ? 0.f : 4.f) * S, Font, (bMajor ? 0.95f : 0.65f) * S);
		}
	}
	// POI markers on the tape.
	for (int32 i = 0; i < POIs.Num(); ++i)
	{
		const float Brg = BearingDeg(Ship->GetActorLocation(), POIs[i].Loc);
		float Dl = FMath::FindDeltaAngleDegrees(ViewYaw, Brg);
		const bool bEdge = FMath::Abs(Dl) > 60.f;
		Dl = FMath::Clamp(Dl, -60.f, 60.f);
		const float X = CX + Dl * PxPerDeg, Y = TY + TapeH + 9.f * S;
		const FLinearColor C = i == NextPOI ? Hi : (POIs[i].bVisited ? Dim : FLinearColor(0.75f, 0.78f, 0.8f, 0.8f));
		const float R = (i == NextPOI ? 6.f : 4.f) * S;
		H->DrawLine(X, Y - R, X + R, Y, C, 2.f); H->DrawLine(X + R, Y, X, Y + R, C, 2.f);
		H->DrawLine(X, Y + R, X - R, Y, C, 2.f); H->DrawLine(X - R, Y, X, Y - R, C, 2.f);
		if (!bEdge || i == NextPOI) H->DrawText(POIs[i].Tag, C, X + R + 2.f * S, Y - 7.f * S, Font, 0.6f * S);
	}
	// A1: autopilot set-course marker (hollow brass box) on the tape.
	if (ApMode != 0)
	{
		const float Dl = FMath::Clamp(FMath::FindDeltaAngleDegrees(ViewYaw, ApMode == 1 ? HoldYaw : ApTargetYaw), -60.f, 60.f);
		const float X = CX + Dl * PxPerDeg, Y0 = TY + TapeH - 2.f * S;
		const FLinearColor Ac(0.45f, 0.95f, 0.75f, 1.f);
		H->DrawLine(X - 6.f * S, Y0, X + 6.f * S, Y0, Ac, 2.f); H->DrawLine(X - 6.f * S, Y0, X, Y0 - 12.f * S, Ac, 2.f); H->DrawLine(X + 6.f * S, Y0, X, Y0 - 12.f * S, Ac, 2.f);
	}
	// Ship heading caret.
	{
		const float Hd = Ship->GetHeadingDeg();
		const float Dl = FMath::Clamp(FMath::FindDeltaAngleDegrees(ViewYaw, Hd), -60.f, 60.f);
		const float X = CX + Dl * PxPerDeg;
		H->DrawLine(X - 7.f * S, TY - 4.f * S, X, TY + 6.f * S, Brass, 2.5f);
		H->DrawLine(X + 7.f * S, TY - 4.f * S, X, TY + 6.f * S, Brass, 2.5f);
		const FString HS = FString::Printf(TEXT("HDG %03.0f %s"), Hd, CompassName(Hd));
		H->DrawText(HS, Brass, CX + TapeW * 0.5f + 16.f * S, TY + 6.f * S, Font, 0.9f * S);
	}
	// Bearing to the next point.
	float LineY = TY + TapeH + 34.f * S;
	if (POIs.IsValidIndex(NextPOI))
	{
		const FIronwakePOI& Pt = POIs[NextPOI];
		const float Brg = BearingDeg(Ship->GetActorLocation(), Pt.Loc);
		const float Rel = FMath::FindDeltaAngleDegrees(Ship->GetHeadingDeg(), Brg);
		const float Km = FVector::Dist2D(Ship->GetActorLocation(), Pt.Loc) / 100000.f;
		const FString Side = FMath::Abs(Rel) < 3.f ? FString(TEXT("dead ahead")) : FString::Printf(TEXT("%.0f deg to %s"), FMath::Abs(Rel), Rel < 0.f ? TEXT("port") : TEXT("starboard"));
		const FString T = FString::Printf(TEXT("NEXT  %s   %.1f km   brg %03.0f  (%s)%s"), *Pt.Name, Km, Brg, *Side, bHelm ? TEXT("   [HELMSMAN]") : TEXT(""));
		float W2, H2; H->GetTextSize(T, W2, H2, Font, 0.9f * S);
		H->DrawRect(Bg, CX - W2 * 0.5f - 10.f * S, LineY - 3.f * S, W2 + 20.f * S, H2 + 6.f * S);
		H->DrawText(T, Hi, CX - W2 * 0.5f, LineY, Font, 0.9f * S);
		LineY += H2 + 10.f * S;
	}
	// A1: AUTOPILOT chip.
	{
		const FString Ap = FString::Printf(TEXT("AUTOPILOT: %s"), *AutopilotLabel());
		const bool bOn = ApMode != 0;
		float W2, H2; H->GetTextSize(Ap, W2, H2, Font, 0.95f * S);
		const FLinearColor Ac = bOn ? FLinearColor(0.45f, 0.95f, 0.75f, 1.f) : Dim;
		H->DrawRect(bOn ? FLinearColor(0.02f, 0.10f, 0.08f, 0.75f) : Bg, CX - W2 * 0.5f - 12.f * S, LineY - 3.f * S, W2 + 24.f * S, H2 + 6.f * S);
		H->DrawLine(CX - W2 * 0.5f - 12.f * S, LineY - 3.f * S, CX + W2 * 0.5f + 12.f * S, LineY - 3.f * S, Ac, 1.5f);
		H->DrawText(Ap, Ac, CX - W2 * 0.5f, LineY, Font, 0.95f * S);
		LineY += H2 + 10.f * S;
	}
	// Status chips: only what the player needs (anchor, fast sail, clock + weather). Preset/sea debug lines need -iwdebug.
	{
		FString St;
		if (bAnchored) St += TEXT("ANCHORED   ");
		if (bFastSail) St += TEXT("FAST SAIL x4   ");
		if (bLiving) St += FString::Printf(TEXT("%s   %s"), *ClockString(), *LiveWeatherName());
		if (FParse::Param(FCommandLine::Get(), TEXT("iwdebug")))
		{
			St += FString(TEXT("   [")) + WeatherNames[WeatherTarget] + TEXT("]");
			if (AIronwakeStorm* Sx = AIronwakeStorm::Get(GetWorld())) St += FString::Printf(TEXT("   SEA: %s  %.0f m"), IronwakeSea::SeaStateName(Sx->GetLocalState()), IronwakeSea::SigWaveHeightM());
		}
		if (!St.IsEmpty())
		{
			float W2, H2; H->GetTextSize(St, W2, H2, Font, 0.75f * S);
			H->DrawText(St, Dim, CX - W2 * 0.5f, LineY, Font, 0.75f * S);
			LineY += H2 + 8.f * S;
		}
		// P29: Purse & Hold chip + Arrival Mooring / Salvage Prompt when slowed in a port or anchored at a landmark
		{
			const int32 HoldT = IwPorts::TotalCargoTons();
			const int32 CapT = IwPorts::HullCargoCapTons(IronwakeFleet::GetSelected());
			const FString Econ = FString::Printf(TEXT("PURSE: %d cr   |   HOLD: %d / %d t   |   O: logbook"), IwPorts::GetCrowns(), HoldT, CapT);
			float W3, H3; H->GetTextSize(Econ, W3, H3, Font, 0.72f * S);
			H->DrawText(Econ, Brass, CX - W3 * 0.5f, LineY, Font, 0.72f * S);
		}
		if (!bDockOpen && (CanDockPortIdx >= 0 || CanLogPoiIdx >= 0))
		{
			const FString Prompt = CanDockPortIdx >= 0
				? FString::Printf(TEXT("[E / Enter]  Make Fast — Port of %s"), *WorldPorts[CanDockPortIdx].Name)
				: FString::Printf(TEXT("[E / Enter]  Log Discovery & Salvage — %s"), *POIs[CanLogPoiIdx].Name);
			float PW, PH; H->GetTextSize(Prompt, PW, PH, Font, 1.12f * S);
			const float PY = Cv->ClipY * 0.77f;
			H->DrawRect(FLinearColor(0.015f, 0.025f, 0.032f, 0.88f), CX - PW * 0.5f - 22.f * S, PY - 9.f * S, PW + 44.f * S, PH + 18.f * S);
			H->DrawLine(CX - PW * 0.5f - 22.f * S, PY - 9.f * S, CX + PW * 0.5f + 22.f * S, PY - 9.f * S, Hi, 2.f);
			H->DrawLine(CX - PW * 0.5f - 22.f * S, PY + PH + 9.f * S, CX + PW * 0.5f + 22.f * S, PY + PH + 9.f * S, Brass, 1.5f);
			H->DrawText(Prompt, Hi, CX - PW * 0.5f, PY, Font, 1.12f * S);
		}
	}
	// Minimap (bottom right): +/-4 km, north (+X) up, east (+Y) right.
	{
		const float MS = 230.f * S, MX = Cv->ClipX - MS - 26.f * S, MY = Cv->ClipY - MS - 26.f * S;
		const float Half = 400000.f;
		H->DrawRect(FLinearColor(0.02f, 0.035f, 0.045f, 0.62f), MX, MY, MS, MS);
		H->DrawLine(MX, MY, MX + MS, MY, Dim, 1.f); H->DrawLine(MX, MY + MS, MX + MS, MY + MS, Dim, 1.f);
		H->DrawLine(MX, MY, MX, MY + MS, Dim, 1.f); H->DrawLine(MX + MS, MY, MX + MS, MY + MS, Dim, 1.f);
		auto Map = [&](const FVector& W) { return FVector2D(MX + MS * 0.5f + (W.Y - Ship->GetActorLocation().Y) / Half * MS * 0.5f, MY + MS * 0.5f - (W.X - Ship->GetActorLocation().X) / Half * MS * 0.5f); };
		H->DrawText(TEXT("N"), Brass, MX + MS * 0.5f - 5.f * S, MY + 2.f * S, Font, 0.7f * S);
		if (AIronwakeLivingSea* LS = AIronwakeLivingSea::Get(GetWorld()))
		{
			for (AIronwakeMerchant* Mc : LS->Merchants) if (IsValid(Mc)) { const FVector2D P = Map(Mc->GetActorLocation()); H->DrawRect(FLinearColor(0.6f, 0.62f, 0.65f, 0.8f), P.X - 2.f * S, P.Y - 2.f * S, 4.f * S, 4.f * S); }
			if (LS->Airship) { const FVector2D P = Map(LS->Airship->GetActorLocation()); H->DrawText(TEXT("~"), Dim, P.X - 4.f * S, P.Y - 8.f * S, Font, 0.8f * S); }
		}
		// P21: NPC contacts. Known (hailed / scoped / named) = brass dot with a short name; unknown = small grey dot. Airships draw as a diamond.
		if (AIronwakeNpcDirector* Nd = AIronwakeNpcDirector::Get(GetWorld()))
		{
			for (const FNpcRec& Rc : Nd->Recs)
			{
				if (Rc.bAway) continue;
				const FVector2D P = Map(Rc.Pos);
				if (P.X < MX || P.X > MX + MS || P.Y < MY || P.Y > MY + MS) continue;
				const bool bAirC = Rc.Def && Rc.Def->bAir;
				const FLinearColor C = Rc.bKnown ? (Rc.Faction == ENpcFaction::Crown ? FLinearColor(0.85f, 0.35f, 0.3f, 0.95f) : (Rc.Faction == ENpcFaction::Pirate ? FLinearColor(0.9f, 0.2f, 0.2f, 0.95f) : Brass)) : FLinearColor(0.55f, 0.57f, 0.6f, 0.75f);
				const float Rd = (Rc.bKnown ? 3.f : 2.f) * S;
				if (bAirC) { H->DrawLine(P.X - Rd, P.Y, P.X, P.Y - Rd, C, 1.f); H->DrawLine(P.X, P.Y - Rd, P.X + Rd, P.Y, C, 1.f); H->DrawLine(P.X + Rd, P.Y, P.X, P.Y + Rd, C, 1.f); H->DrawLine(P.X, P.Y + Rd, P.X - Rd, P.Y, C, 1.f); }
				else H->DrawRect(C, P.X - Rd * 0.5f, P.Y - Rd * 0.5f, Rd, Rd);
				if (Rc.bKnown) H->DrawText(Rc.Name, C, P.X + 4.f * S, P.Y - 7.f * S, Font, 0.5f * S);
			}
		}
		for (int32 i = 0; i < POIs.Num(); ++i)
		{
			const FVector2D P = Map(POIs[i].Loc);
			if (P.X < MX || P.X > MX + MS || P.Y < MY || P.Y > MY + MS) continue;
			const FLinearColor C = i == NextPOI ? Hi : (POIs[i].bVisited ? Dim : FLinearColor(0.8f, 0.8f, 0.8f, 0.9f));
			H->DrawRect(C, P.X - 3.5f * S, P.Y - 3.5f * S, 7.f * S, 7.f * S);
			H->DrawText(POIs[i].Tag, C, P.X + 5.f * S, P.Y - 8.f * S, Font, 0.65f * S);
		}
		if (ApMode == 0 && POIs.IsValidIndex(NextPOI)) { const FVector2D A = Map(Ship->GetActorLocation()), B = Map(POIs[NextPOI].Loc); H->DrawLine(A.X, A.Y, B.X, B.Y, FLinearColor(1.f, 0.85f, 0.45f, 0.35f), 1.f); }
		// A1: autopilot route (rest of the current leg + remaining waypoints) and the hold-course line.
		auto Clip = [&](FVector2D Q) { return FVector2D(FMath::Clamp(Q.X, MX, MX + MS), FMath::Clamp(Q.Y, MY, MY + MS)); };
		const FLinearColor RouteC(0.45f, 0.95f, 0.75f, 0.9f);
		if (ApMode == 1)
		{
			const FVector2D A = Map(Ship->GetActorLocation()), B = Map(Ship->GetActorLocation() + FRotator(0.f, HoldYaw, 0.f).Vector() * 300000.f);
			H->DrawLine(A.X, A.Y, Clip(B).X, Clip(B).Y, RouteC, 1.5f);
		}
		else if (ApMode >= 2)
		{
			FVector2D Prv = Map(Ship->GetActorLocation());
			for (int32 i = LegIdx; i < LegPath.Num(); ++i) { const FVector2D Q = Map(LegPath[i]); H->DrawLine(Clip(Prv).X, Clip(Prv).Y, Clip(Q).X, Clip(Q).Y, RouteC, 1.5f); Prv = Q; }
			if (ApMode == 2) for (int32 i = CurWp + 1; i < Waypoints.Num(); ++i) { const FVector2D Q = Map(Waypoints[i]); H->DrawLine(Clip(Prv).X, Clip(Prv).Y, Clip(Q).X, Clip(Q).Y, FLinearColor(0.45f, 0.95f, 0.75f, 0.5f), 1.f); Prv = Q; }
		}
		if (ApMode != 1) for (int32 i = 0; i < Waypoints.Num(); ++i) { const FVector2D Q = Clip(Map(Waypoints[i])); H->DrawText(FString::FromInt(i + 1), RouteC, Q.X - 3.f * S, Q.Y - 8.f * S, Font, 0.6f * S); }
		// Player arrow.
		const FVector2D P = Map(Ship->GetActorLocation());
		const float Hd = FMath::DegreesToRadians(Ship->GetHeadingDeg());
		const FVector2D F(FMath::Sin(Hd), -FMath::Cos(Hd)), Rt(-F.Y, F.X);
		const FVector2D Tip = P + F * 9.f * S, L = P - F * 6.f * S + Rt * 5.f * S, R = P - F * 6.f * S - Rt * 5.f * S;
		H->DrawLine(Tip.X, Tip.Y, L.X, L.Y, Brass, 2.f); H->DrawLine(L.X, L.Y, R.X, R.Y, Brass, 2.f); H->DrawLine(R.X, R.Y, Tip.X, Tip.Y, Brass, 2.f);
	}
	// P11: one short hint line, bottom-left, under the ship block (full list: H / F1).
	{
		const FString K = TEXT("H: help   M: chart   P: autopilot   Esc: menu");
		H->DrawText(K, FLinearColor(0.62f, 0.66f, 0.70f, 0.8f), 28.f * S, Cv->ClipY - 22.f * S, Font, 0.62f * S);
	}
	if (GetWorld()->GetRealTimeSeconds() < ToastUntil && !Toast.IsEmpty())
	{
		float W2, H2; H->GetTextSize(Toast, W2, H2, Font, 1.2f * S);
		const float Y = Cv->ClipY * 0.30f;
		H->DrawRect(Bg, CX - W2 * 0.5f - 16.f * S, Y - 6.f * S, W2 + 32.f * S, H2 + 12.f * S);
		H->DrawText(Toast, Brass, CX - W2 * 0.5f, Y, Font, 1.2f * S);
	}
	(void)bPeaceful;
}

// ---------------------------------------------------------------------------- S1 PIE test driver
void AIronwakeVoyage::TestLog(const TCHAR* What)
{
	AIronwakeShipPawn* P = Player();
	const FIronwakePOI* Pt = POIs.IsValidIndex(NextPOI) ? &POIs[NextPOI] : nullptr;
	UE_LOG(LogIronwake, Display, TEXT("IWS1 %s t=%.1f pos=(%.0f,%.0f) hdg=%.0f kn=%.1f pitch=%.2f roll=%.2f tel=%s rud=%d next=%s dist=%.0fm helm=%d anchor=%d fast=%d photo=%d wx=%d dil=%.2f"), What, TestT,
		P ? P->GetActorLocation().X / 100.f : 0.f, P ? P->GetActorLocation().Y / 100.f : 0.f, P ? P->GetHeadingDeg() : 0.f, P ? P->GetSpeedKnots() : 0.f,
		P ? P->GetActorRotation().Pitch : 0.f, P ? P->GetActorRotation().Roll : 0.f, P ? *P->GetTelegraphName() : TEXT("-"), P ? P->RudderStep : 0,
		Pt ? *Pt->Name : TEXT("-"), (P && Pt) ? FVector::Dist2D(P->GetActorLocation(), Pt->Loc) / 100.f : -1.f, bHelm, bAnchored, bFastSail, bPhoto, WeatherTarget,
		UGameplayStatics::GetGlobalTimeDilation(this));
}

void AIronwakeVoyage::AimControlAt(const FVector& Target, float Pitch)
{
	AIronwakePlayerController* C = PC();
	AIronwakeShipPawn* P = Player();
	if (!C || !P) return;
	const FVector Eye = P->CameraArm->GetComponentLocation();
	C->SetControlRotation(FRotator(Pitch, (Target - Eye).Rotation().Yaw, 0.f));
}

void AIronwakeVoyage::TickTest(float RealDt)
{
	AIronwakePlayerController* C = PC();
	AIronwakeShipPawn* P = Player();
	if (!C || !P) return;
	const float Prev = TestT;
	TestT += RealDt;
	auto Crossed = [&](float T) { return Prev < T && TestT >= T; };
	auto Shot = [&](const TCHAR* N) { C->IWShot(N); TestLog(*FString::Printf(TEXT("shot %s"), N)); };
	TestLogAccum += RealDt;
	if (TestLogAccum >= 10.f) { TestLogAccum = 0.f; TestLog(TEXT("tel")); }
	const float SinceMark = TestT - TestMark;
	auto Go = [&](int32 Step) { TestStep = Step; TestMark = TestT; };

	if (TestMode == TEXT("look") && TestStep == 0 && TestT >= 2.f)
	{
		if (!C->bStarted) C->StartGame(true);
		const FVector L(-3000.f, 144000.f, P->GetActorLocation().Z);
		P->SetActorLocationAndRotation(L, FRotator(0.f, 129.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
		P->HullBody->SetPhysicsLinearVelocity(FVector::ZeroVector);
		for (FIronwakePOI& Pt : POIs) Pt.bVisited = true;
		NextPOI = 3;
		TestLog(TEXT("look start (teleported near the light)"));
		AimControlAt(POIs[2].Loc + FVector(0, 0, 4000.f), -5.f);
		bTestCamHold = true;
		TestStep = 11; TestMark = TestT - 2.f; // continue the tour at "lighthouse shot, then anchor"
		return;
	}
	if (TestMode != TEXT("tour") && TestMode != TEXT("look")) return;
	switch (TestStep)
	{
	case 0:
		if (TestT >= 2.f)
		{
			if (!C->bStarted) C->StartGame(true);
			TestLog(TEXT("tour start (peaceful)"));
			bTestCamHold = true;
			AimControlAt(HarbourC + FVector(0, 0, 1500.f), -7.f); // look back at the harbour we sail from
			Go(1);
		}
		break;
	case 1:
		if (SinceMark >= 5.f) { Shot(TEXT("S1_0_harbour")); Go(2); }
		break;
	case 2:
		if (SinceMark >= 1.f)
		{
			P->ResetCameraToHorizon();
			bTestCamHold = false;
			NextPOI = 0;
			ToggleFastSail();
			P->IWTelegraph(5); // FULL AHEAD
			ToggleHelm();
			P->IWTelegraph(5);
			Go(3);
		}
		break;
	case 3:
		if (SinceMark >= 6.f) { Shot(TEXT("S1_1_hud_compass")); Go(4); }
		break;
	case 4: // sail to the wreck
		if (POIs[0].bVisited)
		{
			TestLog(TEXT("arrived wreck"));
			bTestCamHold = true;
			AimControlAt(WreckC + FVector(0, 0, 800.f), -9.f);
			Go(5);
		}
		else if (SinceMark > 400.f) { TestLog(TEXT("TIMEOUT wreck")); TestMode.Empty(); }
		break;
	case 5:
		if (SinceMark >= 4.f) { Shot(TEXT("S1_2_wreck_arrival")); P->IWRudder(2); P->IWTelegraph(4); Go(6); } // circle the wreck: STBD 20, HALF
		break;
	case 6:
		AimControlAt(WreckC + FVector(0, 0, 800.f), -9.f);
		if (SinceMark >= 25.f) { Shot(TEXT("S1_3_circling_wreck")); Go(7); }
		break;
	case 7:
		AimControlAt(WreckC + FVector(0, 0, 800.f), -9.f);
		if (SinceMark >= 20.f)
		{
			bTestCamHold = false;
			P->ResetCameraToHorizon();
			NextPOI = 1;
			P->IWTelegraph(5);
			if (!bHelm) ToggleHelm();
			P->IWTelegraph(5);
			Go(8);
		}
		break;
	case 8: // sail to the stacks
		if (POIs[1].bVisited)
		{
			TestLog(TEXT("arrived stacks"));
			bTestCamHold = true;
			AimControlAt(StacksC + FVector(0, 0, 3000.f), -4.f);
			Go(9);
		}
		else if (SinceMark > 400.f) { TestLog(TEXT("TIMEOUT stacks")); TestMode.Empty(); }
		break;
	case 9:
		if (SinceMark >= 4.f)
		{
			Shot(TEXT("S1_4_stacks"));
			bTestCamHold = false;
			P->ResetCameraToHorizon();
			NextPOI = 2;
			P->IWTelegraph(5);
			if (!bHelm) ToggleHelm();
			P->IWTelegraph(5);
			Go(10);
		}
		break;
	case 10: // sail to the lighthouse
		if (POIs[2].bVisited)
		{
			TestLog(TEXT("arrived lighthouse"));
			bTestCamHold = true;
			AimControlAt(POIs[2].Loc + FVector(0, 0, 4000.f), -5.f);
			Go(11);
		}
		else if (SinceMark > 400.f) { TestLog(TEXT("TIMEOUT lighthouse")); TestMode.Empty(); }
		break;
	case 11:
		if (SinceMark >= 4.f) { Shot(TEXT("S1_5_lighthouse")); ToggleAnchor(); Go(12); }
		break;
	case 12: // anchor: log the run-off, then photo mode
		if (FMath::Fmod(SinceMark, 4.f) < RealDt) TestLog(TEXT("anchored"));
		if (SinceMark >= 16.f)
		{
			TogglePhoto();
			// Low over the ship's quarter on the far side from the island: ship in the foreground, lighthouse behind.
			const FVector SL = P->GetActorLocation();
			const FVector ToI = (POIs[2].Loc - SL).GetSafeNormal2D();
			const FVector Sd(-ToI.Y, ToI.X, 0.f);
			const FVector CamL = SL - ToI * 26000.f + Sd * 9000.f + FVector(0, 0, 2200.f);
			SetPhotoView(CamL, (SL + ToI * 30000.f + FVector(0, 0, 1200.f) - CamL).Rotation());
			Go(13);
		}
		break;
	case 13:
		if (SinceMark >= 3.f) { Shot(TEXT("S1_6_photo_mode")); Go(14); }
		break;
	case 14:
		if (SinceMark >= 2.f) { TogglePhoto(); SetWeather(1, 4.f); bTestCamHold = true; AimControlAt(POIs[2].Loc + FVector(0, 0, 4000.f), -4.f); Go(15); }
		break;
	case 15:
		if (SinceMark >= 9.f) { Shot(TEXT("S1_7_dusk")); Go(19); }
		break;
	case 16:
		if (SinceMark >= 9.f) { Shot(TEXT("S1_8_foggy_night")); Go(17); }
		break;
	case 17:
		if (SinceMark >= 4.f) { Shot(TEXT("S1_9_foggy_night_b")); SetWeather(0, 2.f); bTestCamHold = false; Go(18); }
		break;
	case 18:
		if (SinceMark >= 4.f) { TestLog(TEXT("tour done")); TestMode.Empty(); }
		break;
	case 19:
		if (SinceMark >= 1.f) { SetWeather(2, 4.f); Go(16); }
		break;
	default: break;
	}
	if (TestT > 1500.f && !TestMode.IsEmpty()) { TestLog(TEXT("tour TIMEOUT")); TestMode.Empty(); }
}

// ---------------------------------------------------------------------------- E1 capture director (-iwe1=shots [-iwe1quit])
// Teleports the player to a stand-off point facing each place, resets the DEFAULT chase camera, waits for it to settle and
// screenshots (with HUD). Then a photo-mode close-up of the docked airship.
void AIronwakeVoyage::TickE1Test(float RealDt)
{
	AIronwakePlayerController* C = PC();
	AIronwakeShipPawn* P = Player();
	if (!C || !P) return;
	E1T += RealDt;
	const float Since = E1T - E1Mark;
	FVector Island(-50000.f, 190000.f, 0.f);
	AIronwakeLivingSea* LS = AIronwakeLivingSea::Get(GetWorld());
	if (LS) Island = LS->IslandCentre;
	struct FV { const TCHAR* Shot; FVector Target; float StandM; float FromYaw; };
	// FromYaw: compass direction (UE yaw) from the target to the ship.
	const FV Views[] = {
		{ TEXT("E_harbour"), HarbourC + FVector(-2000.f, 2000.f, 0.f), 650.f, 45.f },
		{ TEXT("E_island"), Island, 1300.f, 230.f },
		{ TEXT("E_wreck"), WreckC, 300.f, 20.f },
		{ TEXT("E_needles"), StacksC, 2600.f, 200.f },
		{ TEXT("E_needles_1km"), StacksC, 1100.f, 200.f },
		{ TEXT("E_harbour_close"), HarbourC + FVector(-2000.f, 2000.f, 0.f), 330.f, 60.f },
		{ TEXT("E_wreck_close"), WreckC, 170.f, 330.f },
		{ TEXT("E_wreck_100m"), WreckC, 100.f, 330.f },
	};
	const int32 NV = UE_ARRAY_COUNT(Views);
	auto Place = [&](const FV& V)
	{
		const FVector D = FRotator(0.f, V.FromYaw, 0.f).Vector();
		const FVector L = FVector(V.Target.X, V.Target.Y, 0.f) + D * V.StandM * 100.f;
		P->SetActorLocationAndRotation(FVector(L.X, L.Y, P->GetActorLocation().Z), FRotator(0.f, V.FromYaw + 180.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
		if (P->HullBody) { P->HullBody->SetPhysicsLinearVelocity(FVector::ZeroVector); P->HullBody->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector); }
		P->IWTelegraph(2);
		P->ResetCameraToHorizon();
		UE_LOG(LogIronwake, Display, TEXT("IWE1 view %s: ship at %s, %.0f m from target"), V.Shot, *L.ToString(), V.StandM);
	};
	if (E1Mode.StartsWith(TEXT("bench")))
	{
		// FPS bench: hold the default camera on a hero view (harbour 330 m) for the whole CSV capture.
		if (E1Step == 0 && E1T >= 3.f)
		{
			if (!C->bStarted) C->StartGame(true);
			const int32 Vi = E1Mode == TEXT("bench_island") ? 1 : 5;
			Place(Views[Vi]); E1Step = 1; E1Mark = E1T;
		}
		if (E1Step == 1 && Since > 2.f && Since < 2.3f) P->ResetCameraToHorizon();
		return;
	}
	if (E1Step == 0)
	{
		if (E1T < 3.f) return;
		if (!C->bStarted) C->StartGame(true);
		for (FIronwakePOI& Pt : POIs) Pt.bVisited = true;
		Place(Views[0]); E1Step = 1; E1Mark = E1T; return;
	}
	if (E1Step >= 1 && E1Step <= NV)
	{
		const int32 i = E1Step - 1;
		if (Since > 2.f && Since < 2.3f) P->ResetCameraToHorizon();
		// Screenshot requests resolve at the end of a later frame: shoot at 9 s, move on only at 10.5 s.
		if (Since >= 9.f && !bE1Shot) { C->IWShot(Views[i].Shot); bE1Shot = true; UE_LOG(LogIronwake, Display, TEXT("IWE1 shot %s"), Views[i].Shot); }
		if (Since >= 10.5f)
		{
			bE1Shot = false;
			++E1Step; E1Mark = E1T;
			if (E1Step <= NV) Place(Views[E1Step - 1]);
		}
		return;
	}
	AIronwakeAirship* As = LS ? LS->Airship.Get() : nullptr;
	if (E1Step == NV + 1)
	{
		if (As)
		{
			if (!bPhoto) TogglePhoto();
			const FVector A = As->GetActorLocation();
			const FVector F = As->GetActorForwardVector(), R = As->GetActorRightVector();
			const FVector Eye = A - F * 2500.f + R * 13000.f + FVector(0, 0, 4500.f);
			SetPhotoView(Eye, (A + F * 1500.f + FVector(0, 0, 4000.f) - Eye).Rotation());
			UE_LOG(LogIronwake, Display, TEXT("IWE1 airship at %s dockstate=%d docked_for=%.0f"), *A.ToString(), As->DockState, As->DockedFor());
		}
		E1Step = NV + 2; E1Mark = E1T; return;
	}
	if (E1Step == NV + 2 && Since >= 6.f && !bE1Shot) { C->IWShot(TEXT("E_airship_close")); bE1Shot = true; }
	if (E1Step == NV + 2 && Since >= 7.5f)
	{
		bE1Shot = false;
		if (As)
		{
			const FVector A = As->GetActorLocation();
			const FVector F = As->GetActorForwardVector(), R = As->GetActorRightVector();
			// Seaward of the mooring mast, looking back at mast + docked airship with the town below.
			const FVector Mast = As->bHasDock ? As->DockTop : A;
			const FVector Eye = Mast + FVector(0.7071f, 0.7071f, 0.f) * 30000.f + FVector(0, 0, -2500.f);
			SetPhotoView(Eye, ((Mast + A) * 0.5f + FVector(0, 0, -1500.f) - Eye).Rotation());
		}
		E1Step = NV + 3; E1Mark = E1T; return;
	}
	if (E1Step == NV + 3 && Since >= 5.f && !bE1Shot) { C->IWShot(TEXT("E_airship_mast")); bE1Shot = true; }
	if (E1Step == NV + 3 && Since >= 6.5f)
	{
		bE1Shot = false;
		if (bPhoto) TogglePhoto();
		UE_LOG(LogIronwake, Display, TEXT("IWE1 DONE"));
		E1Step = NV + 4; E1Mark = E1T; return;
	}
	if (E1Step == NV + 4 && Since >= 3.f)
	{
		E1Mode.Empty();
		if (FParse::Param(FCommandLine::Get(), TEXT("iwe1quit"))) UKismetSystemLibrary::QuitGame(this, C, EQuitPreference::Quit, false);
	}
}
