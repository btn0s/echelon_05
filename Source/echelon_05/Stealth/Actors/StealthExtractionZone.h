#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "StealthExtractionZone.generated.h"

UCLASS()
class AStealthExtractionZone : public AActor
{
	GENERATED_BODY()

public:
	AStealthExtractionZone();

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stealth")
	TObjectPtr<class UBoxComponent> Trigger;
};
