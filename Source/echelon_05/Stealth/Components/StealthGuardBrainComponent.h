#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Stealth/Types/StealthTypes.h"

#include "StealthGuardBrainComponent.generated.h"

class AAIController;
class AStealthPatrolRouteActor;
class APawn;
class UStealthTuningDataAsset;
class UStealthSimulationSubsystem;

/**
 * Perception, suspicion, guard intent, and AI navigation requests for stealth guards.
 * Used by baseline AStealthGuard and ALS-backed AStealthAlsGuard.
 */
UCLASS(ClassGroup = (Stealth), meta = (BlueprintSpawnableComponent))
class UStealthGuardBrainComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UStealthGuardBrainComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	TObjectPtr<AStealthPatrolRouteActor> PatrolRoute;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	FPerceptionSensor Sensor;

	UPROPERTY(BlueprintReadOnly, Category = "Stealth")
	FSuspicionState Suspicion;

	UPROPERTY(BlueprintReadOnly, Category = "Stealth")
	FGuardBrain Brain;

	UPROPERTY(BlueprintReadOnly, Category = "Stealth")
	bool bCombatLocked = false;

	/** When true, ApplyStealthMovement is driven by BT task (StealthAls); Tick only updates perception/brain mode. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	bool bMovementAppliedByBehaviorTreeTask = false;

	UFUNCTION(BlueprintPure, Category = "Stealth")
	bool IsMovementAppliedByBehaviorTreeTask() const { return bMovementAppliedByBehaviorTreeTask; }

	UFUNCTION(BlueprintPure, Category = "Stealth")
	FString GetDebugBrainLine() const;

	void ApplyStealthMovement(AAIController* AI, float DeltaTime);

protected:
	void UpdatePerceptionAndSuspicion(float DeltaTime, APawn* PlayerPawn, const UStealthTuningDataAsset* Tuning,
		UStealthSimulationSubsystem* Sim);
	void UpdateBrainMode(UStealthSimulationSubsystem* Sim);
	void RefreshSuspicionBucket(const UStealthTuningDataAsset* Tuning);
	void ResolvePatrolRoute();
	bool ComputeVisionToPlayer(APawn* Player, float& OutNormalizedStrength) const;
	bool ComputeAuditoryStimulus(const UStealthSimulationSubsystem* Sim, float& OutStrength) const;

	float MoveToAcceptanceRadius = 75.f;
	float PatrolPointReachedRadius = 140.f;
	float CombatMinimumDistance = 450.f;
	float CombatPreferredDistance = 650.f;
	float CombatMaximumDistance = 900.f;
	float StimulusAttentionHoldSeconds = 1.5f;
	float LastMoveRequestTime = 0.f;
	float MoveRequestMinInterval = 0.35f;
};
