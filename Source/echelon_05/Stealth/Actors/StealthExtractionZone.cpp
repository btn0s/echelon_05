#include "Stealth/Actors/StealthExtractionZone.h"

#include "Components/BoxComponent.h"
#include "Stealth/StealthLog.h"
#include "Stealth/Subsystems/StealthSimulationSubsystem.h"

#include "GameFramework/Pawn.h"

AStealthExtractionZone::AStealthExtractionZone()
{
	PrimaryActorTick.bCanEverTick = false;

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	SetRootComponent(Trigger);
	Trigger->InitBoxExtent(FVector(100.f, 100.f, 120.f));
	Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	Trigger->OnComponentBeginOverlap.AddDynamic(this, &AStealthExtractionZone::OnOverlapBegin);
}

void AStealthExtractionZone::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	APawn* Pawn = Cast<APawn>(Other);
	if (!Pawn || !Pawn->IsPlayerControlled())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UStealthSimulationSubsystem* Sim = World->GetSubsystem<UStealthSimulationSubsystem>();
	if (!Sim)
	{
		return;
	}

	FObjectiveState Obj = Sim->GetObjectiveState();
	FExtractionState Ext = Sim->GetExtractionState();

	if (!Obj.bCompleted || !Ext.bAvailable)
	{
		UE_LOG(LogStealth, Log, TEXT("Extraction blocked: complete objective first."));
		return;
	}

	if (Ext.bUsed)
	{
		return;
	}

	Ext.bUsed = true;
	Sim->SetExtractionState(Ext);

	const EStealthMissionOutcome Outcome =
		Sim->HasAlertOccurred() ? EStealthMissionOutcome::SuccessCompromised : EStealthMissionOutcome::SuccessClean;
	Sim->SetMissionOutcome(Outcome);

	UE_LOG(LogStealth, Log, TEXT("Mission complete (%s)."),
		Outcome == EStealthMissionOutcome::SuccessClean ? TEXT("clean") : TEXT("compromised"));
}
