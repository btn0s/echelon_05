#include "Stealth/UI/StealthGuardStatusWidget.h"

#include "Stealth/Components/StealthGuardBrainComponent.h"

#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

namespace
{
// ── Splinter Cell palette (mirrors StealthDebugHUD) ──────────────────────────
FLinearColor GBg()    { return FLinearColor(0.008f, 0.018f, 0.012f, 0.76f); }
FLinearColor GEdge()  { return FLinearColor(0.30f,  0.44f,  0.36f,  0.60f); }
FLinearColor GGreen() { return FLinearColor(0.48f,  0.74f,  0.58f,  1.f);  }
FLinearColor GAmber() { return FLinearColor(0.78f,  0.70f,  0.35f,  1.f);  }
FLinearColor GWarn()  { return FLinearColor(0.84f,  0.56f,  0.28f,  1.f);  }
FLinearColor GHot()   { return FLinearColor(0.90f,  0.42f,  0.20f,  1.f);  }
FLinearColor GWhite() { return FLinearColor(0.88f,  0.90f,  0.86f,  1.f);  }
FLinearColor GDim()   { return FLinearColor(0.32f,  0.40f,  0.35f,  1.f);  }

FLinearColor StateAccent(const EGuardSuspicionState State)
{
	switch (State)
	{
	case EGuardSuspicionState::Alert:         return GHot();
	case EGuardSuspicionState::Investigating: return GWarn();
	case EGuardSuspicionState::Suspicious:    return GAmber();
	case EGuardSuspicionState::Curious:       return GGreen();
	case EGuardSuspicionState::Unaware:
	default:                                  return GDim();
	}
}

// Bar fill uses the same ramp as the player HUD detection strip.
FLinearColor BarRamp(const float N)
{
	if (N >= 0.78f) return GWarn();
	if (N >= 0.46f) return GAmber();
	return GGreen();
}

template <typename TEnum>
FString EUp(const TEnum V)
{
	if (const UEnum* E = StaticEnum<TEnum>())
	{
		return E->GetDisplayNameTextByValue(static_cast<int64>(V)).ToString().ToUpper();
	}
	return FString::FromInt(static_cast<int32>(V));
}

const FSlateBrush* WhiteBrush()
{
	return FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
}

void GBox(FSlateWindowElementList& Out, int32 Layer, const FGeometry& Geo,
	const FVector2f Pos, const FVector2f Sz, const FLinearColor& C)
{
	FSlateDrawElement::MakeBox(Out, Layer,
		Geo.ToPaintGeometry(Sz, FSlateLayoutTransform(Pos)),
		WhiteBrush(), ESlateDrawEffect::None, C);
}

void GText(FSlateWindowElementList& Out, int32 Layer, const FGeometry& Geo,
	const FString& Str, const FVector2f Pos, const FLinearColor& C, int32 Sz = 10)
{
	const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), Sz);
	FSlateDrawElement::MakeText(Out, Layer,
		Geo.ToPaintGeometry(FVector2f(1.f, 1.f), FSlateLayoutTransform(Pos)),
		Str, Font, ESlateDrawEffect::None, C);
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

	const UStealthGuardBrainComponent* Brain  = GuardBrain.Get();
	const FSuspicionState Suspicion            = Brain ? Brain->Suspicion : FSuspicionState();
	const FGuardBrain     BrainState           = Brain ? Brain->Brain    : FGuardBrain();
	const float           Sus01               = FMath::Clamp(Suspicion.Value / 100.f, 0.f, 1.f);
	const FLinearColor    Accent               = StateAccent(Suspicion.State);

	// ── Background ────────────────────────────────────────────────────────────
	GBox(OutDrawElements, L++, AllottedGeometry, FVector2f(0.f, 0.f), FVector2f(W, H), GBg());

	// ── 1px border ────────────────────────────────────────────────────────────
	const FLinearColor E = GEdge();
	GBox(OutDrawElements, L++, AllottedGeometry, FVector2f(0.f, 0.f),   FVector2f(W, 1.f), E);   // top
	GBox(OutDrawElements, L++, AllottedGeometry, FVector2f(0.f, H-1.f), FVector2f(W, 1.f), E);   // bottom
	GBox(OutDrawElements, L++, AllottedGeometry, FVector2f(0.f, 0.f),   FVector2f(1.f, H), E);   // left
	GBox(OutDrawElements, L++, AllottedGeometry, FVector2f(W-1.f, 0.f), FVector2f(1.f, H), E);   // right

	if (!Brain)
	{
		GText(OutDrawElements, L++, AllottedGeometry, TEXT("--"), FVector2f(10.f, 8.f), GDim(), 10);
		return L;
	}

	// ── Row 1: state name (accent) + mode (dim, right-aligned) ───────────────
	const FString StateName = EUp(Suspicion.State);
	const FString ModeName  = EUp(BrainState.Mode);

	GText(OutDrawElements, L++, AllottedGeometry, StateName,
		FVector2f(10.f, 8.f), Accent, 11);

	// Right-align mode — approximate: 7px per char at size 9
	const float ModeX = W - static_cast<float>(ModeName.Len()) * 7.f - 10.f;
	GText(OutDrawElements, L++, AllottedGeometry, ModeName,
		FVector2f(FMath::Max(ModeX, W * 0.5f), 10.f), GDim(), 9);

	// ── Thin divider ─────────────────────────────────────────────────────────
	GBox(OutDrawElements, L++, AllottedGeometry, FVector2f(10.f, 26.f), FVector2f(W - 20.f, 1.f), GEdge());

	// ── Suspicion bar ────────────────────────────────────────────────────────
	constexpr float BarY = 32.f;
	constexpr float BarH = 7.f;
	const float     BarW = W - 20.f;
	// track
	GBox(OutDrawElements, L++, AllottedGeometry,
		FVector2f(10.f, BarY), FVector2f(BarW, BarH),
		FLinearColor(0.f, 0.f, 0.f, 0.70f));
	// fill
	if (Sus01 > 0.f)
	{
		GBox(OutDrawElements, L++, AllottedGeometry,
			FVector2f(10.f, BarY), FVector2f(BarW * Sus01, BarH),
			BarRamp(Sus01));
	}

	// ── Stimulus text ─────────────────────────────────────────────────────────
	FString Stim;
	if (Suspicion.State == EGuardSuspicionState::Alert)
	{
		Stim = TEXT("ALERT");
	}
	else if (Suspicion.bHadVisualStimulus)
	{
		Stim = TEXT("VISUAL");
	}
	else if (Suspicion.bHadAuditoryStimulus)
	{
		Stim = TEXT("SOUND");
	}
	else if (!Suspicion.LastReason.IsEmpty())
	{
		Stim = Suspicion.LastReason.ToUpper();
	}

	if (!Stim.IsEmpty())
	{
		GText(OutDrawElements, L++, AllottedGeometry, Stim,
			FVector2f(10.f, 44.f), GWhite(), 9);
	}

	return L;
}
