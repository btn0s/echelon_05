#include "Stealth/UI/StealthGuardStatusWidget.h"

#include "Stealth/Components/StealthGuardBrainComponent.h"

#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Engine/World.h"

namespace
{
// ── Palette (mirrors StealthDebugHUD exactly) ─────────────────────────────────
FLinearColor GBg()     { return FLinearColor(0.010f, 0.010f, 0.010f, 0.92f); }
FLinearColor GHdrBg()  { return FLinearColor(0.055f, 0.055f, 0.055f, 0.95f); }
FLinearColor GEdge()   { return FLinearColor(1.f,    1.f,    1.f,    0.20f); }
FLinearColor GDiv()    { return FLinearColor(1.f,    1.f,    1.f,    0.09f); }
FLinearColor GWhite()  { return FLinearColor(0.94f,  0.94f,  0.94f,  1.f);  }
FLinearColor GGray()   { return FLinearColor(0.50f,  0.50f,  0.50f,  1.f);  }
FLinearColor GDim()    { return FLinearColor(0.26f,  0.26f,  0.26f,  1.f);  }
FLinearColor GGreen()  { return FLinearColor(0.08f,  0.94f,  0.38f,  1.f);  }
FLinearColor GAmber()  { return FLinearColor(0.98f,  0.64f,  0.00f,  1.f);  }
FLinearColor GRed()    { return FLinearColor(0.97f,  0.16f,  0.06f,  1.f);  }

FLinearColor StateAccent(const EGuardSuspicionState S)
{
	switch (S)
	{
	case EGuardSuspicionState::Alert:          return GRed();
	case EGuardSuspicionState::Investigating:  return GAmber();
	case EGuardSuspicionState::Suspicious:     return GAmber();
	case EGuardSuspicionState::Curious:        return GGreen();
	default:                                    return GDim();
	}
}

template <typename TEnum>
FString EStr(const TEnum V)
{
	if (const UEnum* E = StaticEnum<TEnum>())
	{
		return E->GetDisplayNameTextByValue(static_cast<int64>(V)).ToString().ToUpper();
	}
	return FString::FromInt(static_cast<int32>(V));
}

const FSlateBrush* WBrush()
{
	return FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
}

void GBox(FSlateWindowElementList& Out, int32 Layer, const FGeometry& Geo,
	const FVector2f Pos, const FVector2f Sz, const FLinearColor& C)
{
	FSlateDrawElement::MakeBox(Out, Layer,
		Geo.ToPaintGeometry(Sz, FSlateLayoutTransform(Pos)),
		WBrush(), ESlateDrawEffect::None, C);
}

void GText(FSlateWindowElementList& Out, int32& Layer, const FGeometry& Geo,
	const FString& Str, const FVector2f Pos, const FLinearColor& C, int32 Sz = 10)
{
	const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), Sz);
	FSlateDrawElement::MakeText(Out, Layer++,
		Geo.ToPaintGeometry(FVector2f(1.f, 1.f), FSlateLayoutTransform(Pos + FVector2f(1.f, 1.f))),
		Str, Font, ESlateDrawEffect::None, FLinearColor(0.f, 0.f, 0.f, 0.65f));
	FSlateDrawElement::MakeText(Out, Layer++,
		Geo.ToPaintGeometry(FVector2f(1.f, 1.f), FSlateLayoutTransform(Pos)),
		Str, Font, ESlateDrawEffect::None, C);
}

// L-shaped corner brackets with parameterised edge colour for animation.
void GBrackets(FSlateWindowElementList& Out, int32& Layer, const FGeometry& Geo,
	float W, float H, const FLinearColor& EdgeColor, float BLen = 12.f)
{
	auto B = [&](FVector2f P, FVector2f S) { GBox(Out, Layer++, Geo, P, S, EdgeColor); };
	B({0.f,    0.f},   {BLen, 1.f});  B({0.f,    0.f},   {1.f, BLen});
	B({W-BLen, 0.f},   {BLen, 1.f});  B({W-1.f,  0.f},   {1.f, BLen});
	B({0.f,    H-1.f}, {BLen, 1.f});  B({0.f,    H-BLen},{1.f, BLen});
	B({W-BLen, H-1.f}, {BLen, 1.f});  B({W-1.f,  H-BLen},{1.f, BLen});
}

