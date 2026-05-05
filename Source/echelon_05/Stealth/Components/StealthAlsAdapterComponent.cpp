#include "Stealth/Components/StealthAlsAdapterComponent.h"

#include "AlsCharacter.h"
#include "Stealth/Data/StealthTuningDataAsset.h"
#include "Stealth/StealthLog.h"
#include "Stealth/Subsystems/StealthSimulationSubsystem.h"
#include "Utility/AlsGameplayTags.h"

#include "GameFramework/Actor.h"

UStealthAlsAdapterComponent::UStealthAlsAdapterComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UStealthAlsAdapterComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AActor* Owner = GetOwner())
	{
		Owner->AddTickPrerequisiteActor(Owner);
	}
}

void UStealthAlsAdapterComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AAlsCharacter* AlsChar = Cast<AAlsCharacter>(GetOwner());
	if (!AlsChar)
	{
		return;
	}

	PushSnapshotFromAls(AlsChar);
}

void UStealthAlsAdapterComponent::PushSnapshotFromAls(AAlsCharacter* AlsCharacter)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UStealthSimulationSubsystem* Sim = World->GetSubsystem<UStealthSimulationSubsystem>();
	if (!Sim)
	{
		return;
	}

	const UStealthTuningDataAsset* Tuning = Sim->GetTuningAsset();
	const float IdleThreshold = Tuning ? Tuning->IdleSpeedThreshold : 10.f;

	const FGameplayTag StanceTag = AlsCharacter->GetStance();
	const FGameplayTag GaitTag = AlsCharacter->GetGait();
	const FGameplayTag LocomotionModeTag = AlsCharacter->GetLocomotionMode();
	const FGameplayTag LocomotionActionTag = AlsCharacter->GetLocomotionAction();
	const FGameplayTag RotationModeTag = AlsCharacter->GetRotationMode();
	const FGameplayTag ViewModeTag = AlsCharacter->GetViewMode();

	const FAlsLocomotionState& Loc = AlsCharacter->GetLocomotionState();
	const FAlsViewState& View = AlsCharacter->GetViewState();

	FStealthMovementState Movement;
	Movement.Stance = StanceTag == AlsStanceTags::Crouching ? EStealthStance::Crouching : EStealthStance::Standing;
	if (StanceTag.IsValid() && StanceTag != AlsStanceTags::Standing && StanceTag != AlsStanceTags::Crouching && bWarnOnUnmappedTags)
	{
		UE_LOG(LogStealth, Warning, TEXT("Unmapped ALS stance tag %s — defaulting to Standing."), *StanceTag.ToString());
		Movement.Stance = EStealthStance::Standing;
	}

	const bool bIdle = !Loc.bMoving || Loc.Speed < IdleThreshold;
	if (bIdle)
	{
		Movement.Locomotion = EStealthLocomotion::Idle;
	}
	else if (GaitTag == AlsGaitTags::Walking)
	{
		Movement.Locomotion = EStealthLocomotion::Walk;
	}
	else if (GaitTag == AlsGaitTags::Running)
	{
		Movement.Locomotion = EStealthLocomotion::Run;
	}
	else if (GaitTag == AlsGaitTags::Sprinting)
	{
		Movement.Locomotion = EStealthLocomotion::Sprint;
	}
	else
	{
		if (bWarnOnUnmappedTags && GaitTag.IsValid())
		{
			UE_LOG(LogStealth, Warning, TEXT("Unmapped ALS gait tag %s — treating as Walk."), *GaitTag.ToString());
		}
		Movement.Locomotion = EStealthLocomotion::Walk;
	}

	Movement.Speed = Loc.Speed;
	Movement.Velocity = Loc.Velocity;
	Movement.bHasInput = Loc.bHasInput;
	Movement.bMoving = Loc.bMoving;
	Movement.Surface = EStealthSurface::Unknown;

	FStealthViewState StealthView;
	StealthView.ViewRotation = View.Rotation;
	StealthView.ViewDirection = View.Rotation.Vector();
	StealthView.ViewYawSpeed = View.YawSpeed;
	StealthView.RotationMode = RotationModeTag;
	StealthView.ViewMode = ViewModeTag;
	StealthView.bRightShoulder = false;

	FStealthBodyState Body;
	Body.bGrounded = LocomotionModeTag == AlsLocomotionModeTags::Grounded;
	Body.bAirborne = LocomotionModeTag == AlsLocomotionModeTags::InAir;

	if (LocomotionActionTag == AlsLocomotionActionTags::Mantling)
	{
		Body.LocomotionAction = EStealthLocomotionAction::Mantling;
	}
	else if (LocomotionActionTag == AlsLocomotionActionTags::Rolling)
	{
		Body.LocomotionAction = EStealthLocomotionAction::Rolling;
	}
	else if (LocomotionActionTag == AlsLocomotionActionTags::Ragdolling)
	{
		Body.LocomotionAction = EStealthLocomotionAction::Ragdolling;
	}
	else if (LocomotionActionTag == AlsLocomotionActionTags::GettingUp)
	{
		Body.LocomotionAction = EStealthLocomotionAction::GettingUp;
	}
	else
	{
		Body.LocomotionAction = EStealthLocomotionAction::None;
		if (LocomotionActionTag.IsValid() && bWarnOnUnmappedTags)
		{
			UE_LOG(LogStealth, Warning, TEXT("Unmapped ALS locomotion action tag %s."), *LocomotionActionTag.ToString());
		}
	}

	FStealthAlsDebugSnapshot AlsDebug;
	AlsDebug.AlsStance = StanceTag;
	AlsDebug.AlsGait = GaitTag;
	AlsDebug.AlsLocomotionMode = LocomotionModeTag;
	AlsDebug.AlsLocomotionAction = LocomotionActionTag;
	AlsDebug.AlsRotationMode = RotationModeTag;
	AlsDebug.AlsViewMode = ViewModeTag;
	AlsDebug.Speed = Loc.Speed;
	AlsDebug.bMoving = Loc.bMoving;
	AlsDebug.bHasInput = Loc.bHasInput;

	Sim->SetPlayerSnapshot(Movement, StealthView, Body, AlsDebug);
}
