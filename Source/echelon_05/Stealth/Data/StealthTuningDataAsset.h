#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/EngineTypes.h"

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

	// --- Scene light body sampling (TH-517) ---

	/** Optional local-space nudges. For capsule actors, base head/chest/feet positions come from capsule height first. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LightSampling")
	FVector HeadSampleOffsetLocal = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LightSampling")
	FVector ChestSampleOffsetLocal = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LightSampling")
	FVector FeetSampleOffsetLocal = FVector::ZeroVector;

	/** Seconds between rescans of the world's enabled visible lights into the candidate cache. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LightSampling", meta = (ClampMin = 0.05))
	float SceneLightCacheRefreshSeconds = 0.35f;

	/** Ignore lights whose component bounds sphere radius exceeds this (filters sky/atmosphere-sized proxies when radius is huge). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LightSampling", meta = (ClampMin = 100))
	float SceneLightMaxBoundsRadius = 250000.f;

	/** Do not evaluate lights farther than this from the sample (directionals exempt). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LightSampling", meta = (ClampMin = 100))
	float SceneLightMaxConsiderDistance = 60000.f;

	/** Trace channel for occlusion between sample point and light (project stealth visibility, not ALS collision). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LightSampling")
	TEnumAsByte<ECollisionChannel> SceneLightOcclusionChannel = ECC_Visibility;

	/** Extra pull toward max(body samples) so a lit head matters even if feet are dark. 1 = pure max; 0 = pure mean. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LightSampling", meta = (ClampMin = 0, ClampMax = 1))
	float SceneLightMaxBiasBlend = 0.85f;

	/** Baseline exposure added before clamp (simulates fill/indirect not modeled by direct light traces). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LightSampling", meta = (ClampMin = 0, ClampMax = 1))
	float SceneLightAmbientExposure = 0.06f;

	/** Raw summed contributions are divided by this before saturating to roughly 0..1 (tweak per level brightness).
	 *  Calibrate against the actual Intensity values of the scene's key lights. For physically-based lumen-scale lights
	 *  (~100 000 lm) use ~180 000. For low-range UE default units (800–2500) use ~1 500. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LightSampling", meta = (ClampMin = 1))
	float SceneLightIntensityNormalization = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LightSampling", meta = (ClampMin = 0, ClampMax = 1))
	float SceneLightDirectionalExposureScale = 1.f;

	/** Directional-light intensity that maps to SceneLightDirectionalExposureScale. UE sun values are lux-like and much smaller than local light lumens. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LightSampling", meta = (ClampMin = 0.01))
	float SceneLightDirectionalReferenceIntensity = 3.f;

	/** Seconds to smooth toward raw target (0 disables smoothing). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LightSampling", meta = (ClampMin = 0))
	float SceneLightExposureSmoothingHalfLife = 0.12f;

	/** Max contribution lines drawn per body sample when stealth.DebugDraw > 0. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LightSampling", meta = (ClampMin = 1, ClampMax = 16))
	int32 SceneLightDebugTopContributors = 4;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspicion", meta = (ClampMin = 0, ClampMax = 100))
	float FootstepEvidence = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspicion", meta = (ClampMin = 0, ClampMax = 100))
	float LandingEvidence = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspicion", meta = (ClampMin = 0, ClampMax = 100))
	float LureEvidence = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspicion", meta = (ClampMin = 0, ClampMax = 100))
	float DoorEvidence = 35.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspicion", meta = (ClampMin = 0, ClampMax = 100))
	float ObjectiveEvidence = 35.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspicion", meta = (ClampMin = 0, ClampMax = 100))
	float FallbackEvidence = 20.f;

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
