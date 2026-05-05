#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Stealth/Interfaces/StealthInteractable.h"

#include "StealthExtractionZone.generated.h"

UCLASS()
class AStealthExtractionZone : public AActor, public IStealthInteractable
{
	GENERATED_BODY()

public:
	AStealthExtractionZone();

	virtual bool CanStealthInteract_Implementation(APawn* InteractingPawn) override;
	virtual FText GetStealthInteractionText_Implementation(APawn* InteractingPawn) override;
	virtual void StealthInteract_Implementation(APawn* InteractingPawn) override;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION(BlueprintCallable, Category = "Stealth")
	bool TryExtract(APawn* InteractingPawn);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stealth")
	TObjectPtr<class UBoxComponent> Trigger;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	bool bAutoExtractOnOverlap = true;
};
