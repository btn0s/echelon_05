#include "Stealth/Subsystems/StealthSimulationSubsystem.h"

#include "Stealth/Actors/StealthLightVolume.h"
#include "Stealth/Components/StealthGuardBrainComponent.h"
#include "Stealth/Components/StealthHealthComponent.h"
#include "Stealth/Data/StealthTuningDataAsset.h"
#include "Stealth/StealthLog.h"

#include "CollisionQueryParams.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/LightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/World.h"
#include "EngineUtils.h"

void UStealthSimulationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UWorld* World = GetWorld();
	if (!TuningAsset && World)
	{
		TuningAsset = NewObject<UStealthTuningDataAsset>(World);
	}

	ResetSimulation();
}

void UStealthSimulationSubsystem::Deinitialize()
{
	LightVolumes.Reset();
	CachedSceneLights.Reset();
	GuardBrains.Reset();
	HealthComponents.Reset();
	ActiveSoundEvents.Reset();
	Super::Deinitialize();
}

TStatId UStealthSimulationSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UStealthSimulationSubsystem, STATGROUP_Tickables);
}

void UStealthSimulationSubsystem::Tick(float DeltaTime)
{
	if (!GetWorld())
	{
		return;
	}

	ExpireSoundEvents(GetWorld()->GetTimeSeconds());
	RefreshObjectiveExtractionGating();
}

void UStealthSimulationSubsystem::ResetSimulation()
{
	PlayerMovement = FStealthMovementState();
	PlayerView = FStealthViewState();
	PlayerBody = FStealthBodyState();
	AlsDebugSnapshot = FStealthAlsDebugSnapshot();
	PlayerVisibility = FVisibilityEmitter();
	PlayerSoundEmission = FSoundEmitter();
	ActiveSoundEvents.Reset();
	NextSoundEventId = 1;
	ObjectiveState = FObjectiveState();
	ExtractionState = FExtractionState();
	AlarmState = FAlarmState();
	ObjectiveRecords.Reset();
	bAlertOccurred = false;
	MissionOutcome = EStealthMissionOutcome::None;
	PlayerSmoothedLightExposure = 0.f;
	LastLightSamplingDebug = FStealthLightSamplingDebug();
	LastSceneLightCacheTime = -100000.f;
	CachedSceneLights.Reset();
	CachedPPVIndirectScale = 1.f;
	CachedPPVExposureScale = 1.f;
}

void UStealthSimulationSubsystem::SetTuningAsset(UStealthTuningDataAsset* InTuning)
{
	TuningAsset = InTuning;
}

void UStealthSimulationSubsystem::SetPlayerSnapshot(const FStealthMovementState& Movement, const FStealthViewState& View,
	const FStealthBodyState& Body, const FStealthAlsDebugSnapshot& AlsDebug)
{
	PlayerMovement = Movement;
	PlayerView = View;
	PlayerBody = Body;
	AlsDebugSnapshot = AlsDebug;
	OnPlayerSnapshotUpdated.Broadcast();
}

void UStealthSimulationSubsystem::SetPlayerVisibilityEmission(const FVisibilityEmitter& Emitter)
{
	PlayerVisibility = Emitter;
}

void UStealthSimulationSubsystem::SetPlayerSoundEmission(const FSoundEmitter& Emitter)
{
	PlayerSoundEmission = Emitter;
}

int32 UStealthSimulationSubsystem::PushSoundEvent(const FStealthSoundEvent& Event)
{
	if (!GetWorld())
	{
		return -1;
	}

	FStealthSoundEvent Copy = Event;
	Copy.EventId = NextSoundEventId++;
	Copy.SpawnTime = GetWorld()->GetTimeSeconds();
	ActiveSoundEvents.Add(Copy);
	OnSoundEvent.Broadcast(Copy);
	return Copy.EventId;
}

void UStealthSimulationSubsystem::RegisterLightVolume(AStealthLightVolume* Volume)
{
	if (Volume)
	{
		LightVolumes.AddUnique(Volume);
	}
}

void UStealthSimulationSubsystem::UnregisterLightVolume(AStealthLightVolume* Volume)
{
	LightVolumes.Remove(Volume);
}

void UStealthSimulationSubsystem::RegisterGuardBrain(UStealthGuardBrainComponent* Brain)
{
	if (Brain)
	{
		GuardBrains.AddUnique(Brain);
	}
}

void UStealthSimulationSubsystem::UnregisterGuardBrain(UStealthGuardBrainComponent* Brain)
{
	GuardBrains.Remove(Brain);
}

