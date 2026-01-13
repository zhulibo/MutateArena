#include "Storage.h"
#include "LoadoutContent.h"
#include "CommonActivatableWidgetSwitcher.h"
#include "CommonHierarchicalScrollBox.h"
#include "CommonTextBlock.h"
#include "DataRegistrySubsystem.h"
#include "StorageButton.h"
#include "MutateArena/Characters/Data/CharacterType.h"
#include "MutateArena/System/Storage/ConfigType.h"
#include "MutateArena/Equipments/Data/EquipmentType.h"
#include "MutateArena/System/Storage/SaveGameLoadout.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/System/Storage/DefaultConfig.h"
#include "MutateArena/System/Storage/StorageSubsystem.h"
#include "MutateArena/UI/Common/CommonButton.h"
#include "MutateArena/Utils/LibraryCommon.h"
#include "MutateArena/Utils/LibraryNotify.h"
#include "Components/ButtonSlot.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

#define LOCTEXT_NAMESPACE "UStorage"

void UStorage::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 获取角色和装备数据
	if (UDataRegistry* DataRegistry = UDataRegistrySubsystem::Get()->GetRegistryForType(DR_HUMAN_CHARACTER_MAIN))
	{
		const UScriptStruct* OutStruct;
		DataRegistry->GetAllCachedItems(HumanCharacterMains, OutStruct);
	}
	if (UDataRegistry* DataRegistry = UDataRegistrySubsystem::Get()->GetRegistryForType(DR_EQUIPMENT_MAIN))
	{
		const UScriptStruct* OutStruct;
		DataRegistry->GetAllCachedItems(EquipmentMains, OutStruct);
	}

	// 添加顶部库存分类Tab按钮
	AddStorageTypeButton();

	EOSSubsystem = GetGameInstance()->GetSubsystem<UEOSSubsystem>();
	if (EOSSubsystem)
	{
		EOSSubsystem->OnQueryEntitlementsComplete.AddUObject(this, &ThisClass::OnQueryEntitlementsComplete);

		EOSSubsystem->OnEnumerateUserFilesComplete.AddUObject(this, &ThisClass::OnEnumerateUserFilesComplete);
		EOSSubsystem->OnReadUserFileComplete.AddUObject(this, &ThisClass::OnReadUserFileComplete);
	}
}

void UStorage::NativeOnActivated()
{
	Super::NativeOnActivated();
	
	if (EOSSubsystem)
	{
		EOSSubsystem->QueryEntitlements();
	}
}

UWidget* UStorage::NativeGetDesiredFocusTarget() const
{
	return StorageTypeButtonContainer->GetChildAt(TypeIndex);
}

// 获取远程已购商品完成
void UStorage::OnQueryEntitlementsComplete(bool bWasSuccessful)
{
	if(bWasSuccessful)
	{
		if (EOSSubsystem)
		{
			// 获取已购商品
			EOSSubsystem->GetEntitlements();

			// 筛选库存分类
			if (UCommonButton* AllTypeButton = Cast<UCommonButton>(StorageTypeButtonContainer->GetChildAt(TypeIndex)))
			{
				OnStorageTypeButtonClicked(AllTypeButton);
			}

			// 获取远程用户文件
			EOSSubsystem->EnumerateUserFiles();
		}
	}
	else
	{
		NOTIFY(this, C_RED, LOCTEXT("QueryEntitlementsFailed", "Get purchased items failed"));
	}
}

// 获取远程用户文件完成
void UStorage::OnEnumerateUserFilesComplete(bool bWasSuccessful)
{
	if (!bWasSuccessful) return;

	if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
	
	if (EOSSubsystem && StorageSubsystem)
	{
		if (EOSSubsystem->GetEnumeratedUserFiles().Contains(StorageSubsystem->SlotLoadout)) // 云包含该存档文件
		{
			EOSSubsystem->ReadUserFile(StorageSubsystem->SlotLoadout); // 读取存档文件
		}
		else // 云不包含该存档文件
		{
			InitPlayerConfig(StorageSubsystem->CacheLoadout); // 使用本地存档
		}
	}
}

// 读取存档文件完成
void UStorage::OnReadUserFileComplete(bool bWasSuccessful, const FUserFileContentsRef& FileContents, const FString& Filename)
{
	if (Filename != StorageSubsystem->SlotLoadout) return;
	
	if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
	if (StorageSubsystem == nullptr) return;

	if (bWasSuccessful)
	{
		USaveGameLoadout* SaveGameLoadout = NewObject<USaveGameLoadout>(this);
		FMemoryReader MemoryReader((FileContents.Get()), true);
		FObjectAndNameAsStringProxyArchive Ar(MemoryReader, false);
		Ar.ArIsSaveGame = false;
		Ar.ArNoDelta = true;
		SaveGameLoadout->Serialize(Ar);

		InitPlayerConfig(SaveGameLoadout);
	}
	else
	{
		InitPlayerConfig(StorageSubsystem->CacheLoadout);
	}
}

