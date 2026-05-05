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

bool AStealthExtractionZone::CanStealthInteract_Implementation(APawn* InteractingPawn)
{
	(void)InteractingPawn;

	if (const UWorld* World = GetWorld())
	{
		if (const UStealthSimulationSubsystem* Sim = World->GetSubsystem<UStealthSimulationSubsystem>())
		{
			const FExtractionState Ext = Sim->GetExtractionState();
			return Sim->AreRequiredObjectivesComplete() && Ext.bAvailable && !Ext.bUsed;
		}
	}

	return false;
}

FText AStealthExtractionZone::GetStealthInteractionText_Implementation(APawn* InteractingPawn)
{
	(void)InteractingPawn;
	return NSLOCTEXT("StealthExtractionZone", "ExtractText", "Extract");
}

void AStealthExtractionZone::StealthInteract_Implementation(APawn* InteractingPawn)
{
	TryExtract(InteractingPawn);
}

void AStealthExtractionZone::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bAutoExtractOnOverlap)
	{
		return;
	}

	APawn* Pawn = Cast<APawn>(Other);
	if (!Pawn || !Pawn->IsPlayerControlled())
	{
		return;
	}

	TryExtract(Pawn);
}

bool AStealthExtractionZone::TryExtract(APawn* InteractingPawn)
{
	APawn* Pawn = InteractingPawn;
	if (!Pawn || !Pawn->IsPlayerControlled())
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	UStealthSimulationSubsystem* Sim = World->GetSubsystem<UStealthSimulationSubsystem>();
	if (!Sim)
	{
		return false;
	}

	FExtractionState Ext = Sim->GetExtractionState();

	if (!Sim->AreRequiredObjectivesComplete() || !Ext.bAvailable)
	{
		UE_LOG(LogStealth, Log, TEXT("Extraction blocked: complete objective first."));
		return false;
	}

	if (Ext.bUsed)
	{
		return false;
	}

	Ext.bUsed = true;
	Sim->SetExtractionState(Ext);

	const EStealthMissionOutcome Outcome =
		Sim->HasAlertOccurred() ? EStealthMissionOutcome::SuccessCompromised : EStealthMissionOutcome::SuccessClean;
	Sim->SetMissionOutcome(Outcome);

	UE_LOG(LogStealth, Log, TEXT("Mission complete (%s)."),
		Outcome == EStealthMissionOutcome::SuccessClean ? TEXT("clean") : TEXT("compromised"));
	return true;
}
