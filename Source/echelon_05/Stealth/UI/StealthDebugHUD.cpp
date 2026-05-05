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
FLinearColor TBg()    { return FLinearColor(0.010f, 0.010f, 0.010f, 0.92f); }
FLinearColor TEdge()  { return FLinearColor(1.f,    1.f,    1.f,    0.20f); }
FLinearColor TDiv()   { return FLinearColor(1.f,    1.f,    1.f,    0.09f); }
FLinearColor TWhite() { return FLinearColor(0.94f,  0.94f,  0.94f,  1.f);  }
FLinearColor TGray()  { return FLinearColor(0.50f,  0.50f,  0.50f,  1.f);  }
FLinearColor TDim()   { return FLinearColor(0.26f,  0.26f,  0.26f,  1.f);  }
FLinearColor TGreen() { return FLinearColor(0.08f,  0.94f,  0.38f,  1.f);  }
FLinearColor TAmber() { return FLinearColor(0.98f,  0.64f,  0.00f,  1.f);  }
FLinearColor TRed()   { return FLinearColor(0.97f,  0.16f,  0.06f,  1.f);  }

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

// ── Drawing primitives ────────────────────────────────────────────────────────

void TText(AHUD& H, const FString& T, const float X, const float Y,
	const FLinearColor& C, UFont* F, const float S = 1.f)
{
	H.DrawText(T, FLinearColor(0.f, 0.f, 0.f, 0.65f), X + 1.f, Y + 1.f, F, S, false);
	H.DrawText(T, C, X, Y, F, S, false);
}

// 9×9 status indicator square — large enough to read at a glance.
void TDot(AHUD& H, const float X, const float Y, const FLinearColor& C)
{
	H.DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.70f), X + 1.f, Y + 1.f, 9.f, 9.f);
	H.DrawRect(C, X, Y, 9.f, 9.f);
}

// L-shaped corner brackets — the defining structural motif.
// Inner panels use full-width TDiv rules; the outer boundary uses bracket corners only.
void TBrackets(AHUD& H, const float X, const float Y, const float W, const float Ht,
	const float BLen = 18.f)
{
	const FLinearColor E = TEdge();
	// Top-left
	H.DrawRect(E, X,          Y,          BLen, 1.f);
	H.DrawRect(E, X,          Y,          1.f,  BLen);
	// Top-right
	H.DrawRect(E, X+W-BLen,   Y,          BLen, 1.f);
	H.DrawRect(E, X+W-1.f,    Y,          1.f,  BLen);
	// Bottom-left
	H.DrawRect(E, X,          Y+Ht-1.f,  BLen, 1.f);
	H.DrawRect(E, X,          Y+Ht-BLen, 1.f,  BLen);
	// Bottom-right
	H.DrawRect(E, X+W-BLen,   Y+Ht-1.f,  BLen, 1.f);
	H.DrawRect(E, X+W-1.f,    Y+Ht-BLen, 1.f,  BLen);
}

// Header row inside a panel: slightly raised bg + bottom separator.
void THeaderRow(AHUD& H, const float X, const float Y, const float W, const float Ht)
{
	H.DrawRect(FLinearColor(0.06f, 0.06f, 0.06f, 0.95f), X, Y, W, Ht);
	H.DrawRect(TDiv(), X, Y + Ht, W, 1.f);
}

// Progress bar: black track, colored fill, hairline top+bottom only.
void TBar(AHUD& H, const float X, const float Y, const float W, const float Ht,
	const float N01, const FLinearColor& Fill)
{
	H.DrawRect(FLinearColor(0.f, 0.f, 0.f, 1.f), X, Y, W, Ht);
	if (const float Cl = FMath::Clamp(N01, 0.f, 1.f); Cl > 0.f)
	{
		H.DrawRect(FLinearColor(Fill.R, Fill.G, Fill.B, 0.92f), X, Y, W * Cl, Ht);
	}
	H.DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.15f), X, Y,           W, 1.f);
	H.DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.15f), X, Y + Ht - 1.f, W, 1.f);
}

void TLine(AHUD& H, const FString& T, float& Y, const float X, UFont* F,
	const FLinearColor& C = FLinearColor(0.94f, 0.94f, 0.94f, 1.f))
{
	TText(H, T, X, Y, C, F);
	Y += GLineH;
}
}

void AStealthDebugHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas || !GetWorld())
	{
		return;
	}

	UStealthSimulationSubsystem* Sim = GetWorld()->GetSubsystem<UStealthSimulationSubsystem>();
	if (!Sim)
	{
		DrawText(FString(TEXT("StealthSim: --")), FLinearColor::Red, 20.f, 40.f, GEngine->GetSmallFont(), 1.f, false);
		return;
	}

	UFont* Font = GEngine->GetSmallFont();
	const float SW = Canvas->SizeX;
	const float SH = Canvas->SizeY;

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

	// ── TOP CENTER: detection bar ─────────────────────────────────────────────
	// Thin proportional fill strip.  Full border because this element stands alone.
	{
		const float BarW = SW * 0.30f;
		const float BarX = (SW - BarW) * 0.5f;
		const float BarY = 18.f;
		constexpr float BarH = 4.f;

		DrawRect(FLinearColor(0.f, 0.f, 0.f, 1.f), BarX, BarY, BarW, BarH);
		if (Susp01 > 0.005f)
		{
			DrawRect(FLinearColor(SuspColor.R, SuspColor.G, SuspColor.B, 0.94f),
				BarX, BarY, BarW * Susp01, BarH);
		}
		// Full hairline border
		DrawRect(TEdge(), BarX,        BarY,        BarW, 1.f);
		DrawRect(TEdge(), BarX,        BarY + BarH, BarW, 1.f);
		DrawRect(TEdge(), BarX,        BarY,         1.f, BarH);
		DrawRect(TEdge(), BarX + BarW, BarY,         1.f, BarH);

		// State label right of bar — only when elevated
		if (Suspicion.State >= EGuardSuspicionState::Suspicious)
		{
			TText(*this, EnumStr(Suspicion.State), BarX + BarW + 8.f, BarY - 2.f, SuspColor, Font, 0.88f);
		}
	}

	// ── BOTTOM RIGHT: stealth readout ─────────────────────────────────────────
	// Transit-board table: header row + data rows separated by full-width rules,
	// outer boundary defined by corner brackets only.
	{
		constexpr float PW   = 272.f;
		constexpr float RowH = 28.f;
		constexpr float HdrH = 24.f;
		// header + VIS + sep + NSE + sep + footer
		constexpr float PHt  = HdrH + 1.f + RowH + 1.f + RowH + 1.f + 26.f;
		const float PX = SW - PW - 20.f;
		const float PY = SH - PHt - 20.f;

		DrawRect(TBg(), PX, PY, PW, PHt);

		// Header row
		THeaderRow(*this, PX + 1.f, PY + 1.f, PW - 2.f, HdrH - 1.f);
		TText(*this, TEXT("STEALTH"), PX + PW - 60.f, PY + 7.f, TDim(), Font, 0.82f);

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
			TBar(*this, BarX, RY + 12.f, BarW, 5.f, Vis01, VC);
			TText(*this, FString::Printf(TEXT("%3.0f%%"), Vis01 * 100.f), ValX, RY + 9.f, TWhite(), Font, 0.88f);
		}
		RY += RowH;
		DrawRect(TDiv(), PX + 1.f, RY, PW - 2.f, 1.f);
		RY += 1.f;

		// NSE row
		{
			const FLinearColor NC = TRamp(Noise01);
			TDot(*this, DotX, RY + (RowH - 9.f) * 0.5f, NC);
			TText(*this, TEXT("NSE"), LblX, RY + 9.f, TGray(), Font, 0.88f);
			TBar(*this, BarX, RY + 12.f, BarW, 5.f, Noise01, NC);
			TText(*this, FString::Printf(TEXT("%3.0f%%"), Noise01 * 100.f), ValX, RY + 9.f, TWhite(), Font, 0.88f);
		}
		RY += RowH;
		DrawRect(TDiv(), PX + 1.f, RY, PW - 2.f, 1.f);
		RY += 1.f;

		// Footer row: stance / locomotion
		TText(*this,
			FString::Printf(TEXT("%s   %s"), *EnumStr(Mv.Stance), *EnumStr(Mv.Locomotion)),
			LblX, RY + 8.f, TGray(), Font, 0.88f);

		TBrackets(*this, PX, PY, PW, PHt, 18.f);
	}

	// ── TOP LEFT: mission status ──────────────────────────────────────────────
	// Same tabular structure as the stealth panel: header + data rows + brackets.
	{
		constexpr float PW   = 194.f;
		constexpr float RowH = 26.f;
		constexpr float HdrH = 24.f;
		const int32 ExtraRows = Sim->HasAlertOccurred() ? 1 : 0;
		// header + OBJ + sep + EXT + sep + (optional COMPROMISED)
		const float PHt = HdrH + 1.f + RowH + 1.f + RowH + (ExtraRows > 0 ? 1.f + RowH : 0.f);
		const float PX  = 20.f;
		const float PY  = 20.f;

		DrawRect(TBg(), PX, PY, PW, PHt);

		// Header row
		THeaderRow(*this, PX + 1.f, PY + 1.f, PW - 2.f, HdrH - 1.f);
		TText(*this, TEXT("MISSION"), PX + PW - 62.f, PY + 7.f, TDim(), Font, 0.82f);

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
				bDone ? TGreen() : TWhite(), Font, 0.88f);
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
			TText(*this, ExtStr, ValX, RY + 8.f, ExtC, Font, 0.88f);
		}
		RY += RowH;

		// COMPROMISED row — only appended when an alert has occurred
		if (ExtraRows > 0)
		{
			DrawRect(TDiv(), PX + 1.f, RY, PW - 2.f, 1.f);
			RY += 1.f;
			TDot(*this, DotX, RY + (RowH - 9.f) * 0.5f, TAmber());
			TText(*this, TEXT("COMPROMISED"), LblX, RY + 8.f, TAmber(), Font, 0.88f);
		}

		TBrackets(*this, PX, PY, PW, PHt, 14.f);
	}

	// ── RIGHT PANEL: diagnostics — only when stealth.DebugDraw > 0 ───────────
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
		const float DY0 = 100.f;
		DrawRect(TBg(), DX, DY0, 376.f, 370.f);
		TBrackets(*this, DX, DY0, 376.f, 370.f, 14.f);
		float DY = DY0 + 14.f;
		TLine(*this, TEXT("DIAGNOSTICS"), DY, DX + 14.f, Font, TDim());
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
