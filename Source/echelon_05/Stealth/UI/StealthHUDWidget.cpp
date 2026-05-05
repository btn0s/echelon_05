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
FSlateFontInfo FNum()   { return FCoreStyle::GetDefaultFontStyle("Bold",    30); }
FSlateFontInfo FHead()  { return FCoreStyle::GetDefaultFontStyle("Bold",    14); }
FSlateFontInfo FMeta()  { return FCoreStyle::GetDefaultFontStyle("Regular", 12); }
FSlateFontInfo FLabel() { return FCoreStyle::GetDefaultFontStyle("Regular", 11); }
FSlateFontInfo FDiag()  { return FCoreStyle::GetDefaultFontStyle("Regular",  9); }

// ── Palette — pure monochromatic wireframe ────────────────────────────────────
// No hue anywhere.  Brightness alone encodes state: dim = idle, bright = danger.
// Solid fill is reserved for ALERT ONLY so it reads as an alarm against the
// otherwise fully-outlined world.
FLinearColor WBright() { return FLinearColor(0.90f, 0.90f, 0.90f, 1.0f); }   // primary text / active
FLinearColor WMid()    { return FLinearColor(0.52f, 0.52f, 0.52f, 1.0f); }   // secondary
FLinearColor WDim()    { return FLinearColor(0.24f, 0.24f, 0.24f, 1.0f); }   // labels / captions
FLinearColor WFaint()  { return FLinearColor(0.08f, 0.08f, 0.08f, 1.0f); }   // bar track interior
FLinearColor WEdge()   { return FLinearColor(1.0f,  1.0f,  1.0f,  0.22f); }  // hairlines

// Brightness ramp: 0→28 % at zero value, 0→90 % at max.
// A full bar is unmistakably bright; an empty bar is nearly invisible.
FLinearColor WRamp(float N01)
{
	const float B = 0.28f + FMath::Clamp(N01, 0.f, 1.f) * 0.62f;
	return FLinearColor(B, B, B, 1.0f);
}

