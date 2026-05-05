#include "Stealth/Actors/StealthDemoGameMode.h"

#include "Stealth/Data/StealthTuningDataAsset.h"
#include "Stealth/Subsystems/StealthSimulationSubsystem.h"
#include "Stealth/UI/StealthDebugHUD.h"

#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"

AStealthDemoGameMode::AStealthDemoGameMode()
{
	HUDClass = AStealthDebugHUD::StaticClass();
}

void AStealthDemoGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	if (UWorld* World = GetWorld())
	{
		if (UStealthSimulationSubsystem* Sim = World->GetSubsystem<UStealthSimulationSubsystem>())
		{
			Sim->ResetSimulation();
			if (TuningAsset)
			{
				Sim->SetTuningAsset(TuningAsset);
			}
		}
	}
}

AActor* AStealthDemoGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	static const FName StealthStartTag(TEXT("StealthStart"));

	if (UWorld* World = GetWorld())
	{
		APlayerStart* FirstUsableStart = nullptr;
		for (TActorIterator<APlayerStart> It(World); It; ++It)
		{
			APlayerStart* Start = *It;
			if (!Start)
			{
				continue;
			}

			if (!FirstUsableStart)
			{
				FirstUsableStart = Start;
			}

			if (Start->PlayerStartTag == StealthStartTag || Start->ActorHasTag(StealthStartTag))
			{
				return Start;
			}
		}

		if (FirstUsableStart)
		{
			return FirstUsableStart;
		}
	}

	return Super::ChoosePlayerStart_Implementation(Player);
}
