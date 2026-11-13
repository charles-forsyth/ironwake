#include "IronwakeShell.h"
#include "IronwakeShipPawn.h"
#include "IronwakeTest.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"
#include "WaterBodyActor.h"
#include "WaterBodyComponent.h"

AIronwakeShell::AIronwakeShell()
{
	PrimaryActorTick.bCanEverTick = true;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cyl(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Tracer(TEXT("/Game/Ironwake/FX/M_IW_Tracer.M_IW_Tracer"));

	Ball = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ball"));
	RootComponent = Ball;
	if (Sphere.Succeeded()) Ball->SetStaticMesh(Sphere.Object);
	Ball->SetRelativeScale3D(FVector(2.5f)); // 2.5 m glowing tracer head (readable at 1-4 km)
	Ball->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Ball->SetCastShadow(false);

	Streak = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Streak"));
	Streak->SetupAttachment(Ball);
	if (Cyl.Succeeded()) Streak->SetStaticMesh(Cyl.Object);
	Streak->SetUsingAbsoluteScale(true);
	Streak->SetWorldScale3D(FVector(0.9f, 0.9f, 30.f)); // 30 m tracer streak behind the shell
	Streak->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f)); // cylinder Z -> actor X
	Streak->SetRelativeLocation(FVector(-600.f, 0.f, 0.f)); // relative to 2.5x scaled ball => -15 m
	Streak->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Streak->SetCastShadow(false);
	if (Tracer.Succeeded())
	{
		Ball->SetMaterial(0, Tracer.Object);
		Streak->SetMaterial(0, Tracer.Object);
	}
	InitialLifeSpan = 30.f;
}

void AIronwakeShell::Launch(const FVector& InVelocity, AIronwakeShipPawn* InShooter, float InDamage)
{
	Velocity = InVelocity;
	Shooter = InShooter;
	Damage = InDamage;
	SetActorRotation(Velocity.Rotation());
}

float AIronwakeShell::WaterZAt(UWorld* World, const FVector& P)
{
	static TWeakObjectPtr<AWaterBody> Cached;
	static TWeakObjectPtr<UWorld> CachedWorld;
	if (!World) return -350.f;
	if (CachedWorld.Get() != World || !Cached.IsValid())
	{
		Cached = nullptr;
		CachedWorld = World;
		for (TActorIterator<AWaterBody> It(World); It; ++It)
		{
			if (It->GetWaterBodyType() == EWaterBodyType::Ocean) { Cached = *It; break; }
		}
	}
	if (AWaterBody* WB = Cached.Get())
	{
		if (UWaterBodyComponent* C = WB->GetWaterBodyComponent())
		{
			FVector Loc, N, V; float Depth = 0.f;
			if (C->GetWaterSurfaceInfoAtLocation(P, Loc, N, V, Depth, false)) return Loc.Z;
		}
	}
	return -350.f;
}

float AIronwakeShell::WaterZAtWaves(UWorld* World, const FVector& P)
{
	// R1: surface height INCLUDING the Gerstner waves (WaterZAt returns the flat water plane only).
	static TWeakObjectPtr<UWaterBodyComponent> CachedC;
	static TWeakObjectPtr<UWorld> CachedW;
	if (!World) return -350.f;
	if (CachedW.Get() != World || !CachedC.IsValid())
	{
		CachedC = nullptr; CachedW = World;
		for (TActorIterator<AWaterBody> It(World); It; ++It)
			if (It->GetWaterBodyType() == EWaterBodyType::Ocean) { CachedC = It->GetWaterBodyComponent(); break; }
	}
	if (UWaterBodyComponent* C = CachedC.Get())
	{
		const auto R = C->TryQueryWaterInfoClosestToWorldLocation(P, EWaterBodyQueryFlags::ComputeLocation | EWaterBodyQueryFlags::IncludeWaves);
		if (R.HasValue()) return R.GetValue().GetWaterSurfaceLocation().Z;
	}
	return WaterZAt(World, P);
}

void AIronwakeShell::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UWorld* W = GetWorld();
	const float Dt = FMath::Min(DeltaSeconds, 0.1f);
	Age += Dt;
	const int32 N = FMath::Clamp(FMath::CeilToInt(Velocity.Size() * Dt / 400.f), 1, 64);
	const float H = Dt / N;
	FVector P = GetActorLocation();
	AIronwakeShipPawn* From = Shooter.Get();
	for (int32 s = 0; s < N; ++s)
	{
		Velocity.Z -= Gravity * H;
		P += Velocity * H;
		// Ship hull boxes (body frame: bow +X, origin ~ waterline)
		for (TActorIterator<AIronwakeShipPawn> It(W); It; ++It)
		{
			AIronwakeShipPawn* Ship = *It;
			if (Ship == From) continue;
			const FVector L = Ship->GetActorTransform().InverseTransformPositionNoScale(P);
			if (FMath::Abs(L.X) < 7000.f && FMath::Abs(L.Y) < 1150.f && L.Z > -700.f && L.Z < 1700.f)
			{
				Ship->ApplyShellHit(P, Damage, From);
				if (From) From->OnOwnShellImpact(true, P, Ship);
				Destroy();
				return;
			}
		}
		if (Age > 0.15f)
		{
			const float WZ = WaterZAt(W, P);
			if (P.Z < WZ)
			{
				const FVector Hit(P.X, P.Y, WZ);
				if (UNiagaraSystem* Splash = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/Ironwake/FX/NS_IW_Splash.NS_IW_Splash")))
				{
					UNiagaraFunctionLibrary::SpawnSystemAtLocation(W, Splash, Hit, FRotator::ZeroRotator, FVector(1.f), true);
				}
				if (From) From->OnOwnShellImpact(false, Hit, nullptr);
				Destroy();
				return;
			}
		}
	}
	SetActorLocationAndRotation(P, Velocity.Rotation());
}
