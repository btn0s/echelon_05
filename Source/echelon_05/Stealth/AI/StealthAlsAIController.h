#pragma once

#include "AlsAIController.h"

#include "StealthAlsAIController.generated.h"

/** Project ALS AI controller: runs project stealth Behavior Tree (movement driven by BT task + GuardBrain). */
UCLASS()
class AStealthAlsAIController : public AAlsAIController
{
	GENERATED_BODY()

public:
	AStealthAlsAIController();

protected:
	virtual void OnPossess(APawn* NewPawn) override;
};
