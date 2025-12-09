#include "MenuLayout.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "Menu.h"
#include "ProjectTags.h"

void UMenuLayout::NativeOnInitialized()
{
	Super::NativeOnInitialized();
}

void UMenuLayout::NativeConstruct()
{
	Super::NativeConstruct();

	RegisterLayer(TAG_UI_LAYER_MENU, MenuStack);
	RegisterLayer(TAG_UI_LAYER_MODAL, ModalStack);
	
	MenuStack->AddWidget(MenuClass);
}
