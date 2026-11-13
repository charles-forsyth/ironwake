#include "IronwakeGameMode.h"
#include "IronwakeSettings.h"
#include "IronwakeHUD.h"
#include "IronwakeLivingSea.h"
#include "IronwakeVoyage.h"
#include "IronwakeStorm.h"
#include "IronwakePlayerController.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "IronwakeShipPawn.h"
#include "IronwakeTest.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"

AIronwakeGameMode::AIronwakeGameMode()
{
	static ConstructorHelpers::FClassFinder<APawn> ShipBP(TEXT("/Game/Ironwake/Ships/BP_PlayerShip"));
	DefaultPawnClass = ShipBP.Succeeded() ? ShipBP.Class : TSubclassOf<APawn>(AIronwakeShipPawn::StaticClass());
	HUDClass = AIronwakeHUD::StaticClass();
	PlayerControllerClass = AIronwakePlayerController::StaticClass();
}

AActor* AIronwakeGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	// Honour a PlayerStart if a level adds one; otherwise null -> SpawnDefaultPawnFor uses SeaSpawnLocation.
	TActorIterator<APlayerStart> It(GetWorld());
	return It ? *It : nullptr;
}

APawn* AIronwakeGameMode::SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot)
{
	FTransform T = (StartSpot && StartSpot->IsA<APlayerStart>()) ? StartSpot->GetActorTransform() : FTransform(FRotator(0.f, SeaSpawnYaw, 0.f), SeaSpawnLocation);
	T.SetScale3D(FVector::OneVector);
	UE_LOG(LogIronwake, Display, TEXT("IWGM spawning %s at %s"), *GetNameSafe(GetDefaultPawnClassForController(NewPlayer)), *T.GetLocation().ToString());
	return SpawnDefaultPawnAtTransform(NewPlayer, T);
}

void AIronwakeGameMode::StartPlay()
{
	// Automation runs (card B/C demos, -iwtour clip) skip the title menu.
	const FString Saved = FPaths::ProjectSavedDir();
	const bool bAutomation = IFileManager::Get().FileExists(*(Saved / TEXT("iw_combatdemo.txt"))) || IFileManager::Get().FileExists(*(Saved / TEXT("iw_autodemo.txt")))
		|| FParse::Param(FCommandLine::Get(), TEXT("iwnomenu"));
	const bool bTour = IFileManager::Get().FileExists(*(Saved / TEXT("iw_tour.txt"))) || FParse::Param(FCommandLine::Get(), TEXT("iwtour"));
	if (bAutomation) bShowTitleMenu = false;
	if (FParse::Param(FCommandLine::Get(), TEXT("iww1"))) bSpawnEnemy = false; // W1 test: peaceful sea
	{ FString W2; if (FParse::Value(FCommandLine::Get(), TEXT("iww2="), W2) || FParse::Param(FCommandLine::Get(), TEXT("iwstorm"))) { bSpawnEnemy = false; if (!W2.IsEmpty()) bShowTitleMenu = false; } }
	{
		FString S2;
		if (FParse::Value(FCommandLine::Get(), TEXT("iws2="), S2)) bSpawnEnemy = false; // S2 fleet tests: peaceful sea
	}
	{
		FString A1;
		if (FParse::Value(FCommandLine::Get(), TEXT("iwa1="), A1) && A1 != TEXT("fire")) bSpawnEnemy = false; // A1 autopilot tests: peaceful sea
	}
	// P1: RESTART reopens the map with ?iwrestart[&iwpeaceful=1]: skip the title, keep the mode.
	bRestarted = UGameplayStatics::HasOption(OptionsString, TEXT("iwrestart"));
	if (bRestarted)
	{
		bShowTitleMenu = false;
		bSpawnEnemy = UGameplayStatics::ParseOption(OptionsString, TEXT("iwpeaceful")) != TEXT("1");
		UE_LOG(LogIronwake, Display, TEXT("IWGM restart (options '%s') peaceful=%d"), *OptionsString, !bSpawnEnemy);
	}
	Super::StartPlay();
	IronwakeSettings::ApplyGraphics(); // H1: saved LOW/MEDIUM/HIGH (default MEDIUM = 60 fps class)

	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	GetWorld()->SpawnActor<AIronwakeLivingSea>(AIronwakeLivingSea::StaticClass(), FTransform::Identity, SP);
	// W2: weather -> sea state director (storm, spume, rain, lightning, wind audio).
	GetWorld()->SpawnActor<AIronwakeStorm>(AIronwakeStorm::StaticClass(), FTransform::Identity, SP);
	// S1: Voyager director (points of interest, gulls, weather, photo mode, compass/minimap HUD).
	GetWorld()->SpawnActor<AIronwakeVoyage>(AIronwakeVoyage::StaticClass(), FTransform::Identity, SP);

	// With the title menu up, the enemy waits for Start (Captain mode); the tour spawns it itself.
	if (bSpawnEnemy && !bShowTitleMenu && !bTour) SpawnEnemy();
}

