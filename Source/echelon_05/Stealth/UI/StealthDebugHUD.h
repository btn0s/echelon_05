#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"

#include "StealthDebugHUD.generated.h"

UCLASS()
class AStealthDebugHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	// Session high-water marks for VIS / NSE peak ticks on bars.
	float PeakVis01   = 0.f;
	float PeakNoise01 = 0.f;
};
