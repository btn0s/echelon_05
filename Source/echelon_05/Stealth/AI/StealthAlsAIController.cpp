#include "Stealth/AI/StealthAlsAIController.h"

#include "BehaviorTree/BehaviorTree.h"
#include "UObject/ConstructorHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(StealthAlsAIController)

AStealthAlsAIController::AStealthAlsAIController()
{
	static ConstructorHelpers::FObjectFinder<UBehaviorTree> DefaultBT(
		TEXT("/Game/Stealth/AI/BT_StealthGuard.BT_StealthGuard"));
	if (DefaultBT.Succeeded())
	{
		BehaviorTree = DefaultBT.Object;
	}

	bAttachToPawn = true;
}

void AStealthAlsAIController::OnPossess(APawn* NewPawn)
{
	AAIController::OnPossess(NewPawn);

	if (BehaviorTree && BehaviorTree->RootNode)
	{
		RunBehaviorTree(BehaviorTree);
	}
}
