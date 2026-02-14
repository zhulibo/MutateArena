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

// 置于菜单最上边
#if WITH_EDITOR
FText UDevSetting::GetSectionText() const
{
	return FText::FromString("! Dev Setting");
}
# endif
