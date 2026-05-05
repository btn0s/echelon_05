#pragma once

#include "BehaviorTree/BTTaskNode.h"

#include "BTTask_StealthGuardDriveMovement.generated.h"

/**
 * Keeps running and delegates navigation each tick to UStealthGuardBrainComponent::ApplyStealthMovement.
 * Used when the brain's perception runs from component tick but MoveTo must align with the behavior tree.
 */
UCLASS()
class UBTTask_StealthGuardDriveMovement : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_StealthGuardDriveMovement();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual uint16 GetInstanceMemorySize() const override { return 0; }
};
