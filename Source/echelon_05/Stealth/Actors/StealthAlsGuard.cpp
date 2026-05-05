#include "Stealth/Actors/StealthAlsGuard.h"

#include "Stealth/AI/StealthAlsAIController.h"
#include "Stealth/Components/StealthGuardBrainComponent.h"
#include "Stealth/Types/StealthEnums.h"
#include "Stealth/UI/StealthGuardStatusWidget.h"

#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Settings/AlsCharacterSettings.h"
#include "Settings/AlsMovementSettings.h"
#include "UObject/ConstructorHelpers.h"
#include "Utility/AlsGameplayTags.h"

AStealthAlsGuard::AStealthAlsGuard(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	GuardBrain = CreateDefaultSubobject<UStealthGuardBrainComponent>(TEXT("GuardBrain"));

	GuardStatusWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("GuardStatusWidget"));
	GuardStatusWidget->SetupAttachment(GetRootComponent());
	GuardStatusWidget->SetRelativeLocation(FVector(0.f, 0.f, 155.f));
	GuardStatusWidget->SetWidgetClass(UStealthGuardStatusWidget::StaticClass());
	GuardStatusWidget->SetWidgetSpace(EWidgetSpace::Screen);
	GuardStatusWidget->SetDrawSize(FVector2D(260.f, 64.f));
	GuardStatusWidget->SetDrawAtDesiredSize(false);
	GuardStatusWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GuardStatusWidget->SetGenerateOverlapEvents(false);

	OverlaySkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("OverlaySkeletalMesh"));
	OverlaySkeletalMeshComponent->SetupAttachment(GetMesh(), TEXT("Rifle"));
	OverlaySkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	OverlaySkeletalMeshComponent->SetGenerateOverlapEvents(false);
	OverlaySkeletalMeshComponent->bUseAttachParentBound = true;
	OverlaySkeletalMeshComponent->PrimaryComponentTick.bCanEverTick = false;

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

	static ConstructorHelpers::FClassFinder<UAnimInstance> DefaultOverlayAnimationClassFinder(
		TEXT("/ALS/ALS/Character/AnimationInstances/Overlays/AB_Als_Default"));
	if (DefaultOverlayAnimationClassFinder.Succeeded())
	{
		DefaultOverlayAnimationClass = DefaultOverlayAnimationClassFinder.Class;
	}

	static ConstructorHelpers::FClassFinder<UAnimInstance> RifleOverlayAnimationClassFinder(
		TEXT("/ALS/ALS/Character/AnimationInstances/Overlays/AB_Als_Rifle"));
	if (RifleOverlayAnimationClassFinder.Succeeded())
	{
		RifleOverlayAnimationClass = RifleOverlayAnimationClassFinder.Class;
	}

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> RifleOverlaySkeletalMeshFinder(
		TEXT("/ALS/ALS/OverlayObjects/Rifle/SKM_Als_Rifle.SKM_Als_Rifle"));
	if (RifleOverlaySkeletalMeshFinder.Succeeded())
	{
		RifleOverlaySkeletalMesh = RifleOverlaySkeletalMeshFinder.Object;
		OverlaySkeletalMeshComponent->SetSkeletalMesh(RifleOverlaySkeletalMesh);
	}
}

void AStealthAlsGuard::BeginPlay()
{
	Super::BeginPlay();

	SetActorTickEnabled(true);
	SetOverlayMode(AlsOverlayModeTags::Rifle);
	if (OverlaySkeletalMeshComponent)
	{
		OverlaySkeletalMeshComponent->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			TEXT("Rifle"));
		OverlaySkeletalMeshComponent->SetSkeletalMesh(RifleOverlaySkeletalMesh);
		OverlaySkeletalMeshComponent->SetHiddenInGame(false);
		OverlaySkeletalMeshComponent->SetVisibility(true, true);
	}
	if (GuardStatusWidget)
	{
		GuardStatusWidget->InitWidget();
		if (UStealthGuardStatusWidget* StatusWidget = Cast<UStealthGuardStatusWidget>(GuardStatusWidget->GetUserWidgetObject()))
		{
			StatusWidget->SetGuardBrain(FindActiveGuardBrain());
		}
	}
	RefreshOverlayAnimationLayer();
}

void AStealthAlsGuard::Tick(float DeltaTime)
{
	ApplyAlsLocomotionPresentation();
	Super::Tick(DeltaTime);
}

void AStealthAlsGuard::OnOverlayModeChanged_Implementation(FGameplayTag PreviousOverlayMode)
{
	Super::OnOverlayModeChanged_Implementation(PreviousOverlayMode);

	RefreshOverlayAnimationLayer();
}

FString AStealthAlsGuard::GetDebugBrainLine() const
{
	if (const UStealthGuardBrainComponent* ActiveBrain = FindActiveGuardBrain())
	{
		return ActiveBrain->GetDebugBrainLine();
	}

	return FString(TEXT("Brain: --"));
}

void AStealthAlsGuard::ApplyAlsLocomotionPresentation()
{
	const UStealthGuardBrainComponent* ActiveBrain = FindActiveGuardBrain();
	if (!ActiveBrain)
	{
		return;
	}

	switch (ActiveBrain->Brain.Mode)
	{
	case EGuardBrainMode::Chase:
		SetDesiredGait(AlsGaitTags::Running);
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
	const bool bChasing = ActiveBrain->Brain.Mode == EGuardBrainMode::Chase;

	SetOverlayMode(AlsOverlayModeTags::Rifle);
	SetDesiredAiming(bChasing);
	SetDesiredRotationMode(bChasing
		? AlsRotationModeTags::Aiming
		: (bMoving ? AlsRotationModeTags::VelocityDirection : AlsRotationModeTags::ViewDirection));
}

void AStealthAlsGuard::RefreshOverlayAnimationLayer()
{
	if (!GetMesh())
	{
		return;
	}

	TSubclassOf<UAnimInstance> OverlayAnimationClass;
	if (GetOverlayMode() == AlsOverlayModeTags::Rifle)
	{
		OverlayAnimationClass = RifleOverlayAnimationClass;
	}
	else if (GetOverlayMode() == AlsOverlayModeTags::Default)
	{
		OverlayAnimationClass = DefaultOverlayAnimationClass;
	}

	GetMesh()->LinkAnimClassLayers(OverlayAnimationClass.Get());
}

UStealthGuardBrainComponent* AStealthAlsGuard::FindActiveGuardBrain() const
{
	TArray<UStealthGuardBrainComponent*> BrainComponents;
	GetComponents<UStealthGuardBrainComponent>(BrainComponents);

	UStealthGuardBrainComponent* BestBrain = GuardBrain;
	float BestScore = BestBrain ? -1.f : -1000000.f;

	for (UStealthGuardBrainComponent* BrainComponent : BrainComponents)
	{
		if (!BrainComponent)
		{
			continue;
		}

		float Score = 0.f;
		if (BrainComponent->IsRegistered())
		{
			Score += 1000.f;
		}
		if (BrainComponent->IsComponentTickEnabled())
		{
			Score += 100.f;
		}
		if (BrainComponent->Suspicion.bHadVisualStimulus)
		{
			Score += 50.f;
		}
		if (BrainComponent->Brain.Mode == EGuardBrainMode::Chase)
		{
			Score += 25.f;
		}

		Score += BrainComponent->Suspicion.Value;

		if (BrainComponent == GuardBrain)
		{
			Score += 1.f;
		}

		if (!BestBrain || Score > BestScore)
		{
			BestBrain = BrainComponent;
			BestScore = Score;
		}
	}

	return BestBrain;
}