void UStealthSimulationSubsystem::RegisterHealthComponent(UStealthHealthComponent* HealthComponent)
{
	if (HealthComponent)
	{
		HealthComponents.AddUnique(HealthComponent);
	}
}

void UStealthSimulationSubsystem::UnregisterHealthComponent(UStealthHealthComponent* HealthComponent)
{
	HealthComponents.Remove(HealthComponent);
}

FName UStealthSimulationSubsystem::RegisterObjective(AActor* ObjectiveActor, FName ObjectiveId, FText DisplayName,
	bool bRequired)
{
	if (!ObjectiveActor)
	{
		return NAME_None;
	}

	const FName ResolvedId = ObjectiveId.IsNone() ? ObjectiveActor->GetFName() : ObjectiveId;
	const FText ResolvedName = DisplayName.IsEmpty() ? FText::FromName(ResolvedId) : DisplayName;

	int32 Index = FindObjectiveIndexByActor(ObjectiveActor);
	if (Index == INDEX_NONE)
	{
		Index = FindObjectiveIndexById(ResolvedId);
	}

	if (Index == INDEX_NONE)
	{
		FStealthObjectiveRecord& Record = ObjectiveRecords.AddDefaulted_GetRef();
		Record.ObjectiveId = ResolvedId;
		Record.DisplayName = ResolvedName;
		Record.bRequired = bRequired;
		Record.SourceActor = ObjectiveActor;
	}
	else
	{
		FStealthObjectiveRecord& Record = ObjectiveRecords[Index];
		Record.ObjectiveId = ResolvedId;
		Record.DisplayName = ResolvedName;
		Record.bRequired = bRequired;
		Record.SourceActor = ObjectiveActor;
	}

	RecomputeObjectiveStateFromRecords();
	RefreshObjectiveExtractionGating();
	return ResolvedId;
}

void UStealthSimulationSubsystem::UnregisterObjective(AActor* ObjectiveActor)
{
	if (!ObjectiveActor)
	{
		return;
	}

	ObjectiveRecords.RemoveAll([ObjectiveActor](const FStealthObjectiveRecord& Record)
	{
		return Record.SourceActor == ObjectiveActor;
	});

	RecomputeObjectiveStateFromRecords();
	RefreshObjectiveExtractionGating();
}

bool UStealthSimulationSubsystem::CompleteObjective(AActor* ObjectiveActor, APawn* InstigatorPawn)
{
	(void)InstigatorPawn;

	const int32 Index = FindObjectiveIndexByActor(ObjectiveActor);
	if (Index == INDEX_NONE)
	{
		return false;
	}

	FStealthObjectiveRecord& Record = ObjectiveRecords[Index];
	if (Record.bCompleted)
	{
		return false;
	}

	Record.bCompleted = true;
	Record.CompletedTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

	RecomputeObjectiveStateFromRecords();
	RefreshObjectiveExtractionGating();
	return true;
}

bool UStealthSimulationSubsystem::CompleteObjectiveById(FName ObjectiveId, APawn* InstigatorPawn)
{
	(void)InstigatorPawn;

	const int32 Index = FindObjectiveIndexById(ObjectiveId);
	if (Index == INDEX_NONE)
	{
		return false;
	}

	FStealthObjectiveRecord& Record = ObjectiveRecords[Index];
	if (Record.bCompleted)
	{
		return false;
	}

	Record.bCompleted = true;
	Record.CompletedTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

	RecomputeObjectiveStateFromRecords();
	RefreshObjectiveExtractionGating();
	return true;
}

void UStealthSimulationSubsystem::SetObjectiveState(const FObjectiveState& State)
{
	ObjectiveState = State;
	RefreshObjectiveExtractionGating();
}

void UStealthSimulationSubsystem::SetExtractionState(const FExtractionState& State)
{
	ExtractionState = State;
}

void UStealthSimulationSubsystem::SetAlarmState(const FAlarmState& State)
{
	AlarmState = State;
}

void UStealthSimulationSubsystem::MarkAlertOccurred()
{
	bAlertOccurred = true;
}

void UStealthSimulationSubsystem::SetMissionOutcome(EStealthMissionOutcome Outcome)
{
	MissionOutcome = Outcome;
}

bool UStealthSimulationSubsystem::AreRequiredObjectivesComplete() const
{
	if (ObjectiveRecords.Num() == 0)
	{
		return ObjectiveState.bCompleted;
	}

	bool bHasRequired = false;
	for (const FStealthObjectiveRecord& Record : ObjectiveRecords)
	{
		if (!Record.bRequired)
		{
			continue;
		}

		bHasRequired = true;
		if (!Record.bCompleted)
		{
			return false;
		}
	}

	return bHasRequired ? true : ObjectiveState.bCompleted;
}

