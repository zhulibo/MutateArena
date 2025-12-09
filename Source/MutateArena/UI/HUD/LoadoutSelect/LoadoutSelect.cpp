#include "LoadoutSelect.h"

#include "LoadoutSelectButton.h"
#include "CommonHierarchicalScrollBox.h"
#include "CommonTextBlock.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/PlayerControllers/BaseController.h"
#include "MutateArena/System/UISubsystem.h"
#include "MutateArena/System/Storage/SaveGameLoadout.h"
#include "MutateArena/System/Storage/StorageSubsystem.h"
#include "MutateArena/UI/GameLayout.h"
#include "MutateArena/UI/ProjectTags.h"
#include "MutateArena/UI/Common/CommonButton.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

void ULoadoutSelect::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	for (int i = 0; i < LoadoutSelectButtonContainer->GetChildrenCount(); ++i)
	{
		if (ULoadoutSelectButton* LoadoutSelectButton = Cast<ULoadoutSelectButton>(LoadoutSelectButtonContainer->GetChildAt(i)))
		{
			LoadoutSelectButton->OnClicked().AddUObject(this, &ThisClass::OnLoadoutSelectButtonClicked, i);
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
	if (UISubsystem == nullptr) UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetOwningLocalPlayer());;
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