// Suspicion state brightness — monotone encoding of threat level.
FLinearColor WSuspBright(EGuardSuspicionState S)
{
	switch (S)
	{
	case EGuardSuspicionState::Alert:          return FLinearColor(0.92f, 0.92f, 0.92f, 1.0f);
	case EGuardSuspicionState::Investigating:  return FLinearColor(0.70f, 0.70f, 0.70f, 1.0f);
	case EGuardSuspicionState::Suspicious:     return FLinearColor(0.58f, 0.58f, 0.58f, 1.0f);
	case EGuardSuspicionState::Curious:        return FLinearColor(0.42f, 0.42f, 0.42f, 1.0f);
	default:                                    return FLinearColor(0.18f, 0.18f, 0.18f, 1.0f);
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

// Text with 1px drop shadow.
void Txt(FSlateWindowElementList& Out, int32& L, const FGeometry& Geo,
	const FString& S, FVector2f Pos, const FLinearColor& C, const FSlateFontInfo& F)
{
	FSlateDrawElement::MakeText(Out, L++,
		Geo.ToPaintGeometry(FVector2f(1.f, 1.f), FSlateLayoutTransform(Pos + FVector2f(1.f, 1.f))),
		S, F, ESlateDrawEffect::None, FLinearColor(0.f, 0.f, 0.f, 0.80f));
	FSlateDrawElement::MakeText(Out, L++,
		Geo.ToPaintGeometry(FVector2f(1.f, 1.f), FSlateLayoutTransform(Pos)),
		S, F, ESlateDrawEffect::None, C);
}

// 1px border rectangle (no fill).
void Outline(FSlateWindowElementList& Out, int32& L, const FGeometry& Geo,
	float X, float Y, float W, float H, const FLinearColor& C)
{
	Box(Out, L++, Geo, {X,       Y      }, {W, 1.f}, C);
	Box(Out, L++, Geo, {X,       Y+H-1.f}, {W, 1.f}, C);
	Box(Out, L++, Geo, {X,       Y      }, {1.f, H}, C);
	Box(Out, L++, Geo, {X+W-1.f, Y      }, {1.f, H}, C);
}

// Wireframe metric bar:
//   track = near-black outlined rect
//   fill  = white at brightness proportional to value  (bright = dangerous)
//   ticks = faint 25/50/75 dividers
//   peak  = bright white 2px tick at session max
void WireBar(FSlateWindowElementList& Out, int32& L, const FGeometry& Geo,
	float X, float Y, float W, float H, float N01, float Peak = -1.f)
{
	const FLinearColor Fill = WRamp(N01);
	// Track
	Box(Out, L++, Geo, {X, Y}, {W, H}, WFaint());
	// Fill
	if (const float Cl = FMath::Clamp(N01, 0.f, 1.f); Cl > 0.f)
	{
		Box(Out, L++, Geo, {X, Y}, {W * Cl, H}, Fill);
	}
	// 25/50/75 tick dividers (dark lines drawn over the fill)
	static constexpr float Ticks[] = { 0.25f, 0.50f, 0.75f };
	for (const float Frac : Ticks)
	{
		Box(Out, L++, Geo, {X + W * Frac, Y}, {1.f, H},
			FLinearColor(0.f, 0.f, 0.f, 0.55f));
	}
	// Peak high-water mark
	if (Peak > N01 + 0.02f && Peak > 0.f)
	{
		Box(Out, L++, Geo, {X + W * FMath::Clamp(Peak, 0.f, 1.f) - 1.f, Y}, {2.f, H},
			FLinearColor(1.f, 1.f, 1.f, 0.65f));
	}
	// Outline border
	Outline(Out, L, Geo, X, Y, W, H, WEdge());
}

// Small square indicator: outlined = inactive/dim, filled = active/bright.
void WireSquare(FSlateWindowElementList& Out, int32& L, const FGeometry& Geo,
	float X, float Y, float Sz, bool bFilled, float Brightness = 0.55f)
{
	const FLinearColor C(Brightness, Brightness, Brightness, 1.0f);
	if (bFilled)
	{
		Box(Out, L++, Geo, {X, Y}, {Sz, Sz}, C);
	}
	else
	{
		Outline(Out, L, Geo, X, Y, Sz, Sz, FLinearColor(Brightness * 0.45f, Brightness * 0.45f, Brightness * 0.45f, 1.f));
	}
}

// Metric card: section label → big number → thin wireframe bar.
// Floats on pure black — no panel background.
void MetricCard(FSlateWindowElementList& Out, int32& L, const FGeometry& Geo,
	float CX, float CY, float CW, float CH,
	const FString& Label, const FString& Value, float N01, float Peak = -1.f)
{
	const FLinearColor VC = WRamp(N01);

	// Section label
	Txt(Out, L, Geo, Label, {CX, CY}, WDim(), FLabel());

	// Large value
	Txt(Out, L, Geo, Value, {CX, CY + 14.f}, WBright(), FNum());

	// Wireframe bar at bottom of card
	WireBar(Out, L, Geo, CX, CY + CH - 12.f, CW, 5.f, N01, Peak);
}

// L-shaped corner brackets — used only on the diagnostics panel.
void Brackets(FSlateWindowElementList& Out, int32& L, const FGeometry& Geo,
	float W, float H, float BLen = 12.f)
{
	const FLinearColor C = WEdge();
	auto B = [&](FVector2f P, FVector2f S) { Box(Out, L++, Geo, P, S, C); };
	B({0.f,    0.f},   {BLen, 1.f});  B({0.f,    0.f},   {1.f, BLen});
	B({W-BLen, 0.f},   {BLen, 1.f});  B({W-1.f,  0.f},   {1.f, BLen});
	B({0.f,    H-1.f}, {BLen, 1.f});  B({0.f,    H-BLen},{1.f, BLen});
	B({W-BLen, H-1.f}, {BLen, 1.f});  B({W-1.f,  H-BLen},{1.f, BLen});
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
	const FLinearColor SuspC = WSuspBright(Suspicion.State);

	PeakVis01   = FMath::Max(PeakVis01,   Vis01);
	PeakNoise01 = FMath::Max(PeakNoise01, Noise01);

	// ── SCREEN EDGE VIGNETTE ──────────────────────────────────────────────────
	// At Alert: bright white pulse at screen edges — a radar-ping effect that
	// reads clearly against the pure-black wireframe world.
	if (Suspicion.State == EGuardSuspicionState::Alert)
	{
		const float A = FMath::Sin(T * 4.5f) * 0.10f + 0.12f;
		const FLinearColor VC(1.f, 1.f, 1.f, A);
		constexpr float VW = 6.f;
		Box(OutDrawElements, L++, AllottedGeometry, {0.f,     0.f},     {SW, VW}, VC);
		Box(OutDrawElements, L++, AllottedGeometry, {0.f,     SH - VW}, {SW, VW}, VC);
		Box(OutDrawElements, L++, AllottedGeometry, {0.f,     0.f},     {VW, SH}, VC);
		Box(OutDrawElements, L++, AllottedGeometry, {SW - VW, 0.f},     {VW, SH}, VC);
	}

	// ── TOP CENTER: detection state ───────────────────────────────────────────
	// Wireframe rule: outlines everywhere — EXCEPT Alert, which flips to a solid
	// white fill block.  That single violation of the outline language makes the
	// alert state unmistakably alarming.
	{
		if (Suspicion.State == EGuardSuspicionState::Alert)
		{
			// Solid white fill — the only solid fill on the entire screen.
			const float PA  = FMath::Sin(T * 4.5f) * 0.08f + 0.92f;
			const float BW  = 280.f;
			const float BH  = 34.f;
			const float BX  = (SW - BW) * 0.5f;
			const float BY  = 18.f;
			Box(OutDrawElements, L++, AllottedGeometry, {BX, BY}, {BW, BH},
				FLinearColor(PA, PA, PA, 1.f));
			Txt(OutDrawElements, L, AllottedGeometry, TEXT("DETECTED"),
				{BX + 16.f, BY + 8.f}, FLinearColor(0.f, 0.f, 0.f, 0.95f), FHead());
			Txt(OutDrawElements, L, AllottedGeometry,
				FString::Printf(TEXT("%2.0f%%"), Susp01 * 100.f),
				{BX + BW - 52.f, BY + 10.f}, FLinearColor(0.f, 0.f, 0.f, 0.60f), FMeta());
		}
		else if (Suspicion.State >= EGuardSuspicionState::Curious)
		{
			// Outlined box — border brightness scales with urgency.
			const float BW = 280.f;
			const float BH = 32.f;
			const float BX = (SW - BW) * 0.5f;
			const float BY = 18.f;
			const float EB = SuspC.R;
			Outline(OutDrawElements, L, AllottedGeometry, BX, BY, BW, BH,
				FLinearColor(EB, EB, EB, 0.55f));
			Txt(OutDrawElements, L, AllottedGeometry,
				FString::Printf(TEXT("%s  %2.0f%%"), *EStr(Suspicion.State), Susp01 * 100.f),
				{BX + 16.f, BY + 8.f}, SuspC, FHead());
		}
	}

	// ── TOP LEFT: mission status ──────────────────────────────────────────────
	// Pure typography.  WireSquare indicators: outlined = inactive, filled = done.
	{
		const float MX = 28.f;
		float MY = 24.f;

		Txt(OutDrawElements, L, AllottedGeometry, TEXT("MISSION"), {MX, MY}, WDim(), FLabel());
		MY += 20.f;

		// OBJ
		{
			const bool bDone = Obj.bCompleted;
			WireSquare(OutDrawElements, L, AllottedGeometry, MX, MY + 1.f, 10.f, bDone, bDone ? 0.85f : 0.30f);
			Txt(OutDrawElements, L, AllottedGeometry, TEXT("OBJ"), {MX + 18.f, MY}, WDim(), FMeta());
			Txt(OutDrawElements, L, AllottedGeometry, bDone ? TEXT("COMPLETE") : TEXT("ACTIVE"),
				{MX + 58.f, MY}, bDone ? WBright() : WMid(), FMeta());
		}
		MY += 22.f;

		// EXT
		{
			const bool bAvail = Ext.bAvailable;
			const bool bUsed  = Ext.bUsed;
			const bool bActive = bAvail && !bUsed;
			const FString ES  = bUsed ? TEXT("USED") : (bAvail ? TEXT("AVAIL") : TEXT("LOCKED"));
			const FLinearColor EC = bUsed ? WBright() : (bAvail ? WMid() : WDim());
			WireSquare(OutDrawElements, L, AllottedGeometry, MX, MY + 1.f, 10.f, bActive, bActive ? 0.75f : 0.25f);
			Txt(OutDrawElements, L, AllottedGeometry, TEXT("EXT"), {MX + 18.f, MY}, WDim(), FMeta());
			Txt(OutDrawElements, L, AllottedGeometry, ES, {MX + 58.f, MY}, EC, FMeta());
		}
		MY += 22.f;

		// COMPROMISED — outlined pulsing square + bright text
		if (Sim->HasAlertOccurred())
		{
			const float PA = FMath::Sin(T * 2.5f) * 0.35f + 0.55f;
			Outline(OutDrawElements, L, AllottedGeometry, MX, MY + 1.f, 10.f, 10.f,
				FLinearColor(PA, PA, PA, 1.f));
			Txt(OutDrawElements, L, AllottedGeometry, TEXT("COMPROMISED"),
				{MX + 18.f, MY}, WMid(), FMeta());
		}
	}

	// ── BOTTOM RIGHT: stealth metric cards ───────────────────────────────────
	// Two floating metric cards — no panel background.
	// Each card: tiny section label → large value → wireframe bar.
	{
		constexpr float CardW = 130.f;
		constexpr float CardH = 76.f;
		constexpr float Gap   = 14.f;
		const float TotalW = CardW * 2.f + Gap;
		const float CX     = SW - TotalW - 28.f;
		const float CY     = SH - CardH - 56.f;

		MetricCard(OutDrawElements, L, AllottedGeometry,
			CX, CY, CardW, CardH,
			TEXT("VISIBILITY"),
			FString::Printf(TEXT("%2.0f%%"), Vis01 * 100.f),
			Vis01, PeakVis01);

		MetricCard(OutDrawElements, L, AllottedGeometry,
			CX + CardW + Gap, CY, CardW, CardH,
			TEXT("NOISE"),
			FString::Printf(TEXT("%2.0f%%"), Noise01 * 100.f),
			Noise01, PeakNoise01);

		// Thin horizontal rule separating cards from footer
		Box(OutDrawElements, L++, AllottedGeometry,
			{CX, CY + CardH + 7.f}, {TotalW, 1.f},
			FLinearColor(1.f, 1.f, 1.f, 0.08f));

		// Footer: stance · locomotion
		Txt(OutDrawElements, L, AllottedGeometry,
			FString::Printf(TEXT("%s  ·  %s"), *EStr(Mv.Stance), *EStr(Mv.Locomotion)),
			{CX, CY + CardH + 12.f}, WDim(), FLabel());

		// LIVE blink dot
		if (FMath::Frac(T * 1.4f) < 0.55f)
		{
			Box(OutDrawElements, L++, AllottedGeometry,
				{CX + TotalW - 7.f, CY + CardH + 14.f}, {5.f, 5.f},
				FLinearColor(0.80f, 0.80f, 0.80f, 1.f));
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

		const float DX  = SW - 350.f;
		const float DY0 = 80.f;
		constexpr float DW = 326.f;
		constexpr float DH = 360.f;

		Box(OutDrawElements, L++, AllottedGeometry, {DX, DY0}, {DW, DH},
			FLinearColor(0.f, 0.f, 0.f, 0.88f));
		Box(OutDrawElements, L++, AllottedGeometry, {DX + 1.f, DY0 + 1.f}, {DW - 2.f, 20.f},
			FLinearColor(0.06f, 0.06f, 0.06f, 0.95f));
		Box(OutDrawElements, L++, AllottedGeometry, {DX + 1.f, DY0 + 21.f}, {DW - 2.f, 1.f},
			FLinearColor(1.f, 1.f, 1.f, 0.08f));
		Txt(OutDrawElements, L, AllottedGeometry, TEXT("DIAGNOSTICS"),
			{DX + 10.f, DY0 + 5.f}, WDim(), FDiag());
		Brackets(OutDrawElements, L, AllottedGeometry, DW, DH, 10.f);

		auto Row = [&](const FString& S, float& DY,
			const FLinearColor& C = FLinearColor(0.42f, 0.42f, 0.42f, 1.f))
		{
			Txt(OutDrawElements, L, AllottedGeometry, S, {DX + 10.f, DY}, C, FDiag());
			DY += 13.f;
		};

		float DY = DY0 + 26.f;
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
			Row(PrimaryBrain->GetDebugBrainLine(), DY, SuspC);
		}
		Row(FString::Printf(TEXT("alarm=%s  outcome=%s"),
			Sim->GetAlarmState().bActive ? TEXT("ON") : TEXT("off"),
			*EStr(Sim->GetMissionOutcome())), DY);
	}

	return L;
}
