#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "StealthLightProbeGrid.generated.h"

USTRUCT(BlueprintType)
struct FStealthLightProbeZone
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	FName Name = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	FVector Center = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	FVector Extent = FVector(200.f, 200.f, 100.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth", meta = (ClampMin = 0, ClampMax = 1))
	float ExpectedMin = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth", meta = (ClampMin = 0, ClampMax = 1))
	float ExpectedMax = 1.f;

	bool Contains2D(const FVector& WorldLocation) const
	{
		return FMath::Abs(WorldLocation.X - Center.X) <= Extent.X &&
			FMath::Abs(WorldLocation.Y - Center.Y) <= Extent.Y;
	}
};

UCLASS()
class AStealthLightProbeGrid : public AActor
{
	GENERATED_BODY()

public:
	AStealthLightProbeGrid();

	/** Run probes for this actor and write a CSV under Saved/StealthLightProbes. */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Stealth|Light Probes")
	void RunProbeGrid();

	/** Run every StealthLightProbeGrid in the current world. Used by the console command. */
	static int32 RunAllProbeGrids(UWorld& World);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth|Light Probes")
	FVector GridCenter = FVector(100.f, 0.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth|Light Probes", meta = (ClampMin = 1))
	FVector2D GridExtent = FVector2D(2600.f, 1300.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth|Light Probes", meta = (ClampMin = 50))
	float Spacing = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth|Light Probes")
	bool bProjectToFloor = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth|Light Probes")
	float TraceStartZ = 700.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth|Light Probes")
	float TraceEndZ = -300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth|Light Probes")
	float FeetHeight = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth|Light Probes")
	float ChestHeight = 95.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth|Light Probes")
	float HeadHeight = 165.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth|Light Probes")
	TArray<FStealthLightProbeZone> Zones;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth|Light Probes")
	bool bDrawDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth|Light Probes", meta = (ClampMin = 0))
	float DebugLifeSeconds = 20.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stealth|Light Probes")
	int32 LastSampleCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stealth|Light Probes")
	int32 LastFailureCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stealth|Light Probes")
	FString LastReportPath;

private:
	bool ProjectPointToFloor(const FVector& XY, FVector& OutFloor) const;
	const FStealthLightProbeZone* FindZoneForPoint(const FVector& WorldLocation) const;
};
