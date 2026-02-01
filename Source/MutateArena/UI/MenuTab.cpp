#include "MenuTab.h"
#include "CommonActionWidget.h"
#include "CommonActivatableWidget.h"
#include "CommonActivatableWidgetSwitcher.h"
#include "CommonTextBlock.h"
#include "MetaSoundSource.h"
#include "Common/CommonButton.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Kismet/GameplayStatics.h"
#include "MutateArena/System/AssetSubsystem.h"
#include "MutateArena/Assets/Data/CommonAsset.h"

#define LOCTEXT_NAMESPACE "UMenuTab"

void UMenuTab::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 把ActionData绑定给CommonActionWidget
	LeftTabAction->SetInputAction(PreviousTabInputActionData);
	RightTabAction->SetInputAction(NextTabInputActionData);
	
	OnTabSelected.AddDynamic(this, &ThisClass::HandleOnTabSelected);
}

void UMenuTab::NativeConstruct()
{
	Super::NativeConstruct();

	bIsInitialized = true;
	
	LinkSwitcher();
}

void UMenuTab::LinkSwitcher()
{
	if (MenuSwitcher && TabButtonContainer && TabButtonClass)
	{
		// 链接Tab和Switcher
		SetLinkedSwitcher(MenuSwitcher);
		
		for (int32 i = 0; i < TabContents.Num(); ++i)
		{
			if (TabContents[i] == nullptr) break;
			FName TabButtonNameID = FName(TabContents[i]->GetName());
			RegisterTab(TabButtonNameID, TabButtonClass, TabContents[i], i);

			if (UCommonButton* TabButton = Cast<UCommonButton>(GetTabButtonBaseByID(TabButtonNameID)))
			{
				if (TabButtonNameID == TEXT("Server"))
				{
					TabButton->ButtonText->SetText(LOCTEXT("Server", "Server"));
				}
				if (TabButtonNameID == TEXT("Storage"))
				{
					TabButton->ButtonText->SetText(LOCTEXT("Storage", "Storage"));
				}
				if (TabButtonNameID == TEXT("Shop"))
				{
					TabButton->ButtonText->SetText(LOCTEXT("Shop", "Shop"));
				}
				if (TabButtonNameID == TEXT("Dev"))
				{
					TabButton->ButtonText->SetText(LOCTEXT("Dev", "Dev"));
				}

				if (UHorizontalBoxSlot* NewSlot = Cast<UHorizontalBoxSlot>(TabButtonContainer->AddChildToHorizontalBox(TabButton)))
				{
					NewSlot->SetPadding(FMargin(10, 0, 10, 0));
				}
			}
		}
		
		// SelectTabByID("shop");
	}
}

void UMenuTab::HandleOnTabSelected(FName TabId)
{
	if (bIsInitialized)
	{
		bIsInitialized = false;
		UE_LOG(LogTemp, Warning, TEXT("MenuTab"));
		return;
	}
	
	UAssetSubsystem* AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
	if (AssetSubsystem && AssetSubsystem->CommonAsset)
	{
		if (UAudioComponent* AudioComponent = UGameplayStatics::SpawnSound2D(this, AssetSubsystem->CommonAsset->TabSwitchSound))
		{
		}
	}
}

#undef LOCTEXT_NAMESPACE
