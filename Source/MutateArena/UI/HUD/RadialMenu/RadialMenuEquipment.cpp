#include "RadialMenuEquipment.h"

#include "DataRegistryId.h"
#include "DataRegistrySubsystem.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/PlayerStates/TeamType.h"
#include "MutateArena/System/UISubsystem.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Characters/Components/CombatComponent.h"
#include "MutateArena/Equipments/Equipment.h"
#include "MutateArena/Equipments/Weapon.h"
#include "MutateArena/Equipments/Throwing.h"
#include "MutateArena/Equipments/Melee.h"
#include "MutateArena/Equipments/Data/EquipmentType.h"
#include "MutateArena/Utils/LibraryCommon.h"

void URadialMenuEquipment::NativeOnInitialized()
{
	Super::NativeOnInitialized();
    
	if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetOwningLocalPlayer()))
	{
		UISubsystem->OnTeamChange.AddUObject(this, &ThisClass::OnTeamChange);
	}
}

void URadialMenuEquipment::RefreshRadialMenu()
{
	if (AHumanCharacter* HumanChar = Cast<AHumanCharacter>(GetOwningPlayerPawn()))
	{
		SetHumanRadialMenuText();
	}
	else
	{
		SetMutantRadialMenuText();
	}
}

void URadialMenuEquipment::OnTeamChange(ETeam Team)
{
	RefreshRadialMenu();
}

void URadialMenuEquipment::SetHumanRadialMenuText()
{
	TArray<FRadialMenuItemData> MenuData;

	// 1. 获取当前玩家角色和战斗组件
	AHumanCharacter* HumanChar = Cast<AHumanCharacter>(GetOwningPlayerPawn());
	UCombatComponent* CombatComp = HumanChar ? HumanChar->CombatComp : nullptr;

	// 2. 提前获取 DataRegistry 子系统
	UDataRegistrySubsystem* DataRegistrySubsystem = UDataRegistrySubsystem::Get();
	
	for (int32 i = 0; i < 8; ++i)
	{
		FRadialMenuItemData ItemData;
		AEquipment* CurrentSlotEquipment = nullptr;

		// 3. 将轮盘索引映射到实际的武器槽位
		if (CombatComp)
		{
			if (i == 0) CurrentSlotEquipment = CombatComp->PrimaryEquipment;
			else if (i == 1) CurrentSlotEquipment = CombatComp->SecondaryEquipment;
			else if (i == 2) CurrentSlotEquipment = CombatComp->MeleeEquipment;
			else if (i == 3) CurrentSlotEquipment = CombatComp->ThrowingEquipment;
		}

		if (CurrentSlotEquipment)
		{
			// 4. 获取武器枚举名并转换为字符串
			EEquipmentName EquipEnumName = CurrentSlotEquipment->EquipmentName;
			FString EnumString = StaticEnum<EEquipmentName>()->GetNameStringByValue(static_cast<int64>(EquipEnumName));
			bool bFoundData = false;

			// 5. 从 Data Registry 获取对应的装备数据
			if (DataRegistrySubsystem)
			{
				// 假设你的数据表行名与枚举字符串一致，例如 "AK47"
				FDataRegistryId RegistryId(DR_EQUIPMENT_MAIN, FName(*EnumString));
				
				// 精确查找，不需要像 Shop 里那样遍历所有 Cache
				if (const FEquipmentMain* EquipData = DataRegistrySubsystem->GetCachedItem<FEquipmentMain>(RegistryId))
				{
					bFoundData = true;

					ItemData.ItemTexture = EquipData->ShowImg;

					// 设置文本 (完全参考你 Shop 中的翻译逻辑)
					FText TranslatedShowName = FText();
					// 注意：装备名字的翻译域是 CULTURE_EQUIPMENT，而不是类型域 CULTURE_EQUIPMENT_TYPE
					FText::FindTextInLiveTable_Advanced(CULTURE_EQUIPMENT, EquipData->ShowName, TranslatedShowName);

					// 如果 StringTable 中未配置翻译，则直接显示原表中填写的 ShowName
					if (TranslatedShowName.IsEmpty())
					{
						ItemData.ItemText = FText::FromString(EquipData->ShowName);
					}
					else
					{
						ItemData.ItemText = TranslatedShowName;
					}
				}
			}

			// 数据注册表中未查找到数据时的兜底：直接显示枚举名
			if (!bFoundData)
			{
				ItemData.ItemText = FText::FromString(EnumString);
			}
		}
		else
		{
			ItemData.ItemText = FText::FromString(TEXT("-1"));
		}

		MenuData.Add(ItemData);
	}
    
	BuildMenu(MenuData);
}

void URadialMenuEquipment::SetMutantRadialMenuText()
{
	TArray<FRadialMenuItemData> MenuData;
    
	for (int32 i = 0; i < 8; ++i)
	{
		FRadialMenuItemData ItemData;
		ItemData.ItemText = FText::FromString(TEXT("-1"));
		MenuData.Add(ItemData);
	}
    
	BuildMenu(MenuData);
}
