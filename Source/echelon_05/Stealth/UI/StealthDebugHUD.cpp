#include "Stealth/UI/StealthDebugHUD.h"

#include "Stealth/Actors/StealthGuard.h"
#include "Stealth/Subsystems/StealthSimulationSubsystem.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"

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
	const float LX = 20.f;
	const float RX = 420.f;
	float Y = 40.f;
	const float LH = 16.f;

	auto LineL = [&](const FString& S, const FLinearColor& C = FLinearColor::White)
	{
		DrawText(S, C, LX, Y, Font, 1.f, false);
		Y += LH;
	};
	float RY = 40.f;
	auto LineR = [&](const FString& S, const FLinearColor& C = FLinearColor::White)
	{
		DrawText(S, C, RX, RY, Font, 1.f, false);
		RY += LH;
	};

	const FStealthAlsDebugSnapshot Als = Sim->GetAlsDebugSnapshot();
	const FStealthMovementState Mv = Sim->GetPlayerMovement();
	const FVisibilityEmitter Vis = Sim->GetPlayerVisibility();
	const FSoundEmitter Snd = Sim->GetPlayerSoundEmission();

	LineL(TEXT("--- ALS facts ---"), FLinearColor::Yellow);
	LineL(FString::Printf(TEXT("StanceTag: %s"), *Als.AlsStance.ToString()));
	LineL(FString::Printf(TEXT("GaitTag: %s"), *Als.AlsGait.ToString()));
	LineL(FString::Printf(TEXT("LocMode: %s"), *Als.AlsLocomotionMode.ToString()));
	LineL(FString::Printf(TEXT("LocAction: %s"), *Als.AlsLocomotionAction.ToString()));
	LineL(FString::Printf(TEXT("RotMode: %s"), *Als.AlsRotationMode.ToString()));
	LineL(FString::Printf(TEXT("ViewMode: %s"), *Als.AlsViewMode.ToString()));
	LineL(FString::Printf(TEXT("Speed: %.1f | Moving: %d | HasInput: %d"), Als.Speed, Als.bMoving ? 1 : 0, Als.bHasInput ? 1 : 0));

	LineR(TEXT("--- Stealth ---"), FLinearColor::Yellow);
	LineR(FString::Printf(TEXT("Norm: stance=%d loc=%d"), static_cast<int32>(Mv.Stance), static_cast<int32>(Mv.Locomotion)));
	LineR(FString::Printf(TEXT("ViewYawRate: %.1f"), Sim->GetPlayerView().ViewYawSpeed));
	LineR(FString::Printf(TEXT("Visibility: %.2f (LExp %.2f x St %.2f x Mv %.2f x Act %.2f)"), Vis.CurrentVisibility,
		Vis.LightExposure, Vis.StanceMultiplier, Vis.MovementMultiplier, Vis.ActionMultiplier));
	LineR(FString::Printf(TEXT("Noise radius: %.0f"), Snd.Radius));

	const TArray<FStealthSoundEvent> Events = Sim->GetActiveSoundEvents();
	LineR(FString::Printf(TEXT("Sound events: %d"), Events.Num()));
	for (int32 i = 0; i < FMath::Min(3, Events.Num()); ++i)
	{
		LineR(FString::Printf(TEXT("  [%d] %s r=%.0f"), Events[i].EventId, *Events[i].DebugLabel, Events[i].Radius));
	}

	AStealthGuard* PrimaryGuard = nullptr;
	for (const TWeakObjectPtr<AStealthGuard>& G : Sim->GetRegisteredGuards())
	{
		if (G.IsValid())
		{
			PrimaryGuard = G.Get();
			break;
		}
	}
	LineR(TEXT("--- Guard ---"), FLinearColor::Yellow);
	if (PrimaryGuard)
	{
		LineR(PrimaryGuard->GetDebugBrainLine());
	}
	else
	{
		LineR(TEXT("Guard: --"));
	}

	LineR(FString::Printf(TEXT("Alarm: %s Lvl %.0f %s"), Sim->GetAlarmState().bActive ? TEXT("ON") : TEXT("off"),
		Sim->GetAlarmState().Level, *Sim->GetAlarmState().Reason));

	const FObjectiveState Obj = Sim->GetObjectiveState();
	const FExtractionState Ext = Sim->GetExtractionState();
	LineR(FString::Printf(TEXT("Objective: %s | Extraction: av=%s used=%s"),
		Obj.bCompleted ? TEXT("done") : TEXT("open"), Ext.bAvailable ? TEXT("Y") : TEXT("N"), Ext.bUsed ? TEXT("Y") : TEXT("N")));
	LineR(FString::Printf(TEXT("MissionOutcome: %d | Compromised: %s"), static_cast<int32>(Sim->GetMissionOutcome()),
		Sim->HasAlertOccurred() ? TEXT("Y") : TEXT("N")));
	LineR(TEXT("stealth.DebugDraw cvar: guard cone/hearing"));
}
