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

	virtual void StealthInteract_Implementation(APawn* InteractingPawn) override;

	UFUNCTION(BlueprintCallable, Category = "Stealth")
	bool IsCompleted() const { return bCompleted; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stealth")
	TObjectPtr<class UStaticMeshComponent> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth", meta = (ClampMin = 0))
	float InteractionSoundLoudness = 0.35f;

	bool bCompleted = false;
};
