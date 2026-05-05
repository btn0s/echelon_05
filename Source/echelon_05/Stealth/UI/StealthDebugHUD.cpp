#include "Stealth/UI/StealthDebugHUD.h"

#include "Stealth/Components/StealthGuardBrainComponent.h"
#include "Stealth/Subsystems/StealthSimulationSubsystem.h"
#include "HAL/IConsoleManager.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"

namespace
{
constexpr float GLineH = 15.f;

// ── Palette ───────────────────────────────────────────────────────────────────
FLinearColor TBg()     { return FLinearColor(0.010f, 0.010f, 0.010f, 0.92f); }
FLinearColor THdrBg()  { return FLinearColor(0.055f, 0.055f, 0.055f, 0.95f); }
FLinearColor TEdge()   { return FLinearColor(1.f,    1.f,    1.f,    0.20f); }
FLinearColor TDiv()    { return FLinearColor(1.f,    1.f,    1.f,    0.09f); }
FLinearColor TWhite()  { return FLinearColor(0.94f,  0.94f,  0.94f,  1.f);  }
FLinearColor TGray()   { return FLinearColor(0.50f,  0.50f,  0.50f,  1.f);  }
FLinearColor TDim()    { return FLinearColor(0.26f,  0.26f,  0.26f,  1.f);  }
FLinearColor TGreen()  { return FLinearColor(0.08f,  0.94f,  0.38f,  1.f);  }
FLinearColor TAmber()  { return FLinearColor(0.98f,  0.64f,  0.00f,  1.f);  }
FLinearColor TRed()    { return FLinearColor(0.97f,  0.16f,  0.06f,  1.f);  }

FLinearColor TRamp(const float N)
{
	if (N >= 0.78f) return TRed();
	if (N >= 0.46f) return TAmber();
	return TGreen();
}

FLinearColor TSuspColor(const EGuardSuspicionState S)
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
FString EnumStr(const TEnum V)
{
	if (const UEnum* E = StaticEnum<TEnum>())
	{
		return E->GetDisplayNameTextByValue(static_cast<int64>(V)).ToString().ToUpper();
	}
	return FString::FromInt(static_cast<int32>(V));
}

// ── Primitives ────────────────────────────────────────────────────────────────

void TText(AHUD& H, const FString& T, float X, float Y,
	const FLinearColor& C, UFont* F, float S = 1.f)
{
	H.DrawText(T, FLinearColor(0.f, 0.f, 0.f, 0.65f), X + 1.f, Y + 1.f, F, S, false);
	H.DrawText(T, C, X, Y, F, S, false);
}

// 9×9 status dot — colour carries state, size makes it readable at a glance.
void TDot(AHUD& H, float X, float Y, const FLinearColor& C)
{
	H.DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.70f), X + 1.f, Y + 1.f, 9.f, 9.f);
	H.DrawRect(C, X, Y, 9.f, 9.f);
}

// Header row: raised bg + bottom separator.
void THeaderRow(AHUD& H, float X, float Y, float W, float Ht)
{
	H.DrawRect(THdrBg(), X, Y, W, Ht);
	H.DrawRect(TDiv(), X, Y + Ht, W, 1.f);
}

// L-shaped corner brackets.  EdgeColor lets callers animate on state changes.
void TBrackets(AHUD& H, float X, float Y, float W, float Ht,
	const FLinearColor& EdgeColor, float BLen = 18.f)
{
	auto R = [&](float Rx, float Ry, float Rw, float Rh) { H.DrawRect(EdgeColor, Rx, Ry, Rw, Rh); };
	R(X,          Y,          BLen, 1.f );   R(X,          Y,          1.f,  BLen);
	R(X+W-BLen,   Y,          BLen, 1.f );   R(X+W-1.f,    Y,          1.f,  BLen);
	R(X,          Y+Ht-1.f,  BLen, 1.f );   R(X,          Y+Ht-BLen, 1.f,  BLen);
	R(X+W-BLen,   Y+Ht-1.f,  BLen, 1.f );   R(X+W-1.f,    Y+Ht-BLen, 1.f,  BLen);
}

