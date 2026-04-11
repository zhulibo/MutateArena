#include "Storage.h"
#include "LoadoutContent.h"
#include "CommonActivatableWidgetSwitcher.h"
#include "CommonHierarchicalScrollBox.h"
#include "CommonLazyImage.h"
#include "CommonTextBlock.h"
#include "DataRegistrySubsystem.h"
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
#include "MutateArena/UI/Common/CommonButton.h"
#include "MutateArena/Utils/LibraryCommon.h"
#include "MutateArena/Utils/LibraryNotify.h"
#include "Components/ButtonSlot.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Internationalization/StringTable.h"
#include "MutateArena/Assets/Data/CommonAsset.h"
#include "MutateArena/Characters/Data/DNAAsset2.h"
#include "MutateArena/System/AssetSubsystem.h"
#include "MutateArena/System/DataAssetManager.h"
#include "MutateArena/System/UISubsystem.h"
#include "MutateArena/System/Tags/ProjectTags.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

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
	
	if (DNAButton1)
	{
		DNAButton1->OnClicked().AddUObject(this, &ThisClass::OnDNAButton1Clicked);
	}
	if (DNAButton2)
	{
		DNAButton2->OnClicked().AddUObject(this, &ThisClass::OnDNAButton2Clicked);
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
	// 回显装备
	for (int32 i = 0; i < LoadoutSwitcher->GetChildrenCount(); ++i)
	{
		if (ULoadoutContent* LoadoutContent = Cast<ULoadoutContent>(LoadoutSwitcher->GetChildAt(i)))
		{
			FText PrimaryShowName = FText();
			FText SecondaryShowName = FText();
			FText MeleeShowName = FText();
			FText ThrowingShowName = FText();

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

			LoadoutContent->Primary->ShowName->SetText(PrimaryShowName);
			LoadoutContent->Primary->EquipmentName = SaveGameLoadout->Loadouts[i].Primary;
			
			LoadoutContent->Secondary->ShowName->SetText(SecondaryShowName);
			LoadoutContent->Secondary->EquipmentName = SaveGameLoadout->Loadouts[i].Secondary;

			LoadoutContent->Melee->ShowName->SetText(MeleeShowName);
			LoadoutContent->Melee->EquipmentName = SaveGameLoadout->Loadouts[i].Melee;

			LoadoutContent->Throwing->ShowName->SetText(ThrowingShowName);
			LoadoutContent->Throwing->EquipmentName = SaveGameLoadout->Loadouts[i].Throwing;
			
			// 显示图片
			for (const TPair<FDataRegistryId, const uint8*>& Pair : EquipmentMains)
			{
				FEquipmentMain ItemValue = *reinterpret_cast<const FEquipmentMain*>(Pair.Value);
				if (ItemValue.EquipmentName == SaveGameLoadout->Loadouts[i].Primary)
				{
					LoadoutContent->Primary->ShowImg->SetBrushFromLazyTexture(ItemValue.ShowImg);
				}
				else if (ItemValue.EquipmentName == SaveGameLoadout->Loadouts[i].Secondary)
				{
					LoadoutContent->Secondary->ShowImg->SetBrushFromLazyTexture(ItemValue.ShowImg);
				}
				else if (ItemValue.EquipmentName == SaveGameLoadout->Loadouts[i].Melee)
				{
					LoadoutContent->Melee->ShowImg->SetBrushFromLazyTexture(ItemValue.ShowImg);
				}
				else if (ItemValue.EquipmentName == SaveGameLoadout->Loadouts[i].Throwing)
				{
					LoadoutContent->Throwing->ShowImg->SetBrushFromLazyTexture(ItemValue.ShowImg);
				}
			}
		}
	}

	// 判断是否拥有角色，没有则把存档恢复默认
	if (DefaultConfig && !HasHumanCharacter(SaveGameLoadout->HumanCharacterName))
	{
		SaveGameLoadout->HumanCharacterName = DefaultConfig->HumanCharacterName;
	}
	// 回显角色
	for (const TPair<FDataRegistryId, const uint8*>& Pair : HumanCharacterMains)
	{
		FHumanCharacterMain ItemValue = *reinterpret_cast<const FHumanCharacterMain*>(Pair.Value);
		if (ItemValue.HumanCharacterName == SaveGameLoadout->HumanCharacterName)
		{
			Human->ShowName->SetText(ItemValue.ShowName);
			
			if (ItemValue.ShowImgFullBody) Human->ShowImgFullBody->SetBrushFromLazyTexture(ItemValue.ShowImgFullBody);
			
			break;
		}
	}
	
	if (UDNAAsset2* DNAAsset1 = StorageSubsystem->GetDNAAssetByType(SaveGameLoadout->DNA1))
	{
		DNAButton1->UpdateDNAInfo(DNAAsset1);
	}
	if (UDNAAsset2* DNAAsset2 = StorageSubsystem->GetDNAAssetByType(SaveGameLoadout->DNA2))
	{
		DNAButton2->UpdateDNAInfo(DNAAsset2);
	}
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
		
		UAssetSubsystem* AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
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
		FString EnumValue = StaticEnum<EEquipmentType>()->GetNameStringByValue(static_cast<int64>(ItemValue.EquipmentType));

		if (EquipmentType == STORAGE_TYPE_ALL || EnumValue == EquipmentType)
		{
			if (ItemValue.bIsSupply) continue;
			
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
		FText ShowName = FText();
		for (const TPair<FDataRegistryId, const uint8*>& Pair : EquipmentMains)
		{
			FEquipmentMain ItemValue = *reinterpret_cast<const FEquipmentMain*>(Pair.Value);
			if (ItemValue.EquipmentName == EquipmentMain.EquipmentName)
			{
				ShowName = ItemValue.ShowName;
				break;
			}
		}
		EquipmentButton->ShowName->SetText(ShowName);
		EquipmentButton->Name = ShowName;
		EquipmentButton->ShowImg->SetBrushFromLazyTexture(EquipmentMain.ShowImg);
		EquipmentButton->ShowImgPath = EquipmentMain.ShowImg;
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
		FText ShowName = FText();
		for (const TPair<FDataRegistryId, const uint8*>& Pair : HumanCharacterMains)
		{
			FHumanCharacterMain ItemValue = *reinterpret_cast<const FHumanCharacterMain*>(Pair.Value);
			if (ItemValue.HumanCharacterName == HumanCharacterMain.HumanCharacterName)
			{
				ShowName = ItemValue.ShowName;
				break;
			}
		}
		CharacterButton->ShowName->SetText(ShowName);
		CharacterButton->Name = ShowName;
		CharacterButton->ShowImg->SetBrushFromLazyTexture(HumanCharacterMain.ShowImg);
		CharacterButton->ShowImgFullBodyPath = HumanCharacterMain.ShowImgFullBody;
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
		switch (EquipmentButton->EquipmentType)
		{
		case EEquipmentType::Primary:
			LoadoutContent->Primary->ShowName->SetText(EquipmentButton->Name);
			LoadoutContent->Primary->ShowImg->SetBrushFromLazyTexture(EquipmentButton->ShowImgPath);
			LoadoutContent->Primary->EquipmentName = EquipmentButton->EquipmentName;
			break;
		case EEquipmentType::Secondary:
			LoadoutContent->Secondary->ShowName->SetText(EquipmentButton->Name);
			LoadoutContent->Secondary->ShowImg->SetBrushFromLazyTexture(EquipmentButton->ShowImgPath);
			LoadoutContent->Secondary->EquipmentName = EquipmentButton->EquipmentName;
			break;
		case EEquipmentType::Melee:
			LoadoutContent->Melee->ShowName->SetText(EquipmentButton->Name);
			LoadoutContent->Melee->ShowImg->SetBrushFromLazyTexture(EquipmentButton->ShowImgPath);
			LoadoutContent->Melee->EquipmentName = EquipmentButton->EquipmentName;
			break;
		case EEquipmentType::Throwing:
			LoadoutContent->Throwing->ShowName->SetText(EquipmentButton->Name);
			LoadoutContent->Throwing->ShowImg->SetBrushFromLazyTexture(EquipmentButton->ShowImgPath);
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
