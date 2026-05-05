#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "StealthTuningDataAsset.generated.h"

/**
 * Readable tuning table for the stealth prototype (see STEALTH_ALS_PROTOTYPE_IMPL.md).
 */
UCLASS(BlueprintType)
class UStealthTuningDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visibility", meta = (ClampMin = 0, ClampMax = 1))
	float BaseVisibility = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visibility", meta = (ClampMin = 0))
	float StandingMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visibility", meta = (ClampMin = 0))
	float CrouchingMultiplier = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visibility", meta = (ClampMin = 0))
	float IdleMovementMultiplier = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visibility", meta = (ClampMin = 0))
	float WalkMovementMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visibility", meta = (ClampMin = 0))
	float RunMovementMultiplier = 1.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visibility", meta = (ClampMin = 0))
	float SprintMovementMultiplier = 1.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visibility", meta = (ClampMin = 0))
	float MantleActionMultiplier = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visibility", meta = (ClampMin = 0, ClampMax = 1))
	float ShadowLightExposure = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visibility", meta = (ClampMin = 0, ClampMax = 1))
	float LitLightExposure = 0.85f;

	/** When not inside any registered light volume, use this exposure (lit corridor default). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visibility", meta = (ClampMin = 0, ClampMax = 1))
	float DefaultLightExposureOutsideVolumes = 0.85f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise", meta = (ClampMin = 0))
	float WalkNoiseRadius = 450.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise", meta = (ClampMin = 0))
	float RunNoiseRadius = 900.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise", meta = (ClampMin = 0))
	float SprintNoiseRadius = 1400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise", meta = (ClampMin = 0, ClampMax = 1))
	float CrouchNoiseMultiplier = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise", meta = (ClampMin = 1))
	float FootstepStrideCentimeters = 85.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise", meta = (ClampMin = 0))
	float FootstepEventLifetime = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspicion", meta = (ClampMin = 0, ClampMax = 100))
	float SuspicionCurious = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspicion", meta = (ClampMin = 0, ClampMax = 100))
	float SuspicionSuspicious = 45.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspicion", meta = (ClampMin = 0, ClampMax = 100))
	float SuspicionInvestigating = 70.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspicion", meta = (ClampMin = 0, ClampMax = 100))
	float SuspicionAlert = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspicion", meta = (ClampMin = 0))
	float SuspicionDecayPerSecond = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspicion", meta = (ClampMin = 0))
	float VisualStimulusPerSecond = 35.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspicion", meta = (ClampMin = 0))
	float AudioStimulusPerSecond = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspicion", meta = (ClampMin = 1))
	float BrightLightSuspicionMultiplier = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspicion", meta = (ClampMin = 0, ClampMax = 1))
	float BrightLightSuspicionThreshold = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspicion", meta = (ClampMin = 1))
	float LoudSoundSuspicionMultiplier = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspicion", meta = (ClampMin = 0, ClampMax = 1))
	float LoudSoundSuspicionThreshold = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Adapter", meta = (ClampMin = 0))
	float IdleSpeedThreshold = 10.f;
};
