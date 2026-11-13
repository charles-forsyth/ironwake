#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "IronwakeGameMode.generated.h"

/** Ironwake_Sea game mode: spawns the player ship in open water (no PlayerStart needed). */
UCLASS(Blueprintable)
class IRONWAKETEST_API AIronwakeGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	AIronwakeGameMode();
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;
	virtual void StartPlay() override;

	/** Spawn the NPC enemy ironclad now (no-op if one exists). Called at StartPlay or by the title menu. */
	void SpawnEnemy();

	/** Card D: show the IRONWAKE title menu at start; the enemy spawns when Start is pressed (Captain mode). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironwake")
	bool bShowTitleMenu = true;

	/** Card C: one NPC enemy ironclad ~4 km from the player, inside the 8 km ocean tile. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironwake")
	bool bSpawnEnemy = true;

	/** P1: the enemy appears this far out (cm), ahead of the player's bow. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironwake")
	float EnemyRangeCm = 750000.f;

	/** P1: bearing of the enemy relative to the player's bow (deg, + = starboard). Must stay within ~30. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironwake")
	float EnemyBowOffsetDeg = -12.f; // port bow: keeps the lighthouse island (~2.5 km off this line) out of the sight line

	/** Half-size of the tessellated ocean tile kept clear for spawns (cm). The tile is +/-4000 m. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironwake")
	float SpawnBoxHalfCm = 370000.f;

	/** P1: open-water spawn (cm) in the SW corner of the 8 km tile, so a 7.5 km sight line ahead stays on it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironwake")
	FVector SeaSpawnLocation = FVector(-300000.f, -270000.f, 150.f);

	/** Bow heading at spawn (yaw, deg): NE across the tile; the lighthouse island lies ~1.4 km to port of the sight line. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironwake")
	float SeaSpawnYaw = 45.f;

	/** True when this level was opened by RESTART (skip the title, same mode). */
	bool bRestarted = false;
};
