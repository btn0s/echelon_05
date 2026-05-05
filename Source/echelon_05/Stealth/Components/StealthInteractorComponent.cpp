#include "Stealth/Components/StealthInteractorComponent.h"

#include "Stealth/Interfaces/StealthInteractable.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"

namespace
{
bool IsActorOrAttachedTo(AActor* TestActor, const AActor* Candidate)
{
	for (AActor* Cursor = TestActor; Cursor; Cursor = Cursor->GetAttachParentActor())
	{
		if (Cursor == Candidate)
		{
			return true;
		}
	}

	return false;
}

bool IsInteractableCandidate(AActor* Candidate, APawn* Pawn)
{
	return Candidate && Candidate != Pawn && Candidate->Implements<UStealthInteractable>() &&
		IStealthInteractable::Execute_CanStealthInteract(Candidate, Pawn);
}

bool HasInteractionLineOfSight(UWorld* World, APawn* Pawn, const FVector& Start, AActor* Candidate)
{
	if (!World || !Pawn || !Candidate)
	{
		return false;
	}

	const FVector Target = Candidate->GetActorLocation() + FVector(0.f, 0.f, 45.f);
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(StealthInteractLOS), false, Pawn);
	Params.AddIgnoredActor(Pawn);
	const bool bHit = World->LineTraceSingleByChannel(Hit, Start, Target, ECC_Visibility, Params);
	return !bHit || IsActorOrAttachedTo(Hit.GetActor(), Candidate);
}
}

UStealthInteractorComponent::UStealthInteractorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UStealthInteractorComponent::BeginPlay()
{
	Super::BeginPlay();
	AddInputMappingContext();
	TryBindInput();
	RefreshFocusedInteractable();
}

void UStealthInteractorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	RefreshFocusedInteractable();

	if (!bInputBound)
	{
		TryBindInput();
	}
}

bool UStealthInteractorComponent::TryInteract()
{
	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn)
	{
		return false;
	}

	RefreshFocusedInteractable();
	AActor* TargetActor = FocusedInteractable.Get();
	if (IsInteractableCandidate(TargetActor, Pawn))
	{
		IStealthInteractable::Execute_StealthInteract(TargetActor, Pawn);
		return true;
	}

	return false;
}

FText UStealthInteractorComponent::GetFocusedInteractionText() const
{
	APawn* Pawn = Cast<APawn>(GetOwner());
	AActor* TargetActor = FocusedInteractable.Get();
	if (!Pawn || !IsInteractableCandidate(TargetActor, Pawn))
	{
		return FText::GetEmpty();
	}

	return IStealthInteractable::Execute_GetStealthInteractionText(TargetActor, Pawn);
}

void UStealthInteractorComponent::RefreshFocusedInteractable()
{
	FocusedInteractable.Reset();
	FocusedHit = FHitResult();

	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn)
	{
		return;
	}

	AActor* TargetActor = nullptr;
	FHitResult TargetHit;
	if (FindBestInteractable(Pawn, TargetActor, TargetHit))
	{
		FocusedInteractable = TargetActor;
		FocusedHit = TargetHit;
	}
}

bool UStealthInteractorComponent::FindBestInteractable(APawn* Pawn, AActor*& OutActor, FHitResult& OutHit) const
{
	OutActor = nullptr;
	OutHit = FHitResult();

	FVector ViewLoc;
	FRotator ViewRot;
	Pawn->GetActorEyesViewPoint(ViewLoc, ViewRot);

	const FVector Start = ViewLoc;
	const FVector End = Start + ViewRot.Vector() * TraceDistance;

	TArray<FHitResult> Hits;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(StealthInteract), false, Pawn);
	Params.AddIgnoredActor(Pawn);

	if (GetWorld()->SweepMultiByChannel(Hits, Start, End, FQuat::Identity, ECC_Visibility,
		FCollisionShape::MakeSphere(TraceRadius), Params))
	{
		Hits.Sort([](const FHitResult& A, const FHitResult& B)
		{
			return A.Distance < B.Distance;
		});

		for (const FHitResult& Hit : Hits)
		{
			for (AActor* TestActor : {Hit.GetActor(), Hit.GetActor() ? Hit.GetActor()->GetAttachParentActor() : nullptr})
			{
				if (IsInteractableCandidate(TestActor, Pawn))
				{
					OutActor = TestActor;
					OutHit = Hit;
					return true;
				}
			}
		}
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams ObjectQuery;
	ObjectQuery.AddObjectTypesToQuery(ECC_Pawn);
	ObjectQuery.AddObjectTypesToQuery(ECC_WorldDynamic);

	if (!GetWorld()->OverlapMultiByObjectType(Overlaps, Pawn->GetActorLocation(), FQuat::Identity, ObjectQuery,
		FCollisionShape::MakeSphere(NearbyInteractionRadius), Params))
	{
		return false;
	}

	const FVector ViewDir = ViewRot.Vector().GetSafeNormal();
	float BestScore = -TNumericLimits<float>::Max();
	AActor* BestActor = nullptr;
	FHitResult BestHit;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		for (AActor* TestActor : {Overlap.GetActor(), Overlap.GetActor() ? Overlap.GetActor()->GetAttachParentActor() : nullptr})
		{
			if (!IsInteractableCandidate(TestActor, Pawn))
			{
				continue;
			}

			const FVector ToCandidate = (TestActor->GetActorLocation() - ViewLoc).GetSafeNormal();
			const float FacingDot = FVector::DotProduct(ViewDir, ToCandidate);
			if (FacingDot < NearbyMinimumFacingDot)
			{
				continue;
			}

			if (!HasInteractionLineOfSight(GetWorld(), Pawn, ViewLoc, TestActor))
			{
				continue;
			}

			const float Distance = FVector::Dist(Pawn->GetActorLocation(), TestActor->GetActorLocation());
			const float DistanceScore = 1.f - FMath::Clamp(Distance / FMath::Max(1.f, NearbyInteractionRadius), 0.f, 1.f);
			const float Score = FacingDot * 1.5f + DistanceScore;
			if (Score > BestScore)
			{
				BestScore = Score;
				BestActor = TestActor;
				BestHit = FHitResult(TestActor, nullptr, TestActor->GetActorLocation(), FVector::ZeroVector);
			}
		}
	}

	if (BestActor)
	{
		OutActor = BestActor;
		OutHit = BestHit;
		return true;
	}

	return false;
}

void UStealthInteractorComponent::AddInputMappingContext() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const APlayerController* PlayerController = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	const ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	if (!UseMappingContext || !LocalPlayer)
	{
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
	{
		Subsystem->AddMappingContext(UseMappingContext, UseMappingPriority);
	}
}

void UStealthInteractorComponent::TryBindInput()
{
	if (bInputBound || !UseAction)
	{
		return;
	}

	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn || !Pawn->InputComponent)
	{
		return;
	}

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(Pawn->InputComponent))
	{
		EnhancedInput->BindAction(UseAction, ETriggerEvent::Triggered, this, &UStealthInteractorComponent::HandleUseAction);
		bInputBound = true;
	}
}

void UStealthInteractorComponent::HandleUseAction(const FInputActionValue& Value)
{
	(void)Value;
	TryInteract();
}
