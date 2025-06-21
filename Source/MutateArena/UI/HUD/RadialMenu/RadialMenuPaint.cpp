#include "RadialMenuPaint.h"

#include "CommonTextBlock.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/System/AssetSubsystem.h"
#include "MutateArena/System/Data/CommonAsset.h"

void URadialMenuPaint::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (AssetSubsystem == nullptr) AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
	if (AssetSubsystem && AssetSubsystem->CommonAsset)
	{
		auto SprayPaints = AssetSubsystem->CommonAsset->SprayPaints;
		
		if (SprayPaints.IsValidIndex(0))
		{
			FText Text = FText();
			FText::FindTextInLiveTable_Advanced(CULTURE_SPRAY_PAINT, SprayPaints[0].Name, Text);
			Item1->SetText(Text);
		}
		if (SprayPaints.IsValidIndex(1))
		{
			FText Text = FText();
			FText::FindTextInLiveTable_Advanced(CULTURE_SPRAY_PAINT, SprayPaints[1].Name, Text);
			Item2->SetText(Text);
		}
		if (SprayPaints.IsValidIndex(2))
		{
			FText Text = FText();
			FText::FindTextInLiveTable_Advanced(CULTURE_SPRAY_PAINT, SprayPaints[2].Name, Text);
			Item3->SetText(Text);
		}
		if (SprayPaints.IsValidIndex(3))
		{
			FText Text = FText();
			FText::FindTextInLiveTable_Advanced(CULTURE_SPRAY_PAINT, SprayPaints[3].Name, Text);
			Item4->SetText(Text);
		}
		if (SprayPaints.IsValidIndex(4))
		{
			FText Text = FText();
			FText::FindTextInLiveTable_Advanced(CULTURE_SPRAY_PAINT, SprayPaints[4].Name, Text);
			Item5->SetText(Text);
		}
		if (SprayPaints.IsValidIndex(5))
		{
			FText Text = FText();
			FText::FindTextInLiveTable_Advanced(CULTURE_SPRAY_PAINT, SprayPaints[5].Name, Text);
			Item6->SetText(Text);
		}
		if (SprayPaints.IsValidIndex(6))
		{
			FText Text = FText();
			FText::FindTextInLiveTable_Advanced(CULTURE_SPRAY_PAINT, SprayPaints[6].Name, Text);
			Item7->SetText(Text);
		}
		if (SprayPaints.IsValidIndex(7))
		{
			FText Text = FText();
			FText::FindTextInLiveTable_Advanced(CULTURE_SPRAY_PAINT, SprayPaints[7].Name, Text);
			Item8->SetText(Text);
		}
	}
}
