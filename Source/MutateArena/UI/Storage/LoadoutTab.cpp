#include "LoadoutTab.h"

#include "CommonActionWidget.h"
#include "LoadoutContent.h"
#include "CommonActivatableWidgetSwitcher.h"
#include "MutateArena/UI/Common/CommonButton.h"
#include "Components/HorizontalBox.h"
#include "CommonTextBlock.h"
#include "Components/SizeBox.h"

void ULoadoutTab::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 把ActionData绑定给CommonActionWidget
	LeftTabAction->SetInputAction(PreviousTabInputActionData);
	RightTabAction->SetInputAction(NextTabInputActionData);
}

void ULoadoutTab::NativeConstruct()
{
	Super::NativeConstruct();

	LinkSwitcher();
}

void ULoadoutTab::LinkSwitcher()
{
	if (LoadoutSwitcher && TabButtonContainer && TabButtonClass)
	{
		// 链接Tab和Switcher
		SetLinkedSwitcher(LoadoutSwitcher);

		for (int32 i = 0; i < TabContents.Num(); ++i)
		{
			if (TabContents[i] == nullptr) break;

			FString TabWidgetName = TabContents[i]->GetName();
			FName TabButtonNameID = FName(*FString::FromInt(i + 1));
			RegisterTab(TabButtonNameID, TabButtonClass, TabContents[i], i);

			USizeBox* SizeBox = NewObject<USizeBox>(this);
			UCommonButton* TabButton = Cast<UCommonButton>(GetTabButtonBaseByID(TabButtonNameID));
			if (SizeBox && TabButton)
			{
				TabButton->ButtonText->SetText(FText::FromName(TabButtonNameID));
				SizeBox->SetWidthOverride(50);
				SizeBox->AddChild(TabButton);
				TabButtonContainer->AddChild(SizeBox);
			}
		}
	}
}