// Segmented bar (mirrors main HUD's TSegBar — same threshold zones, same colours).
void GSegBar(FSlateWindowElementList& Out, int32& Layer, const FGeometry& Geo,
	float X, float Y, float W, float H, float N01)
{
	static constexpr float Thresh[4] = { 0.20f, 0.45f, 0.70f, 1.00f };
	const FLinearColor ZoneColor[4]  = { GDim(), GGreen(), GAmber(), GRed() };
	constexpr float Gap  = 2.f;
	constexpr int   NSeg = 4;
	const float ContentW = W - Gap * (NSeg - 1);

	float CurX = X;
	float PrevT = 0.f;

	for (int32 i = 0; i < NSeg; ++i)
	{
		const float SegRange = Thresh[i] - PrevT;
		const float SegW     = ContentW * SegRange;
		const FLinearColor& SC = ZoneColor[i];

		// Dark track
		GBox(Out, Layer++, Geo, FVector2f(CurX, Y), FVector2f(SegW, H),
			FLinearColor(SC.R * 0.10f, SC.G * 0.10f, SC.B * 0.10f, 0.85f));
		// Fill
		const float Frac = FMath::Clamp((N01 - PrevT) / SegRange, 0.f, 1.f);
		if (Frac > 0.f)
		{
			GBox(Out, Layer++, Geo, FVector2f(CurX, Y), FVector2f(SegW * Frac, H),
				FLinearColor(SC.R, SC.G, SC.B, 0.92f));
		}
		// Hairlines top + bottom
		GBox(Out, Layer++, Geo, FVector2f(CurX, Y),         FVector2f(SegW, 1.f), GEdge());
		GBox(Out, Layer++, Geo, FVector2f(CurX, Y+H-1.f),   FVector2f(SegW, 1.f), GEdge());

		CurX += SegW + Gap;
		PrevT = Thresh[i];
	}
}
}

void UStealthGuardStatusWidget::SetGuardBrain(UStealthGuardBrainComponent* InGuardBrain)
{
	GuardBrain = InGuardBrain;
}

