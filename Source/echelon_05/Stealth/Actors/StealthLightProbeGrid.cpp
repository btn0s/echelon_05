#include "Stealth/Actors/StealthLightProbeGrid.h"

#include "Stealth/StealthLog.h"
#include "Stealth/Subsystems/StealthSimulationSubsystem.h"
#include "Stealth/Types/StealthTypes.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	FAutoConsoleCommandWithWorldAndArgs GRunStealthLightProbeCommand(
		TEXT("stealth.LightProbe.Run"),
		TEXT("Run all AStealthLightProbeGrid actors in the current world and write CSV reports."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (!World)
			{
				UE_LOG(LogStealth, Warning, TEXT("stealth.LightProbe.Run: no world."));
				return;
			}

			const int32 GridCount = AStealthLightProbeGrid::RunAllProbeGrids(*World);
			UE_LOG(LogStealth, Log, TEXT("stealth.LightProbe.Run: processed %d probe grid(s)."), GridCount);
		}));

	FColor ColorForExposure(float Exposure)
	{
		if (Exposure >= 0.65f)
		{
			return FColor::Red;
		}
		if (Exposure >= 0.25f)
		{
			return FColor::Yellow;
		}
		return FColor::Green;
	}

	FString FindTopContributorForMaxBodySample(const FStealthLightSamplingDebug& Debug)
	{
		const FStealthBodyLightSampleDebug* BrightestSample = nullptr;
		for (const FStealthBodyLightSampleDebug& Sample : Debug.BodySamples)
		{
			if (!BrightestSample || Sample.Exposure > BrightestSample->Exposure)
			{
				BrightestSample = &Sample;
			}
		}

		if (!BrightestSample || BrightestSample->Contributions.Num() == 0)
		{
			return FString();
		}

		return BrightestSample->Contributions[0].LightLabel;
	}
}

AStealthLightProbeGrid::AStealthLightProbeGrid()
{
	PrimaryActorTick.bCanEverTick = false;

	Zones = {
		{TEXT("Start_ShadowVestibule"), FVector(-2200.f, -450.f, 90.f), FVector(350.f, 350.f, 150.f), 0.f, 0.10f},
		{TEXT("Left_Arcade"), FVector(-250.f, -980.f, 90.f), FVector(1700.f, 260.f, 150.f), 0.f, 0.15f},
		{TEXT("Torch_Right_01"), FVector(-900.f, 1030.f, 90.f), FVector(350.f, 300.f, 150.f), 0.25f, 0.45f},
		{TEXT("Torch_Right_02"), FVector(450.f, 1030.f, 90.f), FVector(350.f, 300.f, 150.f), 0.25f, 0.45f},
		{TEXT("FixturePool_A"), FVector(-650.f, 0.f, 90.f), FVector(320.f, 520.f, 150.f), 0.65f, 0.90f},
		{TEXT("FixturePool_B"), FVector(850.f, 0.f, 90.f), FVector(320.f, 520.f, 150.f), 0.65f, 0.90f},
		{TEXT("Objective_Dais"), FVector(2250.f, 0.f, 100.f), FVector(420.f, 420.f, 150.f), 0.35f, 0.65f}
	};
}

int32 AStealthLightProbeGrid::RunAllProbeGrids(UWorld& World)
{
	int32 Count = 0;
	for (TActorIterator<AStealthLightProbeGrid> It(&World); It; ++It)
	{
		It->RunProbeGrid();
		++Count;
	}
	return Count;
}

