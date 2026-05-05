#include "Stealth/Actors/StealthGuard.h"

#include "AIController.h"
#include "Stealth/Actors/StealthPatrolRouteActor.h"
#include "Stealth/Data/StealthTuningDataAsset.h"
#include "Stealth/StealthLog.h"
#include "Stealth/Subsystems/StealthSimulationSubsystem.h"

#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"

#include "DrawDebugHelpers.h"

static TAutoConsoleVariable<int32> CVarStealthGuardDebugDraw(
	TEXT("stealth.DebugDraw"),
	0,
	TEXT("When non-zero, draws stealth debug primitives (guard cone, etc.)."),
	ECVF_Default);

AStealthGuard::AStealthGuard()
{
	PrimaryActorTick.bCanEverTick = true;

	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	GetCapsuleComponent()->SetCapsuleHalfHeight(88.f);
	GetCapsuleComponent()->SetCapsuleRadius(34.f);

	if (USkeletalMeshComponent* SkelMesh = GetMesh())
	{
		// DM_Low hides this mesh when the viewport/scalability detail level is higher than Low (typical editor default).
		SkelMesh->DetailMode = DM_Epic;
		SkelMesh->SetUpdateAnimationInEditor(true);
	}

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->MaxWalkSpeed = 320.f;
	}
}

void AStealthGuard::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		if (UStealthSimulationSubsystem* Sim = World->GetSubsystem<UStealthSimulationSubsystem>())
		{
			Sim->RegisterGuard(this);
		}
	}
}

void AStealthGuard::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UStealthSimulationSubsystem* Sim = World->GetSubsystem<UStealthSimulationSubsystem>())
		{
			Sim->UnregisterGuard(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void AStealthGuard::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

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
	UpdateBrain(DeltaTime, PlayerPawn, Tuning, Sim);

	if (CVarStealthGuardDebugDraw.GetValueOnGameThread() > 0)
	{
		const FVector EyeLoc = GetPawnViewLocation();
		const FVector Forward = GetActorForwardVector();
		DrawDebugCone(World, EyeLoc, Forward, Sensor.VisionRange, FMath::DegreesToRadians(Sensor.VisionHalfAngleDegrees),
			FMath::DegreesToRadians(Sensor.VisionHalfAngleDegrees), 12, FColor::Yellow, false, -1.f, 0, 1.f);
		DrawDebugSphere(World, GetActorLocation(), Sensor.HearingRange, 12, FColor::Blue, false, -1.f, 0, 1.f);
	}
}

void AStealthGuard::UpdatePerceptionAndSuspicion(float DeltaTime, APawn* PlayerPawn, const UStealthTuningDataAsset* Tuning,
	UStealthSimulationSubsystem* Sim)
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
			float BestDist = TNumericLimits<float>::Max();
			FVector BestPos = Suspicion.LastKnownPosition;
			for (const FStealthSoundEvent& Ev : Events)
			{
				const float D = FVector::Dist(GetActorLocation(), Ev.Position);
				const float HearDist = Ev.Loudness * Ev.Radius * Sensor.Acuity;
				if (D <= HearDist && D < BestDist)
				{
					BestDist = D;
					BestPos = Ev.Position;
					Suspicion.LastReason = FString::Printf(TEXT("Heard: %s"), *Ev.DebugLabel);
				}
			}
			if (BestDist < TNumericLimits<float>::Max())
			{
				Suspicion.LastKnownPosition = BestPos;
			}
			else if (PlayerPawn)
			{
				Suspicion.LastKnownPosition = PlayerPawn->GetActorLocation();
				Suspicion.LastReason = TEXT("Heard movement noise");
			}
		}

		if (bSeePlayer)
		{
			const float Vis = Sim->GetPlayerVisibility().CurrentVisibility;
			const float Stim = VisualStrength * Vis * (Tuning ? Tuning->VisualStimulusPerSecond : 35.f) * DeltaTime;
			Suspicion.Value = FMath::Clamp(Suspicion.Value + Stim, 0.f, 100.f);
			Suspicion.LastReason = FString::Printf(TEXT("Seen (vis=%.2f)"), Vis);
		}

		if (bHear && !bSeePlayer)
		{
			const float Stim = AudioStrength * (Tuning ? Tuning->AudioStimulusPerSecond : 25.f) * DeltaTime;
			Suspicion.Value = FMath::Clamp(Suspicion.Value + Stim, 0.f, 100.f);
			if (Suspicion.LastReason.IsEmpty())
			{
				Suspicion.LastReason = TEXT("Heard noise");
			}
		}
	}
	else
	{
		const float Decay = Tuning ? Tuning->SuspicionDecayPerSecond : 8.f;
		const float Grace = 0.35f;
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

void AStealthGuard::RefreshSuspicionBucket(const UStealthTuningDataAsset* Tuning)
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

void AStealthGuard::UpdateBrain(float DeltaTime, APawn* PlayerPawn, const UStealthTuningDataAsset* Tuning,
	UStealthSimulationSubsystem* Sim)
{
	AAIController* AI = Cast<AAIController>(GetController());
	if (!AI)
	{
		return;
	}

	(void)Tuning;
	(void)Sim;

	const float Now = GetWorld()->GetTimeSeconds();

	switch (Suspicion.State)
	{
	case EGuardSuspicionState::Alert:
		Brain.Mode = EGuardBrainMode::Chase;
		break;
	case EGuardSuspicionState::Investigating:
	case EGuardSuspicionState::Suspicious:
		Brain.Mode = EGuardBrainMode::Investigate;
		break;
	case EGuardSuspicionState::Curious:
		Brain.Mode = EGuardBrainMode::Pause;
		break;
	default:
		Brain.Mode = EGuardBrainMode::Patrol;
		break;
	}

	if (Brain.Mode == EGuardBrainMode::Chase && PlayerPawn)
	{
		if (Now - LastMoveRequestTime >= MoveRequestMinInterval)
		{
			AI->MoveToActor(PlayerPawn, 90.f, true, true, false, 0, true);
			LastMoveRequestTime = Now;
		}
		return;
	}

	if (Brain.Mode == EGuardBrainMode::Investigate)
	{
		const FVector Target = Suspicion.LastKnownPosition;
		if (Now - LastMoveRequestTime >= MoveRequestMinInterval)
		{
			AI->MoveToLocation(Target, MoveToAcceptanceRadius, true, true, false, false, nullptr, true);
			LastMoveRequestTime = Now;
		}
		return;
	}

	if (Brain.Mode == EGuardBrainMode::Pause)
	{
		AI->StopMovement();
		const FVector ToTarget = Suspicion.LastKnownPosition - GetPawnViewLocation();
		if (!ToTarget.IsNearlyZero())
		{
			const FRotator TargetRot = ToTarget.Rotation();
			SetActorRotation(FMath::RInterpTo(GetActorRotation(), FRotator(0.f, TargetRot.Yaw, 0.f), DeltaTime, 6.f));
		}
		return;
	}

	Brain.Mode = EGuardBrainMode::Patrol;
	if (PatrolRoute && PatrolRoute->GetNumPoints() > 0)
	{
		const int32 Idx = FMath::Clamp(Brain.PatrolIndex, 0, PatrolRoute->GetNumPoints() - 1);
		const FVector Dest = PatrolRoute->GetWorldPatrolPoint(Idx);
		if (FVector::Dist2D(GetActorLocation(), Dest) < MoveToAcceptanceRadius)
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
			AI->MoveToLocation(PatrolRoute->GetWorldPatrolPoint(NextIdx), MoveToAcceptanceRadius, true, true, false, false, nullptr, true);
			LastMoveRequestTime = Now;
		}
	}
}

