#pragma once

#include "MutateArena/Equipments/Data/EquipmentType.h"

#include "ConfigType.generated.h"

enum class EEquipmentName : uint8;

USTRUCT(BlueprintType)
struct FLoadout
{
	GENERATED_BODY()

	UPROPERTY()
	EEquipmentName Slot1EquipmentName = EEquipmentName::None;
	UPROPERTY()
	EEquipmentName Slot2EquipmentName = EEquipmentName::None;
	UPROPERTY()
	EEquipmentName Slot3EquipmentName = EEquipmentName::None;
	UPROPERTY()
	EEquipmentName Slot4EquipmentName = EEquipmentName::None;

};
