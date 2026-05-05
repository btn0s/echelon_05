#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "StealthGuard.generated.h"

class UStealthGuardBrainComponent;

/** Baseline non-ALS stealth guard (legacy locomotion). Logic lives on GuardBrain. */
UCLASS()
class AStealthGuard : public ACharacter
{
	GENERATED_BODY()

public:
	AStealthGuard();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintPure, Category = "Stealth")
	FString GetDebugBrainLine() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stealth")
	TObjectPtr<UStealthGuardBrainComponent> GuardBrain;
};
