#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "StealthPatrolRouteActor.generated.h"

/** Holds patrol points in space relative to this actor's root transform. */
UCLASS()
class AStealthPatrolRouteActor : public AActor
{
	GENERATED_BODY()

public:
	AStealthPatrolRouteActor();

	UFUNCTION(BlueprintPure, Category = "Stealth")
	int32 GetNumPoints() const { return LocalPatrolOffsets.Num(); }

	UFUNCTION(BlueprintPure, Category = "Stealth")
	FVector GetWorldPatrolPoint(int32 Index) const;

	UFUNCTION(BlueprintPure, Category = "Stealth")
	TArray<FVector> GetAllWorldPatrolPoints() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	TArray<FVector> LocalPatrolOffsets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	bool bLoopPatrol = true;
};
