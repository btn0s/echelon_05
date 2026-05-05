#pragma once

#include "CoreMinimal.h"
#include "Stealth/Types/StealthEnums.h"

#include "GameplayTagContainer.h"

#include "StealthTypes.generated.h"

USTRUCT(BlueprintType)
struct FStealthMovementState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	EStealthStance Stance = EStealthStance::Standing;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	EStealthLocomotion Locomotion = EStealthLocomotion::Idle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth", meta = (ClampMin = 0))
	float Speed = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	FVector Velocity = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	bool bHasInput = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	bool bMoving = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	EStealthSurface Surface = EStealthSurface::Unknown;
};

USTRUCT(BlueprintType)
struct FStealthViewState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	FRotator ViewRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	FVector ViewDirection = FVector::ForwardVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth", meta = (ClampMin = 0))
	float ViewYawSpeed = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	FGameplayTag RotationMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	FGameplayTag ViewMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	bool bRightShoulder = false;
};

USTRUCT(BlueprintType)
struct FStealthBodyState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	bool bGrounded = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	bool bAirborne = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	EStealthLocomotionAction LocomotionAction = EStealthLocomotionAction::None;
};

USTRUCT(BlueprintType)
struct FVisibilityEmitter
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth", meta = (ClampMin = 0, ClampMax = 1))
	float CurrentVisibility = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth", meta = (ClampMin = 0, ClampMax = 1))
	float LightExposure = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth", meta = (ClampMin = 0))
	float SilhouetteExposure = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth", meta = (ClampMin = 0))
	float StanceMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth", meta = (ClampMin = 0))
	float MovementMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth", meta = (ClampMin = 0))
	float ActionMultiplier = 1.f;
};

/** Single scene light contribution toward one body sample (debug / HUD). */
USTRUCT(BlueprintType)
struct FStealthLightContributionDebug
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	FString LightLabel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	FString LightClassName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth", meta = (ClampMin = 0, ClampMax = 1))
	float Contribution = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	bool bOccluded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth", meta = (ClampMin = 0))
	float DistanceFromSample = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	FVector LightWorldLocation = FVector::ZeroVector;
};

/** One body sample point after scene-light evaluation. */
USTRUCT(BlueprintType)
struct FStealthBodyLightSampleDebug
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	FString SampleName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	FVector WorldPosition = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth", meta = (ClampMin = 0, ClampMax = 1))
	float Exposure = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	TArray<FStealthLightContributionDebug> Contributions;
};

/** Aggregate debug for TH-517 scene-light body sampling (HUD + optional debug draw). */
USTRUCT(BlueprintType)
struct FStealthLightSamplingDebug
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	TArray<FStealthBodyLightSampleDebug> BodySamples;

	/** Max exposure across body samples before smoothing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth", meta = (ClampMin = 0, ClampMax = 1))
	float RawMaxExposure = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth", meta = (ClampMin = 0, ClampMax = 1))
	float SmoothedExposure = 0.f;

	/** Value fed into visibility after smoothing (same as SmoothedExposure when smoothing enabled). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth", meta = (ClampMin = 0, ClampMax = 1))
	float FinalExposure = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	int32 CachedLightCount = 0;
};

USTRUCT(BlueprintType)
struct FSoundEmitter
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth", meta = (ClampMin = 0))
	float CurrentNoise = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth", meta = (ClampMin = 0))
	float Radius = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth", meta = (ClampMin = 0))
	float LastFootstepTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth", meta = (ClampMin = 0))
	float LastDiscreteSoundTime = 0.f;
};

USTRUCT(BlueprintType)
struct FStealthSoundEvent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	FVector Position = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth", meta = (ClampMin = 0))
	float Loudness = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth", meta = (ClampMin = 0))
	float Radius = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	EStealthSurface Surface = EStealthSurface::Unknown;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	EStealthSoundSource SourceType = EStealthSoundSource::Fallback;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	float SpawnTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth", meta = (ClampMin = 0))
	float Lifetime = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	int32 EventId = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	FString DebugLabel;
};

USTRUCT(BlueprintType)
struct FPerceptionSensor
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth", meta = (ClampMin = 0))
	float VisionRange = 2500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth", meta = (ClampMin = 0, ClampMax = 180))
	float VisionHalfAngleDegrees = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth", meta = (ClampMin = 0))
	float HearingRange = 3000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth", meta = (ClampMin = 0))
	float Acuity = 1.f;
};

USTRUCT(BlueprintType)
struct FSuspicionState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth", meta = (ClampMin = 0, ClampMax = 100))
	float Value = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	EGuardSuspicionState State = EGuardSuspicionState::Unaware;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	FVector LastKnownPosition = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth", meta = (ClampMin = 0))
	float LastStimulusTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	FString LastReason;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	bool bHadVisualStimulus = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	bool bHadAuditoryStimulus = false;
};

USTRUCT(BlueprintType)
struct FGuardBrain
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	EGuardBrainMode Mode = EGuardBrainMode::Patrol;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	FVector IntentTarget = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	int32 PatrolIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	float SearchTimer = 0.f;
};

USTRUCT(BlueprintType)
struct FPatrolRoute
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	TArray<FVector> Points;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	int32 CurrentIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	bool bLoop = true;
};

USTRUCT(BlueprintType)
struct FObjectiveState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	bool bRequired = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	bool bCompleted = false;
};

USTRUCT(BlueprintType)
struct FExtractionState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	bool bAvailable = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	bool bUsed = false;
};

USTRUCT(BlueprintType)
struct FAlarmState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth", meta = (ClampMin = 0))
	float Level = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	bool bActive = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	FString Reason;
};
