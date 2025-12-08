#include "RadialMenuRadio.h"

#include "CommonTextBlock.h"
#include "MutateArena/System/AssetSubsystem.h"
#include "MutateArena/System/Data/CommonAsset.h"

void URadialMenuRadio::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (AssetSubsystem == nullptr) AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
	if (AssetSubsystem && AssetSubsystem->CommonAsset)
	{
		auto RadioTexts = AssetSubsystem->CommonAsset->RadioTexts;
		if (RadioTexts.IsValidIndex(0)) Item1->SetText(FText::FromString(RadioTexts[0]));
		if (RadioTexts.IsValidIndex(1)) Item2->SetText(FText::FromString(RadioTexts[1]));
		if (RadioTexts.IsValidIndex(2)) Item3->SetText(FText::FromString(RadioTexts[2]));
		if (RadioTexts.IsValidIndex(3)) Item4->SetText(FText::FromString(RadioTexts[3]));
		if (RadioTexts.IsValidIndex(4)) Item5->SetText(FText::FromString(RadioTexts[4]));
		if (RadioTexts.IsValidIndex(5)) Item6->SetText(FText::FromString(RadioTexts[5]));
		if (RadioTexts.IsValidIndex(6)) Item7->SetText(FText::FromString(RadioTexts[6]));
		if (RadioTexts.IsValidIndex(7)) Item8->SetText(FText::FromString(RadioTexts[7]));
	}
}
