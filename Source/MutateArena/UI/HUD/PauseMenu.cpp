#include "PauseMenu.h"
#include "CommonTextBlock.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Characters/Components/CombatComponent.h"
#include "MutateArena/UI/HUD/Mutation/MutationContainer.h"
#include "MutateArena/PlayerControllers/BaseController.h"
#include "MutateArena/System/AssetSubsystem.h"
#include "MutateArena/System/EOSSubsystem.h"
#include "MutateArena/System/Data/CommonAsset.h"
#include "MutateArena/UI/GameLayout.h"
#include "MutateArena/UI/Common/CommonButton.h"
#include "MutateArena/UI/Common/ConfirmScreen.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "MutateArena/UI/Setting/Setting.h"
#include "MutateArena/UI/HUD/LoadoutSelect/LoadoutSelect.h"
#include "MutateArena/UI/HUD/MutantSelect/MutantSelect.h"
#include "MutateArena/Utils/LibraryNotify.h"

#define LOCTEXT_NAMESPACE "UPauseMenu"

void UPauseMenu::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	OnActivated().AddWeakLambda(this, [this]() {
		bWantToBack = true;
	});

	OnDeactivated().AddUObject(this, &ThisClass::OnDeactivatedInternal);

	LoadoutSelectButton->ButtonText->SetText(LOCTEXT("Loadout", "Loadout"));
	LoadoutSelectButton->OnClicked().AddUObject(this, &ThisClass::OnLoadoutSelectClicked);

	MutantSelectButton->ButtonText->SetText(LOCTEXT("Mutant", "Mutant"));
	MutantSelectButton->OnClicked().AddUObject(this, &ThisClass::OnMutantSelectButtonClicked);

	SettingButton->ButtonText->SetText(LOCTEXT("Setting", "Setting"));
	SettingButton->OnClicked().AddUObject(this, &ThisClass::OnSettingButtonClicked);

	VoteButton->ButtonText->SetText(LOCTEXT("Vote", "Vote"));
	VoteButton->OnClicked().AddUObject(this, &ThisClass::OnVoteButtonClicked);
	VoteButton->SetIsEnabled(false);

	QuitButton->ButtonText->SetText(LOCTEXT("Quit", "Quit"));
	QuitButton->OnClicked().AddUObject(this, &ThisClass::OnQuitButtonClicked);
}

UWidget* UPauseMenu::NativeGetDesiredFocusTarget() const
{
	return LoadoutSelectButton;
}

void UPauseMenu::OnDeactivatedInternal()
{
	// 进入二级菜单时不需要FocusGame
	if (!bWantToBack)
	{
		return;
	}

	// 返回游戏时FocusGame
	if (BaseController == nullptr) BaseController = Cast<ABaseController>(GetOwningPlayer());
	if (BaseController)
	{
		BaseController->FocusGame();
	}
}

void UPauseMenu::OnLoadoutSelectClicked()
{
	if (BaseController == nullptr) BaseController = Cast<ABaseController>(GetOwningPlayer());
	if (BaseController && BaseController->GameLayout)
	{
		bWantToBack = false;

		BaseController->GameLayout->MenuStack->AddWidget(LoadoutSelectClass);
	}
}

void UPauseMenu::OnMutantSelectButtonClicked()
{
	if (BaseController == nullptr) BaseController = Cast<ABaseController>(GetOwningPlayer());
	if (BaseController && BaseController->GameLayout)
	{
		bWantToBack = false;
		
		BaseController->GameLayout->MenuStack->AddWidget(MutantSelectClass);
	}
}

void UPauseMenu::OnSettingButtonClicked()
{
	if (BaseController == nullptr) BaseController = Cast<ABaseController>(GetOwningPlayer());
	if (BaseController && BaseController->GameLayout)
	{
		bWantToBack = false;
		
		BaseController->GameLayout->MenuStack->AddWidget(SettingClass);
	}
}

void UPauseMenu::OnVoteButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("OnVoteButtonClicked"));
}

void UPauseMenu::OnQuitButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("OnQuitButtonClicked -------------------------------------------"));
	
	if (BaseController == nullptr) BaseController = Cast<ABaseController>(GetOwningPlayer());
	UAssetSubsystem* AssetSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UAssetSubsystem>();
	
	if (BaseController && BaseController->GameLayout && AssetSubsystem && AssetSubsystem->CommonAsset)
	{
		FConfirmScreenComplete ResultCallback = FConfirmScreenComplete::CreateUObject(this, &ThisClass::Quit);

		if (EOSSubsystem == nullptr) EOSSubsystem = GetGameInstance()->GetSubsystem<UEOSSubsystem>();
		if (EOSSubsystem)
		{
			if (EOSSubsystem->IsLobbyHost())
			{
				BaseController->GameLayout->ModalStack->AddWidget<UConfirmScreen>(
					AssetSubsystem->CommonAsset->ConfirmScreenClass,
					[ResultCallback](UConfirmScreen& Dialog) {
						Dialog.Setup(LOCTEXT("SureToQuitHost", "You are host, all clients will lose connection, sure to quit?"), ResultCallback);
					}
				);

				return;
			}
		}

		BaseController->GameLayout->ModalStack->AddWidget<UConfirmScreen>(
			AssetSubsystem->CommonAsset->ConfirmScreenClass,
			[ResultCallback](UConfirmScreen& Dialog) {
				Dialog.Setup(LOCTEXT("SureToQuit", "Sure to quit?"), ResultCallback);
			}
		);
	}
}

void UPauseMenu::Quit(EMsgResult MsgResult)
{
	if (MsgResult == EMsgResult::Confirm)
	{
		if (BaseController == nullptr) BaseController = Cast<ABaseController>(GetOwningPlayer());
		if (BaseController)
		{
			if (EOSSubsystem == nullptr) EOSSubsystem = GetGameInstance()->GetSubsystem<UEOSSubsystem>();
			if (EOSSubsystem)
			{
				// 房主退出游戏时修改大厅状态（房主一般不主动退出）
				if (EOSSubsystem->IsLobbyHost())
				{
					EOSSubsystem->ModifyLobbyAttr(TMap<FSchemaAttributeId, FSchemaVariant>{
						{LOBBY_STATUS, static_cast<int64>(0)},
					});
				}
				// 非房主退出游戏时把自己置为未准备状态
				else
				{
					EOSSubsystem->ModifyLobbyMemberAttr(TMap<FSchemaAttributeId, FSchemaVariant>{
						{ LOBBY_MEMBER_READY, false}
					});
				}
			}

			// 销毁装备
			AHumanCharacter* HumanCharacter = Cast<AHumanCharacter>(GetOwningPlayer()->GetPawn());
			if (HumanCharacter && HumanCharacter->CombatComponent)
			{
				HumanCharacter->CombatComponent->ServerDestroyEquipments();
			}

			BaseController->ClientTravel(MAP_MENU, ETravelType::TRAVEL_Absolute);
		}
	}
}

#undef LOCTEXT_NAMESPACE
