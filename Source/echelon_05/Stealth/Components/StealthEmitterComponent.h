#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "StealthEmitterComponent.generated.h"

/**
 * Derives visibility and movement noise from player state + scene-light body sampling (via subsystem).
 */
UCLASS(ClassGroup = (Stealth), meta = (BlueprintSpawnableComponent))
class UStealthEmitterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UStealthEmitterComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth|Health")
	bool bEnsureOwnerHasPlayerHealth = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth|Health", meta = (ClampMin = 1))
	float DefaultPlayerMaxHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth|Presentation")
	TObjectPtr<class USoundBase> PlayerDamagePresentationSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth|Presentation")
	float PlayerDamagePresentationVolume = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth|Presentation")
	TSubclassOf<class UCameraShakeBase> PlayerDamagePresentationCameraShake = nullptr;

protected:
	void EnsureOwnerHealthComponent();
	void ApplyPresentationToRuntimeHealth(class UStealthHealthComponent* Health) const;
	void UpdateEmissions(float DeltaTime);

	UPROPERTY(Transient)
	TObjectPtr<class UStealthHealthComponent> RuntimeHealthComponent;

	float PreviousFootstepDistance = 0.f;
	FVector LastFootstepSampleLocation = FVector::ZeroVector;
	bool bHasLastFootstepSample = false;
	bool bHasBodySample = false;
	bool bWasAirborne = false;
};