// 初始化玩家配置
void UStorage::InitPlayerConfig(USaveGameLoadout* SaveGameLoadout)
{
	if (SaveGameLoadout == nullptr) return;
	const UDefaultConfig* DefaultConfig = GetDefault<UDefaultConfig>();
	if (DefaultConfig == nullptr) return;

	// 判断是否拥有装备，没有则把存档恢复默认
	for (int32 i = 0; i < SaveGameLoadout->Loadouts.Num(); ++i)
	{
		if (!HasEquipment(SaveGameLoadout->Loadouts[i].Primary, EEquipmentType::Primary))
		{
			SaveGameLoadout->Loadouts[i].Primary = DefaultConfig->Primary;
		}
		if (!HasEquipment(SaveGameLoadout->Loadouts[i].Secondary, EEquipmentType::Secondary))
		{
			SaveGameLoadout->Loadouts[i].Secondary = DefaultConfig->Secondary;
		}
		if (!HasEquipment(SaveGameLoadout->Loadouts[i].Melee, EEquipmentType::Melee))
		{
			SaveGameLoadout->Loadouts[i].Melee = DefaultConfig->Melee;
		}
		if (!HasEquipment(SaveGameLoadout->Loadouts[i].Throwing, EEquipmentType::Throwing))
		{
			SaveGameLoadout->Loadouts[i].Throwing = DefaultConfig->Throwing;
		}
	}
	// 设置装备
	for (int32 i = 0; i < LoadoutSwitcher->GetChildrenCount(); ++i)
	{
		if (ULoadoutContent* LoadoutContent = Cast<ULoadoutContent>(LoadoutSwitcher->GetChildAt(i)))
		{
			FText ButtonText = FText();
			
			FString PrimaryShowName = FString();
			FString SecondaryShowName = FString();
			FString MeleeShowName = FString();
			FString ThrowingShowName = FString();

			for (const TPair<FDataRegistryId, const uint8*>& Pair : EquipmentMains)
			{
				FEquipmentMain ItemValue = *reinterpret_cast<const FEquipmentMain*>(Pair.Value);
				if (ItemValue.EquipmentName == SaveGameLoadout->Loadouts[i].Primary)
				{
					PrimaryShowName = ItemValue.ShowName;
				}
				else if (ItemValue.EquipmentName == SaveGameLoadout->Loadouts[i].Secondary)
				{
					SecondaryShowName = ItemValue.ShowName;
				}
				else if (ItemValue.EquipmentName == SaveGameLoadout->Loadouts[i].Melee)
				{
					MeleeShowName = ItemValue.ShowName;
				}
				else if (ItemValue.EquipmentName == SaveGameLoadout->Loadouts[i].Throwing)
				{
					ThrowingShowName = ItemValue.ShowName;
				}
			}

			FText::FindTextInLiveTable_Advanced(CULTURE_EQUIPMENT, PrimaryShowName, ButtonText);
			LoadoutContent->Primary->ButtonText->SetText(ButtonText);
			LoadoutContent->Primary->EquipmentName = SaveGameLoadout->Loadouts[i].Primary;
			
			FText::FindTextInLiveTable_Advanced(CULTURE_EQUIPMENT, SecondaryShowName, ButtonText);
			LoadoutContent->Secondary->ButtonText->SetText(ButtonText);
			LoadoutContent->Secondary->EquipmentName = SaveGameLoadout->Loadouts[i].Secondary;

			FText::FindTextInLiveTable_Advanced(CULTURE_EQUIPMENT, MeleeShowName, ButtonText);
			LoadoutContent->Melee->ButtonText->SetText(ButtonText);
			LoadoutContent->Melee->EquipmentName = SaveGameLoadout->Loadouts[i].Melee;

			FText::FindTextInLiveTable_Advanced(CULTURE_EQUIPMENT, ThrowingShowName, ButtonText);
			LoadoutContent->Throwing->ButtonText->SetText(ButtonText);
			LoadoutContent->Throwing->EquipmentName = SaveGameLoadout->Loadouts[i].Throwing;
		}
	}

	// 判断是否拥有角色，没有则把存档恢复默认
	if (DefaultConfig && !HasHumanCharacter(SaveGameLoadout->HumanCharacterName))
	{
		SaveGameLoadout->HumanCharacterName = DefaultConfig->HumanCharacterName;
	}

	FString ShowName = FString();
	for (const TPair<FDataRegistryId, const uint8*>& Pair : HumanCharacterMains)
	{
		FHumanCharacterMain ItemValue = *reinterpret_cast<const FHumanCharacterMain*>(Pair.Value);
		if (ItemValue.HumanCharacterName == SaveGameLoadout->HumanCharacterName)
		{
			ShowName = ItemValue.ShowName;
			break;
		}
	}
	FText ButtonText = FText();
	FText::FindTextInLiveTable_Advanced(CULTURE_HUMAN, ShowName, ButtonText);
	Character->SetText(ButtonText);
}

