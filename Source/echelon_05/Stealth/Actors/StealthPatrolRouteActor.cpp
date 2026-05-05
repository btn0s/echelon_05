#include "Stealth/Actors/StealthPatrolRouteActor.h"

AStealthPatrolRouteActor::AStealthPatrolRouteActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

FVector AStealthPatrolRouteActor::GetWorldPatrolPoint(int32 Index) const
{
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
