#include "Stealth/Actors/StealthLightVolume.h"

#include "Components/BoxComponent.h"
#include "Stealth/Subsystems/StealthSimulationSubsystem.h"

AStealthLightVolume::AStealthLightVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
	SetRootComponent(Bounds);
	Bounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Bounds->SetCollisionResponseToAllChannels(ECR_Ignore);
}

void AStealthLightVolume::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		if (UStealthSimulationSubsystem* Sim = World->GetSubsystem<UStealthSimulationSubsystem>())
		{
			Sim->RegisterLightVolume(this);
		}
	}
}

void AStealthLightVolume::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UStealthSimulationSubsystem* Sim = World->GetSubsystem<UStealthSimulationSubsystem>())
		{
			Sim->UnregisterLightVolume(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

bool AStealthLightVolume::EncompassesPoint(const FVector& WorldPoint) const
{
	if (!Bounds)
	{
		return false;
	}
	const FVector Local = Bounds->GetComponentTransform().InverseTransformPosition(WorldPoint);
	return Bounds->Bounds.GetBox().IsInside(Local);
}
