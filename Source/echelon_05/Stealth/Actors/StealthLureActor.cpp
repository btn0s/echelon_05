#include "Stealth/Actors/StealthLureActor.h"

#include "Stealth/Subsystems/StealthSimulationSubsystem.h"

AStealthLureActor::AStealthLureActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AStealthLureActor::TriggerLure(const float LoudnessScale)
{
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
}
