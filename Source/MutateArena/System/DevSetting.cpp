#include "DevSetting.h"

#include "Storage/DefaultConfig.h"

UDevSetting::UDevSetting()
{
	if (const UDefaultConfig* DefaultConfig = GetDefault<UDefaultConfig>())
	{
		Slot1EquipmentName = DefaultConfig->Slot1EquipmentName;
		Slot2EquipmentName = DefaultConfig->Slot2EquipmentName;
		Slot3EquipmentName = DefaultConfig->Slot3EquipmentName;
		Slot4EquipmentName = DefaultConfig->Slot4EquipmentName;
	}
}

// 置于菜单最上边
#if WITH_EDITOR
FText UDevSetting::GetSectionText() const
{
	return FText::FromString("! Dev Setting");
}
# endif
