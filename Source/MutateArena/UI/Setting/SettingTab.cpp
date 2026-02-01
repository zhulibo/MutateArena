#include "SettingTab.h"
#include "CommonActionWidget.h"
#include "CommonActivatableWidget.h"
#include "CommonActivatableWidgetSwitcher.h"
#include "CommonTextBlock.h"
#include "MetaSoundSource.h"
#include "MutateArena/UI/Common/CommonButton.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Kismet/GameplayStatics.h"
#include "MutateArena/Assets/Data/CommonAsset.h"
#include "MutateArena/System/AssetSubsystem.h"

#define LOCTEXT_NAMESPACE "USettingTab"

void USettingTab::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 把ActionData绑定给CommonActionWidget
	LeftTabAction->SetInputAction(PreviousTabInputActionData);
	RightTabAction->SetInputAction(NextTabInputActionData);
	
	OnTabSelected.AddDynamic(this, &ThisClass::HandleOnTabSelected);
}

void USettingTab::NativeConstruct()
{
	Super::NativeConstruct();
	
	bIsInitialized = true;

	LinkSwitcher();
}

void USettingTab::LinkSwitcher()
{
	if (SettingSwitcher && TabButtonContainer && TabButtonClass)
	{
		// 链接Tab和Switcher
		SetLinkedSwitcher(SettingSwitcher);
		
		for (int32 i = 0; i < TabContents.Num(); ++i)
		{
			if (TabContents[i] == nullptr) break;
			FString TabWidgetName = TabContents[i]->GetName();
			FName TabButtonNameID = FName(*TabWidgetName.Right(TabWidgetName.Len() - 3));
			RegisterTab(TabButtonNameID, TabButtonClass, TabContents[i], i);

			if (UCommonButton* TabButton = Cast<UCommonButton>(GetTabButtonBaseByID(TabButtonNameID)))
			{
				if (TabButtonNameID == TEXT("Game"))
				{
					TabButton->ButtonText->SetText(LOCTEXT("Game", "Game"));
				}
				if (TabButtonNameID == TEXT("Control"))
				{
					TabButton->ButtonText->SetText(LOCTEXT("Control", "Control"));
				}
				if (TabButtonNameID == TEXT("Video"))
				{
					TabButton->ButtonText->SetText(LOCTEXT("Video", "Video"));
				}
				if (TabButtonNameID == TEXT("Audio"))
				{
					TabButton->ButtonText->SetText(LOCTEXT("Audio", "Audio"));
				}

				UHorizontalBoxSlot* NewSlot = Cast<UHorizontalBoxSlot>(TabButtonContainer->AddChildToHorizontalBox(TabButton));
				if (NewSlot) NewSlot->SetPadding(FMargin(10, 0, 10, 0));
			}
		}
		
		// SelectTabByID("Game");
		// SelectTabByID("Control");
		// SelectTabByID("Video");
		// SelectTabByID("Audio);
	}
}

void USettingTab::HandleOnTabSelected(FName TabId)
{
	if (bIsInitialized)
	{
		bIsInitialized = false;
		UE_LOG(LogTemp, Warning, TEXT("USettingTab"));
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
