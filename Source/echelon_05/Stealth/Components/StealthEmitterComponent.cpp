#include "Stealth/Components/StealthEmitterComponent.h"

#include "Stealth/Data/StealthTuningDataAsset.h"
#include "Stealth/Subsystems/StealthSimulationSubsystem.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"

UStealthEmitterComponent::UStealthEmitterComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UStealthEmitterComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateEmissions(DeltaTime);
}

void UStealthEmitterComponent::UpdateEmissions(float DeltaTime)
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World)
	{
		return;
	}

	UStealthSimulationSubsystem* Sim = World->GetSubsystem<UStealthSimulationSubsystem>();
	if (!Sim)
	{
		return;
	}

	const UStealthTuningDataAsset* Tuning = Sim->GetTuningAsset();
	if (!Tuning)
	{
		return;
	}

	const FVector SampleLocation = Owner->GetActorLocation();
	const FStealthMovementState Move = Sim->GetPlayerMovement();
	const FStealthBodyState Body = Sim->GetPlayerBody();

	float LightExposure = Sim->SampleLightExposureAt(SampleLocation);

	float StanceMul = Move.Stance == EStealthStance::Crouching ? Tuning->CrouchingMultiplier : Tuning->StandingMultiplier;

	float MoveMul = Tuning->IdleMovementMultiplier;
	switch (Move.Locomotion)
	{
	case EStealthLocomotion::Idle:
		MoveMul = Tuning->IdleMovementMultiplier;
		break;
	case EStealthLocomotion::Walk:
		MoveMul = Tuning->WalkMovementMultiplier;
		break;
	case EStealthLocomotion::Run:
		MoveMul = Tuning->RunMovementMultiplier;
		break;
	case EStealthLocomotion::Sprint:
		MoveMul = Tuning->SprintMovementMultiplier;
		break;
	default:
		break;
	}

	float ActionMul = 1.f;
	if (Body.LocomotionAction == EStealthLocomotionAction::Mantling || Body.LocomotionAction == EStealthLocomotionAction::Rolling
		|| Body.LocomotionAction == EStealthLocomotionAction::Ragdolling)
	{
		ActionMul = Tuning->MantleActionMultiplier;
	}

	FVisibilityEmitter Vis;
	Vis.LightExposure = LightExposure;
	Vis.StanceMultiplier = StanceMul;
	Vis.MovementMultiplier = MoveMul;
	Vis.ActionMultiplier = ActionMul;
	Vis.SilhouetteExposure = 0.f;
	Vis.CurrentVisibility =
		FMath::Clamp(Tuning->BaseVisibility * StanceMul * MoveMul * ActionMul * LightExposure, 0.f, 1.f);

	float NoiseRadius = Tuning->WalkNoiseRadius;
	switch (Move.Locomotion)
	{
	case EStealthLocomotion::Idle:
		NoiseRadius = Tuning->WalkNoiseRadius * 0.35f;
		break;
	case EStealthLocomotion::Walk:
		NoiseRadius = Tuning->WalkNoiseRadius;
		break;
	case EStealthLocomotion::Run:
		NoiseRadius = Tuning->RunNoiseRadius;
		break;
	case EStealthLocomotion::Sprint:
		NoiseRadius = Tuning->SprintNoiseRadius;
		break;
	default:
		break;
	}

	if (Move.Stance == EStealthStance::Crouching)
	{
		NoiseRadius *= Tuning->CrouchNoiseMultiplier;
	}

	FSoundEmitter Sound;
	Sound.CurrentNoise = NoiseRadius;
	Sound.Radius = NoiseRadius;

	if (!bHasLastFootstepSample)
	{
		LastFootstepSampleLocation = SampleLocation;
		bHasLastFootstepSample = true;
	}
	else
	{
		const float StepDist = FVector::Dist2D(SampleLocation, LastFootstepSampleLocation);
		LastFootstepSampleLocation = SampleLocation;
		PreviousFootstepDistance += StepDist;

		const float Stride = FMath::Max(10.f, Tuning->FootstepStrideCentimeters);
		if (Move.Locomotion != EStealthLocomotion::Idle && PreviousFootstepDistance >= Stride)
		{
			while (PreviousFootstepDistance >= Stride)
			{
				PreviousFootstepDistance -= Stride;
			}

			FStealthSoundEvent FootEvent;
			FootEvent.Position = SampleLocation;
			FootEvent.Loudness = FMath::Clamp(NoiseRadius / FMath::Max(1.f, Tuning->SprintNoiseRadius), 0.1f, 1.f);
			FootEvent.Radius = NoiseRadius * 0.35f;
			FootEvent.Surface = EStealthSurface::Unknown;
			FootEvent.SourceType = EStealthSoundSource::Footstep;
			FootEvent.Lifetime = Tuning->FootstepEventLifetime;
			FootEvent.DebugLabel = TEXT("Footstep");

			const int32 Id = Sim->PushSoundEvent(FootEvent);
			Sound.LastFootstepTime = World->GetTimeSeconds();
			(void)Id;
		}
	}

	Sim->SetPlayerVisibilityEmission(Vis);
	Sim->SetPlayerSoundEmission(Sound);
}
