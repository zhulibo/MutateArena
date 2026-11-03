#pragma once

#include "Engine/DataTable.h"
#include "CharacterType.generated.h"

UENUM(BlueprintType)
enum class EHumanName : uint8
{
	Solider,
	Doctor,

	None
};

UENUM(BlueprintType)
enum class EMutantName : uint8
{
	Tank,
	Ghost,
	Smoker,
	Reaper,
	Boomer,

	// 未实现
	Cook,
	Caller,
	Clicker,
	Generator,

	None
};

UENUM(BlueprintType)
enum class EDNA2 : uint8
{
	EnhancedVision,
	EnhancedHearing,
	EnhancedSmell,
	SubconsciousAwareness,
	HighBoneDensity,
	AcceleratedClotting,
	ToxicityImmunity,

	// 未实现
	AcceleratedMetabolism,
	ThermalRegulation,
	PainModulation,
	CoreMaintenance,

	None
};

UENUM(BlueprintType)
enum class ESpawnMutantReason : uint8
{
	// 开局随机突变
	RoundStartMutate,
	// 突变倒计时已结束时进入游戏
	FirstJoinGame,
	// 被感染
	Infected,
	// 被突变体造成伤害
	Damaged,
	// 通过菜单选择突变体
	SelectMutant,
	// 突变体死亡重生
	Respawn,
	// 摔死的
	Fall,
};
