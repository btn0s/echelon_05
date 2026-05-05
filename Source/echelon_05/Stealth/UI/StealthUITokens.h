#pragma once

#include "CoreMinimal.h"
#include "Styling/CoreStyle.h"
#include "Fonts/SlateFontInfo.h"
#include "Math/Color.h"

/**
 * Design tokens for the Echelon Stealth Instrument UI.
 *
 * Source: Docs/ui/DESIGN.md (front-matter token block).
 *
 * The HUD direction is intentionally low-color: contrast, brightness, line weight,
 * and segmentation carry meaning. Hue is reserved for diegetic terminal screens
 * (signal green) and is NOT used for player-state encoding on the always-on HUD.
 *
 * All helpers here are inline so both UStealthHUDWidget and UStealthGuardStatusWidget
 * compile against the same numbers without a translation-unit boundary.
 */
namespace StealthUI
{
	// ── Color ────────────────────────────────────────────────────────────────
	//
	// Token names mirror DESIGN.md's `tokens.color.*` block.
	// Naming distinction: `Line*` = stroke / divider / outline, `Text*` = glyphs.
	// Brightness is the only state encoding on the gameplay HUD.

	inline FLinearColor Field()         { return FLinearColor(0.000f, 0.000f, 0.000f, 1.0f); }   // #000000
	inline FLinearColor Panel()         { return FLinearColor(0.020f, 0.020f, 0.020f, 1.0f); }   // #050505
	inline FLinearColor PanelMuted()    { return FLinearColor(0.039f, 0.039f, 0.039f, 1.0f); }   // #0A0A0A

	inline FLinearColor LinePrimary()   { return FLinearColor(0.949f, 0.949f, 0.949f, 1.0f); }   // #F2F2F2
	inline FLinearColor LineSecondary() { return FLinearColor(0.659f, 0.659f, 0.659f, 1.0f); }   // #A8A8A8
	inline FLinearColor LineMuted()     { return FLinearColor(0.314f, 0.314f, 0.314f, 1.0f); }   // #505050

	inline FLinearColor TextPrimary()   { return FLinearColor(0.957f, 0.957f, 0.957f, 1.0f); }   // #F4F4F4
	inline FLinearColor TextSecondary() { return FLinearColor(0.722f, 0.722f, 0.722f, 1.0f); }   // #B8B8B8
	inline FLinearColor TextMuted()     { return FLinearColor(0.416f, 0.416f, 0.416f, 1.0f); }   // #6A6A6A

	inline FLinearColor InverseFill()   { return FLinearColor(0.957f, 0.957f, 0.957f, 1.0f); }   // #F4F4F4
	inline FLinearColor InverseText()   { return FLinearColor(0.000f, 0.000f, 0.000f, 1.0f); }   // #000000

	inline FLinearColor SignalGreen()   { return FLinearColor(0.439f, 0.851f, 0.471f, 1.0f); }   // #70D978

	// ── Opacity ──────────────────────────────────────────────────────────────
	constexpr float OpacityHairlineIdle   = 0.28f;
	constexpr float OpacityHairlineActive = 0.72f;
	constexpr float OpacityPanelBacking   = 0.72f;
	constexpr float OpacityGridIdle       = 0.12f;
	constexpr float OpacityDisabled       = 0.22f;

	/** Hairline at idle opacity (the default for static frames / dividers). */
	inline FLinearColor HairlineIdle()   { return LinePrimary().CopyWithNewOpacity(OpacityHairlineIdle); }

	/** Hairline at active opacity (selected frame / pressure state). */
	inline FLinearColor HairlineActive() { return LinePrimary().CopyWithNewOpacity(OpacityHairlineActive); }

	/** Near-black panel backing for cases where level brightness harms readability. */
	inline FLinearColor PanelBacking()   { return Field().CopyWithNewOpacity(OpacityPanelBacking); }

	// ── Typography ───────────────────────────────────────────────────────────
	//
	// All HUD type is monospace, all-caps, with generous tracking. Slate's bundled
	// "Mono" face matches the reference aesthetic; "Bold" weight is reserved for
	// large readouts so they read as primary instrumentation.

	// Sizes are nudged ~20% above the DESIGN.md token defaults (9/11/13/18/24)
	// so the always-on HUD reads cleanly on a 1080p PIE viewport without losing
	// the "small instrumentation" feel — see DESIGN.md `## Scale And Spacing`.
	inline FSlateFontInfo FMicro()   { return FCoreStyle::GetDefaultFontStyle("Mono", 11); }
	inline FSlateFontInfo FLabel()   { return FCoreStyle::GetDefaultFontStyle("Mono", 13); }
	inline FSlateFontInfo FBody()    { return FCoreStyle::GetDefaultFontStyle("Mono", 16); }
	inline FSlateFontInfo FBanner()  { return FCoreStyle::GetDefaultFontStyle("Mono", 22); }
	inline FSlateFontInfo FReadout() { return FCoreStyle::GetDefaultFontStyle("Mono", 30); }

	// ── Spacing (all px, scale * 4) ──────────────────────────────────────────
	constexpr float SpacingXS = 4.f;
	constexpr float SpacingSM = 8.f;
	constexpr float SpacingMD = 12.f;
	constexpr float SpacingLG = 16.f;
	constexpr float SpacingXL = 24.f;
	constexpr float ScreenMargin = 32.f;

	// ── Stroke widths ────────────────────────────────────────────────────────
	constexpr float StrokeHairline = 1.f;
	constexpr float StrokeActive   = 1.f;
	constexpr float StrokeAlert    = 2.f;

	// ── Motion (ms) ──────────────────────────────────────────────────────────
	constexpr float BlinkSlowMs   = 1400.f;
	constexpr float FlashMs       =  180.f;
	constexpr float MeterStepMs   =   80.f;
	constexpr float AlertPulseMs  =  420.f;

	/** Periodic blink in [0,1) using world time T (seconds) and period in ms. */
	inline float Blink(float T, float PeriodMs)
	{
		return FMath::Frac(T * 1000.f / FMath::Max(PeriodMs, 1.f));
	}
}
