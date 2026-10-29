#include "TabGame.h"

#include "CommonButtonBase.h"
#include "CommonHierarchicalScrollBox.h"
#include "Engine/GameInstance.h"
#include "MutateArena/System/Storage/DefaultConfig.h"
#include "MutateArena/System/Storage/SaveGameSetting.h"
#include "MutateArena/System/Storage/StorageSubsystem.h"
#include "Input/CommonUIInputTypes.h"
#include "MutateArena/Utils/LibraryCommon.h"
#include "MutateArena/UI/Common/CommonComboBox2.h"
#include "MutateArena/UI/Common/ComboBoxItem.h"
#include "MutateArena/System/AssetSubsystem.h"
#include "MutateArena/Assets/Data/CommonAsset.h"
#include "Internationalization/StringTable.h"

void UTabGame::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 绑定提示信息切换菜单
	auto Tabs = ULibraryCommon::GetAllChildrenOfClass<UCommonButtonBase>(SettingLeft);
	for (int i = 0; i < Tabs.Num(); ++i)
	{
		Tabs[i]->OnHovered().AddUObject(this, &ThisClass::OnTabButtonHovered, i);
	}

	LanguageComboBox->OnGenerateItemWidget.BindDynamic(this, &ThisClass::GenerateComboBoxWidget);
	LanguageComboBox->OnGenerateContentWidget.BindDynamic(this, &ThisClass::GenerateComboBoxWidget);
	ObfuscateNameComboBox->OnGenerateItemWidget.BindDynamic(this, &ThisClass::GenerateComboBoxWidget);
	ObfuscateNameComboBox->OnGenerateContentWidget.BindDynamic(this, &ThisClass::GenerateComboBoxWidget);
	ObfuscateTextChatComboBox->OnGenerateItemWidget.BindDynamic(this, &ThisClass::GenerateComboBoxWidget);
	ObfuscateTextChatComboBox->OnGenerateContentWidget.BindDynamic(this, &ThisClass::GenerateComboBoxWidget);

	LanguageComboBox->AddOption(Settings::EN);
	LanguageComboBox->AddOption(Settings::ZH);
	ObfuscateNameComboBox->AddOption(Settings::On);
	ObfuscateNameComboBox->AddOption(Settings::Off);
	ObfuscateTextChatComboBox->AddOption(Settings::On);
	ObfuscateTextChatComboBox->AddOption(Settings::Off);
	
	SetUISavedValue();
	
	LanguageComboBox->OnSelectionChanged.AddUniqueDynamic(this, &ThisClass::OnLanguageChanged);
	ObfuscateNameComboBox->OnSelectionChanged.AddUniqueDynamic(this, &ThisClass::OnObfuscateNameChanged);
	ObfuscateTextChatComboBox->OnSelectionChanged.AddUniqueDynamic(this, &ThisClass::OnObfuscateTextChatChanged);

	SetDefaultHandle = RegisterUIActionBinding(FBindUIActionArgs(SetDefaultData, true, FSimpleDelegate::CreateUObject(this, &ThisClass::SetDefault)));
}

void UTabGame::NativeConstruct()
{
	Super::NativeConstruct();
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

UWidget* UTabGame::GenerateComboBoxWidget(FName ItemName)
{
	UAssetSubsystem* AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
	if (AssetSubsystem == nullptr || AssetSubsystem->CommonAsset == nullptr) return nullptr;
	
	UComboBoxItem* ItemWidget = CreateWidget<UComboBoxItem>(this, AssetSubsystem->CommonAsset->ComboBoxItemClass);
	if (!ItemWidget) return nullptr;
	
	FText DisplayText;
	if (AssetSubsystem->CommonAsset->ST_Common)
	{
		FText TableText = FText::FromStringTable(AssetSubsystem->CommonAsset->ST_Common->GetStringTableId(), ItemName.ToString());
		if (!TableText.IsEmpty())
		{
			DisplayText = TableText;
		}
		else
		{
			DisplayText = FText::FromName(ItemName);
		}
	}
	else
	{
		DisplayText = FText::FromName(ItemName);
	}

	ItemWidget->ItemText->SetText(DisplayText);

	return ItemWidget;
}

void UTabGame::SetUISavedValue()
{
	if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
	if (StorageSubsystem && StorageSubsystem->CacheSetting)
	{
		// PIE在游戏中修改语言为zh时，编辑器语言会变成英文，可能跟语言回退有关（编辑器支持zh-Hans/zh-Hant，无法回退到zh）
		// UE_LOG(LogTemp, Warning, TEXT("Language: %s"), *StorageSubsystem->CacheSetting->Language.ToString());
		
		FName Language = StorageSubsystem->CacheSetting->Language;
		// TODO 存在漏洞 可能是 zh-Hant/zh-Hans 不等于 zh 最终下拉框显示了en
		if (Language != Settings::EN || Language != Settings::ZH)
		{
			Language = Settings::EN;
		}
		LanguageComboBox->SetSelectedOption(Language);
		
		ObfuscateNameComboBox->SetSelectedOption(StorageSubsystem->CacheSetting->bObfuscateName ? Settings::On : Settings::Off);
		ObfuscateTextChatComboBox->SetSelectedOption(StorageSubsystem->CacheSetting->bObfuscateTextChat ? Settings::On : Settings::Off);
	}
}

void UTabGame::OnLanguageChanged(FName SelectedItem, ESelectInfo::Type SelectionType)
{
	FInternationalization::Get().SetCurrentCulture(SelectedItem.ToString());

	if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
	if (StorageSubsystem && StorageSubsystem->CacheSetting)
	{
		StorageSubsystem->CacheSetting->Language = SelectedItem;
		StorageSubsystem->SaveSetting();
	}
}

void UTabGame::OnObfuscateNameChanged(FName SelectedItem, ESelectInfo::Type SelectionType)
{
	if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
	if (StorageSubsystem && StorageSubsystem->CacheSetting)
	{
		StorageSubsystem->CacheSetting->bObfuscateName = SelectedItem == Settings::On;
		StorageSubsystem->SaveSetting();
	}
}

void UTabGame::OnObfuscateTextChatChanged(FName SelectedItem, ESelectInfo::Type SelectionType)
{
	if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
	if (StorageSubsystem && StorageSubsystem->CacheSetting)
	{
		StorageSubsystem->CacheSetting->bObfuscateTextChat = SelectedItem == Settings::On;
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
			FName Language = DefaultConfig->Language;
			// TODO 存在漏洞 可能是 zh-Hant/zh-Hans 不等于 zh 最终下拉框显示了en
			if (Language != Settings::EN || Language != Settings::ZH)
			{
				Language = Settings::EN;
			}
			LanguageComboBox->SetSelectedOption(Language);
			ObfuscateNameComboBox->SetSelectedOption(DefaultConfig->bObfuscateName ? Settings::On : Settings::Off);
			ObfuscateTextChatComboBox->SetSelectedOption(DefaultConfig->bObfuscateTextChat ? Settings::On : Settings::Off);

			StorageSubsystem->CacheSetting->Language = DefaultConfig->Language;
			StorageSubsystem->CacheSetting->bObfuscateName = DefaultConfig->bObfuscateName;
			StorageSubsystem->CacheSetting->bObfuscateTextChat = DefaultConfig->bObfuscateTextChat;
			StorageSubsystem->SaveSetting();
		}
	}
}
