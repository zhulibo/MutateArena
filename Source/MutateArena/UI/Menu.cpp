#include "Menu.h"

#include "CommonTextBlock.h"
#include "MenuLayout.h"
#include "MutateArena/PlayerControllers/MenuController.h"
#include "MutateArena/System/AssetSubsystem.h"
#include "MutateArena/System/Data/CommonAsset.h"
#include "Common/CommonButton.h"
#include "Components/HorizontalBox.h"
#include "MutateArena/UI/Setting/Setting.h"
#include "MutateArena/UI/Common/ConfirmScreen.h"
#include "Kismet/GameplayStatics.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/System/EOSSubsystem.h"
#include "MutateArena/Utils/LibraryCommon.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

#define LOCTEXT_NAMESPACE "UMenu"

void UMenu::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	SettingButton->OnClicked().AddUObject(this, &ThisClass::OnSettingButtonClicked);

	QuitButton->OnClicked().AddUObject(this, &ThisClass::OnQuitButtonClicked);

	EOSSubsystem = GetGameInstance()->GetSubsystem<UEOSSubsystem>();
	if (EOSSubsystem)
	{
		EOSSubsystem->OnEnumerateTitleFilesComplete.AddUObject(this, &ThisClass::OnEnumerateTitleFilesComplete);
		EOSSubsystem->OnReadTitleFileComplete.AddUObject(this, &ThisClass::OnReadTitleFileComplete);

		// 读取标题文件显示消息
		FTimerDelegate TimerDelegate;
		TimerDelegate.BindWeakLambda(this, [this]() {
			if (EOSSubsystem)
			{
				EOSSubsystem->EnumerateTitleFiles();
			}
		});
		GetWorld()->GetTimerManager().SetTimer(EnumerateTitleFileTimerHandle, TimerDelegate, 600.f, true, 0.f);
	}
}

void UMenu::NativeDestruct()
{
	GetWorld()->GetTimerManager().ClearTimer(EnumerateTitleFileTimerHandle);

	Super::NativeDestruct();
}

void UMenu::OnSettingButtonClicked()
{
	if (MenuController == nullptr) MenuController = Cast<AMenuController>(GetOwningPlayer());
	if (MenuController && MenuController->MenuLayout && SettingClass)
	{
		// 本来应该添加到MenuStack, 但是为了显示Menu背景，添加到了ModalStack
		MenuController->MenuLayout->ModalStack->AddWidget(SettingClass);
	}
}

void UMenu::OnQuitButtonClicked()
{
	if (MenuController == nullptr) MenuController = Cast<AMenuController>(GetOwningPlayer());
	UAssetSubsystem* AssetSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UAssetSubsystem>();

	if (MenuController && MenuController->MenuLayout && AssetSubsystem && AssetSubsystem->CommonAsset)
	{
		FConfirmScreenComplete ResultCallback = FConfirmScreenComplete::CreateUObject(this, &ThisClass::Quit);
		MenuController->MenuLayout->ModalStack->AddWidget<UConfirmScreen>(
			AssetSubsystem->CommonAsset->ConfirmScreenClass,
			[ResultCallback](UConfirmScreen& Dialog) {
				Dialog.Setup(LOCTEXT("SureToQuit", "Sure to quit?"), ResultCallback);
			}
		);
	}
}

void UMenu::Quit(EMsgResult MsgResult)
{
	if (MsgResult == EMsgResult::Confirm)
	{
		APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
		UKismetSystemLibrary::QuitGame(this, PlayerController, EQuitPreference::Quit, false);
	}
}

void UMenu::OnEnumerateTitleFilesComplete(bool bWasSuccessful)
{
	if (!bWasSuccessful) return;
	
	if (EOSSubsystem && EOSSubsystem->GetEnumeratedTitleFiles().Contains(TitleFile_Message))
	{
		EOSSubsystem->ReadTitleFile(TitleFile_Message);
	}
}

void UMenu::OnReadTitleFileComplete(bool bWasSuccessful, const FTitleFileContentsRef& FileContents)
{
	if (!bWasSuccessful) return;

	FString JsonString;
	FFileHelper::BufferToString(JsonString, FileContents->GetData(), FileContents->Num());
    
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
	if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
	{
		if (JsonObject->GetStringField(TEXT("FileName")) != TitleFile_Message) return;
		// UE_LOG(LogTemp, Log, TEXT("JsonString %s"), *JsonString);
		
		const FString StartTimeString = JsonObject->GetStringField(TEXT("StartTime"));
		const FString EndTimeString = JsonObject->GetStringField(TEXT("EndTime"));
		const int32 Level = JsonObject->GetIntegerField(TEXT("Level"));

		FString Content;
		if (ULibraryCommon::GetLanguage().Contains(TEXT("zh")))
		{
			Content = JsonObject->GetStringField(TEXT("Content_zh"));
		}
		else
		{
			Content = JsonObject->GetStringField(TEXT("Content_en"));
		}

		if (IsBeijingTimeInRange(StartTimeString, EndTimeString))
		{
			MessageBox->SetVisibility(ESlateVisibility::Visible);
			Message->SetText(FText::FromString(Content));
			
			FColor Color = C_WHITE;
			if (Level == 1)
			{
				Color = C_WHITE;
			}
			else if (Level == 2)
			{
				Color = C_YELLOW;
			}
			else if (Level == 3)
			{
				Color = C_RED;
			}
			Message->SetColorAndOpacity(Color);
		}
		else
		{
			Message->SetText(FText::FromString(TEXT("")));
			MessageBox->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

bool UMenu::IsBeijingTimeInRange(const FString& StartStr, const FString& EndStr)
{
	FDateTime StartTime, EndTime;
	if (!FDateTime::Parse(StartStr, StartTime) || !FDateTime::Parse(EndStr, EndTime))
	{
		return false;
	}

	FDateTime UtcNow = FDateTime::UtcNow();
	FTimespan Offset = FTimespan(8, 0, 0);
	FDateTime BeijingNow = UtcNow + Offset;

	return BeijingNow >= StartTime && BeijingNow <= EndTime;
}

#undef LOCTEXT_NAMESPACE