// 是否拥有装备
bool UStorage::HasEquipment(EEquipmentName EquipmentName, EEquipmentType EquipmentType)
{
	for (const TPair<FDataRegistryId, const uint8*>& Pair : EquipmentMains)
	{
		FEquipmentMain ItemValue = *reinterpret_cast<const FEquipmentMain*>(Pair.Value);
		if (EquipmentType == ItemValue.EquipmentType && EquipmentName == ItemValue.EquipmentName)
		{
			// 免费装备
			if (ItemValue.AudienceItemId.IsEmpty())
			{
				return true;
			}
			else
			{
				// 付费装备判断是否已拥有
				if (EOSSubsystem && EOSSubsystem->OwnEntitlement(ItemValue.AudienceItemId))
				{
					return true;
				}
			}
		}
	}

	return false;
}

// 是否拥有角色
bool UStorage::HasHumanCharacter(EHumanCharacterName HumanCharacterName)
{
	for (const TPair<FDataRegistryId, const uint8*>& Pair : HumanCharacterMains)
	{
		FHumanCharacterMain ItemValue = *reinterpret_cast<const FHumanCharacterMain*>(Pair.Value);

		if (HumanCharacterName == ItemValue.HumanCharacterName)
		{
			// 免费角色
			if (ItemValue.AudienceItemId.IsEmpty())
			{
				return true;
			}
			else
			{
				// 付费角色判断是否已拥有
				if (EOSSubsystem && EOSSubsystem->OwnEntitlement(ItemValue.AudienceItemId))
				{
					return true;
				}
			}
		}
	}

	return false;
}

// 添加顶部库存分类Tab按钮
void UStorage::AddStorageTypeButton()
{
	if (StorageTypeButtonContainer && StorageTypeButtonClass)
	{
		// 全部按钮
		if (UCommonButton* AllTypeButton = CreateWidget<UCommonButton>(this, StorageTypeButtonClass))
		{
			AllTypeButton->ButtonText->SetText(LOCTEXT("All", "All"));
			AllTypeButton->Name = STORAGE_TYPE_ALL;
			AllTypeButton->SetIsSelectable(true);
			AllTypeButton->OnClicked().AddUObject(this, &ThisClass::OnStorageTypeButtonClicked, AllTypeButton);
			if (UScrollBoxSlot* NewSlot = Cast<UScrollBoxSlot>(StorageTypeButtonContainer->AddChild(AllTypeButton)))
			{
				NewSlot->SetPadding(FMargin(0, 0, 20, 0));
			}
		}

		// 装备按钮
		for (int32 i = 0; i < static_cast<int32>(EEquipmentType::None); ++i)
		{
			if (UCommonButton* EquipmentTypeButton = CreateWidget<UCommonButton>(this, StorageTypeButtonClass))
			{
				FString EnumValue = ULibraryCommon::GetEnumValue(UEnum::GetValueAsString(static_cast<EEquipmentType>(i)));
				FText ButtonText = FText();
				FText::FindTextInLiveTable_Advanced(CULTURE_EQUIPMENT_TYPE, EnumValue, ButtonText);
				EquipmentTypeButton->ButtonText->SetText(ButtonText);
				
				EquipmentTypeButton->Name = EnumValue;
				EquipmentTypeButton->SetIsSelectable(true);
				EquipmentTypeButton->OnClicked().AddUObject(this, &ThisClass::OnStorageTypeButtonClicked, EquipmentTypeButton);
				if (UScrollBoxSlot* NewSlot = Cast<UScrollBoxSlot>(StorageTypeButtonContainer->AddChild(EquipmentTypeButton)))
				{
					NewSlot->SetPadding(FMargin(0, 0, 20, 0));
				}
			}
		}

		// 角色按钮
		if (UCommonButton* CharacterTypeButton = CreateWidget<UCommonButton>(this, StorageTypeButtonClass))
		{
			CharacterTypeButton->ButtonText->SetText(LOCTEXT("Character", "Character"));
			CharacterTypeButton->Name = STORAGE_TYPE_CHARACTER;
			CharacterTypeButton->SetIsSelectable(true);
			CharacterTypeButton->OnClicked().AddUObject(this, &ThisClass::OnStorageTypeButtonClicked, CharacterTypeButton);
			StorageTypeButtonContainer->AddChild(CharacterTypeButton);
		}
	}
}

