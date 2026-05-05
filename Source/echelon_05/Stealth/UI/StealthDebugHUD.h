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
};
