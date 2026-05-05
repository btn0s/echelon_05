#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Stealth/Types/StealthTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"

#include "StealthSimulationSubsystem.generated.h"

class AStealthLightVolume;
class UStealthGuardBrainComponent;
class UStealthHealthComponent;
class UStealthTuningDataAsset;
class ULightComponentBase;

USTRUCT(BlueprintType)
struct FStealthAlsDebugSnapshot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	FGameplayTag AlsStance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	FGameplayTag AlsGait;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	FGameplayTag AlsLocomotionMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	FGameplayTag AlsLocomotionAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	FGameplayTag AlsRotationMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	FGameplayTag AlsViewMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	float Speed = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	bool bMoving = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	bool bHasInput = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStealthPlayerSnapshotUpdated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStealthSoundEvent, const FStealthSoundEvent&, Event);

UCLASS()
class UStealthSimulationSubsystem : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Always; }
	virtual bool IsTickableWhenPaused() const override { return false; }
	virtual bool IsTickableInEditor() const override { return false; }

	UFUNCTION(BlueprintCallable, Category = "Stealth")
	void ResetSimulation();

	UFUNCTION(BlueprintCallable, Category = "Stealth")
	void SetTuningAsset(UStealthTuningDataAsset* InTuning);

	UFUNCTION(BlueprintPure, Category = "Stealth")
	UStealthTuningDataAsset* GetTuningAsset() const { return TuningAsset; }

	void SetPlayerSnapshot(const FStealthMovementState& Movement, const FStealthViewState& View, const FStealthBodyState& Body,
		const FStealthAlsDebugSnapshot& AlsDebug);
	void SetPlayerVisibilityEmission(const FVisibilityEmitter& Emitter);
	void SetPlayerSoundEmission(const FSoundEmitter& Emitter);

	UFUNCTION(BlueprintCallable, Category = "Stealth")
	int32 PushSoundEvent(const FStealthSoundEvent& Event);

	UFUNCTION(BlueprintCallable, Category = "Stealth")
	void RegisterLightVolume(AStealthLightVolume* Volume);

	UFUNCTION(BlueprintCallable, Category = "Stealth")
	void UnregisterLightVolume(AStealthLightVolume* Volume);

	UFUNCTION(BlueprintCallable, Category = "Stealth")
	void RegisterGuardBrain(UStealthGuardBrainComponent* Brain);

	UFUNCTION(BlueprintCallable, Category = "Stealth")
	void UnregisterGuardBrain(UStealthGuardBrainComponent* Brain);

	UFUNCTION(BlueprintCallable, Category = "Stealth")
	void RegisterHealthComponent(UStealthHealthComponent* HealthComponent);

	UFUNCTION(BlueprintCallable, Category = "Stealth")
	void UnregisterHealthComponent(UStealthHealthComponent* HealthComponent);

	UFUNCTION(BlueprintCallable, Category = "Stealth")
	FName RegisterObjective(AActor* ObjectiveActor, FName ObjectiveId, FText DisplayName, bool bRequired);

	UFUNCTION(BlueprintCallable, Category = "Stealth")
	void UnregisterObjective(AActor* ObjectiveActor);

	UFUNCTION(BlueprintCallable, Category = "Stealth")
	bool CompleteObjective(AActor* ObjectiveActor, APawn* InstigatorPawn);

	UFUNCTION(BlueprintCallable, Category = "Stealth")
	bool CompleteObjectiveById(FName ObjectiveId, APawn* InstigatorPawn);

	UFUNCTION(BlueprintPure, Category = "Stealth")
	FStealthMovementState GetPlayerMovement() const { return PlayerMovement; }

	UFUNCTION(BlueprintPure, Category = "Stealth")
	FStealthViewState GetPlayerView() const { return PlayerView; }

	UFUNCTION(BlueprintPure, Category = "Stealth")
	FStealthBodyState GetPlayerBody() const { return PlayerBody; }

	UFUNCTION(BlueprintPure, Category = "Stealth")
	FStealthAlsDebugSnapshot GetAlsDebugSnapshot() const { return AlsDebugSnapshot; }

	UFUNCTION(BlueprintPure, Category = "Stealth")
	FVisibilityEmitter GetPlayerVisibility() const { return PlayerVisibility; }

	UFUNCTION(BlueprintPure, Category = "Stealth")
	FSoundEmitter GetPlayerSoundEmission() const { return PlayerSoundEmission; }

	UFUNCTION(BlueprintPure, Category = "Stealth")
	TArray<FStealthSoundEvent> GetActiveSoundEvents() const { return ActiveSoundEvents; }

	UFUNCTION(BlueprintPure, Category = "Stealth")
	FObjectiveState GetObjectiveState() const { return ObjectiveState; }

	UFUNCTION(BlueprintPure, Category = "Stealth")
	TArray<FStealthObjectiveRecord> GetObjectiveRecords() const { return ObjectiveRecords; }

	UFUNCTION(BlueprintPure, Category = "Stealth")
	bool AreRequiredObjectivesComplete() const;

	UFUNCTION(BlueprintPure, Category = "Stealth")
	int32 GetRequiredObjectiveCount() const;

	UFUNCTION(BlueprintPure, Category = "Stealth")
	int32 GetCompletedRequiredObjectiveCount() const;

	UFUNCTION(BlueprintPure, Category = "Stealth")
	FExtractionState GetExtractionState() const { return ExtractionState; }

	UFUNCTION(BlueprintPure, Category = "Stealth")
	FAlarmState GetAlarmState() const { return AlarmState; }

	UFUNCTION(BlueprintCallable, Category = "Stealth")
	void SetObjectiveState(const FObjectiveState& State);

	UFUNCTION(BlueprintCallable, Category = "Stealth")
	void SetExtractionState(const FExtractionState& State);

	UFUNCTION(BlueprintCallable, Category = "Stealth")
	void SetAlarmState(const FAlarmState& State);

	UFUNCTION(BlueprintCallable, Category = "Stealth")
	void MarkAlertOccurred();

	UFUNCTION(BlueprintPure, Category = "Stealth")
	bool HasAlertOccurred() const { return bAlertOccurred; }

	UFUNCTION(BlueprintPure, Category = "Stealth")
	EStealthMissionOutcome GetMissionOutcome() const { return MissionOutcome; }

	UFUNCTION(BlueprintCallable, Category = "Stealth")
	void SetMissionOutcome(EStealthMissionOutcome Outcome);

	/** Scene-light sampling at one world position (no stealth volumes). Legacy helper / debug. */
	float SampleLightExposureAt(const FVector& WorldLocation, const AActor* OcclusionIgnoreActor = nullptr);

	/** TH-517: sample multiple body points, max-bias blend, temporal smoothing; fills LastLightSamplingDebug. */
	float SampleBodyLightExposureMaxBias(const TArray<FVector>& BodyWorldPositions, const TArray<FString>& BodyLabels,
		const AActor* OcclusionIgnoreActor, float DeltaTime);

	/** Sample body points without mutating player smoothing or HUD debug state. Intended for automated level probes. */
	float SampleBodyLightExposureRaw(const TArray<FVector>& BodyWorldPositions, const TArray<FString>& BodyLabels,
		const AActor* OcclusionIgnoreActor, FStealthLightSamplingDebug* OutDebug = nullptr);

	UFUNCTION(BlueprintPure, Category = "Stealth")
	FStealthLightSamplingDebug GetLastLightSamplingDebug() const { return LastLightSamplingDebug; }

	TArray<TWeakObjectPtr<AStealthLightVolume>> GetRegisteredLightVolumes() const { return LightVolumes; }

	TArray<TWeakObjectPtr<UStealthGuardBrainComponent>> GetRegisteredGuardBrains() const { return GuardBrains; }

	UFUNCTION(BlueprintPure, Category = "Stealth")
	FStealthHealthState GetPlayerHealthState() const;

	TArray<TWeakObjectPtr<UStealthHealthComponent>> GetRegisteredHealthComponents() const { return HealthComponents; }

	UPROPERTY(BlueprintAssignable, Category = "Stealth")
	FOnStealthPlayerSnapshotUpdated OnPlayerSnapshotUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Stealth")
	FOnStealthSoundEvent OnSoundEvent;