int32 UStealthSimulationSubsystem::GetRequiredObjectiveCount() const
{
	int32 Count = 0;
	for (const FStealthObjectiveRecord& Record : ObjectiveRecords)
	{
		if (Record.bRequired)
		{
			++Count;
		}
	}
	return Count;
}

int32 UStealthSimulationSubsystem::GetCompletedRequiredObjectiveCount() const
{
	int32 Count = 0;
	for (const FStealthObjectiveRecord& Record : ObjectiveRecords)
	{
		if (Record.bRequired && Record.bCompleted)
		{
			++Count;
		}
	}
	return Count;
}

FStealthHealthState UStealthSimulationSubsystem::GetPlayerHealthState() const
{
	for (const TWeakObjectPtr<UStealthHealthComponent>& HealthPtr : HealthComponents)
	{
		const UStealthHealthComponent* Health = HealthPtr.Get();
		if (Health && Health->GetTeam() == EStealthTeam::Player)
		{
			return Health->GetHealthState();
		}
	}

	return FStealthHealthState();
}

namespace StealthLightSamplingPrivate
{
	static FString BuildLightLabel(const ULightComponentBase* Light)
	{
		if (!Light)
		{
			return TEXT("(null)");
		}
		const AActor* Owner = Light->GetOwner();
		const FString OwnerLabel = Owner ? Owner->GetActorNameOrLabel() : FString(TEXT("(no owner)"));
		return FString::Printf(TEXT("%s.%s"), *OwnerLabel, *Light->GetName());
	}

