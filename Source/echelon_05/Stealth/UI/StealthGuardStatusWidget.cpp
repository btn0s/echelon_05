#include "Stealth/UI/StealthGuardStatusWidget.h"

#include "Stealth/Components/StealthGuardBrainComponent.h"

#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

namespace
{
// ── Palette (identical to StealthDebugHUD) ────────────────────────────────────
FLinearColor GBg()    { return FLinearColor(0.010f, 0.010f, 0.010f, 0.92f); }
FLinearColor GEdge()  { return FLinearColor(1.f,    1.f,    1.f,    0.20f); }
FLinearColor GDiv()   { return FLinearColor(1.f,    1.f,    1.f,    0.09f); }
FLinearColor GWhite() { return FLinearColor(0.94f,  0.94f,  0.94f,  1.f);  }
FLinearColor GGray()  { return FLinearColor(0.50f,  0.50f,  0.50f,  1.f);  }
FLinearColor GDim()   { return FLinearColor(0.26f,  0.26f,  0.26f,  1.f);  }
FLinearColor GGreen() { return FLinearColor(0.08f,  0.94f,  0.38f,  1.f);  }
FLinearColor GAmber() { return FLinearColor(0.98f,  0.64f,  0.00f,  1.f);  }
FLinearColor GRed()   { return FLinearColor(0.97f,  0.16f,  0.06f,  1.f);  }

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

FLinearColor BarRamp(const float N)
{
	if (N >= 0.78f) return GRed();
	if (N >= 0.46f) return GAmber();
	return GGreen();
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

void GBox(FSlateWindowElementList& Out, const int32 Layer, const FGeometry& Geo,
	const FVector2f Pos, const FVector2f Sz, const FLinearColor& C)
{
	FSlateDrawElement::MakeBox(Out, Layer,
		Geo.ToPaintGeometry(Sz, FSlateLayoutTransform(Pos)),
		WBrush(), ESlateDrawEffect::None, C);
}

void GText(FSlateWindowElementList& Out, int32& Layer, const FGeometry& Geo,
	const FString& Str, const FVector2f Pos, const FLinearColor& C, const int32 Sz = 10)
{
	const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), Sz);
	FSlateDrawElement::MakeText(Out, Layer++,
		Geo.ToPaintGeometry(FVector2f(1.f, 1.f), FSlateLayoutTransform(Pos + FVector2f(1.f, 1.f))),
		Str, Font, ESlateDrawEffect::None, FLinearColor(0.f, 0.f, 0.f, 0.65f));
	FSlateDrawElement::MakeText(Out, Layer++,
		Geo.ToPaintGeometry(FVector2f(1.f, 1.f), FSlateLayoutTransform(Pos)),
		Str, Font, ESlateDrawEffect::None, C);
}

