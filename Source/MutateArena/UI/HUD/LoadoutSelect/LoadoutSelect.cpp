#include "LoadoutSelect.h"

#include "LoadoutSelectButton.h"
#include "CommonHierarchicalScrollBox.h"
#include "CommonTextBlock.h"
#include "Components/ScrollBoxSlot.h"
#include "Engine/GameInstance.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Equipments/Data/EquipmentAsset.h"
#include "MutateArena/System/UISubsystem.h"
#include "MutateArena/System/Storage/ConfigType.h"
#include "MutateArena/System/Storage/SaveGameLoadout.h"
#include "MutateArena/System/Storage/StorageSubsystem.h"
#include "MutateArena/UI/GameLayout.h"
#include "MutateArena/System/GameplayTags.h"
#include "MutateArena/UI/Common/CommonButton.h"
#include "MutateArena/Utils/LibraryCommon.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "MutateArena/System/AssetSubsystem.h"

void ULoadoutSelect::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
	UAssetSubsystem* AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();

	if (StorageSubsystem && StorageSubsystem->CacheSetting && StorageSubsystem->CacheLoadout && AssetSubsystem)
	{
		for (int32 i = 0; i < StorageSubsystem->CacheLoadout->Loadouts.Num(); ++i)
		{
			if (ULoadoutSelectButton* LoadoutSelectButton = CreateWidget<ULoadoutSelectButton>(this, LoadoutSelectButtonClass))
			{
				const FLoadout& CurrentLoadout = StorageSubsystem->CacheLoadout->Loadouts[i];

				// Slot1
				if (UEquipmentAsset* Asset = AssetSubsystem->GetEquipmentAssetByName(CurrentLoadout.Slot1EquipmentName))
				{
					LoadoutSelectButton->Slot1EquipmentText->SetText(Asset->ShowName);
				}

				// Slot2
				if (UEquipmentAsset* Asset = AssetSubsystem->GetEquipmentAssetByName(CurrentLoadout.Slot2EquipmentName))
				{
					LoadoutSelectButton->Slot2EquipmentText->SetText(Asset->ShowName);
				}

				// Melee
				if (UEquipmentAsset* Asset = AssetSubsystem->GetEquipmentAssetByName(CurrentLoadout.Slot3EquipmentName))
				{
					LoadoutSelectButton->Slot3EquipmentText->SetText(Asset->ShowName);
				}

				// Throwable
				if (UEquipmentAsset* Asset = AssetSubsystem->GetEquipmentAssetByName(CurrentLoadout.Slot4EquipmentName))
				{
					LoadoutSelectButton->Slot4EquipmentText->SetText(Asset->ShowName);
				}

				LoadoutSelectButton->OnClicked().AddUObject(this, &ThisClass::OnLoadoutSelectButtonClicked, i);

				if (UScrollBoxSlot* NewSlot = Cast<UScrollBoxSlot>(LoadoutSelectButtonContainer->AddChild(LoadoutSelectButton)))
				{
					NewSlot->SetPadding(FMargin(20, 0, 0, 0));
				}
			}
		}
	}
}

UWidget* ULoadoutSelect::NativeGetDesiredFocusTarget() const
{
	if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
	if (StorageSubsystem && StorageSubsystem->CacheLoadout)
	{
		int32 LoadoutIndex = StorageSubsystem->CacheLoadout->LoadoutIndex;
		if (LoadoutIndex > 0 && LoadoutIndex < LoadoutSelectButtonContainer->GetChildrenCount())
		{
			return LoadoutSelectButtonContainer->GetChildAt(LoadoutIndex);
		}
	}

	return LoadoutSelectButtonContainer->GetChildAt(0);
}

void ULoadoutSelect::OnLoadoutSelectButtonClicked(int32 LoadoutIndex)
{
	if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
	if (StorageSubsystem && StorageSubsystem->CacheLoadout)
	{
		StorageSubsystem->CacheLoadout->LoadoutIndex = LoadoutIndex;
		StorageSubsystem->SaveLoadouts();

		if (AHumanCharacter* HumanCharacter = Cast<AHumanCharacter>(GetOwningPlayerPawn()))
		{
			HumanCharacter->TrySwitchLoadout();
		}
	}

	CloseMenu(true);
}

void ULoadoutSelect::CloseMenu(bool bClosePauseMenu)
{
	if (UISubsystem == nullptr) UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetOwningLocalPlayer());
	if (UISubsystem)
	{
		DeactivateWidget();
		
		if (bClosePauseMenu)
		{
			if (auto Layer = UISubsystem->GetLayerStack(TAG_UI_LAYER_MENU))
			{
				Layer->RemoveWidget(*Layer->GetActiveWidget());
			}
		}
	}
}
