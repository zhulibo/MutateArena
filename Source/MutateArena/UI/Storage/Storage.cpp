#include "Storage.h"

#include "LoadoutContent.h"
#include "CommonActivatableWidgetSwitcher.h"
#include "CommonHierarchicalScrollBox.h"
#include "CommonLazyImage.h"
#include "CommonTextBlock.h"
#include "Human.h"
#include "DNAButton.h"
#include "DNASelectScreen.h"
#include "LoadoutItem.h"
#include "StorageButton.h"
#include "MutateArena/Characters/Data/CharacterType.h"
#include "MutateArena/System/Storage/ConfigType.h"
#include "MutateArena/Equipments/Data/EquipmentType.h"
#include "MutateArena/System/Storage/SaveGameLoadout.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/System/Storage/DefaultConfig.h"
#include "MutateArena/System/Storage/StorageSubsystem.h"
#include "MutateArena/System/AssetSubsystem.h"
#include "MutateArena/UI/Common/CommonButton.h"
#include "MutateArena/Utils/LibraryCommon.h"
#include "MutateArena/Utils/LibraryNotify.h"
#include "Components/ButtonSlot.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Internationalization/StringTable.h"
#include "MutateArena/Assets/Data/CommonAsset.h"
#include "MutateArena/Characters/Data/DNAAsset2.h"
#include "MutateArena/System/UISubsystem.h"
#include "MutateArena/System/GameplayTags.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "MutateArena/Equipments/Data/EquipmentAsset.h"
#include "MutateArena/Characters/Data/CharacterCommonAsset.h"

#define LOCTEXT_NAMESPACE "UStorage"

void UStorage::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
	EOSSubsystem = GetGameInstance()->GetSubsystem<UEOSSubsystem>();

	if (EOSSubsystem)
	{
		EOSSubsystem->OnQueryEntitlementsComplete.AddUObject(this, &ThisClass::OnQueryEntitlementsComplete);
		EOSSubsystem->OnEnumerateUserFilesComplete.AddUObject(this, &ThisClass::OnEnumerateUserFilesComplete);
		EOSSubsystem->OnReadUserFileComplete.AddUObject(this, &ThisClass::OnReadUserFileComplete);
	}

	if (DNAButton1)
	{
		DNAButton1->OnClicked().AddUObject(this, &ThisClass::OnDNAButton1Clicked);
	}
	if (DNAButton2)
	{
		DNAButton2->OnClicked().AddUObject(this, &ThisClass::OnDNAButton2Clicked);
	}

	// 添加顶部库存分类Tab按钮
	CreateStorageTypeTabs();
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
	if (bWasSuccessful)
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
		NOTIFY(this, Colors::Red, LOCTEXT("QueryEntitlementsFailed", "Get purchased items failed"));
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
			ApplyLoadoutData(StorageSubsystem->CacheLoadout); // 使用本地存档
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

		ApplyLoadoutData(SaveGameLoadout);
	}
	else
	{
		ApplyLoadoutData(StorageSubsystem->CacheLoadout);
	}
}

