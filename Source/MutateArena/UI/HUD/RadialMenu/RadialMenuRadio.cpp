#include "RadialMenuRadio.h"
#include "MutateArena/System/AssetSubsystem.h"
#include "MutateArena/Assets/Data/CommonAsset.h"

void URadialMenuRadio::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (AssetSubsystem == nullptr) 
		AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();

	TArray<FRadialMenuItemData> MenuData;

	if (AssetSubsystem && AssetSubsystem->CommonAsset)
	{
		auto RadioTexts = AssetSubsystem->CommonAsset->RadioTexts;
        
		for (int32 i = 0; i < RadioTexts.Num(); ++i)
		{
			FRadialMenuItemData ItemData;
            
			if (RadioTexts.IsValidIndex(i))
			{
				ItemData.ItemText = FText::FromString(RadioTexts[i]);
			}
			else
			{
				ItemData.ItemText = FText::FromString(TEXT("-1"));
			}
            
			MenuData.Add(ItemData);
		}
	}

	BuildMenu(MenuData);
}
