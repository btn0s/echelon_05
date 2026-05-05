#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"

#include "StealthDebugHUD.generated.h"

class UStealthHUDWidget;

/**
 * Minimal HUD class.  All rendering is delegated to UStealthHUDWidget
 * so the drawing pipeline goes through Slate (proper font rendering)
 * rather than the AHUD canvas DrawText path.
 */
UCLASS()
class AStealthDebugHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TObjectPtr<UStealthHUDWidget> HUDWidget;
};
