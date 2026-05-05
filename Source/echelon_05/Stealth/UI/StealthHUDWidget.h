#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StealthHUDWidget.generated.h"

/**
 * Full-screen Slate-rendered HUD overlay for the stealth prototype.
 *
 * Runs entirely through NativePaint so text uses the Slate font pipeline
 * (proper anti-aliased rendering at real sizes) rather than the AHUD canvas
 * DrawText path which produces low-quality bitmap output regardless of layout.
 */
UCLASS()
class UStealthHUDWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	// Session high-water marks for peak ticks on VIS/NSE bars.
	// mutable: NativePaint is const but these are rendering state, not semantic state.
	mutable float PeakVis01   = 0.f;
	mutable float PeakNoise01 = 0.f;
};