void AIronwakeGameMode::SpawnEnemy()
{
	for (TActorIterator<AIronwakeShipPawn> It(GetWorld()); It; ++It) if (It->bIsNPC && !It->bSinking) return;
	UClass* Cls = DefaultPawnClass ? DefaultPawnClass.Get() : AIronwakeShipPawn::StaticClass();
	// P1: the enemy appears 7-7.5 km out, AHEAD of the player's bow (within ~25 deg), inside the tessellated tile.
	FVector Base = SeaSpawnLocation;
	float BowYaw = SeaSpawnYaw;
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (APawn* P = PC->GetPawn()) { Base = P->GetActorLocation(); BowYaw = P->GetActorRotation().Yaw; }
	}
	const float Ranges[] = { EnemyRangeCm, EnemyRangeCm - 25000.f, EnemyRangeCm - 50000.f };
	const float Offsets[] = { EnemyBowOffsetDeg, -EnemyBowOffsetDeg, 0.f, 20.f, -20.f, 25.f, -25.f };
	FVector Loc = FVector::ZeroVector;
	float UsedOff = 0.f, UsedRange = 0.f;
	bool bFound = false;
	for (float R : Ranges)
	{
		for (float O : Offsets)
		{
			const FVector C = Base + FRotator(0.f, BowYaw + O, 0.f).Vector() * R;
			if (FMath::Abs(C.X) <= SpawnBoxHalfCm && FMath::Abs(C.Y) <= SpawnBoxHalfCm) { Loc = C; UsedOff = O; UsedRange = R; bFound = true; break; }
		}
		if (bFound) break;
	}
	if (!bFound)
	{
		// The player sailed to an edge: fall back to the bow line clamped into the tile.
		UsedOff = EnemyBowOffsetDeg; UsedRange = EnemyRangeCm;
		Loc = Base + FRotator(0.f, BowYaw + UsedOff, 0.f).Vector() * UsedRange;
		Loc.X = FMath::Clamp(Loc.X, -SpawnBoxHalfCm, SpawnBoxHalfCm);
		Loc.Y = FMath::Clamp(Loc.Y, -SpawnBoxHalfCm, SpawnBoxHalfCm);
		UE_LOG(LogIronwake, Warning, TEXT("IWGM no in-tile spawn ahead of the bow; clamped"));
	}
	Loc.Z = 150.f;
	// Heading: angled across the player's bow (shows its profile), then the AI turns to close.
	const float ToPlayer = (Base - Loc).Rotation().Yaw;
	const FTransform T(FRotator(0.f, ToPlayer + 35.f, 0.f), Loc);
	AIronwakeShipPawn* E = GetWorld()->SpawnActorDeferred<AIronwakeShipPawn>(Cls, T, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (E)
	{
		E->bIsNPC = true;
		E->MaxHP = 120.f;
		E->Tags.Add(TEXT("IronwakeEnemy"));
		E->FinishSpawning(T);
		const float Rel = FRotator::NormalizeAxis((Loc - Base).Rotation().Yaw - BowYaw);
		UE_LOG(LogIronwake, Display, TEXT("IWGM enemy ironclad spawned at %s: %.0f m from player, bearing %+.0f deg off the bow (player at %s hdg %.0f)"),
			*Loc.ToString(), FVector::Dist2D(Loc, Base) / 100.f, Rel, *Base.ToString(), BowYaw);
	}
}
