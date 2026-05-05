#include "Stealth/Actors/StealthPatrolRouteActor.h"

AStealthPatrolRouteActor::AStealthPatrolRouteActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AStealthPatrolRouteActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (LocalPatrolPointTransforms.Num() == 0 && LocalPatrolOffsets.Num() > 0)
	{
		LocalPatrolPointTransforms.Reserve(LocalPatrolOffsets.Num());
		for (const FVector& Offset : LocalPatrolOffsets)
		{
			LocalPatrolPointTransforms.Emplace(FRotator::ZeroRotator, Offset, FVector::OneVector);
		}
	}
}

FVector AStealthPatrolRouteActor::GetWorldPatrolPoint(int32 Index) const
{
	if (LocalPatrolPointTransforms.IsValidIndex(Index))
	{
		return GetActorTransform().TransformPosition(LocalPatrolPointTransforms[Index].GetLocation());
	}

	if (!LocalPatrolOffsets.IsValidIndex(Index))
	{
		return GetActorLocation();
	}
	return GetActorTransform().TransformPosition(LocalPatrolOffsets[Index]);
}

TArray<FVector> AStealthPatrolRouteActor::GetAllWorldPatrolPoints() const
{
	TArray<FVector> Out;
	Out.Reserve(LocalPatrolOffsets.Num());
	for (int32 i = 0; i < LocalPatrolOffsets.Num(); ++i)
	{
		Out.Add(GetWorldPatrolPoint(i));
	}
	return Out;
}