// 初始化背包 UI
void UStorage::ApplyLoadoutData(USaveGameLoadout* SaveGameLoadout)
{
	if (SaveGameLoadout == nullptr || AssetSubsystem == nullptr) return;
	const UDefaultConfig* DefaultConfig = GetDefault<UDefaultConfig>();
	if (DefaultConfig == nullptr) return;

	// 判断是否拥有装备且槽位类型匹配，否则把存档恢复默认
	auto IsLoadoutSlotValid = [&](EEquipmentName EquipName, EEquipmentSlot ExpectedType)
	{
		if (!HasEquipment(EquipName)) return false;

		const UEquipmentAsset* EquipAsset = AssetSubsystem->GetEquipmentAssetByName(EquipName);
		return EquipAsset && EquipAsset->GetEquipmentSlot() == ExpectedType;
	};

	for (int32 i = 0; i < SaveGameLoadout->Loadouts.Num(); ++i)
	{
		if (!IsLoadoutSlotValid(SaveGameLoadout->Loadouts[i].Slot1EquipmentName, EEquipmentSlot::Slot1))
		{
			SaveGameLoadout->Loadouts[i].Slot1EquipmentName = DefaultConfig->Slot1EquipmentName;
		}
		if (!IsLoadoutSlotValid(SaveGameLoadout->Loadouts[i].Slot2EquipmentName, EEquipmentSlot::Slot2))
		{
			SaveGameLoadout->Loadouts[i].Slot2EquipmentName = DefaultConfig->Slot2EquipmentName;
		}
		if (!IsLoadoutSlotValid(SaveGameLoadout->Loadouts[i].Slot3EquipmentName, EEquipmentSlot::Slot3))
		{
			SaveGameLoadout->Loadouts[i].Slot3EquipmentName = DefaultConfig->Slot3EquipmentName;
		}
		if (!IsLoadoutSlotValid(SaveGameLoadout->Loadouts[i].Slot4EquipmentName, EEquipmentSlot::Slot4))
		{
			SaveGameLoadout->Loadouts[i].Slot4EquipmentName = DefaultConfig->Slot4EquipmentName;
		}
	}

	// 回显装备
	for (int32 i = 0; i < LoadoutSwitcher->GetChildrenCount(); ++i)
	{
		if (ULoadoutContent* LoadoutContent = Cast<ULoadoutContent>(LoadoutSwitcher->GetChildAt(i)))
		{
			auto AssignEquipmentUI = [&](EEquipmentName EquipName, ULoadoutItem* LoadoutItemTarget)
			{
				if (UEquipmentAsset* EquipAsset = AssetSubsystem->GetEquipmentAssetByName(EquipName))
				{
					LoadoutItemTarget->ShowName->SetText(EquipAsset->ShowName);
					LoadoutItemTarget->EquipmentName = EquipAsset->EquipmentName;
					LoadoutItemTarget->ShowImg->SetBrushFromLazyTexture(EquipAsset->ShowImg);
				}
			};

			AssignEquipmentUI(SaveGameLoadout->Loadouts[i].Slot1EquipmentName, LoadoutContent->Slot1Item);
			AssignEquipmentUI(SaveGameLoadout->Loadouts[i].Slot2EquipmentName, LoadoutContent->Slot2Item);
			AssignEquipmentUI(SaveGameLoadout->Loadouts[i].Slot3EquipmentName, LoadoutContent->Slot3Item);
			AssignEquipmentUI(SaveGameLoadout->Loadouts[i].Slot4EquipmentName, LoadoutContent->Slot4Item);
		}
	}

	// 判断是否拥有角色，没有则把存档恢复默认
	if (!HasHumanCharacter(SaveGameLoadout->HumanCharacterName))
	{
		SaveGameLoadout->HumanCharacterName = DefaultConfig->HumanCharacterName;
	}

	// 回显角色
	if (UCharacterHumanAsset* CharacterAsset = AssetSubsystem->GetCharacterHumanAssetByName(SaveGameLoadout->HumanCharacterName))
	{
		Human->ShowName->SetText(CharacterAsset->ShowName);
		if (CharacterAsset->ShowImgFullBody)
		{
			Human->ShowImgFullBody->SetBrushFromLazyTexture(CharacterAsset->ShowImgFullBody);
		}
	}

	// DNA
	if (UDNAAsset2* DNAAsset1 = AssetSubsystem->GetDNAAssetByType(SaveGameLoadout->DNA1))
	{
		DNAButton1->UpdateDNAInfo(DNAAsset1);
	}
	if (UDNAAsset2* DNAAsset2 = AssetSubsystem->GetDNAAssetByType(SaveGameLoadout->DNA2))
	{
		DNAButton2->UpdateDNAInfo(DNAAsset2);
	}
}

// 是否拥有装备
bool UStorage::HasEquipment(EEquipmentName EquipmentName)
{
	if (!AssetSubsystem) return false;

	if (UEquipmentAsset* Asset = AssetSubsystem->GetEquipmentAssetByName(EquipmentName))
	{
		// 补给箱武器直接排除
		if (Asset->bIsSupply) return false;

		// 基础装备
		if (!Asset->IsSkin())
		{
			return true;
		}

		// 皮肤
		if (EOSSubsystem && EOSSubsystem->OwnEntitlement(Asset->AudienceItemId))
		{
			return true;
		}
	}

	return false;
}