// ── Detection bar (segmented, state-aware) ────────────────────────────────────
// The bar is divided into four zones matching guard suspicion state thresholds.
// Each zone fills with its state colour as suspicion climbs through it, so the
// player sees which state the guard is entering, not just a raw percentage.
void TSegBar(AHUD& H, float X, float Y, float W, float Ht, float N01)
{
	// Default thresholds (match StealthGuardBrainComponent defaults)
	static constexpr float Thresh[4] = { 0.20f, 0.45f, 0.70f, 1.00f };
	// Zone colour = state the guard enters when fill reaches that zone
	const FLinearColor ZoneColor[4] = {
		TDim(),    // 0–20 %  : guard barely aware
		TGreen(),  // 20–45 % : Curious
		TAmber(),  // 45–70 % : Suspicious
		TRed(),    // 70–100%  : Investigating → Alert
	};
	constexpr float Gap  = 3.f;
	constexpr int   NSeg = 4;
	const float ContentW = W - Gap * (NSeg - 1);

	float CurX = X;
	float PrevT = 0.f;

	for (int32 i = 0; i < NSeg; ++i)
	{
		const float SegRange = Thresh[i] - PrevT;
		const float SegW     = ContentW * SegRange;
		const FLinearColor& SC = ZoneColor[i];

		// Dark unfilled track (8 % of segment colour so the zone tint is visible)
		H.DrawRect(FLinearColor(SC.R * 0.10f, SC.G * 0.10f, SC.B * 0.10f, 0.85f),
			CurX, Y, SegW, Ht);
		// Coloured fill
		const float Frac = FMath::Clamp((N01 - PrevT) / SegRange, 0.f, 1.f);
		if (Frac > 0.f)
		{
			H.DrawRect(FLinearColor(SC.R, SC.G, SC.B, 0.92f), CurX, Y, SegW * Frac, Ht);
		}
		// Hairline top + bottom
		H.DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.14f), CurX, Y,           SegW, 1.f);
		H.DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.14f), CurX, Y + Ht - 1.f, SegW, 1.f);

		CurX += SegW + Gap;
		PrevT = Thresh[i];
	}
}

// ── Analog bar (VIS / NSE): continuous fill + 25% tick marks + peak mark ─────
void TAnalogBar(AHUD& H, float X, float Y, float W, float Ht,
	float N01, const FLinearColor& Fill, float PeakN01 = -1.f)
{
	// Track
	H.DrawRect(FLinearColor(0.f, 0.f, 0.f, 1.f), X, Y, W, Ht);
	// Fill
	if (const float Cl = FMath::Clamp(N01, 0.f, 1.f); Cl > 0.f)
	{
		H.DrawRect(FLinearColor(Fill.R, Fill.G, Fill.B, 0.92f), X, Y, W * Cl, Ht);
	}
	// 25 % / 50 % / 75 % dividers — faint black ticks overlaid on fill
	static constexpr float TickFracs[] = { 0.25f, 0.50f, 0.75f };
	for (const float Frac : TickFracs)
	{
		H.DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.45f), X + W * Frac, Y, 1.f, Ht);
	}
	// Session-peak high-water mark (white 2px tick)
	if (PeakN01 > N01 + 0.02f && PeakN01 > 0.f)
	{
		const float PX = X + W * FMath::Clamp(PeakN01, 0.f, 1.f);
		H.DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.60f), PX - 1.f, Y, 2.f, Ht);
	}
	// Hairline top + bottom
	H.DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.14f), X, Y,           W, 1.f);
	H.DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.14f), X, Y + Ht - 1.f, W, 1.f);
}

void TLine(AHUD& H, const FString& T, float& Y, float X, UFont* F,
	const FLinearColor& C = FLinearColor(0.94f, 0.94f, 0.94f, 1.f))
{
	TText(H, T, X, Y, C, F);
	Y += GLineH;
}
}

// ─────────────────────────────────────────────────────────────────────────────

void AStealthDebugHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas || !GetWorld()) { return; }

	UStealthSimulationSubsystem* Sim = GetWorld()->GetSubsystem<UStealthSimulationSubsystem>();
	if (!Sim)
	{
		DrawText(TEXT("StealthSim: --"), FLinearColor::Red, 20.f, 40.f, GEngine->GetSmallFont(), 1.f, false);
		return;
	}

	UFont*      Font = GEngine->GetSmallFont();
	const float SW   = Canvas->SizeX;
	const float SH   = Canvas->SizeY;
	const float T    = GetWorld()->GetTimeSeconds();   // drives all animations

	// ── Data fetch ────────────────────────────────────────────────────────────
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
	const float Susp01   = FMath::Clamp(Suspicion.Value / 100.f, 0.f, 1.f);
	const float Vis01    = FMath::Clamp(Vis.CurrentVisibility, 0.f, 1.f);
	const float Noise01  = FMath::Clamp(Snd.Radius / 1400.f, 0.f, 1.f);
	const FLinearColor SuspColor = TSuspColor(Suspicion.State);

	// Track session peaks for high-water marks on analog bars
	PeakVis01   = FMath::Max(PeakVis01,   Vis01);
	PeakNoise01 = FMath::Max(PeakNoise01, Noise01);

	// ── SCREEN EDGE VIGNETTE — state-reactive ─────────────────────────────────
	// At Alert: pulsing red edges.  At Investigating: faint static amber.
	// This is the earliest visual signal that something is wrong.
	if (Suspicion.State == EGuardSuspicionState::Alert)
	{
		const float VigA = FMath::Sin(T * 4.5f) * 0.10f + 0.14f;
		const FLinearColor VC(TRed().R, TRed().G, TRed().B, VigA);
		constexpr float VW = 8.f;
		DrawRect(VC, 0.f,     0.f,     SW, VW);
		DrawRect(VC, 0.f,     SH - VW, SW, VW);
		DrawRect(VC, 0.f,     0.f,     VW, SH);
		DrawRect(VC, SW - VW, 0.f,     VW, SH);
	}
	else if (Suspicion.State == EGuardSuspicionState::Investigating)
	{
		const FLinearColor VC(TAmber().R, TAmber().G, TAmber().B, 0.06f);
		constexpr float VW = 5.f;
		DrawRect(VC, 0.f,     0.f,     SW, VW);
		DrawRect(VC, 0.f,     SH - VW, SW, VW);
		DrawRect(VC, 0.f,     0.f,     VW, SH);
		DrawRect(VC, SW - VW, 0.f,     VW, SH);
	}

	// ── TOP CENTER: segmented detection bar ───────────────────────────────────
	// Four zones colour-coded to guard suspicion states.  The partially-filled
	// zone immediately tells the player which state the guard is entering.
	{
		const float BarW = SW * 0.32f;
		const float BarX = (SW - BarW) * 0.5f;
		const float BarY = 20.f;
		constexpr float BarH = 6.f;

		TSegBar(*this, BarX, BarY, BarW, BarH, Susp01);

		// Full hairline border around the whole composite bar
		DrawRect(TEdge(), BarX,        BarY,        BarW, 1.f);
		DrawRect(TEdge(), BarX,        BarY + BarH, BarW, 1.f);
		DrawRect(TEdge(), BarX,        BarY,         1.f, BarH);
		DrawRect(TEdge(), BarX + BarW, BarY,         1.f, BarH);

		// Percentage — right of bar, small, always visible when any fill
		if (Susp01 > 0.01f)
		{
			TText(*this, FString::Printf(TEXT("%2.0f%%"), Susp01 * 100.f),
				BarX + BarW + 8.f, BarY - 1.f, SuspColor, Font, 0.88f);
		}

		// State name — right of bar, only at Suspicious+
		if (Suspicion.State >= EGuardSuspicionState::Suspicious)
		{
			TText(*this, EnumStr(Suspicion.State),
				BarX + BarW + 44.f, BarY - 1.f, SuspColor, Font, 0.88f);
		}

		// Caption below bar when guard is completely unaware
		if (Susp01 < 0.01f)
		{
			TText(*this, TEXT("DETECTION"), BarX + BarW * 0.5f - 30.f, BarY + BarH + 4.f, TDim(), Font, 0.78f);
		}
	}

	// ── BOTTOM RIGHT: stealth readout ─────────────────────────────────────────
	{
		constexpr float PW   = 272.f;
		constexpr float RowH = 28.f;
		constexpr float HdrH = 24.f;
		constexpr float PHt  = HdrH + 1.f + RowH + 1.f + RowH + 1.f + 26.f;
		const float PX = SW - PW - 20.f;
		const float PY = SH - PHt - 20.f;

		DrawRect(TBg(), PX, PY, PW, PHt);
		THeaderRow(*this, PX + 1.f, PY + 1.f, PW - 2.f, HdrH - 1.f);

		// "LIVE" blinking dot in header — shows the system is ticking
		if (FMath::Frac(T * 1.4f) < 0.55f)
		{
			DrawRect(TGreen(), PX + 12.f, PY + 9.f, 5.f, 5.f);
		}
		TText(*this, TEXT("STEALTH"), PX + PW - 62.f, PY + 7.f, TDim(), Font, 0.82f);

		// Bracket colour reacts to suspicion state — pulses at Alert
		const FLinearColor BracketC = [&]() -> FLinearColor {
			if (Suspicion.State == EGuardSuspicionState::Alert)
			{
				const float A = FMath::Sin(T * 4.5f) * 0.35f + 0.50f;
				return FLinearColor(TRed().R, TRed().G, TRed().B, A);
			}
			if (Suspicion.State >= EGuardSuspicionState::Suspicious)
			{
				return FLinearColor(SuspColor.R, SuspColor.G, SuspColor.B, 0.55f);
			}
			return TEdge();
		}();

		// Column anchors
		const float DotX = PX + 10.f;
		const float LblX = PX + 24.f;
		const float BarX = PX + 54.f;
		const float BarW = PW - 102.f;
		const float ValX = PX + PW - 44.f;

		// VIS row
		float RY = PY + HdrH + 1.f;
		{
			const FLinearColor VC = TRamp(Vis01);
			TDot(*this, DotX, RY + (RowH - 9.f) * 0.5f, VC);
			TText(*this, TEXT("VIS"), LblX, RY + 9.f, TGray(), Font, 0.88f);
			TAnalogBar(*this, BarX, RY + 12.f, BarW, 5.f, Vis01, VC, PeakVis01);
			TText(*this, FString::Printf(TEXT("%3.0f%%"), Vis01 * 100.f), ValX, RY + 9.f, TWhite(), Font, 1.0f);
		}
		RY += RowH;
		DrawRect(TDiv(), PX + 1.f, RY, PW - 2.f, 1.f);
		RY += 1.f;

		// NSE row
		{
			const FLinearColor NC = TRamp(Noise01);
			TDot(*this, DotX, RY + (RowH - 9.f) * 0.5f, NC);
			TText(*this, TEXT("NSE"), LblX, RY + 9.f, TGray(), Font, 0.88f);
			TAnalogBar(*this, BarX, RY + 12.f, BarW, 5.f, Noise01, NC, PeakNoise01);
			TText(*this, FString::Printf(TEXT("%3.0f%%"), Noise01 * 100.f), ValX, RY + 9.f, TWhite(), Font, 1.0f);
		}
		RY += RowH;
		DrawRect(TDiv(), PX + 1.f, RY, PW - 2.f, 1.f);
		RY += 1.f;

		// Footer: stance / locomotion
		TText(*this,
			FString::Printf(TEXT("%s   %s"), *EnumStr(Mv.Stance), *EnumStr(Mv.Locomotion)),
			LblX, RY + 8.f, TGray(), Font, 0.88f);

		TBrackets(*this, PX, PY, PW, PHt, BracketC, 18.f);
	}

	// ── TOP LEFT: mission status ──────────────────────────────────────────────
	{
		constexpr float PW   = 194.f;
		constexpr float RowH = 26.f;
		constexpr float HdrH = 24.f;
		const bool bCompromised = Sim->HasAlertOccurred();
		const float PHt = HdrH + 1.f + RowH + 1.f + RowH + (bCompromised ? 1.f + RowH : 0.f);
		const float PX  = 20.f;
		const float PY  = 20.f;

		DrawRect(TBg(), PX, PY, PW, PHt);
		THeaderRow(*this, PX + 1.f, PY + 1.f, PW - 2.f, HdrH - 1.f);
		TText(*this, TEXT("MISSION"), PX + PW - 64.f, PY + 7.f, TDim(), Font, 0.82f);

		const float DotX = PX + 10.f;
		const float LblX = PX + 24.f;
		const float ValX = PX + 58.f;
		float RY = PY + HdrH + 1.f;

		// OBJ row
		{
			const bool bDone = Obj.bCompleted;
			TDot(*this, DotX, RY + (RowH - 9.f) * 0.5f, bDone ? TGreen() : TDim());
			TText(*this, TEXT("OBJ"), LblX, RY + 8.f, TGray(), Font, 0.88f);
			TText(*this, bDone ? TEXT("COMPLETE") : TEXT("ACTIVE"), ValX, RY + 8.f,
				bDone ? TGreen() : TWhite(), Font, 1.0f);
		}
		RY += RowH;
		DrawRect(TDiv(), PX + 1.f, RY, PW - 2.f, 1.f);
		RY += 1.f;

		// EXT row
		{
			const bool bUsed  = Ext.bUsed;
			const bool bAvail = Ext.bAvailable;
			const FString ExtStr = bUsed ? TEXT("USED") : (bAvail ? TEXT("AVAILABLE") : TEXT("LOCKED"));
			const FLinearColor ExtC = bUsed ? TGreen() : (bAvail ? TWhite() : TDim());
			TDot(*this, DotX, RY + (RowH - 9.f) * 0.5f, bAvail && !bUsed ? TGreen() : TDim());
			TText(*this, TEXT("EXT"), LblX, RY + 8.f, TGray(), Font, 0.88f);
			TText(*this, ExtStr, ValX, RY + 8.f, ExtC, Font, 1.0f);
		}
		RY += RowH;

		// COMPROMISED row
		if (bCompromised)
		{
			DrawRect(TDiv(), PX + 1.f, RY, PW - 2.f, 1.f);
			RY += 1.f;
			// Pulse amber dot when compromised
			const float DotA = FMath::Sin(T * 2.f) * 0.4f + 0.6f;
			const FLinearColor PulsedAmber(TAmber().R, TAmber().G, TAmber().B, DotA);
			TDot(*this, DotX, RY + (RowH - 9.f) * 0.5f, PulsedAmber);
			TText(*this, TEXT("COMPROMISED"), LblX, RY + 8.f, TAmber(), Font, 0.88f);
		}

		// Mission bracket colour: amber when compromised
		const FLinearColor MBracket = bCompromised
			? FLinearColor(TAmber().R, TAmber().G, TAmber().B, 0.50f)
			: TEdge();
		TBrackets(*this, PX, PY, PW, PHt, MBracket, 14.f);
	}

	// ── RIGHT: diagnostics — only when stealth.DebugDraw > 0 ─────────────────
	int32 DebugDrawValue = 0;
	if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("stealth.DebugDraw")))
	{
		DebugDrawValue = CVar->GetInt();
	}

	if (DebugDrawValue > 0)
	{
		const FStealthAlsDebugSnapshot   Als    = Sim->GetAlsDebugSnapshot();
		const FStealthLightSamplingDebug Ls     = Sim->GetLastLightSamplingDebug();
		const TArray<FStealthSoundEvent> Events = Sim->GetActiveSoundEvents();

		const float DX  = SW - 400.f;
		const float DY0 = 80.f;
		DrawRect(TBg(), DX, DY0, 376.f, 380.f);
		THeaderRow(*this, DX + 1.f, DY0 + 1.f, 374.f, 22.f);
		TText(*this, TEXT("DIAGNOSTICS"), DX + 14.f, DY0 + 5.f, TDim(), Font, 0.82f);
		TBrackets(*this, DX, DY0, 376.f, 380.f, TEdge(), 14.f);

		float DY = DY0 + 28.f;
		TLine(*this, FString::Printf(TEXT("als stance=%s gait=%s"), *Als.AlsStance.ToString(), *Als.AlsGait.ToString()),
			DY, DX + 14.f, Font, TGray());
		TLine(*this, FString::Printf(TEXT("speed=%.1f moving=%d input=%d"), Als.Speed, Als.bMoving ? 1 : 0, Als.bHasInput ? 1 : 0),
			DY, DX + 14.f, Font, TGray());
		TLine(*this, FString::Printf(TEXT("locmode=%s action=%s"), *Als.AlsLocomotionMode.ToString(), *Als.AlsLocomotionAction.ToString()),
			DY, DX + 14.f, Font, TGray());
		DY += 4.f;
		TLine(*this, FString::Printf(TEXT("vis=%.2f L=%.2f st=%.2f mv=%.2f act=%.2f"),
			Vis.CurrentVisibility, Vis.LightExposure, Vis.StanceMultiplier, Vis.MovementMultiplier, Vis.ActionMultiplier),
			DY, DX + 14.f, Font, TGray());
		TLine(*this, FString::Printf(TEXT("scene final=%.2f max=%.2f lights=%d"),
			Ls.FinalExposure, Ls.RawMaxExposure, Ls.CachedLightCount),
			DY, DX + 14.f, Font, TGray());
		for (const FStealthBodyLightSampleDebug& Pt : Ls.BodySamples)
		{
			TLine(*this, FString::Printf(TEXT("  %s: %.2f"), *Pt.SampleName, Pt.Exposure), DY, DX + 14.f, Font, TGray());
		}
		TLine(*this, FString::Printf(TEXT("noise=%.0f sounds=%d"), Snd.Radius, Events.Num()), DY, DX + 14.f, Font, TGray());
		for (int32 i = 0; i < FMath::Min(3, Events.Num()); ++i)
		{
			TLine(*this, FString::Printf(TEXT("  [%d] %s r=%.0f"), Events[i].EventId, *Events[i].DebugLabel, Events[i].Radius),
				DY, DX + 14.f, Font, TGray());
		}
		if (PrimaryBrain)
		{
			TLine(*this, PrimaryBrain->GetDebugBrainLine(), DY, DX + 14.f, Font, SuspColor);
		}
		const FString AlarmLine = FString::Printf(TEXT("alarm=%s outcome=%s"),
			Sim->GetAlarmState().bActive ? TEXT("ON") : TEXT("off"),
			*EnumStr(Sim->GetMissionOutcome()));
		TLine(*this, AlarmLine, DY, DX + 14.f, Font, TGray());
	}
}
