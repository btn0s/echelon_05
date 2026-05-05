#include "Stealth/Components/StealthGuardBrainComponent.h"

#include "AIController.h"
#include "Stealth/Actors/StealthPatrolRouteActor.h"
#include "Stealth/Data/StealthTuningDataAsset.h"
#include "Stealth/StealthLog.h"
#include "Stealth/Subsystems/StealthSimulationSubsystem.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Navigation/PathFollowingComponent.h"

static TAutoConsoleVariable<int32> CVarStealthGuardDebugDrawBrain(
	TEXT("stealth.DebugDraw"),
	0,
	TEXT("When non-zero, draws stealth debug primitives (guard cone, hearing sphere, scene-light samples/rays)."),
	ECVF_Default);

static float GetSoundEvidencePoints(const UStealthTuningDataAsset* Tuning, EStealthSoundSource SourceType)
{
	if (!Tuning)
	{
		return 20.f;
	}

	switch (SourceType)
	{
	case EStealthSoundSource::Footstep:
		return Tuning->FootstepEvidence;
	case EStealthSoundSource::Landing:
		return Tuning->LandingEvidence;
	case EStealthSoundSource::Lure:
		return Tuning->LureEvidence;
	case EStealthSoundSource::Door:
		return Tuning->DoorEvidence;
	case EStealthSoundSource::Objective:
		return Tuning->ObjectiveEvidence;
	default:
		return Tuning->FallbackEvidence;
	}
}

UStealthGuardBrainComponent::UStealthGuardBrainComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UStealthGuardBrainComponent::BeginPlay()
{
	Super::BeginPlay();

	ResolvePatrolRoute();

	if (UWorld* World = GetWorld())
	{
		if (UStealthSimulationSubsystem* Sim = World->GetSubsystem<UStealthSimulationSubsystem>())
		{
			Sim->RegisterGuardBrain(this);
		}
	}
}

void UStealthGuardBrainComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UStealthSimulationSubsystem* Sim = World->GetSubsystem<UStealthSimulationSubsystem>())
		{
			Sim->UnregisterGuardBrain(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void UStealthGuardBrainComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* OwnerActor = GetOwner();
	const APawn* OwnerPawn = Cast<APawn>(OwnerActor);
	if (!OwnerPawn)
	{
		return;
	}

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
	APawn* PlayerPawn = World->GetFirstPlayerController() ? World->GetFirstPlayerController()->GetPawn() : nullptr;

	UpdatePerceptionAndSuspicion(DeltaTime, PlayerPawn, Tuning, Sim);
	RefreshSuspicionBucket(Tuning);
	UpdateBrainMode(Sim);

	if (CVarStealthGuardDebugDrawBrain.GetValueOnGameThread() > 0)
	{
		const FVector EyeLoc = OwnerPawn->GetPawnViewLocation();
		const FVector Forward = OwnerPawn->GetActorForwardVector();
		DrawDebugCone(World, EyeLoc, Forward, Sensor.VisionRange, FMath::DegreesToRadians(Sensor.VisionHalfAngleDegrees),
			FMath::DegreesToRadians(Sensor.VisionHalfAngleDegrees), 12, FColor::Yellow, false, -1.f, 0, 1.f);
		DrawDebugSphere(World, OwnerActor->GetActorLocation(), Sensor.HearingRange, 12, FColor::Blue, false, -1.f, 0, 1.f);
	}

	if (!bMovementAppliedByBehaviorTreeTask)
	{
		ApplyStealthMovement(Cast<AAIController>(OwnerPawn->GetController()), DeltaTime);
	}
}

void UStealthGuardBrainComponent::UpdatePerceptionAndSuspicion(float DeltaTime, APawn* PlayerPawn,
	const UStealthTuningDataAsset* Tuning, UStealthSimulationSubsystem* Sim)
{
	float VisualStrength = 0.f;
	const bool bSeePlayer = PlayerPawn && ComputeVisionToPlayer(PlayerPawn, VisualStrength);

	float AudioStrength = 0.f;
	const bool bHear = Sim && ComputeAuditoryStimulus(Sim, AudioStrength);

	Suspicion.bHadVisualStimulus = bSeePlayer;
	Suspicion.bHadAuditoryStimulus = bHear;

	const float Now = GetWorld()->GetTimeSeconds();

	if (bSeePlayer || bHear)
	{
		Suspicion.LastStimulusTime = Now;
		if (bSeePlayer && PlayerPawn)
		{
			Suspicion.LastKnownPosition = PlayerPawn->GetActorLocation();
		}
		else if (bHear)
		{
			const TArray<FStealthSoundEvent> Events = Sim->GetActiveSoundEvents();
			float BestStrength = -1.f;
			FVector BestPos = Suspicion.LastKnownPosition;
			FString BestLabel;
			for (const FStealthSoundEvent& Ev : Events)
			{
				const float D = FVector::Dist(GetOwner()->GetActorLocation(), Ev.Position);
				const float HearDist = Ev.Radius * Sensor.Acuity;
				if (D <= HearDist && D <= Sensor.HearingRange)
				{
					const float Attenuation = FMath::Clamp(1.f - D / FMath::Max(1.f, HearDist), 0.f, 1.f);
					const float EventStrength = Attenuation * FMath::Clamp(Ev.Loudness, 0.f, 1.f);
					if (EventStrength > BestStrength)
					{
						BestStrength = EventStrength;
						BestPos = Ev.Position;
						BestLabel = Ev.DebugLabel;
					}
				}
			}
			if (BestStrength >= 0.f)
			{
				Suspicion.LastKnownPosition = BestPos;
				Suspicion.LastReason = FString::Printf(TEXT("Heard: %s"), *BestLabel);
			}
			else if (PlayerPawn)
			{
				Suspicion.LastKnownPosition = PlayerPawn->GetActorLocation();
				Suspicion.LastReason = TEXT("Heard movement noise");
			}
		}

		if (bSeePlayer)
		{
			const FVisibilityEmitter Visibility = Sim->GetPlayerVisibility();
			const float Vis = Visibility.CurrentVisibility;
			const float LightThreshold = Tuning ? Tuning->BrightLightSuspicionThreshold : 0.65f;
			const float LightAlpha = FMath::Clamp((Visibility.LightExposure - LightThreshold) /
				FMath::Max(1.f - LightThreshold, KINDA_SMALL_NUMBER), 0.f, 1.f);
			const float LightMultiplier = FMath::Lerp(1.f, Tuning ? Tuning->BrightLightSuspicionMultiplier : 3.f, LightAlpha);
			const float Stim = VisualStrength * Vis * LightMultiplier *
				(Tuning ? Tuning->VisualStimulusPerSecond : 35.f) * DeltaTime;
			Suspicion.Value = FMath::Clamp(Suspicion.Value + Stim, 0.f, 100.f);
			Suspicion.LastReason = FString::Printf(TEXT("Seen (vis=%.2f light=%.2f x%.1f)"), Vis,
				Visibility.LightExposure, LightMultiplier);
		}

		if (bHear && !bSeePlayer)
		{
			const float CuriousTh = Tuning ? Tuning->SuspicionCurious : 20.f;
			const float SuspiciousTh = Tuning ? Tuning->SuspicionSuspicious : 45.f;
			const float SoundThreshold = Tuning ? Tuning->LoudSoundSuspicionThreshold : 0.7f;
			float SoundStimulus = 0.f;
			float StrongestNewSound = 0.f;

			for (const FStealthSoundEvent& Ev : Sim->GetActiveSoundEvents())
			{
				if (ProcessedSoundEventIds.Contains(Ev.EventId))
				{
					continue;
				}

				const float HearDist = Ev.Radius * Sensor.Acuity;
				const float DistanceToSound = FVector::Dist(GetOwner()->GetActorLocation(), Ev.Position);
				if (DistanceToSound > HearDist || DistanceToSound > Sensor.HearingRange)
				{
					continue;
				}

				ProcessedSoundEventIds.Add(Ev.EventId);
				const float Attenuation = FMath::Clamp(1.f - DistanceToSound / FMath::Max(1.f, HearDist), 0.f, 1.f);
				const float EventStrength = Attenuation * FMath::Clamp(Ev.Loudness, 0.f, 1.f);
				const float SoundAlpha = FMath::Clamp((EventStrength - SoundThreshold) /
					FMath::Max(1.f - SoundThreshold, KINDA_SMALL_NUMBER), 0.f, 1.f);
				const float SoundMultiplier = FMath::Lerp(1.f, Tuning ? Tuning->LoudSoundSuspicionMultiplier : 2.5f,
					SoundAlpha);
				const float SourceEvidence = GetSoundEvidencePoints(Tuning, Ev.SourceType);

				SoundStimulus += SourceEvidence * EventStrength * SoundMultiplier;
				StrongestNewSound = FMath::Max(StrongestNewSound, EventStrength);
			}

			if (SoundStimulus > 0.f)
			{
				Suspicion.Value = FMath::Clamp(Suspicion.Value + SoundStimulus, 0.f, 100.f);
				Suspicion.Value = FMath::Max(Suspicion.Value, CuriousTh);
				if (StrongestNewSound >= SoundThreshold)
				{
					Suspicion.Value = FMath::Max(Suspicion.Value, SuspiciousTh);
				}
				Suspicion.LastReason = FString::Printf(TEXT("Heard noise (strength=%.2f +%.1f)"), AudioStrength,
					SoundStimulus);
			}
		}
	}
	else
	{
		const float Decay = Tuning ? Tuning->SuspicionDecayPerSecond : 8.f;
		const float Grace = StimulusAttentionHoldSeconds;
		if (Now - Suspicion.LastStimulusTime > Grace)
		{
			Suspicion.Value = FMath::Max(0.f, Suspicion.Value - Decay * DeltaTime);
			if (Suspicion.Value <= KINDA_SMALL_NUMBER)
			{
				Suspicion.LastReason.Reset();
			}
		}
	}
}

void UStealthGuardBrainComponent::RefreshSuspicionBucket(const UStealthTuningDataAsset* Tuning)
{
	const float CuriousTh = Tuning ? Tuning->SuspicionCurious : 20.f;
	const float SuspTh = Tuning ? Tuning->SuspicionSuspicious : 45.f;
	const float InvTh = Tuning ? Tuning->SuspicionInvestigating : 70.f;
	const float AlertTh = Tuning ? Tuning->SuspicionAlert : 100.f;

	EGuardSuspicionState NewState = EGuardSuspicionState::Unaware;
	if (Suspicion.Value >= AlertTh - KINDA_SMALL_NUMBER)
	{
		NewState = EGuardSuspicionState::Alert;
	}
	else if (Suspicion.Value >= InvTh)
	{
		NewState = EGuardSuspicionState::Investigating;
	}
	else if (Suspicion.Value >= SuspTh)
	{
		NewState = EGuardSuspicionState::Suspicious;
	}
	else if (Suspicion.Value >= CuriousTh)
	{
		NewState = EGuardSuspicionState::Curious;
	}

	Suspicion.State = NewState;

	if (NewState == EGuardSuspicionState::Alert)
	{
		bCombatLocked = true;

		if (UStealthSimulationSubsystem* Sim = GetWorld()->GetSubsystem<UStealthSimulationSubsystem>())
		{
			Sim->MarkAlertOccurred();
			FAlarmState Alarm;
			Alarm.bActive = true;
			Alarm.Level = Suspicion.Value;
			Alarm.Reason = Suspicion.LastReason;
			Sim->SetAlarmState(Alarm);
		}
	}
}

void UStealthGuardBrainComponent::UpdateBrainMode(UStealthSimulationSubsystem* Sim)
{
	(void)Sim;

	if (bCombatLocked)
	{
		Brain.Mode = EGuardBrainMode::Chase;
		return;
	}

	if (GetWorld() && GetWorld()->GetTimeSeconds() - Suspicion.LastStimulusTime <= StimulusAttentionHoldSeconds &&
		Suspicion.State == EGuardSuspicionState::Unaware && !Suspicion.LastKnownPosition.IsNearlyZero())
	{
		Brain.Mode = EGuardBrainMode::Pause;
		return;
	}

	switch (Suspicion.State)
	{
	case EGuardSuspicionState::Alert:
		Brain.Mode = EGuardBrainMode::Chase;
		break;
	case EGuardSuspicionState::Investigating:
	case EGuardSuspicionState::Suspicious:
		Brain.Mode = Suspicion.bHadVisualStimulus ? EGuardBrainMode::Pause : EGuardBrainMode::Investigate;
		break;
	case EGuardSuspicionState::Curious:
		Brain.Mode = EGuardBrainMode::Pause;
		break;
	default:
		Brain.Mode = EGuardBrainMode::Patrol;
		break;
	}
}

void UStealthGuardBrainComponent::ApplyStealthMovement(AAIController* AI, float DeltaTime)
{
	if (!AI)
	{
		return;
	}

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		return;
	}

	APawn* PlayerPawn = GetWorld()->GetFirstPlayerController() ? GetWorld()->GetFirstPlayerController()->GetPawn() : nullptr;

	const float Now = GetWorld()->GetTimeSeconds();

	if (Brain.Mode == EGuardBrainMode::Chase && PlayerPawn)
	{
		const FVector OwnerLocation = OwnerPawn->GetActorLocation();
		const FVector PlayerLocation = PlayerPawn->GetActorLocation();
		const float DistanceToPlayer2D = FVector::Dist2D(OwnerLocation, PlayerLocation);
		const bool bHasLineOfSight = Suspicion.bHadVisualStimulus;

		FVector AwayFromPlayer = OwnerLocation - PlayerLocation;
		AwayFromPlayer.Z = 0.f;
		if (!AwayFromPlayer.Normalize())
		{
			AwayFromPlayer = -OwnerPawn->GetActorForwardVector();
			AwayFromPlayer.Z = 0.f;
			AwayFromPlayer.Normalize();
		}

		if (bHasLineOfSight)
		{
			AI->SetFocus(PlayerPawn, EAIFocusPriority::Gameplay);
			const FVector TacticalPosition = PlayerLocation + AwayFromPlayer * CombatPreferredDistance;

			if (DistanceToPlayer2D < CombatMinimumDistance || DistanceToPlayer2D > CombatMaximumDistance)
			{
				if (Now - LastMoveRequestTime >= MoveRequestMinInterval)
				{
					const EPathFollowingRequestResult::Type Result =
						AI->MoveToLocation(TacticalPosition, MoveToAcceptanceRadius, true, true, true, true, nullptr, true);
					if (Result == EPathFollowingRequestResult::Failed)
					{
						UE_LOG(LogStealth, Warning, TEXT("%s failed combat standoff MoveToLocation %s."),
							*OwnerPawn->GetName(), *TacticalPosition.ToCompactString());
					}
					LastMoveRequestTime = Now;
				}
				return;
			}

			AI->StopMovement();
			return;
		}

		const FVector ReacquirePosition = Suspicion.LastKnownPosition.IsNearlyZero()
			? PlayerLocation
			: Suspicion.LastKnownPosition;
		AI->SetFocalPoint(ReacquirePosition, EAIFocusPriority::Gameplay);
		if (Now - LastMoveRequestTime >= MoveRequestMinInterval)
		{
			const EPathFollowingRequestResult::Type Result =
				AI->MoveToLocation(ReacquirePosition, MoveToAcceptanceRadius, true, true, true, true, nullptr, true);
			if (Result == EPathFollowingRequestResult::Failed)
			{
				UE_LOG(LogStealth, Warning, TEXT("%s failed combat reacquire MoveToLocation %s."),
					*OwnerPawn->GetName(), *ReacquirePosition.ToCompactString());
			}
			LastMoveRequestTime = Now;
		}
		return;
	}

	AI->ClearFocus(EAIFocusPriority::Gameplay);

	if (Brain.Mode == EGuardBrainMode::Investigate)
	{
		const FVector Target = Suspicion.LastKnownPosition;
		if (Now - LastMoveRequestTime >= MoveRequestMinInterval)
		{
			const EPathFollowingRequestResult::Type Result =
				AI->MoveToLocation(Target, MoveToAcceptanceRadius, true, true, true, false, nullptr, true);
			if (Result == EPathFollowingRequestResult::Failed)
			{
				UE_LOG(LogStealth, Warning, TEXT("%s failed investigate MoveToLocation %s."),
					*OwnerPawn->GetName(), *Target.ToCompactString());
			}
			LastMoveRequestTime = Now;
		}
		return;
	}

	if (Brain.Mode == EGuardBrainMode::Pause)
	{
		AI->StopMovement();
		const FVector ToTarget = Suspicion.LastKnownPosition - OwnerPawn->GetPawnViewLocation();
		if (!ToTarget.IsNearlyZero())
		{
			const FRotator TargetRot = ToTarget.Rotation();
			OwnerPawn->SetActorRotation(
				FMath::RInterpTo(OwnerPawn->GetActorRotation(), FRotator(0.f, TargetRot.Yaw, 0.f), DeltaTime, 6.f));
		}
		return;
	}

	Brain.Mode = EGuardBrainMode::Patrol;
	ResolvePatrolRoute();
	if (PatrolRoute && PatrolRoute->GetNumPoints() > 0)
	{
		const int32 Idx = FMath::Clamp(Brain.PatrolIndex, 0, PatrolRoute->GetNumPoints() - 1);
		const FVector Dest = PatrolRoute->GetWorldPatrolPoint(Idx);
		if (FVector::Dist2D(OwnerPawn->GetActorLocation(), Dest) <= PatrolPointReachedRadius)
		{
			Brain.PatrolIndex++;
			if (Brain.PatrolIndex >= PatrolRoute->GetNumPoints())
			{
				Brain.PatrolIndex = PatrolRoute->bLoopPatrol ? 0 : PatrolRoute->GetNumPoints() - 1;
			}
		}
		if (Now - LastMoveRequestTime >= MoveRequestMinInterval)
		{
			const int32 NextIdx = FMath::Clamp(Brain.PatrolIndex, 0, PatrolRoute->GetNumPoints() - 1);
			const FVector Target = PatrolRoute->GetWorldPatrolPoint(NextIdx);
			const EPathFollowingRequestResult::Type Result =
				AI->MoveToLocation(Target, MoveToAcceptanceRadius, true, true, true, false, nullptr, true);
			if (Result == EPathFollowingRequestResult::Failed)
			{
				UE_LOG(LogStealth, Warning, TEXT("%s failed patrol MoveToLocation idx=%d target=%s route=%s."),
					*OwnerPawn->GetName(), NextIdx, *Target.ToCompactString(), *PatrolRoute->GetName());
			}
			LastMoveRequestTime = Now;
		}
	}
}

