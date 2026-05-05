#include "Stealth/UI/StealthDebugHUD.h"

#include "Stealth/Components/StealthGuardBrainComponent.h"
#include "Stealth/Subsystems/StealthSimulationSubsystem.h"
#include "HAL/IConsoleManager.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"

namespace
{
constexpr float GLineH = 15.f;

// Splinter Cell aesthetic: near-black bg, desaturated military green, thin borders.
FLinearColor SCBg()    { return FLinearColor(0.008f, 0.018f, 0.012f, 0.62f); }
FLinearColor SCEdge()  { return FLinearColor(0.30f,  0.44f,  0.36f,  0.65f); }
FLinearColor SCGreen() { return FLinearColor(0.48f,  0.74f,  0.58f,  1.f);  }
FLinearColor SCAmber() { return FLinearColor(0.78f,  0.70f,  0.35f,  1.f);  }
FLinearColor SCWarn()  { return FLinearColor(0.84f,  0.56f,  0.28f,  1.f);  }
FLinearColor SCWhite() { return FLinearColor(0.88f,  0.90f,  0.86f,  1.f);  }
FLinearColor SCDim()   { return FLinearColor(0.32f,  0.40f,  0.35f,  1.f);  }

FLinearColor SCRamp(const float N)
{
	if (N >= 0.78f) return SCWarn();
	if (N >= 0.46f) return SCAmber();
	return SCGreen();
}

FLinearColor SCSuspColor(const EGuardSuspicionState S)
{
	switch (S)
	{
	case EGuardSuspicionState::Alert:          return SCWarn();
	case EGuardSuspicionState::Investigating:
	case EGuardSuspicionState::Suspicious:     return SCAmber();
	case EGuardSuspicionState::Curious:        return SCGreen();
	default:                                    return SCDim();
	}
}

template <typename TEnum>
FString SCEnumName(const TEnum V)
{
	if (const UEnum* E = StaticEnum<TEnum>())
	{
		return E->GetDisplayNameTextByValue(static_cast<int64>(V)).ToString().ToUpper();
	}
	return FString::FromInt(static_cast<int32>(V));
}

void SCText(AHUD& H, const FString& T, const float X, const float Y, const FLinearColor& C, UFont* F, const float S = 1.f)
{
	H.DrawText(T, FLinearColor(0.f, 0.f, 0.f, 0.55f), X + 1.f, Y + 1.f, F, S, false);
	H.DrawText(T, C, X, Y, F, S, false);
}

// Dark panel with single-pixel border on all four sides.
void SCPanel(AHUD& H, const float X, const float Y, const float W, const float Ht)
{
	const FLinearColor Bg = SCBg();
	const FLinearColor Ed = SCEdge();
	H.DrawRect(Bg, X, Y, W, Ht);
	H.DrawRect(Ed, X,           Y,            W,  1.f);
	H.DrawRect(Ed, X,           Y + Ht - 1.f, W,  1.f);
	H.DrawRect(Ed, X,           Y,            1.f, Ht);
	H.DrawRect(Ed, X + W - 1.f, Y,            1.f, Ht);
}

// Thin track-and-fill bar with no label.
void SCBar(AHUD& H, const float X, const float Y, const float W, const float Ht, const float N01, const FLinearColor& Fill)
{
	const FLinearColor Ed = SCEdge();
	H.DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.80f), X, Y, W, Ht);
	const float Clamped = FMath::Clamp(N01, 0.f, 1.f);
	if (Clamped > 0.f)
	{
		H.DrawRect(FLinearColor(Fill.R, Fill.G, Fill.B, 0.88f), X, Y, W * Clamped, Ht);
	}
	H.DrawRect(FLinearColor(Ed.R, Ed.G, Ed.B, 0.55f), X, Y,          W, 1.f);
	H.DrawRect(FLinearColor(Ed.R, Ed.G, Ed.B, 0.55f), X, Y + Ht - 1.f, W, 1.f);
}

