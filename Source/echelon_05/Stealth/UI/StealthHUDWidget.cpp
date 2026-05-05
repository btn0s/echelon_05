#include "Stealth/UI/StealthHUDWidget.h"

#include "Stealth/Components/StealthGuardBrainComponent.h"
#include "Stealth/Subsystems/StealthSimulationSubsystem.h"

#include "HAL/IConsoleManager.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Engine/World.h"

namespace
{
// ── Fonts ─────────────────────────────────────────────────────────────────────
// All rendering goes through Slate — proper subpixel antialiasing at every size.
FSlateFontInfo FNum()   { return FCoreStyle::GetDefaultFontStyle("Bold",    22); }  // metric value
FSlateFontInfo FHead()  { return FCoreStyle::GetDefaultFontStyle("Bold",    11); }  // state name / alert
FSlateFontInfo FLabel() { return FCoreStyle::GetDefaultFontStyle("Regular",  8); }  // tiny section / unit label
FSlateFontInfo FMeta()  { return FCoreStyle::GetDefaultFontStyle("Regular",  9); }  // secondary info
FSlateFontInfo FDiag()  { return FCoreStyle::GetDefaultFontStyle("Regular",  9); }  // diagnostics panel

// ── Palette ───────────────────────────────────────────────────────────────────
FLinearColor TBg()    { return FLinearColor(0.010f, 0.010f, 0.010f, 0.90f); }
FLinearColor THdrBg() { return FLinearColor(0.055f, 0.055f, 0.055f, 0.95f); }
FLinearColor TEdge()  { return FLinearColor(1.f,    1.f,    1.f,    0.20f); }
FLinearColor TDiv()   { return FLinearColor(1.f,    1.f,    1.f,    0.09f); }
FLinearColor TWhite() { return FLinearColor(0.94f,  0.94f,  0.94f,  1.f);  }
FLinearColor TGray()  { return FLinearColor(0.50f,  0.50f,  0.50f,  1.f);  }
FLinearColor TDim()   { return FLinearColor(0.26f,  0.26f,  0.26f,  1.f);  }
FLinearColor TGreen() { return FLinearColor(0.08f,  0.94f,  0.38f,  1.f);  }
FLinearColor TAmber() { return FLinearColor(0.98f,  0.64f,  0.00f,  1.f);  }
FLinearColor TRed()   { return FLinearColor(0.97f,  0.16f,  0.06f,  1.f);  }

FLinearColor TRamp(float N)
{
	if (N >= 0.78f) return TRed();
	if (N >= 0.46f) return TAmber();
	return TGreen();
}

FLinearColor TSuspColor(EGuardSuspicionState S)
{
	switch (S)
	{
	case EGuardSuspicionState::Alert:          return TRed();
	case EGuardSuspicionState::Investigating:  return TAmber();
	case EGuardSuspicionState::Suspicious:     return TAmber();
	case EGuardSuspicionState::Curious:        return TGreen();
	default:                                    return TDim();
	}
}

template <typename TEnum>
FString EStr(TEnum V)
{
	if (const UEnum* E = StaticEnum<TEnum>())
	{
		return E->GetDisplayNameTextByValue(static_cast<int64>(V)).ToString().ToUpper();
	}
	return FString::FromInt(static_cast<int32>(V));
}

// ── Primitives ────────────────────────────────────────────────────────────────

const FSlateBrush* WBrush() { return FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")); }

void Box(FSlateWindowElementList& Out, int32 L, const FGeometry& Geo,
	FVector2f Pos, FVector2f Sz, const FLinearColor& C)
{
	FSlateDrawElement::MakeBox(Out, L,
		Geo.ToPaintGeometry(Sz, FSlateLayoutTransform(Pos)),
		WBrush(), ESlateDrawEffect::None, C);
}

// Text with a single-pixel drop shadow for legibility on any background.
void Txt(FSlateWindowElementList& Out, int32& L, const FGeometry& Geo,
	const FString& S, FVector2f Pos, const FLinearColor& C, const FSlateFontInfo& F)
{
	FSlateDrawElement::MakeText(Out, L++,
		Geo.ToPaintGeometry(FVector2f(1.f, 1.f), FSlateLayoutTransform(Pos + FVector2f(1.f, 1.f))),
		S, F, ESlateDrawEffect::None, FLinearColor(0.f, 0.f, 0.f, 0.70f));
	FSlateDrawElement::MakeText(Out, L++,
		Geo.ToPaintGeometry(FVector2f(1.f, 1.f), FSlateLayoutTransform(Pos)),
		S, F, ESlateDrawEffect::None, C);
}

// L-shaped corner brackets.
void Brackets(FSlateWindowElementList& Out, int32& L, const FGeometry& Geo,
	float W, float H, const FLinearColor& C, float BLen = 14.f)
{
	auto B = [&](FVector2f P, FVector2f Sz) { Box(Out, L++, Geo, P, Sz, C); };
	B({0.f,    0.f},   {BLen, 1.f});  B({0.f,    0.f},   {1.f, BLen});
	B({W-BLen, 0.f},   {BLen, 1.f});  B({W-1.f,  0.f},   {1.f, BLen});
	B({0.f,    H-1.f}, {BLen, 1.f});  B({0.f,    H-BLen},{1.f, BLen});
	B({W-BLen, H-1.f}, {BLen, 1.f});  B({W-1.f,  H-BLen},{1.f, BLen});
}

// Thin metric bar with 25/50/75 tick marks and optional peak marker.
void MetricBar(FSlateWindowElementList& Out, int32& L, const FGeometry& Geo,
	float X, float Y, float W, float H, float N01, const FLinearColor& Fill, float Peak = -1.f)
{
	Box(Out, L++, Geo, {X, Y}, {W, H}, FLinearColor(0.f, 0.f, 0.f, 1.f));
	if (const float C = FMath::Clamp(N01, 0.f, 1.f); C > 0.f)
	{
		Box(Out, L++, Geo, {X, Y}, {W * C, H}, FLinearColor(Fill.R, Fill.G, Fill.B, 0.92f));
	}
	static constexpr float Ticks[] = { 0.25f, 0.50f, 0.75f };
	for (const float F : Ticks)
	{
		Box(Out, L++, Geo, {X + W * F, Y}, {1.f, H}, FLinearColor(0.f, 0.f, 0.f, 0.50f));
	}
	if (Peak > N01 + 0.02f && Peak > 0.f)
	{
		Box(Out, L++, Geo, {X + W * FMath::Clamp(Peak, 0.f, 1.f) - 1.f, Y}, {2.f, H},
			FLinearColor(1.f, 1.f, 1.f, 0.55f));
	}
	Box(Out, L++, Geo, {X, Y},       {W, 1.f}, FLinearColor(1.f, 1.f, 1.f, 0.14f));
	Box(Out, L++, Geo, {X, Y+H-1.f}, {W, 1.f}, FLinearColor(1.f, 1.f, 1.f, 0.14f));
}

// One stealth metric card: label (top) → large value → thin bar (bottom).
// CardX/CardY are the top-left corner.  CardW/CardH are the card dimensions.
void MetricCard(FSlateWindowElementList& Out, int32& L, const FGeometry& Geo,
	float CardX, float CardY, float CardW, float CardH,
	const FString& Label, const FString& Value, float N01, const FLinearColor& Fill,
	float Peak = -1.f)
{
	// Subtle card background
	Box(Out, L++, Geo, {CardX, CardY}, {CardW, CardH}, TBg());
	// Thin left accent stripe — colour matches metric state
	Box(Out, L++, Geo, {CardX, CardY}, {2.f, CardH}, FLinearColor(Fill.R, Fill.G, Fill.B, 0.65f));

	// Label (tiny, top-left)
	Txt(Out, L, Geo, Label, {CardX + 8.f, CardY + 6.f}, TDim(), FLabel());
	// Value (large, primary)
	Txt(Out, L, Geo, Value, {CardX + 8.f, CardY + 18.f}, TWhite(), FNum());
	// Bar (bottom)
	const float BarY = CardY + CardH - 8.f;
	MetricBar(Out, L, Geo, CardX + 8.f, BarY, CardW - 16.f, 3.f, N01, Fill, Peak);
}
}

// ─────────────────────────────────────────────────────────────────────────────

int32 UStealthHUDWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 Base = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements,
		LayerId, InWidgetStyle, bParentEnabled);
	int32 L = Base + 1;

	if (!GetWorld()) { return L; }

	UStealthSimulationSubsystem* Sim = GetWorld()->GetSubsystem<UStealthSimulationSubsystem>();
	if (!Sim) { return L; }

	const float SW = static_cast<float>(AllottedGeometry.GetLocalSize().X);
	const float SH = static_cast<float>(AllottedGeometry.GetLocalSize().Y);
	const float T  = GetWorld()->GetTimeSeconds();

	// ── Data ──────────────────────────────────────────────────────────────────
	const FStealthMovementState Mv  = Sim->GetPlayerMovement();
	const FVisibilityEmitter    Vis = Sim->GetPlayerVisibility();
	const FSoundEmitter         Snd = Sim->GetPlayerSoundEmission();
	const FObjectiveState       Obj = Sim->GetObjectiveState();
	const FExtractionState      Ext = Sim->GetExtractionState();

	const TArray<TWeakObjectPtr<UStealthGuardBrainComponent>> Brains = Sim->GetRegisteredGuardBrains();
	const UStealthGuardBrainComponent* PrimaryBrain = nullptr;
	for (const TWeakObjectPtr<UStealthGuardBrainComponent>& B : Brains)
	{
		if (B.IsValid()) { PrimaryBrain = B.Get(); break; }
	}

	const FSuspicionState Suspicion = PrimaryBrain ? PrimaryBrain->Suspicion : FSuspicionState();
	const float Susp01  = FMath::Clamp(Suspicion.Value / 100.f, 0.f, 1.f);
	const float Vis01   = FMath::Clamp(Vis.CurrentVisibility, 0.f, 1.f);
	const float Noise01 = FMath::Clamp(Snd.Radius / 1400.f, 0.f, 1.f);
	const FLinearColor SuspColor = TSuspColor(Suspicion.State);

	// Peak tracking (mutable state — safe in NativePaint)
	PeakVis01   = FMath::Max(PeakVis01,   Vis01);
	PeakNoise01 = FMath::Max(PeakNoise01, Noise01);

	// ── SCREEN EDGE VIGNETTE ──────────────────────────────────────────────────
	// Pulsing red at Alert; faint static amber at Investigating.
	if (Suspicion.State == EGuardSuspicionState::Alert)
	{
		const float A = FMath::Sin(T * 4.5f) * 0.10f + 0.14f;
		const FLinearColor VC(TRed().R, TRed().G, TRed().B, A);
		constexpr float VW = 8.f;
		Box(OutDrawElements, L++, AllottedGeometry, {0.f,     0.f},     {SW, VW}, VC);
		Box(OutDrawElements, L++, AllottedGeometry, {0.f,     SH - VW}, {SW, VW}, VC);
		Box(OutDrawElements, L++, AllottedGeometry, {0.f,     0.f},     {VW, SH}, VC);
		Box(OutDrawElements, L++, AllottedGeometry, {SW - VW, 0.f},     {VW, SH}, VC);
	}
	else if (Suspicion.State == EGuardSuspicionState::Investigating)
	{
		const FLinearColor VC(TAmber().R, TAmber().G, TAmber().B, 0.06f);
		constexpr float VW = 5.f;
		Box(OutDrawElements, L++, AllottedGeometry, {0.f,     0.f},     {SW, VW}, VC);
		Box(OutDrawElements, L++, AllottedGeometry, {0.f,     SH - VW}, {SW, VW}, VC);
		Box(OutDrawElements, L++, AllottedGeometry, {0.f,     0.f},     {VW, SH}, VC);
		Box(OutDrawElements, L++, AllottedGeometry, {SW - VW, 0.f},     {VW, SH}, VC);
	}

	// ── TOP CENTER: detection state ───────────────────────────────────────────
	// Hierarchy: nothing when Unaware → coloured text when curious/suspicious →
	// solid filled block when Alert.  The absence of an indicator IS the signal
	// that the guard has not noticed the player.
	{
		if (Suspicion.State == EGuardSuspicionState::Alert)
		{
			// Solid red block with white text — the highest urgency signal.
			const float BW  = 224.f;
			const float BH  = 26.f;
			const float BX  = (SW - BW) * 0.5f;
			const float BY  = 16.f;
			// Pulse the block slightly
			const float PA  = FMath::Sin(T * 4.5f) * 0.10f + 0.90f;
			Box(OutDrawElements, L++, AllottedGeometry, {BX, BY}, {BW, BH},
				FLinearColor(TRed().R, TRed().G, TRed().B, PA));
			Txt(OutDrawElements, L, AllottedGeometry, TEXT("DETECTED"),
				{BX + 16.f, BY + 6.f}, TWhite(), FHead());
			// Percentage right side of block
			Txt(OutDrawElements, L, AllottedGeometry,
				FString::Printf(TEXT("%2.0f%%"), Susp01 * 100.f),
				{BX + BW - 38.f, BY + 7.f}, FLinearColor(1.f, 1.f, 1.f, 0.65f), FMeta());
		}
		else if (Suspicion.State >= EGuardSuspicionState::Curious)
		{
			// Text only — state name + percentage.  No box.
			const FString StateStr = FString::Printf(TEXT("%s   %2.0f%%"),
				*EStr(Suspicion.State), Susp01 * 100.f);
			Txt(OutDrawElements, L, AllottedGeometry, StateStr,
				{SW * 0.5f - 54.f, 22.f}, SuspColor, FHead());

			// Thin underline below the text for Investigating+
			if (Suspicion.State >= EGuardSuspicionState::Investigating)
			{
				Box(OutDrawElements, L++, AllottedGeometry,
					{SW * 0.5f - 56.f, 38.f}, {112.f, 1.f},
					FLinearColor(SuspColor.R, SuspColor.G, SuspColor.B, 0.45f));
			}
		}
	}

	// ── TOP LEFT: mission status ──────────────────────────────────────────────
	// Wayfinding style: pure typography, no panel.  Section header → data rows.
	{
		const float MX = 24.f;
		float MY = 22.f;

		// Section header
		Txt(OutDrawElements, L, AllottedGeometry, TEXT("MISSION"), {MX, MY}, TDim(), FLabel());
		MY += 16.f;

		// OBJ row
		{
			const bool bDone = Obj.bCompleted;
			const FLinearColor DotC = bDone ? TGreen() : TDim();
			Box(OutDrawElements, L++, AllottedGeometry,
				{MX, MY + 1.f}, {8.f, 8.f}, DotC);
			Txt(OutDrawElements, L, AllottedGeometry,
				TEXT("OBJ"), {MX + 14.f, MY}, TGray(), FMeta());
			Txt(OutDrawElements, L, AllottedGeometry,
				bDone ? TEXT("COMPLETE") : TEXT("ACTIVE"),
				{MX + 44.f, MY}, bDone ? TGreen() : TWhite(), FMeta());
		}
		MY += 18.f;

		// EXT row
		{
			const bool bAvail = Ext.bAvailable;
			const bool bUsed  = Ext.bUsed;
			const FLinearColor DotC = bAvail && !bUsed ? TGreen() : TDim();
			const FString ExtStr    = bUsed ? TEXT("USED") : (bAvail ? TEXT("AVAIL") : TEXT("LOCKED"));
			const FLinearColor ValC = bUsed ? TGreen() : (bAvail ? TWhite() : TDim());
			Box(OutDrawElements, L++, AllottedGeometry,
				{MX, MY + 1.f}, {8.f, 8.f}, DotC);
			Txt(OutDrawElements, L, AllottedGeometry,
				TEXT("EXT"), {MX + 14.f, MY}, TGray(), FMeta());
			Txt(OutDrawElements, L, AllottedGeometry,
				ExtStr, {MX + 44.f, MY}, ValC, FMeta());
		}
		MY += 18.f;

		// COMPROMISED — only shown when an alert has occurred
		if (Sim->HasAlertOccurred())
		{
			const float DA = FMath::Sin(T * 2.5f) * 0.4f + 0.6f;
			Box(OutDrawElements, L++, AllottedGeometry,
				{MX, MY + 1.f}, {8.f, 8.f},
				FLinearColor(TAmber().R, TAmber().G, TAmber().B, DA));
			Txt(OutDrawElements, L, AllottedGeometry,
				TEXT("COMPROMISED"), {MX + 14.f, MY}, TAmber(), FMeta());
		}
	}

	// ── BOTTOM RIGHT: stealth metric cards ───────────────────────────────────
	// Two side-by-side cards (image-1 widget style): label top, large value, thin bar bottom.
	{
		constexpr float CardW  = 104.f;
		constexpr float CardH  = 66.f;
		constexpr float Gap    = 8.f;
		const float TotalW = CardW * 2.f + Gap;
		const float CX     = SW - TotalW - 24.f;
		const float CY     = SH - CardH - 52.f;   // leave room for footer below

		// VIS card
		{
			const FLinearColor VC = TRamp(Vis01);
			MetricCard(OutDrawElements, L, AllottedGeometry,
				CX, CY, CardW, CardH,
				TEXT("VISIBILITY"),
				FString::Printf(TEXT("%2.0f%%"), Vis01 * 100.f),
				Vis01, VC, PeakVis01);
		}

		// NSE card
		{
			const FLinearColor NC = TRamp(Noise01);
			MetricCard(OutDrawElements, L, AllottedGeometry,
				CX + CardW + Gap, CY, CardW, CardH,
				TEXT("NOISE"),
				FString::Printf(TEXT("%2.0f%%"), Noise01 * 100.f),
				Noise01, NC, PeakNoise01);
		}

		// Footer row: stance · locomotion — below both cards
		Txt(OutDrawElements, L, AllottedGeometry,
			FString::Printf(TEXT("%s  ·  %s"), *EStr(Mv.Stance), *EStr(Mv.Locomotion)),
			{CX + 8.f, CY + CardH + 10.f}, TDim(), FLabel());

		// LIVE blink dot — shows the system is reading live data
		if (FMath::Frac(T * 1.4f) < 0.55f)
		{
			Box(OutDrawElements, L++, AllottedGeometry,
				{CX + TotalW - 8.f, CY + CardH + 12.f}, {5.f, 5.f}, TGreen());
		}
	}

	// ── RIGHT: diagnostics — only when stealth.DebugDraw > 0 ─────────────────
	int32 DebugVal = 0;
	if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("stealth.DebugDraw")))
	{
		DebugVal = CVar->GetInt();
	}

	if (DebugVal > 0)
	{
		const FStealthAlsDebugSnapshot   Als    = Sim->GetAlsDebugSnapshot();
		const FStealthLightSamplingDebug Ls     = Sim->GetLastLightSamplingDebug();
		const TArray<FStealthSoundEvent> Events = Sim->GetActiveSoundEvents();

		const float DX  = SW - 360.f;
		const float DY0 = 80.f;
		constexpr float DW  = 340.f;

		// Panel
		Box(OutDrawElements, L++, AllottedGeometry, {DX, DY0}, {DW, 360.f}, TBg());
		// Header
		Box(OutDrawElements, L++, AllottedGeometry, {DX + 1.f, DY0 + 1.f}, {DW - 2.f, 22.f}, THdrBg());
		Box(OutDrawElements, L++, AllottedGeometry, {DX + 1.f, DY0 + 22.f}, {DW - 2.f, 1.f}, TDiv());
		Txt(OutDrawElements, L, AllottedGeometry, TEXT("DIAGNOSTICS"), {DX + 12.f, DY0 + 6.f}, TDim(), FDiag());
		Brackets(OutDrawElements, L, AllottedGeometry, DW, 360.f, TEdge(), 12.f);

		// Reposition geometry origin to panel top-left so we can use simple Y offsets.
		// Since FGeometry works in widget-local space, we use absolute positions.
		auto Row = [&](const FString& S, float& DY, const FLinearColor& C = FLinearColor(0.50f, 0.50f, 0.50f, 1.f))
		{
			Txt(OutDrawElements, L, AllottedGeometry, S, {DX + 12.f, DY}, C, FDiag());
			DY += 14.f;
		};

		float DY = DY0 + 28.f;
		Row(FString::Printf(TEXT("als  stance=%s  gait=%s"), *Als.AlsStance.ToString(), *Als.AlsGait.ToString()), DY);
		Row(FString::Printf(TEXT("spd=%.1f  mv=%d  in=%d"), Als.Speed, Als.bMoving ? 1 : 0, Als.bHasInput ? 1 : 0), DY);
		Row(FString::Printf(TEXT("locmode=%s"), *Als.AlsLocomotionMode.ToString()), DY);
		DY += 4.f;
		Row(FString::Printf(TEXT("vis=%.2f  L=%.2f  st=%.2f"), Vis.CurrentVisibility, Vis.LightExposure, Vis.StanceMultiplier), DY);
		Row(FString::Printf(TEXT("mv=%.2f  act=%.2f"), Vis.MovementMultiplier, Vis.ActionMultiplier), DY);
		Row(FString::Printf(TEXT("scene  final=%.2f  max=%.2f  lights=%d"), Ls.FinalExposure, Ls.RawMaxExposure, Ls.CachedLightCount), DY);
		for (const FStealthBodyLightSampleDebug& Pt : Ls.BodySamples)
		{
			Row(FString::Printf(TEXT("  %s: %.2f"), *Pt.SampleName, Pt.Exposure), DY);
		}
		DY += 4.f;
		Row(FString::Printf(TEXT("noise=%.0f  sounds=%d"), Snd.Radius, Events.Num()), DY);
		for (int32 i = 0; i < FMath::Min(3, Events.Num()); ++i)
		{
			Row(FString::Printf(TEXT("  [%d] %s  r=%.0f"), Events[i].EventId, *Events[i].DebugLabel, Events[i].Radius), DY);
		}
		DY += 4.f;
		if (PrimaryBrain)
		{
			Row(PrimaryBrain->GetDebugBrainLine(), DY, SuspColor);
		}
		Row(FString::Printf(TEXT("alarm=%s  outcome=%s"),
			Sim->GetAlarmState().bActive ? TEXT("ON") : TEXT("off"),
			*EStr(Sim->GetMissionOutcome())), DY);
	}

	return L;
}