void UStealthGuardBrainComponent::ResolvePatrolRoute()
{
	if (PatrolRoute || !GetWorld() || !GetOwner())
	{
		return;
	}

	AStealthPatrolRouteActor* NearestRoute = nullptr;
	float NearestDistanceSq = TNumericLimits<float>::Max();
	const FVector OwnerLocation = GetOwner()->GetActorLocation();

	for (TActorIterator<AStealthPatrolRouteActor> It(GetWorld()); It; ++It)
	{
		AStealthPatrolRouteActor* Candidate = *It;
		if (!Candidate || Candidate->GetNumPoints() <= 0)
		{
			continue;
		}

		const float DistanceSq = FVector::DistSquared(OwnerLocation, Candidate->GetActorLocation());
		if (DistanceSq < NearestDistanceSq)
		{
			NearestDistanceSq = DistanceSq;
			NearestRoute = Candidate;
		}
	}

	if (NearestRoute)
	{
		PatrolRoute = NearestRoute;
		UE_LOG(LogStealth, Log, TEXT("%s auto-bound patrol route %s."), *GetOwner()->GetName(), *NearestRoute->GetName());
	}
}

bool UStealthGuardBrainComponent::ComputeVisionToPlayer(APawn* Player, float& OutNormalizedStrength) const
{
	OutNormalizedStrength = 0.f;
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!Player || !OwnerPawn)
	{
		return false;
	}

	FVector EyeLoc;
	FRotator EyeRot;
	OwnerPawn->GetActorEyesViewPoint(EyeLoc, EyeRot);
	const FVector Forward = EyeRot.Vector();
	const FVector TargetLoc = Player->GetActorLocation() + FVector(0, 0, 50.f);
	const FVector ToTarget = (TargetLoc - EyeLoc);
	const float Dist = ToTarget.Length();
	if (Dist > Sensor.VisionRange + KINDA_SMALL_NUMBER)
	{
		return false;
	}
	const FVector Dir = ToTarget / FMath::Max(Dist, KINDA_SMALL_NUMBER);
	const float Dot = FVector::DotProduct(Forward, Dir);
	const float CosHalf = FMath::Cos(FMath::DegreesToRadians(Sensor.VisionHalfAngleDegrees));
	if (Dot < CosHalf)
	{
		return false;
	}

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(StealthGuardBrainVision), false, OwnerPawn);
	const bool bHit =
		GetWorld()->LineTraceSingleByChannel(Hit, EyeLoc, TargetLoc, ECC_Visibility, Params);
	if (bHit && Hit.GetActor() != Player)
	{
		return false;
	}

	OutNormalizedStrength = FMath::Clamp(1.f - Dist / Sensor.VisionRange, 0.f, 1.f) * Dot;
	return true;
}

