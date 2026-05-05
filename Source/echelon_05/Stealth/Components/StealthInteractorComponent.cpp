#include "Stealth/Components/StealthInteractorComponent.h"

#include "Stealth/Interfaces/StealthInteractable.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

UStealthInteractorComponent::UStealthInteractorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UStealthInteractorComponent::TryInteract()
{
	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn)
	{
		return false;
	}

	FVector ViewLoc;
	FRotator ViewRot;
	Pawn->GetActorEyesViewPoint(ViewLoc, ViewRot);

	const FVector Start = ViewLoc;
	const FVector End = Start + ViewRot.Vector() * TraceDistance;

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(StealthInteract), false, Pawn);
	Params.AddIgnoredActor(Pawn);

	if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		return false;
	}

	for (AActor* TestActor : { Hit.GetActor(), Hit.GetActor() ? Hit.GetActor()->GetAttachParentActor() : nullptr })
	{
		if (!TestActor)
		{
			continue;
		}
		if (TestActor->Implements<UStealthInteractable>())
		{
			IStealthInteractable::Execute_StealthInteract(TestActor, Pawn);
			return true;
		}
	}

	return false;
}
