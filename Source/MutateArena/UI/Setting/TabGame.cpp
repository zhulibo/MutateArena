#include "TabGame.h"

#include "CommonButtonBase.h"
#include "CommonHierarchicalScrollBox.h"
#include "MutateArena/System/Storage/DefaultConfig.h"
#include "MutateArena/System/Storage/SaveGameSetting.h"
#include "MutateArena/System/Storage/StorageSubsystem.h"
#include "Components/ComboBoxString.h"
#include "Input/CommonUIInputTypes.h"
#include "MutateArena/Utils/LibraryCommon.h"

void UTabGame::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 绑定提示信息切换菜单
	auto Tabs = ULibraryCommon::GetAllChildrenOfClass<UCommonButtonBase>(SettingLeft);
	for (int i = 0; i < Tabs.Num(); ++i)
	{
		Tabs[i]->OnHovered().AddUObject(this, &ThisClass::OnTabButtonHovered, i);
	}

	LanguageComboBox->AddOption("en");
	LanguageComboBox->AddOption("zh");
	HideSkinsComboBox->AddOption("on");
	HideSkinsComboBox->AddOption("off");
	ObfuscateNameComboBox->AddOption("on");
	ObfuscateNameComboBox->AddOption("off");
	ObfuscateTextChatComboBox->AddOption("on");
	ObfuscateTextChatComboBox->AddOption("off");
	
	SetUISavedValue();

	LanguageComboBox->OnSelectionChanged.AddUniqueDynamic(this, &ThisClass::OnLanguageChanged);
	HideSkinsComboBox->OnSelectionChanged.AddUniqueDynamic(this, &ThisClass::OnHideSkinsChanged);
	ObfuscateNameComboBox->OnSelectionChanged.AddUniqueDynamic(this, &ThisClass::OnObfuscateNameChanged);
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

		HideSkinsComboBox->SetSelectedOption(StorageSubsystem->CacheSetting->bHideSkins ? "on" : "off");
		ObfuscateNameComboBox->SetSelectedOption(StorageSubsystem->CacheSetting->bObfuscateName ? "on" : "off");
		ObfuscateTextChatComboBox->SetSelectedOption(StorageSubsystem->CacheSetting->bObfuscateTextChat ? "on" : "off");
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

void UTabGame::OnHideSkinsChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
	if (StorageSubsystem && StorageSubsystem->CacheSetting)
	{
		StorageSubsystem->CacheSetting->bHideSkins = SelectedItem == "on";
		StorageSubsystem->SaveSetting();
	}
}

void UTabGame::OnObfuscateNameChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
	if (StorageSubsystem && StorageSubsystem->CacheSetting)
	{
		StorageSubsystem->CacheSetting->bObfuscateName = SelectedItem == "on";
		StorageSubsystem->SaveSetting();
	}
}

void UTabGame::OnObfuscateTextChatChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
	if (StorageSubsystem && StorageSubsystem->CacheSetting)
	{
		StorageSubsystem->CacheSetting->bObfuscateTextChat = SelectedItem == "on";
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
			HideSkinsComboBox->SetSelectedOption(DefaultConfig->bHideSkins ? "on" : "off");
			ObfuscateNameComboBox->SetSelectedOption(DefaultConfig->bObfuscateName ? "on" : "off");
			ObfuscateTextChatComboBox->SetSelectedOption(DefaultConfig->bObfuscateTextChat ? "on" : "off");

			StorageSubsystem->CacheSetting->Language = DefaultConfig->Language;
			StorageSubsystem->CacheSetting->bHideSkins = DefaultConfig->bHideSkins;
			StorageSubsystem->CacheSetting->bObfuscateName = DefaultConfig->bObfuscateName;
			StorageSubsystem->CacheSetting->bObfuscateTextChat = DefaultConfig->bObfuscateTextChat;
			StorageSubsystem->SaveSetting();
		}
	}
}
