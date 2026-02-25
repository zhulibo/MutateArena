#include "ServerContainer.h"

#include "Server.h"
#include "MutateArena/System/Tags/ProjectTags.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

void UServerContainer::NativeOnInitialized()
{
	Super::NativeOnInitialized();
}

void UServerContainer::NativeConstruct()
{
	Super::NativeConstruct();
	
	RegisterLayer(TAG_UI_LAYER_SERVER, ServerStack);

	if (ServerClass)
	{
		ServerStack->AddWidget(ServerClass);
	}
}
