#include "Stealth/UI/StealthGuardStatusWidget.h"

#include "Stealth/Components/StealthGuardBrainComponent.h"

#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

namespace
{
const FSlateBrush* WBrush() { return FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")); }

void Box(FSlateWindowElementList& Out, int32 L, const FGeometry& Geo,
	FVector2f Pos, FVector2f Sz, const FLinearColor& C)
{
	FSlateDrawElement::MakeBox(Out, L,
		Geo.ToPaintGeometry(Sz, FSlateLayoutTransform(Pos)),
		WBrush(), ESlateDrawEffect::None, C);
}

// State → color.  Full saturation; the bar IS the only indicator.
FLinearColor StateColor(EGuardSuspicionState S)
{
	switch (S)
	{
	case EGuardSuspicionState::Alert:          return FLinearColor(0.95f, 0.08f, 0.05f, 1.0f);
	case EGuardSuspicionState::Investigating:  return FLinearColor(0.92f, 0.28f, 0.05f, 1.0f);
	case EGuardSuspicionState::Suspicious:     return FLinearColor(0.92f, 0.52f, 0.05f, 1.0f);
	case EGuardSuspicionState::Curious:        return FLinearColor(0.90f, 0.80f, 0.05f, 1.0f);
	default:                                    return FLinearColor(0.0f,  0.0f,  0.0f,  0.0f);
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

	const UStealthGuardBrainComponent* Brain = GuardBrain.Get();
	if (!Brain || Brain->Suspicion.Value <= 0.f)
	{
		return Base;
	}

	const float W     = static_cast<float>(AllottedGeometry.GetLocalSize().X);
	const float H     = static_cast<float>(AllottedGeometry.GetLocalSize().Y);
	const float Sus01 = FMath::Clamp(Brain->Suspicion.Value / 100.f, 0.f, 1.f);
	int32 L = Base + 1;

	// Faint dark track
	Box(OutDrawElements, L++, AllottedGeometry, {0.f, 0.f}, {W, H},
		FLinearColor(0.f, 0.f, 0.f, 0.55f));

	// Colored fill — width proportional to suspicion, color = state
	Box(OutDrawElements, L++, AllottedGeometry, {0.f, 0.f}, {W * Sus01, H},
		StateColor(Brain->Suspicion.State));

	return L;
}
