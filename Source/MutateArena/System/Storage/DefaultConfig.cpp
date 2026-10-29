#include "DefaultConfig.h"

#include "Internationalization/Culture.h"
#include "MutateArena/MutateArena.h"

UDefaultConfig::UDefaultConfig()
{
	// 设置默认背包
	FLoadout Loadout;
	Loadout.Slot1EquipmentName = Slot1EquipmentName;
	Loadout.Slot2EquipmentName = Slot2EquipmentName;
	Loadout.Slot3EquipmentName = Slot3EquipmentName;
	Loadout.Slot4EquipmentName = Slot4EquipmentName;

	for (int32 i = 0; i < 4; ++i)
	{
		Loadouts.Add(Loadout);
	}

	Language = FName(FInternationalization::Get().GetCurrentCulture()->GetName());
}