// 点击库存分类按钮
void UStorage::OnStorageTypeButtonClicked(UCommonButton* CommonButton)
{
	CommonButton->SetIsSelected(true);

	// 将其他按钮置为未选中状态
	for (int32 i = 0; i < StorageTypeButtonContainer->GetChildrenCount(); ++i)
	{
		if (UCommonButton* EquipmentTypeButton = Cast<UCommonButton>(StorageTypeButtonContainer->GetChildAt(i)))
		{
			if (EquipmentTypeButton != CommonButton)
			{
				EquipmentTypeButton->ClearSelection();
			}
			else
			{
				TypeIndex = i;
			}
		}
	}

	// 清空所有库存按钮
	StorageButtonContainer->ClearChildren();

	// 点击分类按钮添加库存
	FString TypeName = CommonButton->Name;
	if (TypeName == STORAGE_TYPE_ALL)
	{
		AddEquipments(TypeName);
		AddHumanCharacters();
	}
	else if (TypeName == STORAGE_TYPE_CHARACTER)
	{
		AddHumanCharacters();
	}
	else
	{
		AddEquipments(TypeName);
	}
}

// 添加拥有的装备按钮
void UStorage::AddEquipments(FString EquipmentType)
{
	for (const TPair<FDataRegistryId, const uint8*>& Pair : EquipmentMains)
	{
		FEquipmentMain ItemValue = *reinterpret_cast<const FEquipmentMain*>(Pair.Value);
		FString EnumValue = ULibraryCommon::GetEnumValue(UEnum::GetValueAsString(ItemValue.EquipmentType));

		if (EquipmentType == STORAGE_TYPE_ALL || EnumValue == EquipmentType)
		{
			// 免费装备 || 付费装备且已拥有
			if (ItemValue.AudienceItemId.IsEmpty() || EOSSubsystem && EOSSubsystem->OwnEntitlement(ItemValue.AudienceItemId))
			{
				AddEquipmentButton(ItemValue);
			}
		}
	}
}

// 添加拥有的角色按钮
void UStorage::AddHumanCharacters()
{
	for (const TPair<FDataRegistryId, const uint8*>& Pair : HumanCharacterMains)
	{
		FHumanCharacterMain ItemValue = *reinterpret_cast<const FHumanCharacterMain*>(Pair.Value);
		// 免费角色 || 付费角色且已拥有
		if (ItemValue.AudienceItemId.IsEmpty() || EOSSubsystem && EOSSubsystem->OwnEntitlement(ItemValue.AudienceItemId))
		{
			AddCharacterButton(ItemValue);
		}
	}
}

// 添加装备
void UStorage::AddEquipmentButton(FEquipmentMain EquipmentMain)
{
	if (UStorageButton* EquipmentButton = CreateWidget<UStorageButton>(this, EquipmentButtonClass))
	{
		FText ButtonText = FText();
		FString ShowName = FString();
		for (const TPair<FDataRegistryId, const uint8*>& Pair : EquipmentMains)
		{
			FEquipmentMain ItemValue = *reinterpret_cast<const FEquipmentMain*>(Pair.Value);
			if (ItemValue.EquipmentName == EquipmentMain.EquipmentName)
			{
				ShowName = ItemValue.ShowName;
				break;
			}
		}
		FText::FindTextInLiveTable_Advanced(CULTURE_EQUIPMENT, ShowName, ButtonText);
		EquipmentButton->ButtonText->SetText(ButtonText);

		EquipmentButton->ShowName = ShowName;
		EquipmentButton->EquipmentName = EquipmentMain.EquipmentName;
		EquipmentButton->EquipmentType = EquipmentMain.EquipmentType;
		EquipmentButton->OnClicked().AddUObject(this, &ThisClass::OnEquipmentButtonClicked, EquipmentButton);
		if (UWrapBoxSlot* NewSlot = Cast<UWrapBoxSlot>(StorageButtonContainer->AddChild(EquipmentButton)))
		{
			NewSlot->SetPadding(FMargin(0, 0, 20, 20));
		}
	}
}

