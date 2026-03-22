#include "MutantSelect.h"

#include "AbilitySystemComponent.h"
#include "MutantSelectButton.h"
#include "CommonHierarchicalScrollBox.h"
#include "CommonTextBlock.h"
#include "DataRegistrySubsystem.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/Characters/Data/CharacterType.h"
#include "MutateArena/Characters/MutantCharacter.h"
#include "MutateArena/Characters/Data/CharacterAsset.h"
#include "MutateArena/GameStates/MutationGameState.h"
#include "MutateArena/PlayerControllers/BaseController.h"
#include "MutateArena/System/AssetSubsystem.h"
#include "MutateArena/System/Storage/StorageSubsystem.h"
#include "MutateArena/UI/Common/CommonButton.h"
#include "MutateArena/UI/HUD/Mutation/MutationContainer.h"
#include "Components/ScrollBoxSlot.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "MutateArena/Abilities/Mutants/GA_MutantChange.h"
#include "MutateArena/PlayerStates/BasePlayerState.h"
#include "MutateArena/System/UISubsystem.h"
#include "MutateArena/System/Storage/SaveGameLoadout.h"
#include "MutateArena/UI/GameLayout.h"
#include "MutateArena/System/Tags/ProjectTags.h"
#include "MutateArena/Utils/LibraryCommon.h"
#include "MutateArena/Utils/LibraryNotify.h"

#define LOCTEXT_NAMESPACE "UMutantSelect"

void UMutantSelect::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetOwningLocalPlayer()))
	{
		UISubsystem->OnRoundEnded.AddUObject(this, &ThisClass::OnRoundEnded);
	}

	if (UDataRegistry* DataRegistry = UDataRegistrySubsystem::Get()->GetRegistryForType(DR_MUTANT_CHARACTER_MAIN))
	{
		const UScriptStruct* OutStruct;
		DataRegistry->GetAllCachedItems(MutantCharacterMain, OutStruct);

		for (const TPair<FDataRegistryId, const uint8*>& Pair : MutantCharacterMain)
		{
			FMutantCharacterMain ItemValue = *reinterpret_cast<const FMutantCharacterMain*>(Pair.Value);

			if (UMutantSelectButton* MutantSelectButton = CreateWidget<UMutantSelectButton>(this, MutantSelectButtonClass))
			{
				MutantSelectButton->Name->SetText(ItemValue.ShowName);
				MutantSelectButton->Attribute->SetText(ItemValue.Attribute);
				MutantSelectButton->Skill->SetText(ItemValue.Skill);
				MutantSelectButton->Desc->SetText(ItemValue.Desc);
				MutantSelectButton->MutantCharacterName = ItemValue.MutantCharacterName;
				MutantSelectButton->OnClicked().AddUObject(this, &ThisClass::OnMutantSelectButtonClicked, ItemValue.MutantCharacterName);
				UScrollBoxSlot* NewSlot = Cast<UScrollBoxSlot>(MutantSelectButtonContainer->AddChild(MutantSelectButton));
				if (NewSlot) NewSlot->SetPadding(FMargin(0, 0, 15, 0));
			}
		}
	}
}

UWidget* UMutantSelect::NativeGetDesiredFocusTarget() const
{
	if (AMutantCharacter* MutantCharacter = Cast<AMutantCharacter>(GetOwningPlayerPawn()))
	{
		for (int i = 0; i < MutantSelectButtonContainer->GetChildrenCount(); ++i)
		{
			if (UMutantSelectButton* MutantSelectButton = Cast<UMutantSelectButton>(MutantSelectButtonContainer->GetChildAt(i)))
			{
				if (MutantSelectButton->MutantCharacterName == MutantCharacter->MutantCharacterName)
				{
					return MutantSelectButtonContainer->GetChildAt(i);
				}
			}
		}
	}

	return MutantSelectButtonContainer->GetChildAt(0);
}

void UMutantSelect::OnMutantSelectButtonClicked(EMutantCharacterName MutantCharacterName)
{
	// 保存到PlayerState
	if (BasePlayerState == nullptr) BasePlayerState = Cast<ABasePlayerState>(GetOwningPlayerState());
	if (BasePlayerState)
	{
		BasePlayerState->ServerSetMutantCharacterName(MutantCharacterName);
	}

	// 保存到文件
	if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
	if (StorageSubsystem && StorageSubsystem->CacheLoadout)
	{
		StorageSubsystem->CacheLoadout->MutantCharacterName = MutantCharacterName;
		StorageSubsystem->SaveLoadouts();
	}

	// 切换角色
	if (AMutantCharacter* MutantCharacter = Cast<AMutantCharacter>(GetOwningPlayerPawn()))
	{
		UAbilitySystemComponent* ASC = MutantCharacter->GetAbilitySystemComponent();
		if (ASC && ASC->GetTagCount(TAG_STATE_MUTANT_CHANGING) > 0)
		{
			MutantCharacter->ServerSelectMutant(MutantCharacterName);

			// 终止切换角色技能
			FScopedPredictionWindow PredictionWindow(ASC, MutantCharacter->IsLocallyControlled() && !MutantCharacter->HasAuthority());
			FGameplayTagContainer CancelTags;
			CancelTags.AddTag(TAG_ABILITY_MUTANT_CHANGE); 
			ASC->CancelAbilities(&CancelTags);
		}
		else
		{
			NOTIFY(this, C_GREEN, LOCTEXT("MutantChangeApply", "Mutant change will apply in the next round"));
		}
	}
	else
	{
		NOTIFY(this, C_GREEN, LOCTEXT("MutantChangeSave", "Mutant change have saved"));
	}

	CloseMenu(true);
}

void UMutantSelect::OnRoundEnded()
{
	if (IsActivated())
	{
		CloseMenu(true);
	}
}

void UMutantSelect::CloseMenu(bool bClosePauseMenu)
{
	if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetOwningLocalPlayer()))
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

#undef LOCTEXT_NAMESPACE
