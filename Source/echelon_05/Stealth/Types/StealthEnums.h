#pragma once

#include "CoreMinimal.h"

#include "StealthEnums.generated.h"

UENUM(BlueprintType)
enum class EStealthStance : uint8
{
	Standing UMETA(DisplayName = "Standing"),
	Crouching UMETA(DisplayName = "Crouching")
};

UENUM(BlueprintType)
enum class EStealthLocomotion : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	Walk UMETA(DisplayName = "Walk"),
	Run UMETA(DisplayName = "Run"),
	Sprint UMETA(DisplayName = "Sprint")
};

UENUM(BlueprintType)
enum class EStealthLocomotionAction : uint8
{
	None UMETA(DisplayName = "None"),
	Mantling UMETA(DisplayName = "Mantling"),
	Rolling UMETA(DisplayName = "Rolling"),
	Ragdolling UMETA(DisplayName = "Ragdolling"),
	GettingUp UMETA(DisplayName = "GettingUp")
};

UENUM(BlueprintType)
enum class EStealthSurface : uint8
{
	Unknown UMETA(DisplayName = "Unknown"),
	Soft UMETA(DisplayName = "Soft"),
	Normal UMETA(DisplayName = "Normal"),
	Metal UMETA(DisplayName = "Metal"),
	Water UMETA(DisplayName = "Water")
};

UENUM(BlueprintType)
enum class EGuardSuspicionState : uint8
{
	Unaware UMETA(DisplayName = "Unaware"),
	Curious UMETA(DisplayName = "Curious"),
	Suspicious UMETA(DisplayName = "Suspicious"),
	Investigating UMETA(DisplayName = "Investigating"),
	Alert UMETA(DisplayName = "Alert")
};

UENUM(BlueprintType)
enum class EGuardBrainMode : uint8
{
	Patrol UMETA(DisplayName = "Patrol"),
	Pause UMETA(DisplayName = "Pause"),
	Investigate UMETA(DisplayName = "Investigate"),
	Search UMETA(DisplayName = "Search"),
	Chase UMETA(DisplayName = "Chase"),
	Return UMETA(DisplayName = "Return")
};

UENUM(BlueprintType)
enum class EStealthSoundSource : uint8
{
	Footstep UMETA(DisplayName = "Footstep"),
	Landing UMETA(DisplayName = "Landing"),
	Lure UMETA(DisplayName = "Lure"),
	Door UMETA(DisplayName = "Door"),
	Objective UMETA(DisplayName = "Objective"),
	Fallback UMETA(DisplayName = "Fallback")
};

UENUM(BlueprintType)
enum class EStealthMissionOutcome : uint8
{
	None UMETA(DisplayName = "None"),
	SuccessClean UMETA(DisplayName = "Success — Clean"),
	SuccessCompromised UMETA(DisplayName = "Success — Compromised"),
	Failed UMETA(DisplayName = "Failed")
};
