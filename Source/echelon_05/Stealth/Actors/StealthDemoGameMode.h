#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"

#include "StealthDemoGameMode.generated.h"

class UStealthTuningDataAsset;

UCLASS()
class AStealthDemoGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AStealthDemoGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stealth")
	TObjectPtr<UStealthTuningDataAsset> TuningAsset;
};
