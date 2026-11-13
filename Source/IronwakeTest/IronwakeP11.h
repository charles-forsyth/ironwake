#pragma once
// P11: controls help panel (UMG, DPI-scaled so it fits 1280x720 .. 2560x1440).
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "IronwakeP11.generated.h"

UCLASS()
class IRONWAKETEST_API UIronwakeHelpWidget : public UUserWidget
{
	GENERATED_BODY()
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
};
