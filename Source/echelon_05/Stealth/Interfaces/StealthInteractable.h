#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "StealthInteractable.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UStealthInteractable : public UInterface
{
	GENERATED_BODY()
};

class IStealthInteractable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Stealth")
	bool CanStealthInteract(APawn* Instigator);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Stealth")
	FText GetStealthInteractionText(APawn* Instigator);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Stealth")
	void StealthInteract(APawn* Instigator);
};