bool AStealthGuard::ComputeVisionToPlayer(APawn* Player, float& OutNormalizedStrength) const
{
	OutNormalizedStrength = 0.f;
	if (!Player)
	{
		return false;
	}

	FVector EyeLoc;
	FRotator EyeRot;
	GetActorEyesViewPoint(EyeLoc, EyeRot);
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
	FCollisionQueryParams Params(SCENE_QUERY_STAT(StealthGuardVision), false, this);
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, EyeLoc, TargetLoc, ECC_Visibility, Params);
	if (bHit && Hit.GetActor() != Player)
	{
		return false;
	}

	OutNormalizedStrength = FMath::Clamp(1.f - Dist / Sensor.VisionRange, 0.f, 1.f) * Dot;
	return true;
}

bool AStealthGuard::ComputeAuditoryStimulus(const UStealthSimulationSubsystem* Sim, float& OutStrength) const
{
	OutStrength = 0.f;
	if (!Sim)
	{
		return false;
	}

	bool bAny = false;
	for (const FStealthSoundEvent& Ev : Sim->GetActiveSoundEvents())
	{
		const float Dist = FVector::Dist(GetActorLocation(), Ev.Position);
		const float HearDist = Ev.Loudness * Ev.Radius * Sensor.Acuity;
		if (Dist <= HearDist && Dist <= Sensor.HearingRange)
		{
			bAny = true;
			OutStrength = FMath::Max(OutStrength, FMath::Clamp(1.f - Dist / FMath::Max(1.f, HearDist), 0.f, 1.f));
		}
	}

	const float PlayerNoise = Sim->GetPlayerSoundEmission().Radius;
	if (PlayerNoise > 0.f)
	{
		const APawn* Player =
			GetWorld()->GetFirstPlayerController() ? GetWorld()->GetFirstPlayerController()->GetPawn() : nullptr;
		if (Player)
		{
			const float Dist = FVector::Dist(GetActorLocation(), Player->GetActorLocation());
			if (Dist <= PlayerNoise && Dist <= Sensor.HearingRange)
			{
				bAny = true;
				const float N = FMath::Clamp(1.f - Dist / PlayerNoise, 0.f, 1.f);
				OutStrength = FMath::Max(OutStrength, N * 0.65f);
			}
		}
	}

	return bAny;
}

FString AStealthGuard::GetDebugBrainLine() const
{
	return FString::Printf(TEXT("Brain=%d Sus=%.0f State=%d See=%d Hear=%d | %s"), static_cast<int32>(Brain.Mode),
		Suspicion.Value, static_cast<int32>(Suspicion.State), Suspicion.bHadVisualStimulus ? 1 : 0,
		Suspicion.bHadAuditoryStimulus ? 1 : 0, *Suspicion.LastReason);
}