	static bool OcclusionHitWorld(UWorld* World, const FVector& SampleWorldPosition, const FVector& TargetWorldPosition,
		const AActor* OcclusionIgnoreActor, ECollisionChannel Channel)
	{
		if (!World)
		{
			return false;
		}
		const FVector Dir = (TargetWorldPosition - SampleWorldPosition);
		const float Dist = Dir.Size();
		if (Dist <= KINDA_SMALL_NUMBER)
		{
			return false;
		}
		const FVector DirNorm = Dir / Dist;
		const FVector TraceStart = SampleWorldPosition + DirNorm * 2.f;
		const FVector TraceEnd = TargetWorldPosition - DirNorm * 2.f;

		FCollisionQueryParams Params(SCENE_QUERY_STAT(StealthLightOcclusion), true);
		if (OcclusionIgnoreActor)
		{
			Params.AddIgnoredActor(OcclusionIgnoreActor);
		}
		FHitResult Hit;
		const bool bHit = World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, Channel, Params);
		return bHit;
	}

	static float EvaluateDirectional(const UDirectionalLightComponent* DirLight, const FVector& SampleWorldPosition,
		const UStealthTuningDataAsset* Tuning, UWorld* World, const AActor* OcclusionIgnoreActor, ECollisionChannel Channel,
		FStealthLightContributionDebug* OutDebug)
	{
		if (!DirLight || !World || !Tuning)
		{
			return 0.f;
		}
		const FVector ToSunApprox = -DirLight->GetForwardVector();
		const FVector TraceTarget = SampleWorldPosition + ToSunApprox * 250000.f;
		const bool bOccluded = OcclusionHitWorld(World, SampleWorldPosition, TraceTarget, OcclusionIgnoreActor, Channel);

		const float Intensity = DirLight->Intensity;
		const float ReferenceIntensity = FMath::Max(0.01f, Tuning->SceneLightDirectionalReferenceIntensity);
		float Applied = Tuning->SceneLightDirectionalExposureScale * FMath::Clamp(Intensity / ReferenceIntensity, 0.f, 2.f);
		if (bOccluded)
		{
			Applied = 0.f;
		}

		if (OutDebug)
		{
			OutDebug->LightLabel = BuildLightLabel(DirLight);
			OutDebug->LightClassName = DirLight->GetClass()->GetName();
			OutDebug->Contribution = Applied;
			OutDebug->bOccluded = bOccluded;
			OutDebug->DistanceFromSample = 0.f;
			OutDebug->LightWorldLocation = TraceTarget;
		}
		return Applied;
	}

	static float EvaluateRendererRadiusMask(float Distance, float AttenuationRadius, bool bInverseSquared, float FalloffExponent)
	{
		if (AttenuationRadius <= KINDA_SMALL_NUMBER || Distance > AttenuationRadius)
		{
			return 0.f;
		}

		const float DistanceSqr = FMath::Square(Distance);
		const float RadiusSqr = FMath::Square(AttenuationRadius);
		const float DistanceSqrOverRadiusSqr = FMath::Clamp(DistanceSqr / FMath::Max(RadiusSqr, KINDA_SMALL_NUMBER), 0.f, 1.f);

		if (bInverseSquared)
		{
			return FMath::Square(1.f - FMath::Square(DistanceSqrOverRadiusSqr));
		}

		return FMath::Pow(1.f - DistanceSqrOverRadiusSqr, FMath::Max(FalloffExponent, KINDA_SMALL_NUMBER));
	}

	static float EvaluateLocalLightExposure(float Intensity, float Distance, float AttenuationRadius, bool bInverseSquared,
		float FalloffExponent, float ExposureScale, const UStealthTuningDataAsset* Tuning)
	{
		if (!Tuning)
		{
			return 0.f;
		}

		const float RadiusMask = EvaluateRendererRadiusMask(Distance, AttenuationRadius, bInverseSquared, FalloffExponent);
		const float NormalizedEnergy =
			(FMath::Max(0.f, Intensity) * FMath::Max(0.f, ExposureScale)) /
			FMath::Max(1.f, Tuning->SceneLightIntensityNormalization);
		const float Response = FMath::Max(0.f, Tuning->SceneLightLocalExposureResponse);

		return FMath::Clamp(1.f - FMath::Exp(-NormalizedEnergy * RadiusMask * Response), 0.f, 1.f);
	}

	static float EvaluatePointLight(const UPointLightComponent* PL, const FVector& SampleWorldPosition,
		const UStealthTuningDataAsset* Tuning, UWorld* World, const AActor* OcclusionIgnoreActor, ECollisionChannel Channel,
		float ExposureScale, FStealthLightContributionDebug* OutDebug)
	{
		if (!PL || !World || !Tuning)
		{
			return 0.f;
		}
		const FVector LLoc = PL->GetComponentLocation();
		const float Dist = FVector::Dist(SampleWorldPosition, LLoc);
		if (Dist > Tuning->SceneLightMaxConsiderDistance)
		{
			return 0.f;
		}

		const FVector TraceTarget = LLoc;
		const bool bOccluded = OcclusionHitWorld(World, SampleWorldPosition, TraceTarget, OcclusionIgnoreActor, Channel);

		float Applied = EvaluateLocalLightExposure(PL->Intensity, Dist, PL->AttenuationRadius,
			PL->bUseInverseSquaredFalloff != 0, PL->LightFalloffExponent, ExposureScale, Tuning);
		if (bOccluded)
		{
			Applied = 0.f;
		}

		if (OutDebug)
		{
			OutDebug->LightLabel = BuildLightLabel(PL);
			OutDebug->LightClassName = PL->GetClass()->GetName();
			OutDebug->Contribution = Applied;
			OutDebug->bOccluded = bOccluded;
			OutDebug->DistanceFromSample = Dist;
			OutDebug->LightWorldLocation = LLoc;
		}
		return Applied;
	}

	static float EvaluateSpotLight(const USpotLightComponent* SL, const FVector& SampleWorldPosition,
		const UStealthTuningDataAsset* Tuning, UWorld* World, const AActor* OcclusionIgnoreActor, ECollisionChannel Channel,
		float ExposureScale, FStealthLightContributionDebug* OutDebug)
	{
		if (!SL || !World || !Tuning)
		{
			return 0.f;
		}
		const FVector LLoc = SL->GetComponentLocation();
		const FVector LToS = (SampleWorldPosition - LLoc).GetSafeNormal();
		const float Dist = FVector::Dist(SampleWorldPosition, LLoc);
		if (Dist > Tuning->SceneLightMaxConsiderDistance || Dist > SL->AttenuationRadius)
		{
			return 0.f;
		}

		const float OuterHalfRad = FMath::DegreesToRadians(SL->OuterConeAngle * 0.5f);
		const float CosOuter = FMath::Cos(OuterHalfRad);
		const float DotAxis = FVector::DotProduct(LToS, SL->GetForwardVector());
		if (DotAxis < CosOuter)
		{
			return 0.f;
		}

		const float InnerHalfRad = FMath::DegreesToRadians(SL->InnerConeAngle * 0.5f);
		const float CosInner = FMath::Cos(InnerHalfRad);
		float ConeMul = 1.f;
		if (DotAxis < CosInner)
		{
			ConeMul = FMath::Clamp((DotAxis - CosOuter) / FMath::Max(CosInner - CosOuter, KINDA_SMALL_NUMBER), 0.f, 1.f);
		}

		const FVector TraceTarget = LLoc;
		const bool bOccluded = OcclusionHitWorld(World, SampleWorldPosition, TraceTarget, OcclusionIgnoreActor, Channel);

		float Applied =
			EvaluateLocalLightExposure(SL->Intensity, Dist, SL->AttenuationRadius,
				SL->bUseInverseSquaredFalloff != 0, SL->LightFalloffExponent, ExposureScale, Tuning) * ConeMul;
		if (bOccluded)
		{
			Applied = 0.f;
		}

		if (OutDebug)
		{
			OutDebug->LightLabel = BuildLightLabel(SL);
			OutDebug->LightClassName = SL->GetClass()->GetName();
			OutDebug->Contribution = Applied;
			OutDebug->bOccluded = bOccluded;
			OutDebug->DistanceFromSample = Dist;
			OutDebug->LightWorldLocation = LLoc;
		}
		return Applied;
	}

	static float EvaluateRectLight(const URectLightComponent* RL, const FVector& SampleWorldPosition,
		const UStealthTuningDataAsset* Tuning, UWorld* World, const AActor* OcclusionIgnoreActor, ECollisionChannel Channel,
		float ExposureScale, FStealthLightContributionDebug* OutDebug)
	{
		if (!RL || !World || !Tuning)
		{
			return 0.f;
		}
		const FVector LLoc = RL->GetComponentLocation();
		const float Dist = FVector::Dist(SampleWorldPosition, LLoc);
		if (Dist > Tuning->SceneLightMaxConsiderDistance || Dist > RL->AttenuationRadius)
		{
			return 0.f;
		}

		const FVector TraceTarget = LLoc;
		const bool bOccluded = OcclusionHitWorld(World, SampleWorldPosition, TraceTarget, OcclusionIgnoreActor, Channel);

		float Applied = EvaluateLocalLightExposure(RL->Intensity, Dist, RL->AttenuationRadius,
			false, 2.f, ExposureScale, Tuning);
		if (bOccluded)
		{
			Applied = 0.f;
		}

		if (OutDebug)
		{
			OutDebug->LightLabel = BuildLightLabel(RL);
			OutDebug->LightClassName = RL->GetClass()->GetName();
			OutDebug->Contribution = Applied;
			OutDebug->bOccluded = bOccluded;
			OutDebug->DistanceFromSample = Dist;
			OutDebug->LightWorldLocation = LLoc;
		}
		return Applied;
	}
} // namespace StealthLightSamplingPrivate

