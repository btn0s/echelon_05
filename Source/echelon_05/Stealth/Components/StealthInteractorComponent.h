#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "StealthInteractorComponent.generated.h"

UCLASS(ClassGroup = (Stealth), meta = (BlueprintSpawnableComponent))
class UStealthInteractorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UStealthInteractorComponent();

	UFUNCTION(BlueprintCallable, Category = "Stealth")
	bool TryInteract();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth", meta = (ClampMin = 0))
	float TraceDistance = 220.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth", meta = (ClampMin = 0))
	float TraceRadius = 45.f;
};
