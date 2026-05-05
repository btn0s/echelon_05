#include "Stealth/Actors/StealthObjective.h"

#include "Components/StaticMeshComponent.h"
#include "Stealth/StealthLog.h"
#include "Stealth/Subsystems/StealthSimulationSubsystem.h"

#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AStealthObjective::AStealthObjective()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded())
	{
		Mesh->SetStaticMesh(Cube.Object);
		Mesh->SetRelativeScale3D(FVector(0.4f, 0.4f, 0.25f));
	}
}

void AStealthObjective::StealthInteract_Implementation(APawn* InteractingPawn)
{
	if (bCompleted)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	bCompleted = true;

	if (UStealthSimulationSubsystem* Sim = World->GetSubsystem<UStealthSimulationSubsystem>())
	{
		FObjectiveState Obj = Sim->GetObjectiveState();
		Obj.bCompleted = true;
		Sim->SetObjectiveState(Obj);

		FStealthSoundEvent Ev;
		Ev.Position = GetActorLocation();
		Ev.Loudness = InteractionSoundLoudness;
		Ev.Radius = 350.f;
		Ev.SourceType = EStealthSoundSource::Objective;
		Ev.Lifetime = 0.75f;
		Ev.DebugLabel = TEXT("Objective");
		Sim->PushSoundEvent(Ev);
	}

	UE_LOG(LogStealth, Log, TEXT("Objective completed by %s"), InteractingPawn ? *InteractingPawn->GetName() : TEXT("?"));
}