void UStealthSimulationSubsystem::RefreshSceneLightCache(float WorldTimeSeconds)
{
	UWorld* World = GetWorld();
	if (!World || !TuningAsset)
	{
		return;
	}

	const float Interval = FMath::Max(0.05f, TuningAsset->SceneLightCacheRefreshSeconds);
	if ((WorldTimeSeconds - LastSceneLightCacheTime) < Interval && CachedSceneLights.Num() > 0)
	{
		return;
	}

	LastSceneLightCacheTime = WorldTimeSeconds;
	CachedSceneLights.Reset();

	const float MaxBoundsRadius = TuningAsset->SceneLightMaxBoundsRadius;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor)
		{
			continue;
		}

		TArray<ULightComponentBase*> LightComps;
		Actor->GetComponents<ULightComponentBase>(LightComps);

		for (ULightComponentBase* Light : LightComps)
		{
			if (!Light || !Light->IsRegistered())
			{
				continue;
			}

			if (!Light->IsVisible())
			{
				continue;
			}

			if (Cast<USkyLightComponent>(Light))
			{
				continue;
			}

			const float BoundsRadius = Light->Bounds.SphereRadius;
			const bool bDirectional = Cast<UDirectionalLightComponent>(Light) != nullptr;
			if (!bDirectional && BoundsRadius > MaxBoundsRadius)
			{
				continue;
			}

			CachedSceneLights.Add(Light);
		}
	}

	RefreshPPVLightScales();
}

