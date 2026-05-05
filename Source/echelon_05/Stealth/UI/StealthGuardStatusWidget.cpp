// World-space guard suspicion plate.
//
// Spec: Docs/ui/DESIGN.md `### Guard Suspicion Plate` and `### Alert Banner`.
//
// The plate sits on a small Screen-space WidgetComponent above the guard's head.
// It deliberately stays close to the body so the player's eye doesn't snap to a
// floating label — the guard IS the marker, the plate is its instrument readout.

#include "Stealth/UI/StealthGuardStatusWidget.h"

#include "Stealth/Components/StealthGuardBrainComponent.h"
#include "Stealth/UI/StealthUIPrimitives.h"
#include "Stealth/UI/StealthUITokens.h"

#include "Engine/World.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"

namespace
{
	using namespace StealthUI;

	constexpr int32 MaxTicks = 5;

	/** Map suspicion state + value to the number of lit ticks (1-5). */
	int32 ActiveTickCount(EGuardSuspicionState State, float Value01)
	{
		switch (State)
		{
		case EGuardSuspicionState::Curious:        return 1;
		case EGuardSuspicionState::Suspicious:     return 2;
		case EGuardSuspicionState::Investigating:  return FMath::Clamp(3 + FMath::FloorToInt(Value01 * 2.f), 3, 4);
		case EGuardSuspicionState::Alert:          return MaxTicks;
		default:                                    return 0;
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
	using namespace StealthUI;

	const int32 Base = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements,
		LayerId, InWidgetStyle, bParentEnabled);

	const UStealthGuardBrainComponent* Brain = GuardBrain.Get();
	if (!Brain) { return Base; }

	const EGuardSuspicionState State = Brain->Suspicion.State;
	if (State == EGuardSuspicionState::Unaware) { return Base; }

	const float W = static_cast<float>(AllottedGeometry.GetLocalSize().X);
	const float H = static_cast<float>(AllottedGeometry.GetLocalSize().Y);
	if (W < 8.f || H < 6.f) { return Base; }

	const float Value01 = FMath::Clamp(Brain->Suspicion.Value / 100.f, 0.f, 1.f);
	const float T       = (GetWorld() != nullptr) ? GetWorld()->GetTimeSeconds() : 0.f;
	int32 L = Base + 1;

	// ── ALERT — the only state that uses the hard inversion. ────────────────
	if (State == EGuardSuspicionState::Alert)
	{
		const float Pulse = 0.92f + 0.08f * FMath::Sin(T * (TWO_PI / 0.42f));
		FillRect(OutDrawElements, L++, AllottedGeometry, {0.f, 0.f}, {W, H},
			FLinearColor(Pulse, Pulse, Pulse, 1.f));

		const FString Label = TEXT("ALERT");
		const FSlateFontInfo Font = FLabel();
		const TSharedRef<FSlateFontMeasure> Measurer =
			FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
		const FVector2D Size = Measurer->Measure(Label, Font);

		const float Tx = (W - static_cast<float>(Size.X)) * 0.5f;
		const float Ty = (H - static_cast<float>(Size.Y)) * 0.5f - 1.f;
		DrawText(OutDrawElements, L, AllottedGeometry, Label, {Tx, Ty}, InverseText(), Font);
		return L;
	}

	// ── Curious / Suspicious / Investigating — bracketed plate + ticks ───────
	const FLinearColor Frame = LinePrimary().CopyWithNewOpacity(
		(State >= EGuardSuspicionState::Suspicious) ? 0.55f : 0.32f);

	// Subtle near-black backing keeps the ticks readable when the guard's silhouette
	// behind the plate is bright (panel.opacityPanelBacking = 0.72).
	FillRect(OutDrawElements, L++, AllottedGeometry, {0.f, 0.f}, {W, H},
		Field().CopyWithNewOpacity(0.55f));

	CornerBrackets(OutDrawElements, L, AllottedGeometry, 0.f, 0.f, W, H,
		FMath::Min(6.f, W * 0.12f), Frame);

	// Tick row centered horizontally. Five evenly-spaced ticks, lit ones brighter,
	// most-recent (rightmost lit) tick brightest to encode escalation direction.
	const int32 ActiveTicks = ActiveTickCount(State, Value01);
	const float TickW       = FMath::Max(2.f, (W - 16.f) / static_cast<float>(MaxTicks * 2 - 1));
	const float TickH       = FMath::Max(3.f, H * 0.55f);
	const float TickY       = (H - TickH) * 0.5f;
	const float TotalTicksW = MaxTicks * TickW + (MaxTicks - 1) * TickW;
	float TickX             = (W - TotalTicksW) * 0.5f;

	for (int32 i = 0; i < MaxTicks; ++i)
	{
		const bool bOn = i < ActiveTicks;
		float Brightness = 0.32f;
		if (bOn)
		{
			Brightness = (i == ActiveTicks - 1) ? 0.95f : 0.72f;

			// Investigating leading tick gets a slow blink so the plate reads as
			// "actively searching" without using a separate animation cycle.
			if (State == EGuardSuspicionState::Investigating && i == ActiveTicks - 1)
			{
				Brightness *= 0.65f + 0.35f * FMath::Sin(T * (TWO_PI / 0.6f));
			}
		}

		const FLinearColor C = bOn
			? FLinearColor(Brightness, Brightness, Brightness, 1.f)
			: LinePrimary().CopyWithNewOpacity(OpacityHairlineIdle);

		if (bOn)
		{
			FillRect(OutDrawElements, L++, AllottedGeometry, {TickX, TickY}, {TickW, TickH}, C);
		}
		else
		{
			Outline(OutDrawElements, L, AllottedGeometry, TickX, TickY, TickW, TickH, C);
		}
		TickX += TickW * 2.f;
	}

	return L;
}
