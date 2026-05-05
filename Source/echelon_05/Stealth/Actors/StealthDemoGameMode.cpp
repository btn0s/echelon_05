#include "Stealth/Actors/StealthDemoGameMode.h"

#include "Stealth/Data/StealthTuningDataAsset.h"
#include "Stealth/Subsystems/StealthSimulationSubsystem.h"
#include "Stealth/UI/StealthDebugHUD.h"

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
