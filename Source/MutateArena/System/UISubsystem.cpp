#include "UISubsystem.h"

#include "MutateArena/UI/LayoutBase.h"

void UUISubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UUISubsystem::RegisterGameLayout(ULayoutBase* Layout)
{
	if (Layout)
	{
		ActiveLayouts.AddUnique(Layout);
	}
}

void UUISubsystem::UnregisterGameLayout(ULayoutBase* Layout)
{
	if (Layout)
	{
		ActiveLayouts.Remove(Layout);
	}
}

UCommonActivatableWidgetStack* UUISubsystem::GetLayerStack(FGameplayTag LayerTag)
{
	for (int32 i = ActiveLayouts.Num() - 1; i >= 0; --i)
	{
		ULayoutBase* LayoutPtr = ActiveLayouts[i].Get();
        
		if (LayoutPtr == nullptr) 
		{
			ActiveLayouts.RemoveAt(i);
			
			continue;
		}

		if (UCommonActivatableWidgetStack* FoundStack = LayoutPtr->GetStackByTag(LayerTag))
		{
			return FoundStack;
		}
	}
    
	return nullptr;
}