void UStealthSimulationSubsystem::RefreshPPVLightScales()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		CachedPPVIndirectScale = 1.f;
		CachedPPVExposureScale = 1.f;
		return;
	}

	struct FPPVLightBlend
	{
		float Priority = 0.f;
		float BlendWeight = 0.f;
		float IndirectLightingIntensity = 1.f;
		float AutoExposureBias = 0.f;
		bool bOverridesIndirectLightingIntensity = false;
		bool bOverridesAutoExposureBias = false;
		FString StableName;
	};

	TArray<FPPVLightBlend> Blends;

	// Walk unbounded PostProcessVolumes that affect stealth light interpretation, then apply
	// them in priority order. This keeps the sim in step with post-process blending instead
	// of depending on TActorIterator order.
	for (TActorIterator<APostProcessVolume> It(World); It; ++It)
	{
		const APostProcessVolume* PPV = *It;
		if (!PPV || !PPV->bEnabled || !PPV->bUnbound)
		{
			continue;
		}

		if (PPV->Settings.bOverride_IndirectLightingIntensity || PPV->Settings.bOverride_AutoExposureBias)
		{
			const float BlendWeight = FMath::Clamp(PPV->BlendWeight, 0.f, 1.f);
			if (BlendWeight <= UE_KINDA_SMALL_NUMBER)
			{
				continue;
			}

			FPPVLightBlend& Blend = Blends.AddDefaulted_GetRef();
			Blend.Priority = PPV->Priority;
			Blend.BlendWeight = BlendWeight;
			Blend.bOverridesIndirectLightingIntensity = PPV->Settings.bOverride_IndirectLightingIntensity;
			Blend.bOverridesAutoExposureBias = PPV->Settings.bOverride_AutoExposureBias;
			if (Blend.bOverridesIndirectLightingIntensity)
			{
				Blend.IndirectLightingIntensity = FMath::Clamp(PPV->Settings.IndirectLightingIntensity, 0.f, 1.f);
			}
			if (Blend.bOverridesAutoExposureBias)
			{
				Blend.AutoExposureBias = PPV->Settings.AutoExposureBias;
			}
			Blend.StableName = PPV->GetName();
		}
	}

	Blends.Sort([](const FPPVLightBlend& A, const FPPVLightBlend& B)
	{
		if (!FMath::IsNearlyEqual(A.Priority, B.Priority))
		{
			return A.Priority < B.Priority;
		}

		return A.StableName < B.StableName;
	});

	float IndirectScale = 1.f;
	float ExposureBias = 0.f;
	for (const FPPVLightBlend& Blend : Blends)
	{
		if (Blend.bOverridesIndirectLightingIntensity)
		{
			IndirectScale = FMath::Lerp(IndirectScale, Blend.IndirectLightingIntensity, Blend.BlendWeight);
		}
		if (Blend.bOverridesAutoExposureBias)
		{
			ExposureBias = FMath::Lerp(ExposureBias, Blend.AutoExposureBias, Blend.BlendWeight);
		}
	}

	CachedPPVIndirectScale = IndirectScale;
	CachedPPVExposureScale = FMath::Pow(2.f, ExposureBias);
}

float UStealthSimulationSubsystem::ComputeSceneLightExposureAt(const FVector& SampleWorldPosition,
	const AActor* OcclusionIgnoreActor, TArray<FStealthLightContributionDebug>* OutSortedContributions) const
{
	UWorld* World = GetWorld();
	if (!World || !TuningAsset)
	{
		return TuningAsset ? TuningAsset->ShadowLightExposure : 0.15f;
	}

	const ECollisionChannel Channel = TuningAsset->SceneLightOcclusionChannel.GetValue();

	TArray<FStealthLightContributionDebug> AllContribs;

	// Ambient floor is scaled by the active PPV's IndirectLightingIntensity override so the
	// sim's baseline tracks the renderer: if Lumen GI is suppressed to zero, the floor is zero.
	float Sum = TuningAsset->SceneLightAmbientExposure * CachedPPVIndirectScale;

	for (const TWeakObjectPtr<ULightComponentBase>& Ptr : CachedSceneLights)
	{
		ULightComponentBase* LightBase = Ptr.Get();
		if (!LightBase)
		{
			continue;
		}

		FStealthLightContributionDebug Row;
		float Added = 0.f;

		if (UDirectionalLightComponent* DirLight = Cast<UDirectionalLightComponent>(LightBase))
		{
			Added = StealthLightSamplingPrivate::EvaluateDirectional(
				DirLight, SampleWorldPosition, TuningAsset, World, OcclusionIgnoreActor, Channel, &Row);
		}
		else if (USpotLightComponent* SL = Cast<USpotLightComponent>(LightBase))
		{
			Added = StealthLightSamplingPrivate::EvaluateSpotLight(
				SL, SampleWorldPosition, TuningAsset, World, OcclusionIgnoreActor, Channel, CachedPPVExposureScale, &Row);
		}
		else if (UPointLightComponent* PL = Cast<UPointLightComponent>(LightBase))
		{
			Added = StealthLightSamplingPrivate::EvaluatePointLight(
				PL, SampleWorldPosition, TuningAsset, World, OcclusionIgnoreActor, Channel, CachedPPVExposureScale, &Row);
		}
		else if (URectLightComponent* RL = Cast<URectLightComponent>(LightBase))
		{
			Added = StealthLightSamplingPrivate::EvaluateRectLight(
				RL, SampleWorldPosition, TuningAsset, World, OcclusionIgnoreActor, Channel, CachedPPVExposureScale, &Row);
		}
		else
		{
			continue;
		}

		if (Added > KINDA_SMALL_NUMBER || Row.bOccluded)
		{
			AllContribs.Add(Row);
		}

		Sum += Added;
	}

	const float Saturated = FMath::Clamp(Sum, 0.f, 1.f);

	if (OutSortedContributions)
	{
		AllContribs.Sort([](const FStealthLightContributionDebug& A, const FStealthLightContributionDebug& B)
		{
			return A.Contribution > B.Contribution;
		});

		const int32 MaxRows = FMath::Clamp(TuningAsset->SceneLightDebugTopContributors, 1, 16);
		for (int32 i = 0; i < AllContribs.Num() && i < MaxRows; ++i)
		{
			OutSortedContributions->Add(AllContribs[i]);
		}
	}

	return Saturated;
}

