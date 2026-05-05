#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UObject/WeakObjectPtrTemplates.h"

#include "StealthGuardStatusWidget.generated.h"

class UStealthGuardBrainComponent;

/**
 * World-space guard suspicion plate.
 *
 * Visual contract: Docs/ui/DESIGN.md `### Guard Suspicion Plate`.
 *
 *   Unaware       hidden plate
 *   Curious       1 tick + dim corner brackets
 *   Suspicious    2 ticks
 *   Investigating 3-4 ticks (escalates with the suspicion value)
 *   Alert         inverted white block, black `ALERT` label
 *
 * Monochrome only — no traffic-light hue. Brightness, tick count, and the
 * single inverted-block treatment encode threat.
 */
UCLASS()
class UStealthGuardStatusWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Stealth")
	void SetGuardBrain(UStealthGuardBrainComponent* InGuardBrain);

protected:
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

private:
	TWeakObjectPtr<UStealthGuardBrainComponent> GuardBrain;
};
