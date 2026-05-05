#pragma once

#include "CoreMinimal.h"
#include "Stealth/UI/StealthUITokens.h"

#include "Layout/Geometry.h"
#include "Layout/SlateRect.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

/**
 * Drawing primitives that compose the Echelon Stealth Instrument UI shape language.
 *
 * Source: Docs/ui/DESIGN.md (`components`, `Lines, Shapes, And Texture`,
 * `Component Contracts`).
 *
 * Every primitive is built from the simplest possible technical shape — a 1px box,
 * a thin outline, a row of segments, a column of waveform bars — to keep the
 * surface readable at 1080p with zero corner radius and no decorative fills.
 *
 * The primitives stay file-local-friendly (header inline) so we don't need an
 * additional translation unit for what amounts to drawing helpers.
 */
namespace StealthUI
{
	inline const FSlateBrush* WhiteBrush()
	{
		return FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	}

	/** Single rectangle. Foundation of every primitive below. */
	inline void FillRect(FSlateWindowElementList& Out, int32 Layer, const FGeometry& Geo,
		FVector2f Pos, FVector2f Size, const FLinearColor& Color)
	{
		FSlateDrawElement::MakeBox(Out, Layer,
			Geo.ToPaintGeometry(Size, FSlateLayoutTransform(Pos)),
			WhiteBrush(), ESlateDrawEffect::None, Color);
	}

	/**
	 * Crisp text. We avoid drop shadows on monochrome HUD because they contradict
	 * the "thin signal" pillar; readability comes from contrast against the field.
	 */
	inline void DrawText(FSlateWindowElementList& Out, int32& Layer, const FGeometry& Geo,
		const FString& Str, FVector2f Pos, const FLinearColor& Color, const FSlateFontInfo& Font)
	{
		FSlateDrawElement::MakeText(Out, Layer++,
			Geo.ToPaintGeometry(FVector2f(1.f, 1.f), FSlateLayoutTransform(Pos)),
			Str, Font, ESlateDrawEffect::None, Color);
	}

	/** 1 px outlined rectangle (no fill). */
	inline void Outline(FSlateWindowElementList& Out, int32& Layer, const FGeometry& Geo,
		float X, float Y, float W, float H, const FLinearColor& Color, float Stroke = 1.f)
	{
		FillRect(Out, Layer++, Geo, {X,         Y        }, {W,      Stroke}, Color);
		FillRect(Out, Layer++, Geo, {X,         Y + H - Stroke}, {W,      Stroke}, Color);
		FillRect(Out, Layer++, Geo, {X,         Y        }, {Stroke, H    }, Color);
		FillRect(Out, Layer++, Geo, {X + W - Stroke, Y    }, {Stroke, H    }, Color);
	}

	/**
	 * Four corner brackets — the ref-canonical "selected / framed" treatment.
	 * Used for detection banner (curious state), focused interactables,
	 * and world markers per DESIGN.md `bracketMarker`.
	 */
	inline void CornerBrackets(FSlateWindowElementList& Out, int32& Layer, const FGeometry& Geo,
		float X, float Y, float W, float H, float ArmLen, const FLinearColor& Color)
	{
		FillRect(Out, Layer++, Geo, {X,             Y            }, {ArmLen, 1.f   }, Color);
		FillRect(Out, Layer++, Geo, {X,             Y            }, {1.f,    ArmLen}, Color);
		FillRect(Out, Layer++, Geo, {X + W - ArmLen, Y           }, {ArmLen, 1.f   }, Color);
		FillRect(Out, Layer++, Geo, {X + W - 1.f,    Y           }, {1.f,    ArmLen}, Color);
		FillRect(Out, Layer++, Geo, {X,             Y + H - 1.f  }, {ArmLen, 1.f   }, Color);
		FillRect(Out, Layer++, Geo, {X,             Y + H - ArmLen}, {1.f,   ArmLen}, Color);
		FillRect(Out, Layer++, Geo, {X + W - ArmLen, Y + H - 1.f }, {ArmLen, 1.f   }, Color);
		FillRect(Out, Layer++, Geo, {X + W - 1.f,   Y + H - ArmLen}, {1.f,   ArmLen}, Color);
	}