int32 UStealthGuardStatusWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 Base = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements,
		LayerId, InWidgetStyle, bParentEnabled);
	int32 L = Base + 1;

	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
	const float W = static_cast<float>(LocalSize.X);
	const float H = static_cast<float>(LocalSize.Y);

	const float WorldT = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

	const UStealthGuardBrainComponent* Brain     = GuardBrain.Get();
	const FSuspicionState              Suspicion  = Brain ? Brain->Suspicion : FSuspicionState();
	const FGuardBrain                  BrainState = Brain ? Brain->Brain    : FGuardBrain();
	const float                        Sus01      = FMath::Clamp(Suspicion.Value / 100.f, 0.f, 1.f);
	const FLinearColor                 Accent     = StateAccent(Suspicion.State);

	// ── Background ────────────────────────────────────────────────────────────
	GBox(OutDrawElements, L++, AllottedGeometry, FVector2f(0.f, 0.f), FVector2f(W, H), GBg());

	if (!Brain)
	{
		GText(OutDrawElements, L, AllottedGeometry, TEXT("--"), FVector2f(10.f, 8.f), GDim(), 10);
		GBrackets(OutDrawElements, L, AllottedGeometry, W, H, GEdge());
		return L;
	}

	// ── Layout constants ──────────────────────────────────────────────────────
	//   Header row  (0 → HdrH)        : raised bg, dot + state name
	//   Sep         (HdrH)            : 1px TDiv
	//   Bar row     (HdrH+1 → BarEnd) : segmented suspicion bar
	//   Sep         (BarEnd)
	//   Footer row  (BarEnd+1 → H)    : mode left, % right
	constexpr float HdrH    = 26.f;
	constexpr float BarRowH = 20.f;

	// ── Header ────────────────────────────────────────────────────────────────
	GBox(OutDrawElements, L++, AllottedGeometry,
		FVector2f(1.f, 1.f), FVector2f(W - 2.f, HdrH - 1.f), GHdrBg());

	// 9×9 status dot (shadow + fill)
	GBox(OutDrawElements, L++, AllottedGeometry, FVector2f(11.f, 10.f), FVector2f(9.f, 9.f),
		FLinearColor(0.f, 0.f, 0.f, 0.70f));
	// At Alert, the dot pulses
	const FLinearColor DotColor = (Suspicion.State == EGuardSuspicionState::Alert)
		? FLinearColor(Accent.R, Accent.G, Accent.B, FMath::Sin(WorldT * 4.5f) * 0.35f + 0.65f)
		: Accent;
	GBox(OutDrawElements, L++, AllottedGeometry, FVector2f(10.f, 9.f), FVector2f(9.f, 9.f), DotColor);

	// State name — at Alert use slightly larger text
	const int32 StateTextSz = (Suspicion.State == EGuardSuspicionState::Alert) ? 13 : 11;
	// At Alert, text colour pulses between accent and white
	const FLinearColor StateTextColor = (Suspicion.State == EGuardSuspicionState::Alert)
		? FMath::Lerp(GWhite(), Accent, FMath::Sin(WorldT * 4.5f) * 0.5f + 0.5f)
		: GWhite();
	GText(OutDrawElements, L, AllottedGeometry, EStr(Suspicion.State),
		FVector2f(24.f, 6.f), StateTextColor, StateTextSz);

	// Separator below header
	GBox(OutDrawElements, L++, AllottedGeometry,
		FVector2f(1.f, HdrH), FVector2f(W - 2.f, 1.f), GDiv());

	// ── Bar row: segmented suspicion bar ─────────────────────────────────────
	const float BarY   = HdrH + 1.f;
	const float BarX   = 10.f;
	const float BarW   = W - 20.f;
	const float BarHt  = 8.f;
	const float BarCY  = BarY + (BarRowH - BarHt) * 0.5f;

	GSegBar(OutDrawElements, L, AllottedGeometry, BarX, BarCY, BarW, BarHt, Sus01);

	// Separator below bar row
	GBox(OutDrawElements, L++, AllottedGeometry,
		FVector2f(1.f, BarY + BarRowH), FVector2f(W - 2.f, 1.f), GDiv());

	// ── Footer row: mode + percentage ────────────────────────────────────────
	const float FtrY = BarY + BarRowH + 1.f;

	GText(OutDrawElements, L, AllottedGeometry, EStr(BrainState.Mode),
		FVector2f(10.f, FtrY + 4.f), GGray(), 9);

	// Suspicion percentage — right-aligned
	if (Sus01 > 0.005f)
	{
		const FString PctStr = FString::Printf(TEXT("%2.0f%%"), Sus01 * 100.f);
		const float PctX = W - static_cast<float>(PctStr.Len()) * 7.5f - 10.f;
		GText(OutDrawElements, L, AllottedGeometry, PctStr,
			FVector2f(FMath::Max(PctX, W * 0.6f), FtrY + 4.f), Accent, 9);
	}

	// ── Stimulus tag (centre footer, only when active and worth showing) ──────
	FString Stim;
	if (Suspicion.bHadVisualStimulus)        { Stim = TEXT("VIS"); }
	else if (Suspicion.bHadAuditoryStimulus) { Stim = TEXT("SND"); }
	if (!Stim.IsEmpty())
	{
		GText(OutDrawElements, L, AllottedGeometry, Stim,
			FVector2f(W * 0.5f - 8.f, FtrY + 4.f), GGray(), 9);
	}

	// ── Corner brackets — animate at elevated states ──────────────────────────
	const FLinearColor BracketC = [&]() -> FLinearColor {
		if (Suspicion.State == EGuardSuspicionState::Alert)
		{
			const float A = FMath::Sin(WorldT * 4.5f) * 0.35f + 0.50f;
			return FLinearColor(GRed().R, GRed().G, GRed().B, A);
		}
		if (Suspicion.State >= EGuardSuspicionState::Suspicious)
		{
			return FLinearColor(Accent.R, Accent.G, Accent.B, 0.55f);
		}
		return GEdge();
	}();
	GBrackets(OutDrawElements, L, AllottedGeometry, W, H, BracketC, 12.f);

	return L;
}
