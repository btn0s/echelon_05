#pragma once

#include "AlsCharacter.h"
#include "Stealth/Interfaces/StealthInteractable.h"

#include "StealthAlsGuard.generated.h"

class UStealthHealthComponent;
class UStealthGuardBrainComponent;
class UAnimInstance;
class USkeletalMesh;
class USkeletalMeshComponent;
class UWidgetComponent;

/** ALS-backed stealth guard: presentation via ALS; stealth logic on GuardBrain; AI movement via BT + StealthAls AI controller. */
UCLASS()
class AStealthAlsGuard : public AAlsCharacter, public IStealthInteractable
{
	GENERATED_BODY()

public:
	explicit AStealthAlsGuard(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	virtual void OnOverlayModeChanged_Implementation(FGameplayTag PreviousOverlayMode) override;
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stealth|Presentation")
	TSubclassOf<UAnimInstance> DefaultOverlayAnimationClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stealth|Presentation")
	TSubclassOf<UAnimInstance> RifleOverlayAnimationClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stealth|Presentation")
	TObjectPtr<USkeletalMeshComponent> OverlaySkeletalMeshComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stealth|Presentation")
	TObjectPtr<USkeletalMesh> RifleOverlaySkeletalMesh;

protected:
	void ApplyAlsLocomotionPresentation();

	void RefreshOverlayAnimationLayer();

	UStealthGuardBrainComponent* FindActiveGuardBrain() const;
};
