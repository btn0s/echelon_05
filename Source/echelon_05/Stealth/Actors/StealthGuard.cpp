#include "Stealth/Actors/StealthGuard.h"

#include "Stealth/Components/StealthGuardBrainComponent.h"
#include "Stealth/Components/StealthHealthComponent.h"
#include "Stealth/UI/StealthGuardStatusWidget.h"

#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AStealthGuard::AStealthGuard()
{
	PrimaryActorTick.bCanEverTick = false;

	GuardBrain = CreateDefaultSubobject<UStealthGuardBrainComponent>(TEXT("GuardBrain"));
	Health = CreateDefaultSubobject<UStealthHealthComponent>(TEXT("Health"));
	Health->Team = EStealthTeam::Guard;

	GuardStatusWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("GuardStatusWidget"));
	GuardStatusWidget->SetupAttachment(GetRootComponent());
	GuardStatusWidget->SetRelativeLocation(FVector(0.f, 0.f, 150.f));
	GuardStatusWidget->SetWidgetClass(UStealthGuardStatusWidget::StaticClass());
	GuardStatusWidget->SetWidgetSpace(EWidgetSpace::Screen);
	// Plate size accommodates the 5-tick suspicion row + corner brackets and the
	// inverted ALERT block (Docs/ui/DESIGN.md `### Guard Suspicion Plate`).
	GuardStatusWidget->SetDrawSize(FVector2D(118.f, 22.f));
	GuardStatusWidget->SetDrawAtDesiredSize(false);
	GuardStatusWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GuardStatusWidget->SetGenerateOverlapEvents(false);

	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	GetCapsuleComponent()->SetCapsuleHalfHeight(88.f);
	GetCapsuleComponent()->SetCapsuleRadius(34.f);

	if (USkeletalMeshComponent* SkelMesh = GetMesh())
	{
		SkelMesh->DetailMode = DM_Epic;
		SkelMesh->SetUpdateAnimationInEditor(true);
	}

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->MaxWalkSpeed = 320.f;
	}
}

void AStealthGuard::BeginPlay()
{
	Super::BeginPlay();

	if (GuardStatusWidget)
	{
		GuardStatusWidget->InitWidget();
		if (UStealthGuardStatusWidget* StatusWidget = Cast<UStealthGuardStatusWidget>(GuardStatusWidget->GetUserWidgetObject()))
		{
			StatusWidget->SetGuardBrain(GuardBrain);
		}
	}
}

FString AStealthGuard::GetDebugBrainLine() const
{
	return GuardBrain ? GuardBrain->GetDebugBrainLine() : FString(TEXT("Brain: --"));
}

bool AStealthGuard::CanStealthInteract_Implementation(APawn* InteractingPawn)
{
	return GuardBrain && GuardBrain->CanBeBackTakedownBy(InteractingPawn);
}

FText AStealthGuard::GetStealthInteractionText_Implementation(APawn* InteractingPawn)
{
	(void)InteractingPawn;
	return NSLOCTEXT("StealthGuard", "BackTakedownText", "Takedown");
}

void AStealthGuard::StealthInteract_Implementation(APawn* InteractingPawn)
{
	if (GuardBrain)
	{
		GuardBrain->TryBackTakedown(InteractingPawn);
	}
}
