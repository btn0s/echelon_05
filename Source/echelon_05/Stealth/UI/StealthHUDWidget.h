#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StealthHUDWidget.generated.h"

/**
 * Player-facing stealth HUD overlay.
 *
 * Renders entirely through NativePaint so type goes through the Slate font pipeline
 * (proper anti-aliased monospace) instead of the AHUD canvas DrawText path.
 *
 * Visual direction is sourced from `Docs/ui/DESIGN.md` (Echelon Stealth Instrument UI):
 * monochrome line work over a black field, segmented meters, waveform strips for noise,
 * bracket markers, and an inverted alert block. Hue is intentionally absent from the
 * gameplay layer.
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
	// ── Render-only state ────────────────────────────────────────────────────
	//
	// `mutable` is appropriate: NativePaint is const but these track presentation
	// (decaying peak markers, scrolling waveform history, mission-block flash
	// timers). They are never inputs to gameplay simulation.

	/** Last sample push time (seconds, world time). Used to advance the waveform buffer. */
	mutable float LastSampleTime = -1.f;

	/** Decaying peak markers for the segmented meters. */
	mutable float PeakVis01   = 0.f;
	mutable float PeakNoise01 = 0.f;

	/** Rolling history of normalized noise [0..1] — drives the NSE waveform strip. */
	mutable TArray<float> NoiseHistory;

	/** Last seen objective / extraction states — used to fire one-shot completion flashes. */
	mutable bool  bLastObjCompleted = false;
	mutable float ObjFlashAtTime    = -100.f;
	mutable bool  bLastExtAvailable = false;
	mutable float ExtFlashAtTime    = -100.f;
};