// L-shaped corner brackets for the outer boundary.
void GBrackets(FSlateWindowElementList& Out, int32& Layer, const FGeometry& Geo,
	const float W, const float H, const float BLen = 14.f)
{
	const FLinearColor E = GEdge();
	auto B = [&](const FVector2f P, const FVector2f S) { GBox(Out, Layer++, Geo, P, S, E); };
	B({0.f,      0.f},      {BLen, 1.f});   // TL horiz
	B({0.f,      0.f},      {1.f, BLen});   // TL vert
	B({W-BLen,   0.f},      {BLen, 1.f});   // TR horiz
	B({W-1.f,    0.f},      {1.f, BLen});   // TR vert
	B({0.f,      H-1.f},    {BLen, 1.f});   // BL horiz
	B({0.f,      H-BLen},   {1.f, BLen});   // BL vert
	B({W-BLen,   H-1.f},    {BLen, 1.f});   // BR horiz
	B({W-1.f,    H-BLen},   {1.f, BLen});   // BR vert
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
		GBrackets(OutDrawElements, L, AllottedGeometry, W, H);
		return L;
	}

	// Layout constants:
	//   Row 1 (header): 0 → HdrH  — state name + dot, accent color, larger text
	//   Sep at HdrH
	//   Row 2 (bar):   HdrH+1 → HdrH+BarRowH  — full-width suspicion bar
	//   Sep at HdrH+BarRowH+1
	//   Row 3 (footer): HdrH+BarRowH+2 → H  — mode (left) + stimulus (right)
	constexpr float HdrH    = 26.f;
	constexpr float BarRowH = 18.f;

	// ── Row 1: header — dot + state name ─────────────────────────────────────
	// Slightly raised bg for header (mirrors THeaderRow in AHUD)
	GBox(OutDrawElements, L++, AllottedGeometry,
		FVector2f(1.f, 1.f), FVector2f(W - 2.f, HdrH - 1.f),
		FLinearColor(0.06f, 0.06f, 0.06f, 0.95f));

	// 9×9 status dot
	GBox(OutDrawElements, L++, AllottedGeometry, FVector2f(11.f, 10.f), FVector2f(9.f, 9.f),
		FLinearColor(0.f, 0.f, 0.f, 0.70f));   // shadow
	GBox(OutDrawElements, L++, AllottedGeometry, FVector2f(10.f, 9.f),  FVector2f(9.f, 9.f), Accent);

	// State name — primary element, bigger font
	GText(OutDrawElements, L, AllottedGeometry, EStr(Suspicion.State),
		FVector2f(24.f, 7.f), GWhite(), 12);

	// Separator below header
	GBox(OutDrawElements, L++, AllottedGeometry,
		FVector2f(1.f, HdrH), FVector2f(W - 2.f, 1.f), GDiv());

	// ── Row 2: suspicion bar ──────────────────────────────────────────────────
	const float BarY  = HdrH + 1.f;
	const float BarX  = 10.f;
	const float BarW  = W - 20.f;
	const float BarHt = 8.f;
	const float BarCY = BarY + (BarRowH - BarHt) * 0.5f;   // vertically center in row

	// Track
	GBox(OutDrawElements, L++, AllottedGeometry,
		FVector2f(BarX, BarCY), FVector2f(BarW, BarHt),
		FLinearColor(0.f, 0.f, 0.f, 1.f));
	// Fill
	if (Sus01 > 0.f)
	{
		const FLinearColor FC = BarRamp(Sus01);
		GBox(OutDrawElements, L++, AllottedGeometry,
			FVector2f(BarX, BarCY), FVector2f(BarW * Sus01, BarHt),
			FLinearColor(FC.R, FC.G, FC.B, 0.92f));
	}
	// Hairline top + bottom on bar
	GBox(OutDrawElements, L++, AllottedGeometry, FVector2f(BarX, BarCY),              FVector2f(BarW, 1.f), GEdge());
	GBox(OutDrawElements, L++, AllottedGeometry, FVector2f(BarX, BarCY + BarHt - 1.f), FVector2f(BarW, 1.f), GEdge());

	// Separator below bar row
	GBox(OutDrawElements, L++, AllottedGeometry,
		FVector2f(1.f, BarY + BarRowH), FVector2f(W - 2.f, 1.f), GDiv());

	// ── Row 3: footer — mode (left) + stimulus (right) ───────────────────────
	const float FtrY = BarY + BarRowH + 1.f;

	// Mode label (left, gray)
	GText(OutDrawElements, L, AllottedGeometry, EStr(BrainState.Mode),
		FVector2f(10.f, FtrY + 5.f), GGray(), 9);

	// Stimulus tag (right-of-center, white — only when active)
	FString Stim;
	if (Suspicion.State == EGuardSuspicionState::Alert)        { Stim = TEXT("ALERT"); }
	else if (Suspicion.bHadVisualStimulus)                      { Stim = TEXT("VISUAL"); }
	else if (Suspicion.bHadAuditoryStimulus)                    { Stim = TEXT("SOUND"); }
	else if (!Suspicion.LastReason.IsEmpty())                   { Stim = Suspicion.LastReason.ToUpper(); }

	if (!Stim.IsEmpty())
	{
		const float StimX = W - static_cast<float>(Stim.Len()) * 7.f - 10.f;
		GText(OutDrawElements, L, AllottedGeometry, Stim,
			FVector2f(FMath::Max(StimX, W * 0.55f), FtrY + 5.f), Accent, 9);
	}

	// ── Corner brackets ───────────────────────────────────────────────────────
	GBrackets(OutDrawElements, L, AllottedGeometry, W, H, 12.f);

	return L;
}
