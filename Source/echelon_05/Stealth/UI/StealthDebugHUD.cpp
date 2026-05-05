#include "Stealth/UI/StealthDebugHUD.h"

#include "Stealth/Components/StealthGuardBrainComponent.h"
#include "Stealth/Subsystems/StealthSimulationSubsystem.h"
#include "HAL/IConsoleManager.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"

namespace
{
constexpr float GPanelAlpha = 0.72f;
constexpr float GLineHeight = 17.f;

FLinearColor WithAlpha(const FLinearColor& Color, const float Alpha)
{
	return FLinearColor(Color.R, Color.G, Color.B, Alpha);
}

FLinearColor DangerColor(const float Normalized)
{
	if (Normalized >= 0.82f)
	{
		return FLinearColor(1.f, 0.12f, 0.08f, 1.f);
	}
	if (Normalized >= 0.55f)
	{
		return FLinearColor(1.f, 0.72f, 0.12f, 1.f);
	}
	return FLinearColor(0.12f, 0.82f, 0.92f, 1.f);
}

FLinearColor GuardStateColor(const EGuardSuspicionState State)
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
FString EnumDisplayName(const TEnum Value)
{
	if (const UEnum* Enum = StaticEnum<TEnum>())
	{
		return Enum->GetDisplayNameTextByValue(static_cast<int64>(Value)).ToString();
	}
	return FString::FromInt(static_cast<int32>(Value));
}

void DrawShadowedText(AHUD& Hud, const FString& Text, const float X, const float Y, const FLinearColor& Color, UFont* Font,
	const float Scale = 1.f)
{
	Hud.DrawText(Text, FLinearColor(0.f, 0.f, 0.f, 0.85f), X + 1.f, Y + 1.f, Font, Scale, false);
	Hud.DrawText(Text, Color, X, Y, Font, Scale, false);
}

void DrawPanel(AHUD& Hud, const float X, const float Y, const float W, const float H, const FLinearColor& Accent)
{
	Hud.DrawRect(FLinearColor(0.005f, 0.015f, 0.02f, GPanelAlpha), X, Y, W, H);
	Hud.DrawRect(WithAlpha(Accent, 0.9f), X, Y, 3.f, H);
	Hud.DrawRect(WithAlpha(Accent, 0.35f), X, Y, W, 1.f);
	Hud.DrawRect(WithAlpha(Accent, 0.25f), X, Y + H - 1.f, W, 1.f);
}

void DrawMeter(AHUD& Hud, const FString& Label, const float Value, const float MaxValue, const float X, const float Y, const float W,
	const FLinearColor& FillColor, UFont* Font)
{
	const float ClampedMax = FMath::Max(MaxValue, 1.f);
	const float Normalized = FMath::Clamp(Value / ClampedMax, 0.f, 1.f);
	const float BarY = Y + 18.f;
	const float BarH = 12.f;

	DrawShadowedText(Hud, Label, X, Y, FLinearColor(0.72f, 0.9f, 1.f, 1.f), Font);
	Hud.DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.85f), X, BarY, W, BarH);
	Hud.DrawRect(WithAlpha(FillColor, 0.95f), X, BarY, W * Normalized, BarH);
	Hud.DrawRect(WithAlpha(FLinearColor::White, 0.14f), X, BarY, W, 1.f);
	DrawShadowedText(Hud, FString::Printf(TEXT("%.0f%%"), Normalized * 100.f), X + W + 8.f, Y + 11.f, FillColor, Font);
}

