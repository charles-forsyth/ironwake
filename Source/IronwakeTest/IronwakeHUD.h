#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "IronwakeHUD.generated.h"

/** Small brass-style text block: telegraph, rudder, speed (knots), heading. Canvas only, no widgets. */
UCLASS()
class IRONWAKETEST_API AIronwakeHUD : public AHUD
{
	GENERATED_BODY()
public:
	virtual void DrawHUD() override;
};
