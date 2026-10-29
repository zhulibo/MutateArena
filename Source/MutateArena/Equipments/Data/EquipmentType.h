#pragma once

#include "EquipmentType.generated.h"

UENUM(BlueprintType)
enum class EEquipmentName : uint8
{
	// 主
	AK47,
	AK47_Desert,
	AK47_Gold,
	AK47_Quenching,
	
	M870,
	
	M60,
	
	PKM,
	
	GM6Lynx,

	// 副
	Glock17,
	
	DesertEagle,
	DesertEagle_Quenching,
	
	// 近战
	Kukri,
	Kukri_Damascus,
	Kukri_Quenching,
	
	MilitaryShovel,
	
	FireAxe,
	
	// 投掷
	Grenade,
	Flashbang,
	Smoke,
	FireBottle,
	Stun,
	
	// 补给箱
	AK47_Cyclone,
	MG42,

	None
};

UENUM(BlueprintType)
enum class EEquipmentType : uint8
{
	SMG,
	Rifle,
	LMG,
	Shotgun,
	Sniper,
	Pistol,
	Melee,
	Throwable,

	None
};

UENUM(BlueprintType)
enum class EEquipmentSlot : uint8
{
	Slot1,
	Slot2,
	Slot3,
	Slot4,
	// Slot5,
	// Slot6,

	None
};

UENUM(BlueprintType)
enum class EEquipmentState : uint8
{
	Equipped,
	Dropped,
	Thrown
};

