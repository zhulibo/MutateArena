#pragma once

#include "GameModeType.generated.h"

UENUM(BlueprintType)
enum class EGameMode2 : uint8
{
	Mutation,
	// Cataclysm,
	Melee,
	// TeamDeadMatch,

	None
};

UENUM(BlueprintType)
enum class EMutationMap : uint8
{
	ConstructionSite,
	Playground,

	None
};

UENUM(BlueprintType)
enum class EMeleeMap : uint8
{
	Arena,

	None
};

UENUM(BlueprintType)
enum class ETeamDeadMatchMap : uint8
{
	TrainingGround,

	None
};