void AStealthLightProbeGrid::RunProbeGrid()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UStealthSimulationSubsystem* Sim = World->GetSubsystem<UStealthSimulationSubsystem>();
	if (!Sim)
	{
		UE_LOG(LogStealth, Warning, TEXT("%s: no StealthSimulationSubsystem."), *GetName());
		return;
	}

	LastSampleCount = 0;
	LastFailureCount = 0;
	LastReportPath.Reset();

	TArray<FString> Lines;
	Lines.Add(TEXT("Zone,X,Y,Z,Feet,Chest,Head,Final,ExpectedMin,ExpectedMax,Pass,TopContributor"));

	const int32 StepsX = FMath::Max(1, FMath::FloorToInt((GridExtent.X * 2.f) / Spacing));
	const int32 StepsY = FMath::Max(1, FMath::FloorToInt((GridExtent.Y * 2.f) / Spacing));
	const TArray<FString> Labels = {TEXT("Feet"), TEXT("Chest"), TEXT("Head")};

	for (int32 Ix = 0; Ix <= StepsX; ++Ix)
	{
		const float X = GridCenter.X - GridExtent.X + Ix * Spacing;
		for (int32 Iy = 0; Iy <= StepsY; ++Iy)
		{
			const float Y = GridCenter.Y - GridExtent.Y + Iy * Spacing;

			FVector FloorPoint(X, Y, GridCenter.Z);
			if (bProjectToFloor && !ProjectPointToFloor(FloorPoint, FloorPoint))
			{
				continue;
			}

			const FStealthLightProbeZone* Zone = FindZoneForPoint(FloorPoint);
			if (!Zone)
			{
				continue;
			}

			TArray<FVector> BodyPoints;
			BodyPoints.Add(FloorPoint + FVector(0.f, 0.f, FeetHeight));
			BodyPoints.Add(FloorPoint + FVector(0.f, 0.f, ChestHeight));
			BodyPoints.Add(FloorPoint + FVector(0.f, 0.f, HeadHeight));

			FStealthLightSamplingDebug Debug;
			const float Final = Sim->SampleBodyLightExposureRaw(BodyPoints, Labels, this, &Debug);
			const float Feet = Debug.BodySamples.IsValidIndex(0) ? Debug.BodySamples[0].Exposure : 0.f;
			const float Chest = Debug.BodySamples.IsValidIndex(1) ? Debug.BodySamples[1].Exposure : 0.f;
			const float Head = Debug.BodySamples.IsValidIndex(2) ? Debug.BodySamples[2].Exposure : 0.f;
			const bool bPass = Final >= Zone->ExpectedMin && Final <= Zone->ExpectedMax;

			const FString TopContributor = FindTopContributorForMaxBodySample(Debug);

			Lines.Add(FString::Printf(TEXT("%s,%.1f,%.1f,%.1f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%s,%s"),
				*Zone->Name.ToString(), FloorPoint.X, FloorPoint.Y, FloorPoint.Z, Feet, Chest, Head, Final,
				Zone->ExpectedMin, Zone->ExpectedMax, bPass ? TEXT("PASS") : TEXT("FAIL"), *TopContributor));

			++LastSampleCount;
			if (!bPass)
			{
				++LastFailureCount;
			}

			if (bDrawDebug)
			{
				for (const FStealthBodyLightSampleDebug& BodySample : Debug.BodySamples)
				{
					DrawDebugSphere(World, BodySample.WorldPosition, 14.f, 8,
						ColorForExposure(BodySample.Exposure), false, DebugLifeSeconds);
				}

				if (!bPass)
				{
					DrawDebugLine(World, BodyPoints[0], BodyPoints.Last(), FColor::Magenta, false, DebugLifeSeconds,
						0, 2.f);
				}
			}
		}
	}

	const FString Directory = FPaths::ProjectSavedDir() / TEXT("StealthLightProbes");
	IFileManager::Get().MakeDirectory(*Directory, true);
	LastReportPath = Directory / FString::Printf(TEXT("%s_%s.csv"), *GetActorLabel(),
		*FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));

	const bool bWrote = FFileHelper::SaveStringArrayToFile(Lines, *LastReportPath);
	UE_LOG(LogStealth, Log, TEXT("%s: sampled %d points, failures=%d, report=%s%s"),
		*GetActorLabel(), LastSampleCount, LastFailureCount, *LastReportPath, bWrote ? TEXT("") : TEXT(" (write failed)"));
}

bool AStealthLightProbeGrid::ProjectPointToFloor(const FVector& XY, FVector& OutFloor) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const FVector Start(XY.X, XY.Y, TraceStartZ);
	const FVector End(XY.X, XY.Y, TraceEndZ);
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(StealthLightProbeProjectFloor), false, this);
	if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		OutFloor = Hit.ImpactPoint;
		return true;
	}

	return false;
}

const FStealthLightProbeZone* AStealthLightProbeGrid::FindZoneForPoint(const FVector& WorldLocation) const
{
	for (const FStealthLightProbeZone& Zone : Zones)
	{
		if (Zone.Contains2D(WorldLocation))
		{
			return &Zone;
		}
	}
	return nullptr;
}
