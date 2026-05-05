#include "Stealth/AI/BTTask_StealthGuardDriveMovement.h"

#include "Stealth/Components/StealthGuardBrainComponent.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "GameFramework/Pawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BTTask_StealthGuardDriveMovement)

UBTTask_StealthGuardDriveMovement::UBTTask_StealthGuardDriveMovement()
{
	bNotifyTick = true;
	NodeName = TEXT("Stealth Guard Drive Movement");
}

EBTNodeResult::Type UBTTask_StealthGuardDriveMovement::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	return EBTNodeResult::InProgress;
}

void UBTTask_StealthGuardDriveMovement::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* AI = OwnerComp.GetAIOwner();
	if (!AI)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	APawn* Pawn = AI->GetPawn();
	if (!Pawn)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	UStealthGuardBrainComponent* Brain = Pawn->FindComponentByClass<UStealthGuardBrainComponent>();
	if (!Brain)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	Brain->ApplyStealthMovement(AI, DeltaSeconds);
}