float UStealthSimulationSubsystem::SampleLightExposureAt(const FVector& WorldLocation, const AActor* OcclusionIgnoreActor)
{
	if (!GetWorld())
	{
		return TuningAsset ? TuningAsset->ShadowLightExposure : 0.15f;
	}

	RefreshSceneLightCache(GetWorld()->GetTimeSeconds());
	return ComputeSceneLightExposureAt(WorldLocation, OcclusionIgnoreActor, nullptr);
}

float UStealthSimulationSubsystem::SampleBodyLightExposureRaw(const TArray<FVector>& BodyWorldPositions,
	const TArray<FString>& BodyLabels, const AActor* OcclusionIgnoreActor, FStealthLightSamplingDebug* OutDebug)
{
	FStealthLightSamplingDebug Debug;
	Debug.BodySamples.Reserve(BodyWorldPositions.Num());

	if (!GetWorld() || !TuningAsset)
	{
		if (OutDebug)
		{
			*OutDebug = Debug;
		}
		return 0.f;
	}

	RefreshSceneLightCache(GetWorld()->GetTimeSeconds());
	Debug.CachedLightCount = CachedSceneLights.Num();

	if (BodyWorldPositions.Num() == 0)
	{
		if (OutDebug)
		{
			*OutDebug = Debug;
		}
		return 0.f;
	}

	float MaxExp = 0.f;
	float MeanAccum = 0.f;

	for (int32 i = 0; i < BodyWorldPositions.Num(); ++i)
	{
		FStealthBodyLightSampleDebug Pt;
		Pt.SampleName = BodyLabels.IsValidIndex(i) ? BodyLabels[i] : FString::Printf(TEXT("Sample_%d"), i);
		Pt.WorldPosition = BodyWorldPositions[i];

		TArray<FStealthLightContributionDebug> Contribs;
		Pt.Exposure = ComputeSceneLightExposureAt(Pt.WorldPosition, OcclusionIgnoreActor, &Contribs);
		Pt.Contributions = MoveTemp(Contribs);

		Debug.BodySamples.Add(Pt);
		MaxExp = FMath::Max(MaxExp, Pt.Exposure);
		MeanAccum += Pt.Exposure;
	}

	const float MeanExp = MeanAccum / static_cast<float>(BodyWorldPositions.Num());
	const float BlendAlpha = FMath::Clamp(TuningAsset->SceneLightMaxBiasBlend, 0.f, 1.f);
	const float RawCombined = FMath::Lerp(MeanExp, MaxExp, BlendAlpha);

	Debug.RawMaxExposure = MaxExp;
	Debug.SmoothedExposure = RawCombined;
	Debug.FinalExposure = RawCombined;

	if (OutDebug)
	{
		*OutDebug = Debug;
	}

	return RawCombined;
}

