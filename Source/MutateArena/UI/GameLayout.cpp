#include "GameLayout.h"

#include "MutateArena/PlayerControllers/BaseController.h"
#include "MutateArena/System/UISubsystem.h"
#include "MutateArena/System/Tags/ProjectTags.h"
#include "MutateArena/UI/HUD/PauseMenu.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

void UGameLayout::NativeOnInitialized()
{
	if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetOwningLocalPlayer()))
	{
		UISubsystem->ShowPauseMenu.AddUObject(this, &ThisClass::ShowPauseMenu);
	}
}

void UGameLayout::NativeConstruct()
{
	Super::NativeConstruct();
	
	RegisterLayer(TAG_UI_LAYER_GAME, GameStack);
	RegisterLayer(TAG_UI_LAYER_MENU, MenuStack);
	RegisterLayer(TAG_UI_LAYER_MODAL, ModalStack);
}

void UGameLayout::ShowPauseMenu()
{
	if (ABaseController* BaseController = Cast<ABaseController>(GetOwningPlayer()))
	{
		BaseController->FocusUI();
	}
	
	MenuStack->AddWidget(PauseMenuClass);
}
