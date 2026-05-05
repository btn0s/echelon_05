#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "StealthEmitterComponent.generated.h"

/**
 * Derives visibility and movement noise from player state + stealth light volumes (via subsystem).
 */
UCLASS(ClassGroup = (Stealth), meta = (BlueprintSpawnableComponent))
class UStealthEmitterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UStealthEmitterComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	void UpdateEmissions(float DeltaTime);

	float PreviousFootstepDistance = 0.f;
	FVector LastFootstepSampleLocation = FVector::ZeroVector;
	bool bHasLastFootstepSample = false;
	bool bHasBodySample = false;
	bool bWasAirborne = false;
};