float UStealthSimulationSubsystem::SampleBodyLightExposureMaxBias(const TArray<FVector>& BodyWorldPositions,
	const TArray<FString>& BodyLabels, const AActor* OcclusionIgnoreActor, float DeltaTime)
{
	FStealthLightSamplingDebug Debug;
	Debug.BodySamples.Reserve(BodyWorldPositions.Num());

	if (!GetWorld())
	{
		LastLightSamplingDebug = Debug;
		return PlayerSmoothedLightExposure;
	}

	if (!TuningAsset)
	{
		LastLightSamplingDebug = Debug;
		return PlayerSmoothedLightExposure;
	}

	RefreshSceneLightCache(GetWorld()->GetTimeSeconds());
	Debug.CachedLightCount = CachedSceneLights.Num();

	if (BodyWorldPositions.Num() == 0)
	{
		Debug.FinalExposure = PlayerSmoothedLightExposure;
		LastLightSamplingDebug = Debug;
		return PlayerSmoothedLightExposure;
	}

	float MaxExp = 0.f;
	float MeanAccum = 0.f;

	for (int32 i = 0; i < BodyWorldPositions.Num(); ++i)
	{
		const FVector P = BodyWorldPositions[i];
		FStealthBodyLightSampleDebug Pt;
		Pt.SampleName = BodyLabels.IsValidIndex(i) ? BodyLabels[i] : FString::Printf(TEXT("Sample_%d"), i);
		Pt.WorldPosition = P;

		TArray<FStealthLightContributionDebug> Contribs;
		Pt.Exposure = ComputeSceneLightExposureAt(P, OcclusionIgnoreActor, &Contribs);
		Pt.Contributions = MoveTemp(Contribs);

		Debug.BodySamples.Add(Pt);

		MaxExp = FMath::Max(MaxExp, Pt.Exposure);
		MeanAccum += Pt.Exposure;
	}

	const float MeanExp = MeanAccum / static_cast<float>(BodyWorldPositions.Num());
	const float BlendAlpha = FMath::Clamp(TuningAsset->SceneLightMaxBiasBlend, 0.f, 1.f);
	const float RawCombined = FMath::Lerp(MeanExp, MaxExp, BlendAlpha);

	Debug.RawMaxExposure = MaxExp;

	float Smoothed = PlayerSmoothedLightExposure;
	const float HalfLife = TuningAsset->SceneLightExposureSmoothingHalfLife;
	if (HalfLife <= KINDA_SMALL_NUMBER)
	{
		Smoothed = RawCombined;
	}
	else
	{
		const float Lambda = 0.69314718f / HalfLife;
		const float Alpha = 1.f - FMath::Exp(-Lambda * FMath::Max(DeltaTime, KINDA_SMALL_NUMBER));
		Smoothed = FMath::Lerp(PlayerSmoothedLightExposure, RawCombined, Alpha);
	}

	PlayerSmoothedLightExposure = Smoothed;
	Debug.SmoothedExposure = Smoothed;
	Debug.FinalExposure = Smoothed;
	LastLightSamplingDebug = Debug;

	return Smoothed;
}

void UStealthSimulationSubsystem::ExpireSoundEvents(float WorldTimeSeconds)
{
	ActiveSoundEvents.RemoveAll([&](const FStealthSoundEvent& E)
	{
		return WorldTimeSeconds > E.SpawnTime + E.Lifetime;
	});
}

void UStealthSimulationSubsystem::RefreshObjectiveExtractionGating()
{
	if (ObjectiveRecords.Num() > 0)
	{
		RecomputeObjectiveStateFromRecords();
	}

	FExtractionState Next = ExtractionState;
	Next.bAvailable = AreRequiredObjectivesComplete();
	if (!Next.bAvailable)
	{
		Next.bUsed = false;
	}
	ExtractionState = Next;
}

void UStealthSimulationSubsystem::RecomputeObjectiveStateFromRecords()
{
	if (ObjectiveRecords.Num() == 0)
	{
		return;
	}

	bool bHasRequired = false;
	bool bAllRequiredCompleted = true;
	bool bAnyCompleted = false;

	for (const FStealthObjectiveRecord& Record : ObjectiveRecords)
	{
		bAnyCompleted |= Record.bCompleted;
		if (Record.bRequired)
		{
			bHasRequired = true;
			bAllRequiredCompleted &= Record.bCompleted;
		}
	}

	ObjectiveState.bRequired = bHasRequired;
	ObjectiveState.bCompleted = bHasRequired ? bAllRequiredCompleted : bAnyCompleted;
}

int32 UStealthSimulationSubsystem::FindObjectiveIndexByActor(const AActor* ObjectiveActor) const
{
	if (!ObjectiveActor)
	{
		return INDEX_NONE;
	}

	for (int32 i = 0; i < ObjectiveRecords.Num(); ++i)
	{
		if (ObjectiveRecords[i].SourceActor == ObjectiveActor)
		{
			return i;
		}
	}

	return INDEX_NONE;
}

int32 UStealthSimulationSubsystem::FindObjectiveIndexById(FName ObjectiveId) const
{
	if (ObjectiveId.IsNone())
	{
		return INDEX_NONE;
	}

	for (int32 i = 0; i < ObjectiveRecords.Num(); ++i)
	{
		if (ObjectiveRecords[i].ObjectiveId == ObjectiveId)
		{
			return i;
		}
	}

	return INDEX_NONE;
}
