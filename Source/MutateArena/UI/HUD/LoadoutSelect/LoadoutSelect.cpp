#include "LoadoutSelect.h"

#include "LoadoutSelectButton.h"
#include "CommonHierarchicalScrollBox.h"
#include "CommonTextBlock.h"
#include "DataRegistrySubsystem.h"
#include "Components/ScrollBoxSlot.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/System/UISubsystem.h"
#include "MutateArena/System/Storage/ConfigType.h"
#include "MutateArena/System/Storage/SaveGameLoadout.h"
#include "MutateArena/System/Storage/StorageSubsystem.h"
#include "MutateArena/UI/GameLayout.h"
#include "MutateArena/System/Tags/ProjectTags.h"
#include "MutateArena/UI/Common/CommonButton.h"
#include "MutateArena/Utils/LibraryCommon.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

void ULoadoutSelect::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
	if (StorageSubsystem && StorageSubsystem->CacheSetting)
	{
		for (int32 i = 0; i < StorageSubsystem->CacheLoadout->Loadouts.Num(); ++i)
		{
			if (ULoadoutSelectButton* LoadoutSelectButton = CreateWidget<ULoadoutSelectButton>(this, LoadoutSelectButtonClass))
			{
				{
					FString EnumValue = StaticEnum<EEquipmentName>()->GetNameStringByValue(static_cast<int64>(StorageSubsystem->CacheLoadout->Loadouts[i].Primary));
					FDataRegistryId DataRegistryId(DR_EQUIPMENT_MAIN, FName(EnumValue));
					if (const FEquipmentMain* EquipmentMain = UDataRegistrySubsystem::Get()->GetCachedItem<FEquipmentMain>(DataRegistryId))
					{
						LoadoutSelectButton->PrimaryEquipmentText->SetText(EquipmentMain->ShowName);
					}
				}
				
				{
					FString EnumValue = StaticEnum<EEquipmentName>()->GetNameStringByValue(static_cast<int64>(StorageSubsystem->CacheLoadout->Loadouts[i].Secondary));
					FDataRegistryId DataRegistryId(DR_EQUIPMENT_MAIN, FName(EnumValue));
					if (const FEquipmentMain* EquipmentMain = UDataRegistrySubsystem::Get()->GetCachedItem<FEquipmentMain>(DataRegistryId))
					{
						LoadoutSelectButton->SecondaryEquipmentText->SetText(EquipmentMain->ShowName);
					}
				}
				
				{
					FString EnumValue = StaticEnum<EEquipmentName>()->GetNameStringByValue(static_cast<int64>(StorageSubsystem->CacheLoadout->Loadouts[i].Melee));
					FDataRegistryId DataRegistryId(DR_EQUIPMENT_MAIN, FName(EnumValue));
					if (const FEquipmentMain* EquipmentMain = UDataRegistrySubsystem::Get()->GetCachedItem<FEquipmentMain>(DataRegistryId))
					{
						LoadoutSelectButton->MeleeEquipmentText->SetText(EquipmentMain->ShowName);
					}
				}
				
				{
					FString EnumValue = StaticEnum<EEquipmentName>()->GetNameStringByValue(static_cast<int64>(StorageSubsystem->CacheLoadout->Loadouts[i].Throwing));
					FDataRegistryId DataRegistryId(DR_EQUIPMENT_MAIN, FName(EnumValue));
					if (const FEquipmentMain* EquipmentMain = UDataRegistrySubsystem::Get()->GetCachedItem<FEquipmentMain>(DataRegistryId))
					{
						LoadoutSelectButton->ThrowingEquipmentText->SetText(EquipmentMain->ShowName);
					}
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