void DrawMiniLine(AHUD& Hud, const FString& Text, float& Y, const float X, UFont* Font, const FLinearColor& Color = FLinearColor::White)
{
	DrawShadowedText(Hud, Text, X, Y, Color, Font);
	Y += GLineHeight;
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
	const float ScreenW = Canvas->SizeX;
	const float ScreenH = Canvas->SizeY;

	const FStealthAlsDebugSnapshot Als = Sim->GetAlsDebugSnapshot();
	const FStealthMovementState Mv = Sim->GetPlayerMovement();
	const FVisibilityEmitter Vis = Sim->GetPlayerVisibility();
	const FSoundEmitter Snd = Sim->GetPlayerSoundEmission();
	const FObjectiveState Obj = Sim->GetObjectiveState();
	const FExtractionState Ext = Sim->GetExtractionState();
	const FStealthLightSamplingDebug Ls = Sim->GetLastLightSamplingDebug();
	const TArray<FStealthSoundEvent> Events = Sim->GetActiveSoundEvents();

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

	const float Visibility01 = FMath::Clamp(Vis.CurrentVisibility, 0.f, 1.f);
	const float Noise01 = FMath::Clamp(Snd.Radius / 1400.f, 0.f, 1.f);
	const FLinearColor VisibilityColor = DangerColor(Visibility01);
	const FLinearColor NoiseColor = DangerColor(Noise01);
	const FLinearColor TacticalBlue(0.08f, 0.52f, 0.68f, 1.f);

	// Player-facing stealth gauges: readable at a glance, with labels left in for prototype tuning.
	const float StealthX = 32.f;
	const float StealthY = ScreenH - 188.f;
	DrawPanel(*this, StealthX, StealthY, 360.f, 150.f, TacticalBlue);
	DrawShadowedText(*this, TEXT("STEALTH SUIT"), StealthX + 16.f, StealthY + 12.f, FLinearColor(0.72f, 0.95f, 1.f, 1.f), Font, 1.08f);
	DrawShadowedText(*this,
		FString::Printf(TEXT("%s  |  %s  |  %.0f cm/s"), *EnumDisplayName(Mv.Stance), *EnumDisplayName(Mv.Locomotion), Mv.Speed),
		StealthX + 16.f, StealthY + 34.f, FLinearColor(0.88f, 0.96f, 1.f, 1.f), Font);
	DrawMeter(*this, TEXT("VISIBILITY"), Visibility01, 1.f, StealthX + 16.f, StealthY + 58.f, 245.f, VisibilityColor, Font);
	DrawMeter(*this, TEXT("NOISE"), Snd.Radius, 1400.f, StealthX + 16.f, StealthY + 96.f, 245.f, NoiseColor, Font);
	DrawShadowedText(*this, FString::Printf(TEXT("Light %.0f%%   Sounds %d"), Vis.LightExposure * 100.f, Events.Num()),
		StealthX + 16.f, StealthY + 128.f, FLinearColor(0.62f, 0.82f, 0.9f, 1.f), Font);

	// Mission state stays persistent so the player knows what the current loop expects.
	const float MissionX = 32.f;
	const float MissionY = 38.f;
	DrawPanel(*this, MissionX, MissionY, 330.f, 104.f, FLinearColor(0.1f, 0.72f, 0.52f, 1.f));
	DrawShadowedText(*this, TEXT("MISSION"), MissionX + 16.f, MissionY + 12.f, FLinearColor(0.72f, 1.f, 0.88f, 1.f), Font, 1.08f);
	DrawShadowedText(*this, Obj.bCompleted ? TEXT("Objective: COMPLETE") : TEXT("Objective: recover intel"),
		MissionX + 16.f, MissionY + 38.f, Obj.bCompleted ? FLinearColor(0.22f, 1.f, 0.58f, 1.f) : FLinearColor::White, Font);
	DrawShadowedText(*this,
		Ext.bUsed ? TEXT("Extraction: USED") : (Ext.bAvailable ? TEXT("Extraction: AVAILABLE") : TEXT("Extraction: LOCKED")),
		MissionX + 16.f, MissionY + 58.f, Ext.bAvailable ? FLinearColor(0.25f, 0.92f, 1.f, 1.f) : FLinearColor(0.9f, 0.75f, 0.45f, 1.f), Font);
	DrawShadowedText(*this,
		FString::Printf(TEXT("Run: %s"), Sim->HasAlertOccurred() ? TEXT("compromised") : TEXT("clean")),
		MissionX + 16.f, MissionY + 78.f, Sim->HasAlertOccurred() ? FLinearColor(1.f, 0.62f, 0.18f, 1.f) : FLinearColor(0.58f, 0.95f, 1.f, 1.f), Font);

	int32 DebugDrawValue = 0;
	if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("stealth.DebugDraw")))
	{
		DebugDrawValue = CVar->GetInt();
	}

	if (DebugDrawValue > 0)
	{
		const float DebugX = ScreenW - 430.f;
		const float DebugY = 160.f;
		DrawPanel(*this, DebugX, DebugY, 400.f, 420.f, FLinearColor(0.75f, 0.82f, 1.f, 1.f));
		float DY = DebugY + 14.f;
		DrawMiniLine(*this, TEXT("STEALTH DIAGNOSTICS"), DY, DebugX + 16.f, Font, FLinearColor(0.78f, 0.86f, 1.f, 1.f));
		DrawMiniLine(*this, FString::Printf(TEXT("ALS stance=%s gait=%s speed=%.1f"), *Als.AlsStance.ToString(), *Als.AlsGait.ToString(), Als.Speed),
			DY, DebugX + 16.f, Font);
		DrawMiniLine(*this, FString::Printf(TEXT("LocMode=%s action=%s"), *Als.AlsLocomotionMode.ToString(), *Als.AlsLocomotionAction.ToString()),
			DY, DebugX + 16.f, Font);
		DrawMiniLine(*this, FString::Printf(TEXT("RotMode=%s view=%s yaw=%.1f"), *Als.AlsRotationMode.ToString(), *Als.AlsViewMode.ToString(),
			Sim->GetPlayerView().ViewYawSpeed), DY, DebugX + 16.f, Font);
		DrawMiniLine(*this, FString::Printf(TEXT("Moving=%d input=%d"), Als.bMoving ? 1 : 0, Als.bHasInput ? 1 : 0), DY, DebugX + 16.f, Font);
		DY += 4.f;
		DrawMiniLine(*this, FString::Printf(TEXT("Visibility %.2f = L %.2f x St %.2f x Mv %.2f x Act %.2f"), Vis.CurrentVisibility,
			Vis.LightExposure, Vis.StanceMultiplier, Vis.MovementMultiplier, Vis.ActionMultiplier), DY, DebugX + 16.f, Font);
		DrawMiniLine(*this, FString::Printf(TEXT("SceneLight final=%.2f max=%.2f smooth=%.2f lights=%d"), Ls.FinalExposure,
			Ls.RawMaxExposure, Ls.SmoothedExposure, Ls.CachedLightCount), DY, DebugX + 16.f, Font);
		for (const FStealthBodyLightSampleDebug& Pt : Ls.BodySamples)
		{
			DrawMiniLine(*this, FString::Printf(TEXT("  %s: %.2f"), *Pt.SampleName, Pt.Exposure), DY, DebugX + 16.f, Font);
		}
		DrawMiniLine(*this, FString::Printf(TEXT("Noise radius=%.0f active sounds=%d"), Snd.Radius, Events.Num()), DY, DebugX + 16.f, Font);
		for (int32 i = 0; i < FMath::Min(3, Events.Num()); ++i)
		{
			DrawMiniLine(*this, FString::Printf(TEXT("  [%d] %s r=%.0f"), Events[i].EventId, *Events[i].DebugLabel, Events[i].Radius),
				DY, DebugX + 16.f, Font);
		}
		if (PrimaryBrain)
		{
			const FLinearColor GuardColor = GuardStateColor(PrimaryBrain->Suspicion.State);
			DrawMiniLine(*this, PrimaryBrain->GetDebugBrainLine(), DY, DebugX + 16.f, Font, GuardColor);
		}
		DrawMiniLine(*this, FString::Printf(TEXT("Alarm: %s Lvl %.0f %s"), Sim->GetAlarmState().bActive ? TEXT("ON") : TEXT("off"),
			Sim->GetAlarmState().Level, *Sim->GetAlarmState().Reason), DY, DebugX + 16.f, Font);
		DrawMiniLine(*this, FString::Printf(TEXT("Outcome=%s DebugDraw=%d"), *EnumDisplayName(Sim->GetMissionOutcome()), DebugDrawValue),
			DY, DebugX + 16.f, Font);
	}
}
