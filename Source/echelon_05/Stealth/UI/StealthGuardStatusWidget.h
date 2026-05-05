#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UObject/WeakObjectPtrTemplates.h"

#include "StealthGuardStatusWidget.generated.h"

class UStealthGuardBrainComponent;

/** Lightweight screen-space guard status plate for prototype stealth readability. */
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
