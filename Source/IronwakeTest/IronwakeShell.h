#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IronwakeShell.generated.h"

class UStaticMeshComponent;
class AIronwakeShipPawn;

/**
 * Ballistic shell: integrates gravity itself (no physics engine), sub-stepped so a 750 m/s shell
 * can't tunnel through a 20 m wide hull. Hits = point inside a ship's hull box; misses = below the
 * live Gerstner water surface (queried from the ocean water body).
 */
UCLASS()
class IRONWAKETEST_API AIronwakeShell : public AActor
{
	GENERATED_BODY()

public:
	AIronwakeShell();
	virtual void Tick(float DeltaSeconds) override;

	void Launch(const FVector& InVelocity, AIronwakeShipPawn* InShooter, float InDamage);

	/** Water surface height (cm) at a world XY, including waves. Falls back to -350 if no ocean. */
	/** R1 believed the render sat 4.7 m under the plugin query; the gauge reader was counting post bands seen THROUGH the
	 *  translucent water. Re-measured (bright bands only, 4 sites): render == query. Every hull rode 4.7 m too deep
	 *  (Chuck's 'two water layers': the submerged hull seen through clear water). Offset is 0. */
	static constexpr float RenderedSeaOffsetCm = 0.f;
	static float WaterZAtWaves(UWorld* World, const FVector& P);
	static float WaterZAt(UWorld* World, const FVector& P);

	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Ball;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Streak;

	FVector Velocity = FVector::ZeroVector;
	TWeakObjectPtr<AIronwakeShipPawn> Shooter;
	float Damage = 12.f;
	float Age = 0.f;
	static constexpr float Gravity = 980.f;
};