void SCLine(AHUD& H, const FString& T, float& Y, const float X, UFont* F, const FLinearColor& C = FLinearColor(0.88f, 0.90f, 0.86f, 1.f))
{
	SCText(H, T, X, Y, C, F);
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

	UFont* Font  = GEngine->GetSmallFont();
	const float SW = Canvas->SizeX;
	const float SH = Canvas->SizeY;

	const FStealthMovementState Mv = Sim->GetPlayerMovement();
	const FVisibilityEmitter Vis   = Sim->GetPlayerVisibility();
	const FSoundEmitter Snd        = Sim->GetPlayerSoundEmission();
	const FObjectiveState Obj      = Sim->GetObjectiveState();
	const FExtractionState Ext     = Sim->GetExtractionState();
	const TArray<TWeakObjectPtr<UStealthGuardBrainComponent>> Brains = Sim->GetRegisteredGuardBrains();
	const UStealthGuardBrainComponent* PrimaryBrain = nullptr;
	for (const TWeakObjectPtr<UStealthGuardBrainComponent>& BPtr : Brains)
	{
		if (BPtr.IsValid())
		{
			PrimaryBrain = BPtr.Get();
			break;
		}
	}

	const FSuspicionState Suspicion = PrimaryBrain ? PrimaryBrain->Suspicion : FSuspicionState();
	const float Susp01   = FMath::Clamp(Suspicion.Value / 100.f, 0.f, 1.f);
	const float Vis01    = FMath::Clamp(Vis.CurrentVisibility, 0.f, 1.f);
	const float Noise01  = FMath::Clamp(Snd.Radius / 1400.f, 0.f, 1.f);
	const FLinearColor SuspColor = SCSuspColor(Suspicion.State);

	// ── TOP CENTER: guard detection bar — thin horizontal strip like SC ───────
	{
		const float BarW  = SW * 0.28f;
		const float BarX  = (SW - BarW) * 0.5f;
		const float BarY  = 20.f;
		const float BarHt = 6.f;

		// Outer shadow frame
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.60f), BarX - 1.f, BarY - 1.f, BarW + 2.f, BarHt + 2.f);
		// Fill proportional to suspicion
		if (Susp01 > 0.005f)
		{
			DrawRect(FLinearColor(SuspColor.R, SuspColor.G, SuspColor.B, 0.82f), BarX, BarY, BarW * Susp01, BarHt);
		}
		// 1px border
		const FLinearColor Ed = SCEdge();
		DrawRect(Ed, BarX,          BarY,           BarW, 1.f);
		DrawRect(Ed, BarX,          BarY + BarHt,   BarW, 1.f);
		DrawRect(Ed, BarX,          BarY,            1.f, BarHt);
		DrawRect(Ed, BarX + BarW,   BarY,            1.f, BarHt);

		// State label only when suspicious or higher
		const bool bShowLabel = Suspicion.State == EGuardSuspicionState::Suspicious
			|| Suspicion.State == EGuardSuspicionState::Investigating
			|| Suspicion.State == EGuardSuspicionState::Alert;
		if (bShowLabel)
		{
			SCText(*this, SCEnumName(Suspicion.State), BarX + BarW + 10.f, BarY - 1.f, SuspColor, Font, 0.88f);
		}
	}

	// ── BOTTOM RIGHT: player stealth readout — compact SC equipment panel ─────
	{
		const float PW   = 240.f;
		const float PHt  = 84.f;
		const float PX   = SW - PW - 20.f;
		const float PY   = SH - PHt - 20.f;
		SCPanel(*this, PX, PY, PW, PHt);

		const float IX = PX + 12.f;
		float IY = PY + 10.f;

		// VIS row: label, bar, percent
		const FLinearColor VC = SCRamp(Vis01);
		SCText(*this, TEXT("VIS"), IX, IY, SCDim(), Font, 0.85f);
		SCBar(*this, IX + 30.f, IY + 2.f, PW - 74.f, 8.f, Vis01, VC);
		SCText(*this, FString::Printf(TEXT("%3.0f%%"), Vis01 * 100.f), PX + PW - 40.f, IY, VC, Font, 0.85f);
		IY += 20.f;

		// NSE row
		const FLinearColor NC = SCRamp(Noise01);
		SCText(*this, TEXT("NSE"), IX, IY, SCDim(), Font, 0.85f);
		SCBar(*this, IX + 30.f, IY + 2.f, PW - 74.f, 8.f, Noise01, NC);
		SCText(*this, FString::Printf(TEXT("%3.0f%%"), Noise01 * 100.f), PX + PW - 40.f, IY, NC, Font, 0.85f);
		IY += 18.f;

		// Thin divider
		const FLinearColor Ed = SCEdge();
		DrawRect(FLinearColor(Ed.R, Ed.G, Ed.B, 0.40f), IX, IY, PW - 24.f, 1.f);
		IY += 8.f;

		// Stance | Locomotion
		SCText(*this, FString::Printf(TEXT("%s  |  %s"), *SCEnumName(Mv.Stance), *SCEnumName(Mv.Locomotion)), IX, IY, SCWhite(), Font, 0.88f);
	}

	// ── TOP LEFT: mission status — minimal flat text, no panel, SC-style ─────
	{
		const float MX = 20.f;
		float MY = 20.f;

		SCText(*this, FString::Printf(TEXT("OBJ  %s"), Obj.bCompleted ? TEXT("COMPLETE") : TEXT("ACTIVE")),
			MX, MY, Obj.bCompleted ? SCGreen() : SCDim(), Font, 0.85f);
		MY += 15.f;
		SCText(*this, FString::Printf(TEXT("EXT  %s"), Ext.bUsed ? TEXT("USED") : (Ext.bAvailable ? TEXT("AVAILABLE") : TEXT("LOCKED"))),
			MX, MY, Ext.bAvailable ? SCGreen() : SCDim(), Font, 0.85f);
		if (Sim->HasAlertOccurred())
		{
			MY += 15.f;
			SCText(*this, TEXT("COMPROMISED"), MX, MY, SCAmber(), Font, 0.85f);
		}
	}

	// ── RIGHT PANEL: diagnostics — only when stealth.DebugDraw > 0 ───────────
	int32 DebugDrawValue = 0;
	if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("stealth.DebugDraw")))
	{
		DebugDrawValue = CVar->GetInt();
	}

	if (DebugDrawValue > 0)
	{
		const FStealthAlsDebugSnapshot Als = Sim->GetAlsDebugSnapshot();
		const FStealthLightSamplingDebug Ls = Sim->GetLastLightSamplingDebug();
		const TArray<FStealthSoundEvent> Events = Sim->GetActiveSoundEvents();
		const float DX  = SW - 400.f;
		const float DY0 = 130.f;
		SCPanel(*this, DX, DY0, 376.f, 370.f);
		float DY = DY0 + 12.f;
		SCLine(*this, TEXT("DIAGNOSTICS"), DY, DX + 12.f, Font, SCDim());
		SCLine(*this, FString::Printf(TEXT("als stance=%s gait=%s"), *Als.AlsStance.ToString(), *Als.AlsGait.ToString()),
			DY, DX + 12.f, Font);
		SCLine(*this, FString::Printf(TEXT("speed=%.1f moving=%d input=%d"), Als.Speed, Als.bMoving ? 1 : 0, Als.bHasInput ? 1 : 0),
			DY, DX + 12.f, Font);
		SCLine(*this, FString::Printf(TEXT("locmode=%s action=%s"), *Als.AlsLocomotionMode.ToString(), *Als.AlsLocomotionAction.ToString()),
			DY, DX + 12.f, Font);
		DY += 4.f;
		SCLine(*this, FString::Printf(TEXT("vis=%.2f L=%.2f st=%.2f mv=%.2f act=%.2f"), Vis.CurrentVisibility,
			Vis.LightExposure, Vis.StanceMultiplier, Vis.MovementMultiplier, Vis.ActionMultiplier), DY, DX + 12.f, Font);
		SCLine(*this, FString::Printf(TEXT("scene final=%.2f max=%.2f lights=%d"), Ls.FinalExposure, Ls.RawMaxExposure, Ls.CachedLightCount),
			DY, DX + 12.f, Font);
		for (const FStealthBodyLightSampleDebug& Pt : Ls.BodySamples)
		{
			SCLine(*this, FString::Printf(TEXT("  %s: %.2f"), *Pt.SampleName, Pt.Exposure), DY, DX + 12.f, Font);
		}
		SCLine(*this, FString::Printf(TEXT("noise=%.0f sounds=%d"), Snd.Radius, Events.Num()), DY, DX + 12.f, Font);
		for (int32 i = 0; i < FMath::Min(3, Events.Num()); ++i)
		{
			SCLine(*this, FString::Printf(TEXT("  [%d] %s r=%.0f"), Events[i].EventId, *Events[i].DebugLabel, Events[i].Radius),
				DY, DX + 12.f, Font);
		}
		if (PrimaryBrain)
		{
			SCLine(*this, PrimaryBrain->GetDebugBrainLine(), DY, DX + 12.f, Font, SuspColor);
		}
		const FString AlarmLine = FString::Printf(TEXT("alarm=%s outcome=%s"),
			Sim->GetAlarmState().bActive ? TEXT("ON") : TEXT("off"), *SCEnumName(Sim->GetMissionOutcome()));
		SCLine(*this, AlarmLine, DY, DX + 12.f, Font);
	}
}
