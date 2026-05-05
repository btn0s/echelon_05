#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "StealthInteractorComponent.generated.h"

class UInputAction;
class UInputMappingContext;

UCLASS(ClassGroup = (Stealth), meta = (BlueprintSpawnableComponent))
class UStealthInteractorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UStealthInteractorComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Stealth")
	bool TryInteract();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth", meta = (ClampMin = 0))
	float TraceDistance = 220.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth", meta = (ClampMin = 0))
	float TraceRadius = 45.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth|Input")
	TObjectPtr<UInputAction> UseAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth|Input")
	TObjectPtr<UInputMappingContext> UseMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth|Input")
	int32 UseMappingPriority = 1;

private:
	void AddInputMappingContext() const;
	void TryBindInput();
	void HandleUseAction(const struct FInputActionValue& Value);

	bool bInputBound = false;
};
