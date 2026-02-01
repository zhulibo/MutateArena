#include "DevSetting.h"

#include "Storage/DefaultConfig.h"

UDevSetting::UDevSetting()
{
	if (const UDefaultConfig* DefaultConfig = GetDefault<UDefaultConfig>())
	{
		PrimaryEquipment = DefaultConfig->Primary;
		SecondaryEquipment = DefaultConfig->Secondary;
		MeleeEquipment = DefaultConfig->Melee;
		ThrowingEquipment = DefaultConfig->Throwing;
	}
}