	/**
	 * Discrete segmented meter — DESIGN.md `segmentedMeter`.
	 *
	 * Empty segments stay as low-opacity outlines, active segments brighten from
	 * muted gray toward white based on `Value01`. Danger reads as both fill count
	 * AND brightness: not just "more bars" but "brighter bars", which keeps the
	 * monochrome encoding legible peripherally.
	 *
	 * Optional `Peak01` paints a single 1 px tick that decays slower than the
	 * main value (the caller is responsible for the decay; this just renders).
	 */
	inline void SegmentedMeter(FSlateWindowElementList& Out, int32& Layer, const FGeometry& Geo,
		float X, float Y, float W, float H, float Value01,
		int32 SegmentCount = 16, float Peak01 = -1.f)
	{
		const float V          = FMath::Clamp(Value01, 0.f, 1.f);
		const float SegmentW   = (W - static_cast<float>(SegmentCount - 1)) / static_cast<float>(SegmentCount);
		const int32 ActiveSegs = FMath::CeilToInt(V * SegmentCount);

		// Brightness ramps from a baseline (~0.32) toward primary (~0.95) so the
		// rightmost lit segment is unmistakably bright at full value.
		const float Brightness = 0.32f + V * 0.63f;

		for (int32 i = 0; i < SegmentCount; ++i)
		{
			const float Sx = X + i * (SegmentW + 1.f);
			if (i < ActiveSegs)
			{
				FillRect(Out, Layer++, Geo, {Sx, Y}, {SegmentW, H},
					FLinearColor(Brightness, Brightness, Brightness, 1.f));
			}
			else
			{
				// Idle segment: thin outline at low opacity — present but quiet.
				Outline(Out, Layer, Geo, Sx, Y, SegmentW, H, HairlineIdle());
			}
		}

		// Peak high-water tick (1 px) — bright enough to spot, narrow enough to
		// read as a discrete event marker rather than as part of the fill.
		if (Peak01 > V + 1.f / static_cast<float>(SegmentCount) && Peak01 > 0.f)
		{
			const float Px = X + (W - 1.f) * FMath::Clamp(Peak01, 0.f, 1.f);
			FillRect(Out, Layer++, Geo, {Px, Y - 1.f}, {1.f, H + 2.f},
				LinePrimary().CopyWithNewOpacity(0.85f));
		}
	}

	/**
	 * Waveform strip — DESIGN.md `waveform`.
	 *
	 * Renders a left-to-right history of values [0..1] as thin vertical bars
	 * around a center axis. Index 0 of `History` is the OLDEST sample, the last
	 * entry is the most recent. Quiet samples render as faint center pixels;
	 * transient spikes paint full bars.
	 */
	inline void WaveformStrip(FSlateWindowElementList& Out, int32& Layer, const FGeometry& Geo,
		float X, float Y, float W, float H, const TArray<float>& History,
		const FLinearColor& Color = FLinearColor::White)
	{
		if (History.Num() == 0) { return; }

		// Center axis line (always visible at very low opacity to anchor the strip).
		FillRect(Out, Layer++, Geo, {X, Y + H * 0.5f - 0.5f}, {W, 1.f},
			Color.CopyWithNewOpacity(OpacityGridIdle));

		const float BarStride = W / static_cast<float>(History.Num());
		const float BarWidth  = FMath::Max(1.f, BarStride - 1.f);

		for (int32 i = 0; i < History.Num(); ++i)
		{
			const float V = FMath::Clamp(History[i], 0.f, 1.f);
			if (V <= 0.001f) { continue; }

			// Older samples fade out — sustained activity reads as a moving comet
			// rather than a static row, matching reference audio waveform cards.
			const float AgeAlpha = 0.35f + 0.65f * (static_cast<float>(i) / FMath::Max(1.f, History.Num() - 1.f));
			const float BarHalf = FMath::Max(1.f, V * H * 0.5f);

			FillRect(Out, Layer++, Geo,
				{X + i * BarStride, Y + H * 0.5f - BarHalf},
				{BarWidth, BarHalf * 2.f},
				Color.CopyWithNewOpacity(AgeAlpha * (0.4f + 0.6f * V)));
		}
	}

	/** Small filled / outlined indicator square used in the mission block. */
	inline void IndicatorSquare(FSlateWindowElementList& Out, int32& Layer, const FGeometry& Geo,
		float X, float Y, float Sz, bool bFilled, float Brightness = 0.95f)
	{
		if (bFilled)
		{
			FillRect(Out, Layer++, Geo, {X, Y}, {Sz, Sz},
				FLinearColor(Brightness, Brightness, Brightness, 1.f));
		}
		else
		{
			Outline(Out, Layer, Geo, X, Y, Sz, Sz,
				LinePrimary().CopyWithNewOpacity(OpacityHairlineActive));
		}
	}

	/**
	 * A radial scanning arc — used by the "investigating" detection state.
	 * Approximates an arc with short radial segments. Cheap, no triangulation,
	 * still reads as a sweeping radar tick at small sizes.
	 */
	inline void ScanArc(FSlateWindowElementList& Out, int32& Layer, const FGeometry& Geo,
		float Cx, float Cy, float Radius, float StartRad, float SweepRad,
		const FLinearColor& Color, int32 Segments = 12)
	{
		for (int32 i = 0; i < Segments; ++i)
		{
			const float A = StartRad + SweepRad * (static_cast<float>(i) / static_cast<float>(Segments));
			const float Px = Cx + FMath::Cos(A) * Radius;
			const float Py = Cy + FMath::Sin(A) * Radius;
			const float Alpha = static_cast<float>(i + 1) / static_cast<float>(Segments);
			FillRect(Out, Layer++, Geo, {Px - 1.f, Py - 1.f}, {2.f, 2.f},
				Color.CopyWithNewOpacity(Alpha));
		}
	}

	/** Thin horizontal hairline divider at idle opacity. */
	inline void HRule(FSlateWindowElementList& Out, int32& Layer, const FGeometry& Geo,
		float X, float Y, float W)
	{
		FillRect(Out, Layer++, Geo, {X, Y}, {W, 1.f}, HairlineIdle());
	}
}
