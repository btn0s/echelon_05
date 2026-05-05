#include "Stealth/Subsystems/StealthSimulationSubsystem.h"

#include "Stealth/Actors/StealthLightVolume.h"
#include "Stealth/Components/StealthGuardBrainComponent.h"
#include "Stealth/Data/StealthTuningDataAsset.h"
#include "Stealth/StealthLog.h"

void UStealthSimulationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UWorld* World = GetWorld();
	if (!TuningAsset && World)
	{
		TuningAsset = NewObject<UStealthTuningDataAsset>(World);
	}

	ResetSimulation();
}

void UStealthSimulationSubsystem::Deinitialize()
{
	LightVolumes.Reset();
	GuardBrains.Reset();
	ActiveSoundEvents.Reset();
	Super::Deinitialize();
}

TStatId UStealthSimulationSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UStealthSimulationSubsystem, STATGROUP_Tickables);
}

void UStealthSimulationSubsystem::Tick(float DeltaTime)
{
	if (!GetWorld())
	{
		return;
	}

	ExpireSoundEvents(GetWorld()->GetTimeSeconds());
	RefreshObjectiveExtractionGating();
}

void UStealthSimulationSubsystem::ResetSimulation()
{
	PlayerMovement = FStealthMovementState();
	PlayerView = FStealthViewState();
	PlayerBody = FStealthBodyState();
	AlsDebugSnapshot = FStealthAlsDebugSnapshot();
	PlayerVisibility = FVisibilityEmitter();
	PlayerSoundEmission = FSoundEmitter();
	ActiveSoundEvents.Reset();
	NextSoundEventId = 1;
	ObjectiveState = FObjectiveState();
	ExtractionState = FExtractionState();
	AlarmState = FAlarmState();
	bAlertOccurred = false;
	MissionOutcome = EStealthMissionOutcome::None;
}

void UStealthSimulationSubsystem::SetTuningAsset(UStealthTuningDataAsset* InTuning)
{
	TuningAsset = InTuning;
}

void UStealthSimulationSubsystem::SetPlayerSnapshot(const FStealthMovementState& Movement, const FStealthViewState& View,
	const FStealthBodyState& Body, const FStealthAlsDebugSnapshot& AlsDebug)
{
	PlayerMovement = Movement;
	PlayerView = View;
	PlayerBody = Body;
	AlsDebugSnapshot = AlsDebug;
	OnPlayerSnapshotUpdated.Broadcast();
}

void UStealthSimulationSubsystem::SetPlayerVisibilityEmission(const FVisibilityEmitter& Emitter)
{
	PlayerVisibility = Emitter;
}

void UStealthSimulationSubsystem::SetPlayerSoundEmission(const FSoundEmitter& Emitter)
{
	PlayerSoundEmission = Emitter;
}

int32 UStealthSimulationSubsystem::PushSoundEvent(const FStealthSoundEvent& Event)
{
	if (!GetWorld())
	{
		return -1;
	}

	FStealthSoundEvent Copy = Event;
	Copy.EventId = NextSoundEventId++;
	Copy.SpawnTime = GetWorld()->GetTimeSeconds();
	ActiveSoundEvents.Add(Copy);
	OnSoundEvent.Broadcast(Copy);
	return Copy.EventId;
}

void UStealthSimulationSubsystem::RegisterLightVolume(AStealthLightVolume* Volume)
{
	if (Volume)
	{
		LightVolumes.AddUnique(Volume);
	}
}

void UStealthSimulationSubsystem::UnregisterLightVolume(AStealthLightVolume* Volume)
{
	LightVolumes.Remove(Volume);
}

void UStealthSimulationSubsystem::RegisterGuardBrain(UStealthGuardBrainComponent* Brain)
{
	if (Brain)
	{
		GuardBrains.AddUnique(Brain);
	}
}

void UStealthSimulationSubsystem::UnregisterGuardBrain(UStealthGuardBrainComponent* Brain)
{
	GuardBrains.Remove(Brain);
}

void UStealthSimulationSubsystem::SetObjectiveState(const FObjectiveState& State)
{
	ObjectiveState = State;
	RefreshObjectiveExtractionGating();
}

void UStealthSimulationSubsystem::SetExtractionState(const FExtractionState& State)
{
	ExtractionState = State;
}

void UStealthSimulationSubsystem::SetAlarmState(const FAlarmState& State)
{
	AlarmState = State;
}

void UStealthSimulationSubsystem::MarkAlertOccurred()
{
	bAlertOccurred = true;
}

void UStealthSimulationSubsystem::SetMissionOutcome(EStealthMissionOutcome Outcome)
{
	MissionOutcome = Outcome;
}

float UStealthSimulationSubsystem::SampleLightExposureAt(const FVector& WorldLocation) const
{
	float DefaultExp = TuningAsset ? TuningAsset->DefaultLightExposureOutsideVolumes : 0.85f;

	bool bInsideAny = false;
	float Exposure = 0.f;
	for (const TWeakObjectPtr<AStealthLightVolume>& Ptr : LightVolumes)
	{
		if (const AStealthLightVolume* Vol = Ptr.Get())
		{
			if (Vol->EncompassesPoint(WorldLocation))
			{
				bInsideAny = true;
				Exposure = FMath::Max(Exposure, Vol->GetLightExposure());
			}
		}
	}
	if (!bInsideAny)
	{
		return DefaultExp;
	}
	return Exposure;
}

void UStealthSimulationSubsystem::ExpireSoundEvents(float WorldTimeSeconds)
{
	ActiveSoundEvents.RemoveAll([&](const FStealthSoundEvent& E)
	{
		return WorldTimeSeconds > E.SpawnTime + E.Lifetime;
	});
}

void UStealthSimulationSubsystem::RefreshObjectiveExtractionGating()
{
	FExtractionState Next = ExtractionState;
	Next.bAvailable = ObjectiveState.bCompleted;
	if (!ObjectiveState.bCompleted)
	{
		Next.bUsed = false;
	}
	ExtractionState = Next;
}