bool UStealthGuardBrainComponent::ComputeAuditoryStimulus(const UStealthSimulationSubsystem* Sim, float& OutStrength) const
{
	OutStrength = 0.f;
	if (!Sim || !GetOwner())
	{
		return false;
	}

	bool bAny = false;
	for (const FStealthSoundEvent& Ev : Sim->GetActiveSoundEvents())
	{
		const float Dist = FVector::Dist(GetOwner()->GetActorLocation(), Ev.Position);
		const float HearDist = Ev.Radius * Sensor.Acuity;
		if (Dist <= HearDist && Dist <= Sensor.HearingRange)
		{
			bAny = true;
			const float Attenuation = FMath::Clamp(1.f - Dist / FMath::Max(1.f, HearDist), 0.f, 1.f);
			OutStrength = FMath::Max(OutStrength, Attenuation * FMath::Clamp(Ev.Loudness, 0.f, 1.f));
		}
	}

	return bAny;
}

FString UStealthGuardBrainComponent::GetDebugBrainLine() const
{
	return FString::Printf(TEXT("Brain=%d Sus=%.0f State=%d Combat=%d See=%d Hear=%d | %s"),
		static_cast<int32>(Brain.Mode), Suspicion.Value, static_cast<int32>(Suspicion.State), bCombatLocked ? 1 : 0,
		Suspicion.bHadVisualStimulus ? 1 : 0, Suspicion.bHadAuditoryStimulus ? 1 : 0, *Suspicion.LastReason);
}
