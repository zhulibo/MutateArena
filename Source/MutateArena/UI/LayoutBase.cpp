#include "LayoutBase.h"

#include "MutateArena/System/UISubsystem.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

void ULayoutBase::NativeOnInitialized()
{
	Super::NativeOnInitialized();
}

void ULayoutBase::NativeConstruct()
{
	Super::NativeConstruct();
	
	if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetOwningLocalPlayer()))
	{
		UISubsystem->RegisterGameLayout(this);
	}
}

void ULayoutBase::NativeDestruct()
{
	Super::NativeDestruct();
	
	if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetOwningLocalPlayer()))
	{
		UISubsystem->UnregisterGameLayout(this);
	}
}

void ULayoutBase::RegisterLayer(FGameplayTag LayerTag, UCommonActivatableWidgetStack* Stack)
{
	if (Stack && LayerTag.IsValid())
	{
		LayersMap.Add(LayerTag, Stack);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to register layer, LayerTag [%s]"), *LayerTag.ToString());
	}
}

UCommonActivatableWidgetStack* ULayoutBase::GetStackByTag(FGameplayTag LayerTag) const
{
	return LayersMap.FindRef(LayerTag);
}