private:
	void ExpireSoundEvents(float WorldTimeSeconds);
	void RefreshObjectiveExtractionGating();
	void RecomputeObjectiveStateFromRecords();
	int32 FindObjectiveIndexByActor(const AActor* ObjectiveActor) const;
	int32 FindObjectiveIndexById(FName ObjectiveId) const;
	void RefreshSceneLightCache(float WorldTimeSeconds);
	float ComputeSceneLightExposureAt(const FVector& SampleWorldPosition, const AActor* OcclusionIgnoreActor,
		TArray<FStealthLightContributionDebug>* OutSortedContributions) const;

	/**
	 * Scans active unbounded PostProcessVolumes for lighting-affecting overrides and caches
	 * priority/blend-resolved scales. Called inside RefreshSceneLightCache so stealth sampling
	 * stays in sync with the renderer contract for indirect lighting and exposure compensation.
	 */
	void RefreshPPVLightScales();

	UPROPERTY()
	TObjectPtr<UStealthTuningDataAsset> TuningAsset;

	FStealthMovementState PlayerMovement;
	FStealthViewState PlayerView;
	FStealthBodyState PlayerBody;
	FStealthAlsDebugSnapshot AlsDebugSnapshot;
	FVisibilityEmitter PlayerVisibility;
	FSoundEmitter PlayerSoundEmission;

	TArray<FStealthSoundEvent> ActiveSoundEvents;
	int32 NextSoundEventId = 1;

	TArray<TWeakObjectPtr<AStealthLightVolume>> LightVolumes;
	TArray<TWeakObjectPtr<ULightComponentBase>> CachedSceneLights;
	TArray<TWeakObjectPtr<UStealthGuardBrainComponent>> GuardBrains;
	TArray<TWeakObjectPtr<UStealthHealthComponent>> HealthComponents;

	float LastSceneLightCacheTime = -100000.f;
	float PlayerSmoothedLightExposure = 0.f;
	FStealthLightSamplingDebug LastLightSamplingDebug;

	/** Cached scale derived from active unbounded PPV IndirectLightingIntensity override (1 if none found). */
	float CachedPPVIndirectScale = 1.f;

	/** Cached EV exposure scale derived from active unbounded PPV AutoExposureBias override (1 if none found). */
	float CachedPPVExposureScale = 1.f;

	FObjectiveState ObjectiveState;
	FExtractionState ExtractionState;
	FAlarmState AlarmState;
	TArray<FStealthObjectiveRecord> ObjectiveRecords;

	bool bAlertOccurred = false;
	EStealthMissionOutcome MissionOutcome = EStealthMissionOutcome::None;
};
