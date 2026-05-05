#include "Stealth/Actors/StealthGuard.h"

#include "Stealth/Components/StealthGuardBrainComponent.h"

#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AStealthGuard::AStealthGuard()
{
	PrimaryActorTick.bCanEverTick = false;

	GuardBrain = CreateDefaultSubobject<UStealthGuardBrainComponent>(TEXT("GuardBrain"));

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
}

FString AStealthGuard::GetDebugBrainLine() const
{
	return GuardBrain ? GuardBrain->GetDebugBrainLine() : FString(TEXT("Brain: --"));
}
