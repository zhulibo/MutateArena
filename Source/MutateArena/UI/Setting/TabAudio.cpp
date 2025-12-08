#include "TabAudio.h"
#include "AnalogSlider.h"
#include "CommonButtonBase.h"
#include "CommonHierarchicalScrollBox.h"
#include "CommonTextBlock.h"
#include "MutateArena/System/Storage/SaveGameSetting.h"
#include "MutateArena/System/Storage/DefaultConfig.h"
#include "MutateArena/System/Storage/StorageSubsystem.h"
#include "Input/CommonUIInputTypes.h"
#include "MutateArena/Utils/LibraryCommon.h"

void UTabAudio::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	// 绑定提示信息切换菜单
	auto Tabs = ULibraryCommon::GetAllChildrenOfClass<UCommonButtonBase>(SettingLeft);
	for (int i = 0; i < Tabs.Num(); ++i)
	{
		Tabs[i]->OnHovered().AddUObject(this, &ThisClass::OnTabButtonHovered, i);
	}

	SetUISavedValue();

	MasterVolumeAnalogSlider->OnValueChanged.AddUniqueDynamic(this, &ThisClass::OnMasterVolumeChanged);
	EffectsVolumeAnalogSlider->OnValueChanged.AddUniqueDynamic(this, &ThisClass::OnEffectsVolumeChanged);
	MusicVolumeAnalogSlider->OnValueChanged.AddUniqueDynamic(this, &ThisClass::OnMusicVolumeChanged);
	DialogueVolumeAnalogSlider->OnValueChanged.AddUniqueDynamic(this, &ThisClass::OnDialogueVolumeChanged);

	SetDefaultHandle = RegisterUIActionBinding(FBindUIActionArgs(SetDefaultData, true, FSimpleDelegate::CreateUObject(this, &ThisClass::SetDefault)));
}

void UTabAudio::NativeConstruct()
{
	Super::NativeConstruct();

	// 默认隐藏所有提示信息
	TArray<UWidget*> Contents = SettingRight->GetAllChildren();
	for (int i = 0; i < Contents.Num(); ++i)
	{
		Contents[i]->SetVisibility(ESlateVisibility::Collapsed);
	}
}

UWidget* UTabAudio::NativeGetDesiredFocusTarget() const
{
	return MasterVolumeAnalogSlider;
}

