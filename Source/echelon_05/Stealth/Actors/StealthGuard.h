#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Stealth/Interfaces/StealthInteractable.h"

#include "StealthGuard.generated.h"

class UStealthHealthComponent;
class UStealthGuardBrainComponent;
class UWidgetComponent;

/** Baseline non-ALS stealth guard (legacy locomotion). Logic lives on GuardBrain. */
UCLASS()
class AStealthGuard : public ACharacter, public IStealthInteractable
{
	GENERATED_BODY()

public:
	AStealthGuard();

	virtual void BeginPlay() override;

	virtual bool CanStealthInteract_Implementation(APawn* InteractingPawn) override;
	virtual FText GetStealthInteractionText_Implementation(APawn* InteractingPawn) override;
	virtual void StealthInteract_Implementation(APawn* InteractingPawn) override;

	UFUNCTION(BlueprintPure, Category = "Stealth")
	FString GetDebugBrainLine() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stealth")
	TObjectPtr<UStealthGuardBrainComponent> GuardBrain;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stealth")
	TObjectPtr<UStealthHealthComponent> Health;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stealth|UI")
	TObjectPtr<UWidgetComponent> GuardStatusWidget;
};
