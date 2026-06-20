#pragma once

#include "MutateArena/Equipments/Data/EquipmentType.h"

#include "ConfigType.generated.h"

enum class EEquipmentName : uint8;

USTRUCT(BlueprintType)
struct FLoadout
{
	GENERATED_BODY()

	UPROPERTY()
	EEquipmentName Primary = EEquipmentName::None;
	UPROPERTY()
	EEquipmentName Secondary = EEquipmentName::None;
	UPROPERTY()
	EEquipmentName Melee = EEquipmentName::None;
	UPROPERTY()
	EEquipmentName Throwing = EEquipmentName::None;

};