void UTabAudio::OnTabButtonHovered(int Index)
{
	TArray<UWidget*> Contents = SettingRight->GetAllChildren();
	for (int i = 0; i < Contents.Num(); ++i)
	{
		Contents[i]->SetVisibility(i == Index ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UTabAudio::SetUISavedValue()
{
	if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
	if (StorageSubsystem && StorageSubsystem->CacheSetting)
	{
		MasterVolumeAnalogSlider->SetValue(StorageSubsystem->CacheSetting->MasterVolume * 100.f);
		MasterVolume->SetText(FText::AsNumber(StorageSubsystem->CacheSetting->MasterVolume * 100.f));
		EffectsVolumeAnalogSlider->SetValue(StorageSubsystem->CacheSetting->EffectsVolume * 100.f);
		EffectsVolume->SetText(FText::AsNumber(StorageSubsystem->CacheSetting->EffectsVolume * 100.f));
		MusicVolumeAnalogSlider->SetValue(StorageSubsystem->CacheSetting->MusicVolume * 100.f);
		MusicVolume->SetText(FText::AsNumber(StorageSubsystem->CacheSetting->MusicVolume * 100.f));
		DialogueVolumeAnalogSlider->SetValue(StorageSubsystem->CacheSetting->DialogueVolume * 100.f);
		DialogueVolume->SetText(FText::AsNumber(StorageSubsystem->CacheSetting->DialogueVolume * 100.f));
	}
}

void UTabAudio::OnMasterVolumeChanged(float Value)
{
	Value = FMath::RoundToFloat(Value);
	MasterVolume->SetText(FText::AsNumber(Value));
	if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
	if (StorageSubsystem && StorageSubsystem->CacheSetting)
	{
		StorageSubsystem->SetAudio(Value / 100.f, ESoundClassType::Master);
		StorageSubsystem->CacheSetting->MasterVolume = Value / 100.f;
		StorageSubsystem->SaveSetting();
	}
}

void UTabAudio::OnEffectsVolumeChanged(float Value)
{
	Value = FMath::RoundToFloat(Value);
	EffectsVolume->SetText(FText::AsNumber(Value));
	if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
	if (StorageSubsystem && StorageSubsystem->CacheSetting)
	{
		StorageSubsystem->SetAudio(Value / 100.f, ESoundClassType::Effects);
		StorageSubsystem->CacheSetting->EffectsVolume = Value / 100.f;
		StorageSubsystem->SaveSetting();
	}
}

void UTabAudio::OnMusicVolumeChanged(float Value)
{
	Value = FMath::RoundToFloat(Value);
	MusicVolume->SetText(FText::AsNumber(Value));
	if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
	if (StorageSubsystem && StorageSubsystem->CacheSetting)
	{
		StorageSubsystem->SetAudio(Value / 100.f, ESoundClassType::Music);
		StorageSubsystem->CacheSetting->MusicVolume = Value / 100.f;
		StorageSubsystem->SaveSetting();
	}
}

void UTabAudio::OnDialogueVolumeChanged(float Value)
{
	Value = FMath::RoundToFloat(Value);
	DialogueVolume->SetText(FText::AsNumber(Value));
	if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
	if (StorageSubsystem && StorageSubsystem->CacheSetting)
	{
		StorageSubsystem->SetAudio(Value / 100.f, ESoundClassType::Dialogue);
		StorageSubsystem->CacheSetting->DialogueVolume = Value / 100.f;
		StorageSubsystem->SaveSetting();
	}
}

void UTabAudio::SetDefault()
{
	if (const UDefaultConfig* DefaultConfig = GetDefault<UDefaultConfig>())
	{
		if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
		if (StorageSubsystem && StorageSubsystem->CacheSetting)
		{
			MasterVolumeAnalogSlider->SetValue(DefaultConfig->MasterVolume * 100.f);
			MasterVolume->SetText(FText::AsNumber(DefaultConfig->MasterVolume * 100.f));

			EffectsVolumeAnalogSlider->SetValue(DefaultConfig->EffectsVolume * 100.f);
			EffectsVolume->SetText(FText::AsNumber(DefaultConfig->EffectsVolume * 100.f));

			MusicVolumeAnalogSlider->SetValue(DefaultConfig->MusicVolume * 100.f);
			MusicVolume->SetText(FText::AsNumber(DefaultConfig->MusicVolume * 100.f));

			DialogueVolumeAnalogSlider->SetValue(DefaultConfig->DialogueVolume * 100.f);
			DialogueVolume->SetText(FText::AsNumber(DefaultConfig->DialogueVolume * 100.f));

			StorageSubsystem->SetAudio(DefaultConfig->MasterVolume, ESoundClassType::Master);
			StorageSubsystem->SetAudio(DefaultConfig->EffectsVolume, ESoundClassType::Effects);
			StorageSubsystem->SetAudio(DefaultConfig->MusicVolume, ESoundClassType::Music);
			StorageSubsystem->SetAudio(DefaultConfig->DialogueVolume, ESoundClassType::Dialogue);

			StorageSubsystem->CacheSetting->MasterVolume = DefaultConfig->MasterVolume;
			StorageSubsystem->CacheSetting->EffectsVolume = DefaultConfig->EffectsVolume;
			StorageSubsystem->CacheSetting->MusicVolume = DefaultConfig->MusicVolume;
			StorageSubsystem->CacheSetting->DialogueVolume = DefaultConfig->DialogueVolume;

			StorageSubsystem->SaveSetting();
		}
	}
}
