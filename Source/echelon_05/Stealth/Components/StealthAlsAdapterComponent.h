#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "StealthAlsAdapterComponent.generated.h"

class AAlsCharacter;

/**
 * Samples ALS character state and writes normalized stealth movement/view/body into UStealthSimulationSubsystem.
 * Does not compute visibility, noise, or suspicion.
 */
UCLASS(ClassGroup = (Stealth), meta = (BlueprintSpawnableComponent))
class UStealthAlsAdapterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UStealthAlsAdapterComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	void PushSnapshotFromAls(AAlsCharacter* AlsCharacter);

	UPROPERTY(EditAnywhere, Category = "Stealth")
	bool bWarnOnUnmappedTags = true;
};
