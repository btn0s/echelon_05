#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Stealth/Interfaces/StealthInteractable.h"

#include "StealthLureActor.generated.h"

UCLASS()
class AStealthLureActor : public AActor, public IStealthInteractable
{
	GENERATED_BODY()

public:
	AStealthLureActor();

	virtual bool CanStealthInteract_Implementation(APawn* InteractingPawn) override;
	virtual FText GetStealthInteractionText_Implementation(APawn* InteractingPawn) override;
	virtual void StealthInteract_Implementation(APawn* InteractingPawn) override;

	UFUNCTION(BlueprintCallable, Category = "Stealth")
	void TriggerLure(float LoudnessScale = 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth", meta = (ClampMin = 0))
	float BaseLoudness = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth", meta = (ClampMin = 0))
	float BaseRadius = 1400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	bool bSingleUse = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stealth")
	bool bTriggered = false;
};