// 添加角色
void UStorage::AddCharacterButton(FHumanCharacterMain HumanCharacterMain)
{
	if (UStorageButton* CharacterButton = CreateWidget<UStorageButton>(this, CharacterButtonClass))
	{
		FText ButtonText = FText();
		FString ShowName = FString();
		for (const TPair<FDataRegistryId, const uint8*>& Pair : HumanCharacterMains)
		{
			FHumanCharacterMain ItemValue = *reinterpret_cast<const FHumanCharacterMain*>(Pair.Value);
			if (ItemValue.HumanCharacterName == HumanCharacterMain.HumanCharacterName)
			{
				ShowName = ItemValue.ShowName;
				break;
			}
		}
		FText::FindTextInLiveTable_Advanced(CULTURE_HUMAN, ShowName, ButtonText);
		CharacterButton->ButtonText->SetText(ButtonText);

		CharacterButton->ShowName = ShowName;
		CharacterButton->HumanCharacterName = HumanCharacterMain.HumanCharacterName;
		CharacterButton->OnClicked().AddUObject(this, &ThisClass::OnCharacterButtonClicked, CharacterButton);
		if (UWrapBoxSlot* NewSlot = Cast<UWrapBoxSlot>(StorageButtonContainer->AddChild(CharacterButton)))
		{
			NewSlot->SetPadding(FMargin(0, 0, 20, 20));
		}
	}
}

// 点击装备按钮
void UStorage::OnEquipmentButtonClicked(UStorageButton* EquipmentButton)
{
	if (EquipmentButton == nullptr) return;

	if (ULoadoutContent* LoadoutContent = Cast<ULoadoutContent>(LoadoutSwitcher->GetActiveWidget()))
	{
		FText ButtonText = FText();
		FText::FindTextInLiveTable_Advanced(CULTURE_EQUIPMENT, EquipmentButton->ShowName, ButtonText);

		switch (EquipmentButton->EquipmentType)
		{
		case EEquipmentType::Primary:
			LoadoutContent->Primary->ButtonText->SetText(ButtonText);
			LoadoutContent->Primary->EquipmentName = EquipmentButton->EquipmentName;
			break;
		case EEquipmentType::Secondary:
			LoadoutContent->Secondary->ButtonText->SetText(ButtonText);
			LoadoutContent->Secondary->EquipmentName = EquipmentButton->EquipmentName;
			break;
		case EEquipmentType::Melee:
			LoadoutContent->Melee->ButtonText->SetText(ButtonText);
			LoadoutContent->Melee->EquipmentName = EquipmentButton->EquipmentName;
			break;
		case EEquipmentType::Throwing:
			LoadoutContent->Throwing->ButtonText->SetText(ButtonText);
			LoadoutContent->Throwing->EquipmentName = EquipmentButton->EquipmentName;
			break;
		}
	}

	SaveEquipmentsToLoadouts();
}

// 保存背包到存档
void UStorage::SaveEquipmentsToLoadouts()
{
	TArray<FLoadout> Loadouts;
	for (int32 i = 0; i < LoadoutSwitcher->GetChildrenCount(); ++i)
	{
		if (ULoadoutContent* LoadoutContent = Cast<ULoadoutContent>(LoadoutSwitcher->GetChildAt(i)))
		{
			FLoadout Loadout;
			Loadout.Primary = LoadoutContent->Primary->EquipmentName;
			Loadout.Secondary = LoadoutContent->Secondary->EquipmentName;
			Loadout.Melee = LoadoutContent->Melee->EquipmentName;
			Loadout.Throwing = LoadoutContent->Throwing->EquipmentName;
			Loadouts.Add(Loadout);
		}
	}

	if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
	if (StorageSubsystem && StorageSubsystem->CacheLoadout)
	{
		StorageSubsystem->CacheLoadout->Loadouts = Loadouts;
		StorageSubsystem->SaveLoadouts();
	}
}

// 点击角色按钮
void UStorage::OnCharacterButtonClicked(UStorageButton* CharacterButton)
{
	if (CharacterButton == nullptr) return;

	FText ButtonText = FText();
	FText::FindTextInLiveTable_Advanced(CULTURE_HUMAN, CharacterButton->ShowName, ButtonText);
	Character->SetText(ButtonText);

	if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
	if (StorageSubsystem && StorageSubsystem->CacheLoadout)
	{
		StorageSubsystem->CacheLoadout->HumanCharacterName = CharacterButton->HumanCharacterName;
		StorageSubsystem->SaveLoadouts();
	}
}

#undef LOCTEXT_NAMESPACE
