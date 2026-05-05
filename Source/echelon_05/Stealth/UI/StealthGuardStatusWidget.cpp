#include "Stealth/UI/StealthGuardStatusWidget.h"

#include "Stealth/Components/StealthGuardBrainComponent.h"

#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

namespace
{
FLinearColor GuardWidgetStateColor(const EGuardSuspicionState State)
{
	switch (State)
	{
	case EGuardSuspicionState::Alert:
		return FLinearColor(1.f, 0.08f, 0.05f, 1.f);
	case EGuardSuspicionState::Investigating:
		return FLinearColor(1.f, 0.48f, 0.08f, 1.f);
	case EGuardSuspicionState::Suspicious:
		return FLinearColor(1.f, 0.72f, 0.12f, 1.f);
	case EGuardSuspicionState::Curious:
		return FLinearColor(0.62f, 0.82f, 1.f, 1.f);
	case EGuardSuspicionState::Unaware:
	default:
		return FLinearColor(0.16f, 0.9f, 0.68f, 1.f);
	}
}

template <typename TEnum>
FString GuardWidgetEnumDisplayName(const TEnum Value)
{
	if (const UEnum* Enum = StaticEnum<TEnum>())
	{
		return Enum->GetDisplayNameTextByValue(static_cast<int64>(Value)).ToString();
	}
	return FString::FromInt(static_cast<int32>(Value));
}

void DrawWidgetBox(FSlateWindowElementList& OutDrawElements, const int32 LayerId, const FGeometry& Geometry, const FVector2D& Position,
	const FVector2D& Size, const FLinearColor& Color)
{
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		Geometry.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(Position))),
		FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")),
		ESlateDrawEffect::None,
		Color);
}

void DrawWidgetText(FSlateWindowElementList& OutDrawElements, const int32 LayerId, const FGeometry& Geometry, const FString& Text,
	const FVector2D& Position, const FLinearColor& Color, const int32 Size = 11)
{
	const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), Size);
	FSlateDrawElement::MakeText(
		OutDrawElements,
		LayerId,
		Geometry.ToPaintGeometry(FVector2f(1.f, 1.f), FSlateLayoutTransform(FVector2f(Position))),
		Text,
		Font,
		ESlateDrawEffect::None,
		Color);
}
}

void UStealthGuardStatusWidget::SetGuardBrain(UStealthGuardBrainComponent* InGuardBrain)
{
	GuardBrain = InGuardBrain;
}

int32 UStealthGuardStatusWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 ResultLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	int32 CurrentLayer = ResultLayer + 1;

	const FVector2D Size = AllottedGeometry.GetLocalSize();
	const float PanelW = FMath::Max(Size.X, 260.f);
	const float PanelH = FMath::Max(Size.Y, 64.f);

	const UStealthGuardBrainComponent* Brain = GuardBrain.Get();
	const FSuspicionState Suspicion = Brain ? Brain->Suspicion : FSuspicionState();
	const FGuardBrain BrainState = Brain ? Brain->Brain : FGuardBrain();
	const FLinearColor Accent = GuardWidgetStateColor(Suspicion.State);
	const float Suspicion01 = FMath::Clamp(Suspicion.Value / 100.f, 0.f, 1.f);

	DrawWidgetBox(OutDrawElements, CurrentLayer++, AllottedGeometry, FVector2D(0.f, 0.f), FVector2D(PanelW, PanelH),
		FLinearColor(0.005f, 0.015f, 0.02f, 0.72f));
	DrawWidgetBox(OutDrawElements, CurrentLayer++, AllottedGeometry, FVector2D(0.f, 0.f), FVector2D(4.f, PanelH), Accent);
	DrawWidgetBox(OutDrawElements, CurrentLayer++, AllottedGeometry, FVector2D(0.f, 0.f), FVector2D(PanelW, 1.f),
		FLinearColor(Accent.R, Accent.G, Accent.B, 0.35f));

	const FString StateText = Brain
		? FString::Printf(TEXT("%s  |  %s"), *GuardWidgetEnumDisplayName(Suspicion.State), *GuardWidgetEnumDisplayName(BrainState.Mode))
		: FString(TEXT("Guard: --"));
	DrawWidgetText(OutDrawElements, CurrentLayer++, AllottedGeometry, StateText, FVector2D(12.f, 9.f), Accent, 12);

	const FVector2D BarPos(12.f, 34.f);
	const FVector2D BarSize(PanelW - 24.f, 10.f);
	DrawWidgetBox(OutDrawElements, CurrentLayer++, AllottedGeometry, BarPos, BarSize, FLinearColor(0.f, 0.f, 0.f, 0.86f));
	DrawWidgetBox(OutDrawElements, CurrentLayer++, AllottedGeometry, BarPos, FVector2D(BarSize.X * Suspicion01, BarSize.Y), Accent);

	const FString StimulusText = Suspicion.State == EGuardSuspicionState::Alert
		? FString(TEXT("ALERT"))
		: (Suspicion.bHadVisualStimulus ? FString(TEXT("VISUAL CONTACT"))
			: (Suspicion.bHadAuditoryStimulus ? FString(TEXT("SOUND HEARD")) : Suspicion.LastReason));
	if (!StimulusText.IsEmpty())
	{
		DrawWidgetText(OutDrawElements, CurrentLayer++, AllottedGeometry, StimulusText, FVector2D(12.f, 48.f),
			FLinearColor(0.86f, 0.92f, 0.95f, 1.f), 10);
	}

	return CurrentLayer;
}
