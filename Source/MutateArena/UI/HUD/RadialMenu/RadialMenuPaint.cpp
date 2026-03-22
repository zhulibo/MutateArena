#include "RadialMenuPaint.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/System/AssetSubsystem.h"
#include "MutateArena/Assets/Data/CommonAsset.h"

void URadialMenuPaint::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (AssetSubsystem == nullptr) 
		AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();

	TArray<FRadialMenuItemData> MenuData;

	if (AssetSubsystem && AssetSubsystem->CommonAsset)
	{
		auto SprayPaints = AssetSubsystem->CommonAsset->SprayPaints;
        
		for (int32 i = 0; i < SprayPaints.Num(); ++i)
		{
			FRadialMenuItemData ItemData;
            
			if (SprayPaints.IsValidIndex(i))
			{
				ItemData.ItemText = SprayPaints[i].Name;
				ItemData.ItemTexture = SprayPaints[i].Texture;
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
