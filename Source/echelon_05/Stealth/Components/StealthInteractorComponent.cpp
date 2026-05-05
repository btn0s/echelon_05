#include "Stealth/Components/StealthInteractorComponent.h"

#include "Stealth/Interfaces/StealthInteractable.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"

UStealthInteractorComponent::UStealthInteractorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UStealthInteractorComponent::BeginPlay()
{
	Super::BeginPlay();
	AddInputMappingContext();
	TryBindInput();
}

void UStealthInteractorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bInputBound)
	{
		TryBindInput();
	}
}

bool UStealthInteractorComponent::TryInteract()
{
	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn)
	{
		return false;
	}

	FVector ViewLoc;
	FRotator ViewRot;
	Pawn->GetActorEyesViewPoint(ViewLoc, ViewRot);

	const FVector Start = ViewLoc;
	const FVector End = Start + ViewRot.Vector() * TraceDistance;

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(StealthInteract), false, Pawn);
	Params.AddIgnoredActor(Pawn);

	if (!GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Visibility,
		FCollisionShape::MakeSphere(TraceRadius), Params))
	{
		return false;
	}

	for (AActor* TestActor : { Hit.GetActor(), Hit.GetActor() ? Hit.GetActor()->GetAttachParentActor() : nullptr })
	{
		if (!TestActor)
		{
			continue;
		}
		if (TestActor->Implements<UStealthInteractable>())
		{
			IStealthInteractable::Execute_StealthInteract(TestActor, Pawn);
			return true;
		}
	}

	return false;
}

void UStealthInteractorComponent::AddInputMappingContext() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const APlayerController* PlayerController = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	const ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	if (!UseMappingContext || !LocalPlayer)
	{
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
	{
		Subsystem->AddMappingContext(UseMappingContext, UseMappingPriority);
	}
}

void UStealthInteractorComponent::TryBindInput()
{
	if (bInputBound || !UseAction)
	{
		return;
	}

	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn || !Pawn->InputComponent)
	{
		return;
	}

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(Pawn->InputComponent))
	{
		EnhancedInput->BindAction(UseAction, ETriggerEvent::Triggered, this, &UStealthInteractorComponent::HandleUseAction);
		bInputBound = true;
		SetComponentTickEnabled(false);
	}
}

void UStealthInteractorComponent::HandleUseAction(const FInputActionValue& Value)
{
	(void)Value;
	TryInteract();
}
