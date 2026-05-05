#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Stealth/Types/StealthTypes.h"

#include "StealthGuard.generated.h"

class AStealthPatrolRouteActor;
class UStealthTuningDataAsset;

UCLASS()
class AStealthGuard : public ACharacter
{
	GENERATED_BODY()

public:
	AStealthGuard();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	TObjectPtr<AStealthPatrolRouteActor> PatrolRoute;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	FPerceptionSensor Sensor;

	UPROPERTY(BlueprintReadOnly, Category = "Stealth")
	FSuspicionState Suspicion;

	UPROPERTY(BlueprintReadOnly, Category = "Stealth")
	FGuardBrain Brain;

	UFUNCTION(BlueprintPure, Category = "Stealth")
	FString GetDebugBrainLine() const;

protected:
	void UpdatePerceptionAndSuspicion(float DeltaTime, class APawn* PlayerPawn, const class UStealthTuningDataAsset* Tuning,
		class UStealthSimulationSubsystem* Sim);
	void UpdateBrain(float DeltaTime, APawn* PlayerPawn, const UStealthTuningDataAsset* Tuning,
		class UStealthSimulationSubsystem* Sim);
	void RefreshSuspicionBucket(const UStealthTuningDataAsset* Tuning);
	bool ComputeVisionToPlayer(APawn* Player, float& OutNormalizedStrength) const;
	bool ComputeAuditoryStimulus(const class UStealthSimulationSubsystem* Sim, float& OutStrength) const;

	float MoveToAcceptanceRadius = 75.f;
	float LastMoveRequestTime = 0.f;
	float MoveRequestMinInterval = 0.35f;
};
