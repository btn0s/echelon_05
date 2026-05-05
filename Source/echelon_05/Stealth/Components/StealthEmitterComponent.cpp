#include "Stealth/Components/StealthEmitterComponent.h"

#include "Stealth/Data/StealthTuningDataAsset.h"
#include "Stealth/Subsystems/StealthSimulationSubsystem.h"

#include "DrawDebugHelpers.h"
#include "HAL/IConsoleManager.h"
#include "Components/CapsuleComponent.h"
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

	int32 DebugDrawValue = 0;
	if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("stealth.DebugDraw")))
	{
		DebugDrawValue = CVar->GetInt();
	}
	if (DebugDrawValue <= 0)
	{
		return;
	}

	UWorld* World = GetWorld();
	AActor* Owner = GetOwner();
	if (!World || !Owner)
	{
		return;
	}

	if (UStealthSimulationSubsystem* Sim = World->GetSubsystem<UStealthSimulationSubsystem>())
	{
		const FStealthLightSamplingDebug LDebug = Sim->GetLastLightSamplingDebug();
		const FVector TraceOffset = FVector(0.f, 0.f, 12.f);
		for (const FStealthBodyLightSampleDebug& Pt : LDebug.BodySamples)
		{
			DrawDebugSphere(World, Pt.WorldPosition + TraceOffset, 10.f, 10, FColor::Cyan, false, -1.f, 0, 1.f);
			DrawDebugString(World, Pt.WorldPosition + TraceOffset + FVector(0.f, 0.f, 28.f),
				FString::Printf(TEXT("%s %.2f"), *Pt.SampleName, Pt.Exposure), nullptr, FColor::White, 0.f, true, 1.f);

			for (const FStealthLightContributionDebug& C : Pt.Contributions)
			{
				const FColor RayColor = C.bOccluded ? FColor::Red : FColor::Green;
				DrawDebugLine(World, Pt.WorldPosition + TraceOffset, C.LightWorldLocation, RayColor, false, -1.f, 0, 1.f);
				DrawDebugString(World, C.LightWorldLocation,
					FString::Printf(TEXT("%s %s %.2f%s"), *C.LightLabel, *C.LightClassName, C.Contribution,
						C.bOccluded ? TEXT(" (blk)") : TEXT("")),
					nullptr, RayColor, 0.f, true, 0.65f);
			}
		}

		const FVector HUDAnchor = Owner->GetActorLocation() + FVector(0.f, 0.f, 110.f);
		DrawDebugString(World, HUDAnchor,
			FString::Printf(TEXT("SceneLight exp=%.2f max=%.2f smooth=%.2f | cachedLights=%d"), LDebug.FinalExposure,
				LDebug.RawMaxExposure, LDebug.SmoothedExposure, LDebug.CachedLightCount),
			nullptr, FColor::Yellow, 0.f, true, 1.15f);
	}
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

	const FTransform Xform = Owner->GetActorTransform();
	TArray<FVector> BodyPts;
	if (const UCapsuleComponent* Capsule = Owner->FindComponentByClass<UCapsuleComponent>())
	{
		const FVector CapsuleCenter = Capsule->GetComponentLocation();
		const FVector Up = Capsule->GetUpVector();
		const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();

		BodyPts.Add(CapsuleCenter + Up * (HalfHeight * 0.65f) + Xform.TransformVector(Tuning->HeadSampleOffsetLocal));
		BodyPts.Add(CapsuleCenter + Xform.TransformVector(Tuning->ChestSampleOffsetLocal));
		BodyPts.Add(CapsuleCenter - Up * (HalfHeight * 0.75f) + Xform.TransformVector(Tuning->FeetSampleOffsetLocal));
	}
	else
	{
		BodyPts.Add(Xform.TransformPosition(Tuning->HeadSampleOffsetLocal));
		BodyPts.Add(Xform.TransformPosition(Tuning->ChestSampleOffsetLocal));
		BodyPts.Add(Xform.TransformPosition(Tuning->FeetSampleOffsetLocal));
	}

	TArray<FString> BodyLabels = {TEXT("Head"), TEXT("Chest"), TEXT("Feet")};
	const float LightExposure =
		Sim->SampleBodyLightExposureMaxBias(BodyPts, BodyLabels, Owner, DeltaTime);

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
	const float MultipliedVisibility = Tuning->BaseVisibility * StanceMul * MoveMul * ActionMul * LightExposure;
	Vis.CurrentVisibility = FMath::Clamp(FMath::Max(LightExposure, MultipliedVisibility), 0.f, 1.f);

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

	const bool bLandedThisFrame = bHasBodySample && bWasAirborne && Body.bGrounded;
	if (bLandedThisFrame)
	{
		const float LandingRadius = FMath::Max(Tuning->RunNoiseRadius, NoiseRadius) * 0.75f;

		FStealthSoundEvent LandingEvent;
		LandingEvent.Position = SampleLocation;
		LandingEvent.Loudness = FMath::Clamp(LandingRadius / FMath::Max(1.f, Tuning->SprintNoiseRadius), 0.8f, 1.f);
		LandingEvent.Radius = LandingRadius;
		LandingEvent.Surface = EStealthSurface::Unknown;
		LandingEvent.SourceType = EStealthSoundSource::Landing;
		LandingEvent.Lifetime = Tuning->FootstepEventLifetime;
		LandingEvent.DebugLabel = TEXT("Landing");

		const int32 Id = Sim->PushSoundEvent(LandingEvent);
		Sound.LastDiscreteSoundTime = World->GetTimeSeconds();
		(void)Id;
	}

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

	bHasBodySample = true;
	bWasAirborne = Body.bAirborne;

	Sim->SetPlayerVisibilityEmission(Vis);
	Sim->SetPlayerSoundEmission(Sound);
}