// 是否拥有角色
bool UStorage::HasHumanCharacter(EHumanCharacterName HumanCharacterName)
{
	if (AssetSubsystem)
	{
		if (UCharacterHumanAsset* Asset = AssetSubsystem->GetCharacterHumanAssetByName(HumanCharacterName))
		{
			// 免费角色或者拥有相应权限的付费角色
			if (Asset->AudienceItemId.IsEmpty() || (EOSSubsystem && EOSSubsystem->OwnEntitlement(Asset->AudienceItemId)))
			{
				return true;
			}
		}
	}
	return false;
}

// 添加顶部库存分类Tab按钮
void UStorage::CreateStorageTypeTabs()
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
				FString EnumValue = StaticEnum<EEquipmentType>()->GetNameStringByIndex(i);
				FText TableText = FText::FromString(EnumValue);

				if (AssetSubsystem && AssetSubsystem->CommonAsset && AssetSubsystem->CommonAsset->ST_Common)
				{
					TableText = FText::FromStringTable(AssetSubsystem->CommonAsset->ST_Common->GetStringTableId(), EnumValue);
				}
				EquipmentTypeButton->ButtonText->SetText(TableText);

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
		if (UCommonButton* EquipmentSlotButton = Cast<UCommonButton>(StorageTypeButtonContainer->GetChildAt(i)))
		{
			if (EquipmentSlotButton != CommonButton)
			{
				EquipmentSlotButton->ClearSelection();
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
	if (!AssetSubsystem || !EOSSubsystem) return;
	
	for (const TPair<EEquipmentName, UEquipmentAsset*>& Pair : AssetSubsystem->CachedEquipmentAssets)
	{
		UEquipmentAsset* ItemValue = Pair.Value;
		if (ItemValue == nullptr) continue;

		FString EnumValue = StaticEnum<EEquipmentType>()->GetNameStringByValue(static_cast<int64>(ItemValue->GetEquipmentType()));

		if (EquipmentType == STORAGE_TYPE_ALL || EnumValue == EquipmentType)
		{
			if (HasEquipment(ItemValue->EquipmentName))
			{
				AddEquipmentButton(ItemValue);
			}
		}
	}
}

// 添加拥有的角色按钮
void UStorage::AddHumanCharacters()
{
	if (!AssetSubsystem) return;

	for (const TPair<EHumanCharacterName, UCharacterHumanAsset*>& Pair : AssetSubsystem->CachedCharacterHumanAssets)
	{
		UCharacterHumanAsset* ItemValue = Pair.Value;
		if (ItemValue == nullptr) continue;

		// 免费角色 || 付费角色且已拥有
		if (ItemValue->AudienceItemId.IsEmpty() || (EOSSubsystem && EOSSubsystem->OwnEntitlement(ItemValue->AudienceItemId)))
		{
			AddCharacterButton(ItemValue);
		}
	}
}

// 添加装备
void UStorage::AddEquipmentButton(UEquipmentAsset* EquipmentAsset)
{
	if (EquipmentAsset == nullptr) return;

	if (UStorageButton* EquipmentButton = CreateWidget<UStorageButton>(this, EquipmentButtonClass))
	{
		EquipmentButton->ShowName->SetText(EquipmentAsset->ShowName);
		EquipmentButton->Name = EquipmentAsset->ShowName;
		EquipmentButton->ShowImg->SetBrushFromLazyTexture(EquipmentAsset->ShowImg);
		EquipmentButton->ShowImgPath = EquipmentAsset->ShowImg;
		EquipmentButton->EquipmentName = EquipmentAsset->EquipmentName;
		EquipmentButton->EquipmentSlot = EquipmentAsset->GetEquipmentSlot();
		EquipmentButton->OnClicked().AddUObject(this, &ThisClass::OnEquipmentButtonClicked, EquipmentButton);

		if (UWrapBoxSlot* NewSlot = Cast<UWrapBoxSlot>(StorageButtonContainer->AddChild(EquipmentButton)))
		{
			NewSlot->SetPadding(FMargin(0, 0, 20, 20));
		}
	}
}

// 添加角色
void UStorage::AddCharacterButton(UCharacterHumanAsset* CharacterAsset)
{
	if (CharacterAsset == nullptr) return;

	if (UStorageButton* CharacterButton = CreateWidget<UStorageButton>(this, CharacterButtonClass))
	{
		CharacterButton->ShowName->SetText(CharacterAsset->ShowName);
		CharacterButton->Name = CharacterAsset->ShowName;
		CharacterButton->ShowImg->SetBrushFromLazyTexture(CharacterAsset->ShowImg);
		CharacterButton->ShowImgFullBodyPath = CharacterAsset->ShowImgFullBody;
		CharacterButton->HumanCharacterName = CharacterAsset->CharacterName;
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
		switch (EquipmentButton->EquipmentSlot)
		{
		case EEquipmentSlot::Slot1:
			LoadoutContent->Slot1Item->ShowName->SetText(EquipmentButton->Name);
			LoadoutContent->Slot1Item->ShowImg->SetBrushFromLazyTexture(EquipmentButton->ShowImgPath);
			LoadoutContent->Slot1Item->EquipmentName = EquipmentButton->EquipmentName;
			break;
		case EEquipmentSlot::Slot2:
			LoadoutContent->Slot2Item->ShowName->SetText(EquipmentButton->Name);
			LoadoutContent->Slot2Item->ShowImg->SetBrushFromLazyTexture(EquipmentButton->ShowImgPath);
			LoadoutContent->Slot2Item->EquipmentName = EquipmentButton->EquipmentName;
			break;
		case EEquipmentSlot::Slot3:
			LoadoutContent->Slot3Item->ShowName->SetText(EquipmentButton->Name);
			LoadoutContent->Slot3Item->ShowImg->SetBrushFromLazyTexture(EquipmentButton->ShowImgPath);
			LoadoutContent->Slot3Item->EquipmentName = EquipmentButton->EquipmentName;
			break;
		case EEquipmentSlot::Slot4:
			LoadoutContent->Slot4Item->ShowName->SetText(EquipmentButton->Name);
			LoadoutContent->Slot4Item->ShowImg->SetBrushFromLazyTexture(EquipmentButton->ShowImgPath);
			LoadoutContent->Slot4Item->EquipmentName = EquipmentButton->EquipmentName;
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
			Loadout.Slot1EquipmentName = LoadoutContent->Slot1Item->EquipmentName;
			Loadout.Slot2EquipmentName = LoadoutContent->Slot2Item->EquipmentName;
			Loadout.Slot3EquipmentName = LoadoutContent->Slot3Item->EquipmentName;
			Loadout.Slot4EquipmentName = LoadoutContent->Slot4Item->EquipmentName;
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

	Human->ShowName->SetText(CharacterButton->Name);

	if (CharacterButton->ShowImgFullBodyPath) Human->ShowImgFullBody->SetBrushFromLazyTexture(CharacterButton->ShowImgFullBodyPath);

	if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
	if (StorageSubsystem && StorageSubsystem->CacheLoadout)
	{
		StorageSubsystem->CacheLoadout->HumanCharacterName = CharacterButton->HumanCharacterName;
		StorageSubsystem->SaveLoadouts();
	}
}

void UStorage::OnDNAButton1Clicked()
{
	OpenDNASelectScreen(DNAButton1);
}

void UStorage::OnDNAButton2Clicked()
{
	OpenDNASelectScreen(DNAButton2);
}

void UStorage::OpenDNASelectScreen(UDNAButton* TargetButton)
{
	if (!DNASelectScreenClass) return;

	UUISubsystem* UISubsystem = GetOwningLocalPlayer()->GetSubsystem<UUISubsystem>();
	if (!UISubsystem) return;

	if (auto Layer = UISubsystem->GetLayerStack(TAG_UI_LAYER_MODAL))
	{
		Layer->AddWidget<UDNASelectScreen>(
			DNASelectScreenClass,
			[this, TargetButton](UDNASelectScreen& Dialog)
			{
				Dialog.Setup(FDNASelectComplete::CreateUObject(this, &ThisClass::OnDNASelected, TargetButton));
			}
		);
	}
}

void UStorage::OnDNASelected(UDNAAsset2* SelectedDNA, UDNAButton* TargetButton)
{
	if (SelectedDNA && TargetButton)
	{
		TargetButton->UpdateDNAInfo(SelectedDNA);

		if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
		if (StorageSubsystem && StorageSubsystem->CacheLoadout)
		{
			if (TargetButton == DNAButton1)
			{
				StorageSubsystem->CacheLoadout->DNA1 = SelectedDNA->DNA;
			}
			else if (TargetButton == DNAButton2)
			{
				StorageSubsystem->CacheLoadout->DNA2 = SelectedDNA->DNA;
			}
			StorageSubsystem->SaveLoadouts();
		}
	}
}

#undef LOCTEXT_NAMESPACE
