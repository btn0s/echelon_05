#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Stealth/Interfaces/StealthInteractable.h"

#include "StealthObjective.generated.h"

UCLASS()
class AStealthObjective : public AActor, public IStealthInteractable
{
	GENERATED_BODY()

public:
	AStealthObjective();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual bool CanStealthInteract_Implementation(APawn* InteractingPawn) override;
	virtual FText GetStealthInteractionText_Implementation(APawn* InteractingPawn) override;
	virtual void StealthInteract_Implementation(APawn* InteractingPawn) override;

	UFUNCTION(BlueprintCallable, Category = "Stealth")
	bool IsCompleted() const { return bCompleted; }

	UFUNCTION(BlueprintPure, Category = "Stealth")
	FName GetObjectiveId() const { return ResolvedObjectiveId.IsNone() ? ObjectiveId : ResolvedObjectiveId; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stealth")
	TObjectPtr<class UStaticMeshComponent> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	FName ObjectiveId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	FText ObjectiveDisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	bool bRequired = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	bool bRegisterWithSimulation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth", meta = (ClampMin = 0))
	float InteractionSoundLoudness = 0.35f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stealth")
	bool bCompleted = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stealth")
	FName ResolvedObjectiveId = NAME_None;
};
