#include "Stealth/Actors/StealthAlsGuard.h"

#include "Stealth/AI/StealthAlsAIController.h"
#include "Stealth/Components/StealthGuardBrainComponent.h"
#include "Stealth/Types/StealthEnums.h"

#include "Settings/AlsCharacterSettings.h"
#include "Settings/AlsMovementSettings.h"
#include "UObject/ConstructorHelpers.h"
#include "Utility/AlsGameplayTags.h"

AStealthAlsGuard::AStealthAlsGuard(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GuardBrain = CreateDefaultSubobject<UStealthGuardBrainComponent>(TEXT("GuardBrain"));

	AIControllerClass = AStealthAlsAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// Required for AI MoveTo: without Settings/MovementSettings, AAlsCharacter::Tick exits early and never runs
	// RefreshInput(), so path-following acceleration never becomes ALS input (shuffle-in-place).
	static ConstructorHelpers::FObjectFinder<UAlsCharacterSettings> DefaultCharacterSettings(
		TEXT("/ALS/ALS/Data/Character/CS_Als_Default.CS_Als_Default"));
	if (DefaultCharacterSettings.Succeeded())
	{
		Settings = DefaultCharacterSettings.Object;
	}

	static ConstructorHelpers::FObjectFinder<UAlsMovementSettings> DefaultMovementSettings(
		TEXT("/ALS/ALS/Data/Character/Movement/MS_Als_Normal.MS_Als_Normal"));
	if (DefaultMovementSettings.Succeeded())
	{
		MovementSettings = DefaultMovementSettings.Object;
	}
}

void AStealthAlsGuard::BeginPlay()
{
	Super::BeginPlay();
}

void AStealthAlsGuard::Tick(float DeltaTime)
{
	ApplyAlsLocomotionPresentation();
	Super::Tick(DeltaTime);
}

FString AStealthAlsGuard::GetDebugBrainLine() const
{
	return GuardBrain ? GuardBrain->GetDebugBrainLine() : FString(TEXT("Brain: --"));
}

void AStealthAlsGuard::ApplyAlsLocomotionPresentation()
{
	if (!GuardBrain)
	{
		return;
	}

	switch (GuardBrain->Brain.Mode)
	{
	case EGuardBrainMode::Chase:
		SetDesiredGait(AlsGaitTags::Sprinting);
		break;
	case EGuardBrainMode::Investigate:
		SetDesiredGait(AlsGaitTags::Running);
		break;
	case EGuardBrainMode::Pause:
		SetDesiredGait(AlsGaitTags::Walking);
		break;
	default:
		SetDesiredGait(AlsGaitTags::Walking);
		break;
	}

	SetDesiredStance(AlsStanceTags::Standing);

	const float Speed2D = GetVelocity().Size2D();
	const bool bMoving = Speed2D > 10.f;
	SetDesiredRotationMode(bMoving ? AlsRotationModeTags::VelocityDirection : AlsRotationModeTags::ViewDirection);
}
