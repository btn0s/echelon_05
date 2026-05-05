#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "StealthLureActor.generated.h"

UCLASS()
class AStealthLureActor : public AActor
{
	GENERATED_BODY()

public:
	AStealthLureActor();

	UFUNCTION(BlueprintCallable, Category = "Stealth")
	void TriggerLure(float LoudnessScale = 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth", meta = (ClampMin = 0))
	float BaseLoudness = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth", meta = (ClampMin = 0))
	float BaseRadius = 1400.f;
};
