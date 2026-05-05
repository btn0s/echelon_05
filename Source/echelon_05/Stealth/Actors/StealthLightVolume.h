#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "StealthLightVolume.generated.h"

UCLASS()
class AStealthLightVolume : public AActor
{
	GENERATED_BODY()

public:
	AStealthLightVolume();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintPure, Category = "Stealth")
	float GetLightExposure() const { return LightExposure; }

	bool EncompassesPoint(const FVector& WorldPoint) const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stealth")
	TObjectPtr<class UBoxComponent> Bounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth", meta = (ClampMin = 0, ClampMax = 1))
	float LightExposure = 0.85f;
};
