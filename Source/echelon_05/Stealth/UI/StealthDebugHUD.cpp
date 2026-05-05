#include "Stealth/UI/StealthDebugHUD.h"
#include "Stealth/UI/StealthHUDWidget.h"

void AStealthDebugHUD::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* PC = GetOwningPlayerController())
	{
		HUDWidget = CreateWidget<UStealthHUDWidget>(PC, UStealthHUDWidget::StaticClass());
		if (HUDWidget)
		{
			// ZOrder 0: renders below world-space widgets (guard plates sit at their own depth)
			HUDWidget->AddToViewport(0);
		}
	}
}
