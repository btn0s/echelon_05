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

	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintPure, Category = "Stealth")
	int32 GetNumPoints() const { return LocalPatrolPointTransforms.Num() > 0 ? LocalPatrolPointTransforms.Num() : LocalPatrolOffsets.Num(); }

	UFUNCTION(BlueprintPure, Category = "Stealth")
	FVector GetWorldPatrolPoint(int32 Index) const;

	UFUNCTION(BlueprintPure, Category = "Stealth")
	TArray<FVector> GetAllWorldPatrolPoints() const;

	/** Local patrol point transforms relative to this route actor. Edit these with viewport transform widgets. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth", meta = (MakeEditWidget = true))
	TArray<FTransform> LocalPatrolPointTransforms;

	/** Deprecated offset-only data kept for existing map compatibility. OnConstruction migrates this into LocalPatrolPointTransforms. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	TArray<FVector> LocalPatrolOffsets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	bool bLoopPatrol = true;
};
