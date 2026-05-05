#include "Stealth/Actors/StealthLureActor.h"

#include "Stealth/Subsystems/StealthSimulationSubsystem.h"

AStealthLureActor::AStealthLureActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

bool AStealthLureActor::CanStealthInteract_Implementation(APawn* InteractingPawn)
{
	(void)InteractingPawn;
	return !bSingleUse || !bTriggered;
}

FText AStealthLureActor::GetStealthInteractionText_Implementation(APawn* InteractingPawn)
{
	(void)InteractingPawn;
	return NSLOCTEXT("StealthLureActor", "TriggerLureText", "Trigger Lure");
}

void AStealthLureActor::StealthInteract_Implementation(APawn* InteractingPawn)
{
	(void)InteractingPawn;
	TriggerLure();
}

void AStealthLureActor::TriggerLure(const float LoudnessScale)
{
	if (bSingleUse && bTriggered)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UStealthSimulationSubsystem* Sim = World->GetSubsystem<UStealthSimulationSubsystem>();
	if (!Sim)
	{
		return;
	}

	FStealthSoundEvent Ev;
	Ev.Position = GetActorLocation();
	Ev.Loudness = FMath::Max(0.1f, BaseLoudness * LoudnessScale);
	Ev.Radius = BaseRadius * LoudnessScale;
	Ev.SourceType = EStealthSoundSource::Lure;
	Ev.Lifetime = 2.f;
	Ev.DebugLabel = TEXT("Lure");
	Sim->PushSoundEvent(Ev);
	bTriggered = true;
}
