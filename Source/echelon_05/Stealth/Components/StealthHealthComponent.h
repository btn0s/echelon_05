#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Stealth/Types/StealthTypes.h"

#include "StealthHealthComponent.generated.h"

class UDamageType;
class USoundBase;
class UCameraShakeBase;

class UStealthHealthComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnStealthHealthChanged, UStealthHealthComponent*, Component,
	float, PreviousHealth, float, NewHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnStealthVitalStateChanged, UStealthHealthComponent*, Component,
	EStealthVitalState, PreviousState, EStealthVitalState, NewState, AActor*, InstigatorActor);

UCLASS(ClassGroup = (Stealth), meta = (BlueprintSpawnableComponent))
class UStealthHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UStealthHealthComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "Stealth|Health")
	float ApplyStealthDamage(float DamageAmount, EStealthDamageKind DamageKind, AActor* DamageCauser,
		AController* InstigatorController, FName DamageReason = NAME_None);

	UFUNCTION(BlueprintCallable, Category = "Stealth|Health")
	float Heal(float HealAmount);

	UFUNCTION(BlueprintCallable, Category = "Stealth|Health")
	void ResetHealth();

	UFUNCTION(BlueprintCallable, Category = "Stealth|Health")
	void Kill(EStealthDamageKind DamageKind = EStealthDamageKind::Lethal, AActor* InstigatorActor = nullptr,
		FName DamageReason = NAME_None);

	UFUNCTION(BlueprintCallable, Category = "Stealth|Health")
	void SetTeam(EStealthTeam NewTeam) { Team = NewTeam; }

	UFUNCTION(BlueprintPure, Category = "Stealth|Health")
	bool IsAlive() const { return VitalState == EStealthVitalState::Alive && CurrentHealth > 0.f; }

	UFUNCTION(BlueprintPure, Category = "Stealth|Health")
	bool IsDowned() const { return VitalState == EStealthVitalState::Downed; }

	UFUNCTION(BlueprintPure, Category = "Stealth|Health")
	bool IsDead() const { return VitalState == EStealthVitalState::Dead; }

	UFUNCTION(BlueprintPure, Category = "Stealth|Health")
	bool CanTakeStealthDamage(float DamageAmount) const;

	UFUNCTION(BlueprintPure, Category = "Stealth|Health")
	FStealthHealthState GetHealthState() const;

	UFUNCTION(BlueprintPure, Category = "Stealth|Health")
	EStealthTeam GetTeam() const { return Team; }

	UFUNCTION(BlueprintPure, Category = "Stealth|Health")
	EStealthVitalState GetVitalState() const { return VitalState; }

	UFUNCTION(BlueprintPure, Category = "Stealth|Health")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category = "Stealth|Health")
	float GetMaxHealth() const { return MaxHealth; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth|Health", meta = (ClampMin = 1))
	float MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth|Health", meta = (ClampMin = 0))
	float StartingHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth|Health")
	EStealthTeam Team = EStealthTeam::Neutral;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth|Health")
	bool bInvulnerable = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth|Health")
	bool bStopMovementOnZeroHealth = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth|Health")
	bool bDisableCollisionOnZeroHealth = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth|Presentation")
	USoundBase* DamageTakenSoundCue = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth|Presentation")
	float DamageTakenSoundVolume = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth|Presentation")
	TSubclassOf<UCameraShakeBase> DamageTakenCameraShake = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth|Presentation")
	bool bMuteRuntimeDamagePresentation = false;

	UPROPERTY(BlueprintAssignable, Category = "Stealth|Health")
	FOnStealthHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Stealth|Health")
	FOnStealthVitalStateChanged OnVitalStateChanged;

protected:
	UFUNCTION()
	void HandleAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy,
		AActor* DamageCauser);

	void SetVitalState(EStealthVitalState NewState, AActor* InstigatorActor);
	void HandleZeroHealth(EStealthDamageKind DamageKind, AActor* InstigatorActor);
	void PlayLocalDamagePresentation(float AppliedDamage);

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stealth|Health")
	float CurrentHealth = 100.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stealth|Health")
	EStealthVitalState VitalState = EStealthVitalState::Alive;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stealth|Health")
	float LastDamage = 0.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stealth|Health")
	EStealthDamageKind LastDamageKind = EStealthDamageKind::Generic;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stealth|Health")
	float LastDamageTime = 0.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stealth|Health")
	FName LastDamageReason = NAME_None;
};
