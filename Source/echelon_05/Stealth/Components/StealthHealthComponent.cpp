#include "Stealth/Components/StealthHealthComponent.h"

#include "AIController.h"
#include "Camera/CameraShakeBase.h"
#include "GameFramework/Actor.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Stealth/Subsystems/StealthSimulationSubsystem.h"

UStealthHealthComponent::UStealthHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UStealthHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	MaxHealth = FMath::Max(1.f, MaxHealth);
	StartingHealth = FMath::Clamp(StartingHealth, 0.f, MaxHealth);
	CurrentHealth = StartingHealth;
	VitalState = CurrentHealth > 0.f ? EStealthVitalState::Alive : EStealthVitalState::Dead;

	if (AActor* Owner = GetOwner())
	{
		Owner->OnTakeAnyDamage.AddDynamic(this, &UStealthHealthComponent::HandleAnyDamage);
	}

	if (UWorld* World = GetWorld())
	{
		if (UStealthSimulationSubsystem* Sim = World->GetSubsystem<UStealthSimulationSubsystem>())
		{
			Sim->RegisterHealthComponent(this);
		}
	}
}

void UStealthHealthComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AActor* Owner = GetOwner())
	{
		Owner->OnTakeAnyDamage.RemoveDynamic(this, &UStealthHealthComponent::HandleAnyDamage);
	}

	if (UWorld* World = GetWorld())
	{
		if (UStealthSimulationSubsystem* Sim = World->GetSubsystem<UStealthSimulationSubsystem>())
		{
			Sim->UnregisterHealthComponent(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

float UStealthHealthComponent::ApplyStealthDamage(float DamageAmount, EStealthDamageKind DamageKind, AActor* DamageCauser,
	AController* InstigatorController, FName DamageReason)
{
	if (!CanTakeStealthDamage(DamageAmount))
	{
		return 0.f;
	}

	const float PreviousHealth = CurrentHealth;
	const float AppliedDamage = FMath::Min(CurrentHealth, FMath::Max(0.f, DamageAmount));
	CurrentHealth = FMath::Clamp(CurrentHealth - AppliedDamage, 0.f, MaxHealth);

	LastDamage = AppliedDamage;
	LastDamageKind = DamageKind;
	LastDamageReason = DamageReason;
	LastDamageTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

	OnHealthChanged.Broadcast(this, PreviousHealth, CurrentHealth);

	if (AppliedDamage > KINDA_SMALL_NUMBER)
	{
		PlayLocalDamagePresentation(AppliedDamage);
	}

	if (CurrentHealth <= KINDA_SMALL_NUMBER)
	{
		AActor* InstigatorActor = InstigatorController ? InstigatorController->GetPawn() : nullptr;
		if (!InstigatorActor)
		{
			InstigatorActor = DamageCauser;
		}
		HandleZeroHealth(DamageKind, InstigatorActor);
	}

	return AppliedDamage;
}

float UStealthHealthComponent::Heal(float HealAmount)
{
	if (HealAmount <= 0.f || VitalState != EStealthVitalState::Alive)
	{
		return 0.f;
	}

	const float PreviousHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(CurrentHealth + HealAmount, 0.f, MaxHealth);
	OnHealthChanged.Broadcast(this, PreviousHealth, CurrentHealth);
	return CurrentHealth - PreviousHealth;
}

void UStealthHealthComponent::ResetHealth()
{
	const float PreviousHealth = CurrentHealth;
	const EStealthVitalState PreviousState = VitalState;
	CurrentHealth = FMath::Clamp(StartingHealth > 0.f ? StartingHealth : MaxHealth, 0.f, MaxHealth);
	VitalState = CurrentHealth > 0.f ? EStealthVitalState::Alive : EStealthVitalState::Dead;
	LastDamage = 0.f;
	LastDamageKind = EStealthDamageKind::Generic;
	LastDamageReason = NAME_None;
	LastDamageTime = 0.f;

	OnHealthChanged.Broadcast(this, PreviousHealth, CurrentHealth);
	if (PreviousState != VitalState)
	{
		OnVitalStateChanged.Broadcast(this, PreviousState, VitalState, nullptr);
	}
}

void UStealthHealthComponent::Kill(EStealthDamageKind DamageKind, AActor* InstigatorActor, FName DamageReason)
{
	LastDamageReason = DamageReason;
	ApplyStealthDamage(CurrentHealth, DamageKind, InstigatorActor, Cast<AController>(InstigatorActor), DamageReason);
}

bool UStealthHealthComponent::CanTakeStealthDamage(float DamageAmount) const
{
	return DamageAmount > 0.f && !bInvulnerable && IsAlive();
}

FStealthHealthState UStealthHealthComponent::GetHealthState() const
{
	FStealthHealthState State;
	State.Owner = GetOwner();
	State.Team = Team;
	State.VitalState = VitalState;
	State.CurrentHealth = CurrentHealth;
	State.MaxHealth = MaxHealth;
	State.LastDamage = LastDamage;
	State.LastDamageKind = LastDamageKind;
	State.LastDamageTime = LastDamageTime;
	State.LastDamageReason = LastDamageReason;
	return State;
}

void UStealthHealthComponent::HandleAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
	AController* InstigatedBy, AActor* DamageCauser)
{
	(void)DamagedActor;

	const FName DamageReason = DamageType ? DamageType->GetClass()->GetFName() : NAME_None;
	ApplyStealthDamage(Damage, EStealthDamageKind::Generic, DamageCauser, InstigatedBy, DamageReason);
}

void UStealthHealthComponent::SetVitalState(EStealthVitalState NewState, AActor* InstigatorActor)
{
	if (VitalState == NewState)
	{
		return;
	}

	const EStealthVitalState PreviousState = VitalState;
	VitalState = NewState;
	OnVitalStateChanged.Broadcast(this, PreviousState, VitalState, InstigatorActor);
}

void UStealthHealthComponent::HandleZeroHealth(EStealthDamageKind DamageKind, AActor* InstigatorActor)
{
	const EStealthVitalState ZeroState =
		DamageKind == EStealthDamageKind::NonLethal ? EStealthVitalState::Downed : EStealthVitalState::Dead;
	SetVitalState(ZeroState, InstigatorActor);

	if (AActor* Owner = GetOwner())
	{
		if (bStopMovementOnZeroHealth)
		{
			if (UCharacterMovementComponent* Movement = Owner->FindComponentByClass<UCharacterMovementComponent>())
			{
				Movement->StopMovementImmediately();
				Movement->DisableMovement();
			}

			if (APawn* Pawn = Cast<APawn>(Owner))
			{
				if (AController* Controller = Pawn->GetController())
				{
					Controller->StopMovement();
					if (AAIController* AIController = Cast<AAIController>(Controller))
					{
						AIController->ClearFocus(EAIFocusPriority::Gameplay);
					}
				}
			}
		}

		if (bDisableCollisionOnZeroHealth)
		{
			Owner->SetActorEnableCollision(false);
		}
	}
}

void UStealthHealthComponent::PlayLocalDamagePresentation(float AppliedDamage)
{
	(void)AppliedDamage;

	if (bMuteRuntimeDamagePresentation || Team != EStealthTeam::Player)
	{
		return;
	}

	AActor* Owner = GetOwner();
	APawn* PawnOwner = Owner ? Cast<APawn>(Owner) : nullptr;
	if (!PawnOwner || !PawnOwner->IsLocallyControlled())
	{
		return;
	}

	if (DamageTakenSoundCue)
	{
		UGameplayStatics::PlaySound2D(PawnOwner, DamageTakenSoundCue, DamageTakenSoundVolume);
	}

	if (DamageTakenCameraShake)
	{
		if (APlayerController* PC = Cast<APlayerController>(PawnOwner->GetController()))
		{
			PC->ClientStartCameraShake(DamageTakenCameraShake);
		}
	}
}
