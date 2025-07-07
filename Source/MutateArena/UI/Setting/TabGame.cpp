#include "TabGame.h"

#include "CommonButtonBase.h"
#include "CommonHierarchicalScrollBox.h"
#include "MutateArena/System/Storage/DefaultConfig.h"
#include "MutateArena/System/Storage/SaveGameSetting.h"
#include "MutateArena/System/Storage/StorageSubsystem.h"
#include "Components/ComboBoxString.h"
#include "Input/CommonUIInputTypes.h"

void UTabGame::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 绑定提示信息切换菜单
	TArray<UWidget*> Tabs = SettingLeft->GetAllChildren();
	for (int i = 0; i < Tabs.Num(); ++i)
	{
		if (UCommonButtonBase* TabButton = Cast<UCommonButtonBase>(Tabs[i]))
		{
			TabButton->OnHovered().AddUObject(this, &ThisClass::OnTabButtonHovered, i);
		}
	}

	LanguageComboBox->AddOption("en");
	LanguageComboBox->AddOption("zh");

	ObfuscatePlayerNameComboBox->AddOption("on");
	ObfuscatePlayerNameComboBox->AddOption("off");
	ObfuscateTextChatComboBox->AddOption("on");
	ObfuscateTextChatComboBox->AddOption("off");
	
	SetUISavedValue();

	LanguageComboBox->OnSelectionChanged.AddUniqueDynamic(this, &ThisClass::OnLanguageChanged);
	ObfuscatePlayerNameComboBox->OnSelectionChanged.AddUniqueDynamic(this, &ThisClass::OnObfuscatePlayerNameChanged);
	ObfuscateTextChatComboBox->OnSelectionChanged.AddUniqueDynamic(this, &ThisClass::OnObfuscateTextChatChanged);

	SetDefaultHandle = RegisterUIActionBinding(FBindUIActionArgs(SetDefaultData, true, FSimpleDelegate::CreateUObject(this, &ThisClass::SetDefault)));
}

UWidget* UTabGame::NativeGetDesiredFocusTarget() const
{
	return LanguageComboBox;
}

void UTabGame::OnTabButtonHovered(int Index)
{
	TArray<UWidget*> Contents = SettingRight->GetAllChildren();
	for (int i = 0; i < Contents.Num(); ++i)
	{
		Contents[i]->SetVisibility(i == Index ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UTabGame::SetUISavedValue()
{
	if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
	if (StorageSubsystem && StorageSubsystem->CacheSetting)
	{
		// 在游戏中修改语言为zh时，编辑器语言会变成英文，可能跟语言回退有关（编辑器支持zh-Hans/zh-Hant，无法回退到zh）
		if (GetWorld()->WorldType != EWorldType::PIE)
		{
			LanguageComboBox->SetSelectedOption(StorageSubsystem->CacheSetting->Language);
		}

		ObfuscatePlayerNameComboBox->SetSelectedOption(StorageSubsystem->CacheSetting->ObfuscatePlayerName ? "on" : "off");
		ObfuscateTextChatComboBox->SetSelectedOption(StorageSubsystem->CacheSetting->ObfuscateTextChat ? "on" : "off");
	}
}

void UTabGame::OnLanguageChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	FInternationalization::Get().SetCurrentCulture(SelectedItem);

	if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
	if (StorageSubsystem && StorageSubsystem->CacheSetting)
	{
		StorageSubsystem->CacheSetting->Language = SelectedItem;
		StorageSubsystem->SaveSetting();
	}
}

void UTabGame::OnObfuscatePlayerNameChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
	if (StorageSubsystem && StorageSubsystem->CacheSetting)
	{
		StorageSubsystem->CacheSetting->ObfuscatePlayerName = SelectedItem == "on";
		StorageSubsystem->SaveSetting();
	}
}

void UTabGame::OnObfuscateTextChatChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
	if (StorageSubsystem && StorageSubsystem->CacheSetting)
	{
		StorageSubsystem->CacheSetting->ObfuscateTextChat = SelectedItem == "on";
		StorageSubsystem->SaveSetting();
	}
}

void UTabGame::SetDefault()
{
	if (const UDefaultConfig* DefaultConfig = GetDefault<UDefaultConfig>())
	{
		if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
		if (StorageSubsystem && StorageSubsystem->CacheSetting)
		{
			LanguageComboBox->SetSelectedOption(DefaultConfig->Language);
			ObfuscatePlayerNameComboBox->SetSelectedOption(DefaultConfig->ObfuscatePlayerName ? "on" : "off");
			ObfuscateTextChatComboBox->SetSelectedOption(DefaultConfig->ObfuscateTextChat ? "on" : "off");

			StorageSubsystem->CacheSetting->Language = DefaultConfig->Language;
			StorageSubsystem->CacheSetting->ObfuscatePlayerName = DefaultConfig->ObfuscatePlayerName;
			StorageSubsystem->CacheSetting->ObfuscateTextChat = DefaultConfig->ObfuscateTextChat;
			StorageSubsystem->SaveSetting();
		}
	}
}
