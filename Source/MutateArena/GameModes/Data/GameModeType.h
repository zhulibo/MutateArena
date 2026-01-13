#pragma once

UENUM(BlueprintType)
enum class ECoolGameMode : uint8
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
	Dust,
	Farm,
	Playground,
	Crossroads,

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
