#include "RadialMenuEquipment.h"

#include "MutateArena/PlayerStates/TeamType.h"
#include "MutateArena/System/UISubsystem.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Characters/Components/CombatComponent.h"
#include "MutateArena/Equipments/Equipment.h"
#include "MutateArena/Equipments/Weapon.h"
#include "MutateArena/Equipments/Throwing.h"
#include "MutateArena/Equipments/Melee.h"
#include "MutateArena/Equipments/Data/EquipmentAsset.h"
#include "MutateArena/Equipments/Data/EquipmentType.h"
#include "MutateArena/System/AssetSubsystem.h"
#include "MutateArena/Utils/LibraryCommon.h"

void URadialMenuEquipment::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetOwningLocalPlayer()))
	{
		UISubsystem->OnLocalTeamChange.AddUObject(this, &ThisClass::OnTeamChange);
	}
}

void URadialMenuEquipment::NativeDestruct()
{
	if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetOwningLocalPlayer()))
	{
		UISubsystem->OnLocalTeamChange.RemoveAll(this);
	}

	Super::NativeDestruct();
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

	// 2. 提前获取 AssetSubsystem
	UAssetSubsystem* AssetSubsystem = nullptr;
	if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
	{
		AssetSubsystem = GameInstance->GetSubsystem<UAssetSubsystem>();
	}

	for (int32 i = 0; i < 8; ++i)
	{
		FRadialMenuItemData ItemData;
		AEquipment* CurrentSlotEquipment = nullptr;

		// 3. 将轮盘索引映射到实际的武器槽位
		if (CombatComp)
		{
			if (i == 0) CurrentSlotEquipment = CombatComp->Slot1Equipment;
			else if (i == 1) CurrentSlotEquipment = CombatComp->Slot2Equipment;
			else if (i == 2) CurrentSlotEquipment = CombatComp->Slot3Equipment;
			else if (i == 3) CurrentSlotEquipment = CombatComp->Slot4Equipment;
		}

		if (CurrentSlotEquipment)
		{
			EEquipmentName EquipEnumName = CurrentSlotEquipment->EquipmentName;
			bool bFoundData = false;

			// 4. 从 AssetSubsystem 获取对应的装备数据
			if (AssetSubsystem)
			{
				// 使用 O(1) 直接获取，不需要转换枚举字符串
				if (UEquipmentAsset* EquipAsset = AssetSubsystem->GetEquipmentAssetByName(EquipEnumName))
				{
					bFoundData = true;
					ItemData.ItemTexture = EquipAsset->ShowImg;
					ItemData.ItemText = EquipAsset->ShowName;
				}
			}

			// 5. 资产中未查找到数据时的兜底：直接显示枚举名
			if (!bFoundData)
			{
				FString EnumString = StaticEnum<EEquipmentName>()->GetNameStringByValue(static_cast<int64>(EquipEnumName));
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
