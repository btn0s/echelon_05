#pragma once

#include "AlsCharacter.h"

#include "StealthAlsGuard.generated.h"

class UStealthGuardBrainComponent;

/** ALS-backed stealth guard: presentation via ALS; stealth logic on GuardBrain; AI movement via BT + StealthAls AI controller. */
UCLASS()
class AStealthAlsGuard : public AAlsCharacter
{
	GENERATED_BODY()

public:
	explicit AStealthAlsGuard(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintPure, Category = "Stealth")
	FString GetDebugBrainLine() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stealth")
	TObjectPtr<UStealthGuardBrainComponent> GuardBrain;

protected:
	void ApplyAlsLocomotionPresentation();
};
