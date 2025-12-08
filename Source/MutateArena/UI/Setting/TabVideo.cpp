#include "TabVideo.h"
#include "AnalogSlider.h"
#include "CommonButtonBase.h"
#include "CommonHierarchicalScrollBox.h"
#include "CommonTextBlock.h"
#include "MutateArena/System/Storage/SaveGameSetting.h"
#include "MutateArena/System/Storage/DefaultConfig.h"
#include "MutateArena/System/Storage/StorageSubsystem.h"
#include "Components/ComboBoxString.h"
#include "GameFramework/GameUserSettings.h"
#include "Input/CommonUIInputTypes.h"
#include "MutateArena/Utils/LibraryCommon.h"

void UTabVideo::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 绑定提示信息切换菜单
	auto Tabs = ULibraryCommon::GetAllChildrenOfClass<UCommonButtonBase>(SettingLeft);
	for (int i = 0; i < Tabs.Num(); ++i)
	{
		Tabs[i]->OnHovered().AddUObject(this, &ThisClass::OnTabButtonHovered, i);
	}

	// 设置窗口模式下拉项
	WindowModeComboBox->AddOption("Fullscreen");
	WindowModeComboBox->AddOption("WindowedFullscreen");
	WindowModeComboBox->AddOption("Windowed");

	// 设置分辨率下拉项
	// 添加默认分辨率
	FIntPoint DefaultResolution = FIntPoint(1280, 720);
	if (const UDefaultConfig* DefaultConfig = GetDefault<UDefaultConfig>())
	{
		DefaultResolution = DefaultConfig->ScreenResolution;
	}
	Resolutions.Add(FIntPoint(DefaultResolution.X, DefaultResolution.Y));
	ScreenResolutionComboBox->AddOption(FString::FromInt(DefaultResolution.X) + TEXT("x") + FString::FromInt(DefaultResolution.Y));
	// 添加显示器支持的分辨率（需大于默认分辨率）
	FScreenResolutionArray TempResolutions;
	RHIGetAvailableResolutions(TempResolutions, true);
	for (const FScreenResolutionRHI& Resolution : TempResolutions)
	{
		if (Resolution.Width > static_cast<uint32>(DefaultResolution.X) && Resolution.Height > static_cast<uint32>(DefaultResolution.Y))
		{
			Resolutions.Add(FIntPoint(Resolution.Width, Resolution.Height));
			ScreenResolutionComboBox->AddOption(FString::FromInt(Resolution.Width) + TEXT("x") + FString::FromInt(Resolution.Height));
		}
	}

	// 设置保存值
	SetUISavedValue();

	// 绑定值变化回调
	BrightnessAnalogSlider->OnValueChanged.AddUniqueDynamic(this, &ThisClass::OnBrightnessChanged);
	WindowModeComboBox->OnSelectionChanged.AddUniqueDynamic(this, &ThisClass::OnWindowModeChanged);
	ScreenResolutionComboBox->OnSelectionChanged.AddUniqueDynamic(this, &ThisClass::OnScreenResolutionChanged);

	SetDefaultHandle = RegisterUIActionBinding(FBindUIActionArgs(SetDefaultData, true, FSimpleDelegate::CreateUObject(this, &ThisClass::SetDefault)));
}

void UTabVideo::NativeConstruct()
{
	Super::NativeConstruct();

	// 默认隐藏所有提示信息
	TArray<UWidget*> Contents = SettingRight->GetAllChildren();
	for (int i = 0; i < Contents.Num(); ++i)
	{
		Contents[i]->SetVisibility(ESlateVisibility::Collapsed);
	}
}

UWidget* UTabVideo::NativeGetDesiredFocusTarget() const
{
	return BrightnessAnalogSlider;
}

void UTabVideo::OnTabButtonHovered(int Index)
{
	TArray<UWidget*> Contents = SettingRight->GetAllChildren();
	for (int i = 0; i < Contents.Num(); ++i)
	{
		Contents[i]->SetVisibility(i == Index ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UTabVideo::SetUISavedValue()
{
	if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
	if (StorageSubsystem && StorageSubsystem->CacheSetting)
	{
		BrightnessAnalogSlider->SetValue(StorageSubsystem->CacheSetting->Brightness);
		Brightness->SetText(FText::AsNumber(StorageSubsystem->CacheSetting->Brightness));
		
		if (GameUserSettings == nullptr) GameUserSettings = GetMutableDefault<UGameUserSettings>();
		if (GameUserSettings)
		{
			switch (GameUserSettings->GetFullscreenMode())
			{
			case EWindowMode::Fullscreen:
				WindowModeComboBox->SetSelectedOption("Fullscreen");
				break;
			case EWindowMode::WindowedFullscreen:
				WindowModeComboBox->SetSelectedOption("WindowedFullscreen");
				break;
			case EWindowMode::Windowed:
				WindowModeComboBox->SetSelectedOption("Windowed");
				break;
			}

			FIntPoint CurResolution = GameUserSettings->GetScreenResolution();
			FString CurResolutionStr = FString::FromInt(CurResolution.X) + TEXT("x") + FString::FromInt(CurResolution.Y);
			if (Resolutions.Contains(CurResolution))
			{
				ScreenResolutionComboBox->SetSelectedOption(CurResolutionStr);
			}
		}
	}
}

void UTabVideo::OnBrightnessChanged(float Value)
{
	Value = FMath::RoundToFloat(Value * 10) / 10;

	Brightness->SetText(FText::AsNumber(Value));

	GEngine->DisplayGamma = Value;

	if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
	if (StorageSubsystem && StorageSubsystem->CacheSetting)
	{
		StorageSubsystem->CacheSetting->Brightness = Value;
		StorageSubsystem->SaveSetting();
	}
}

void UTabVideo::OnWindowModeChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	EWindowMode::Type WindowMode = EWindowMode::WindowedFullscreen;

	if (SelectedItem == TEXT("Fullscreen"))
	{
		WindowMode = EWindowMode::Fullscreen;
	}
	else if (SelectedItem == TEXT("WindowedFullscreen"))
	{
		WindowMode = EWindowMode::WindowedFullscreen;
	}
	else if (SelectedItem == TEXT("Windowed"))
	{
		WindowMode = EWindowMode::Windowed;
	}

	if (GameUserSettings == nullptr) GameUserSettings = GetMutableDefault<UGameUserSettings>();
	if (GameUserSettings)
	{
		GameUserSettings->SetFullscreenMode(WindowMode);
		GameUserSettings->ApplyResolutionSettings(false);
		GameUserSettings->SaveSettings();
	}

	if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
	if (StorageSubsystem && StorageSubsystem->CacheSetting)
	{
		StorageSubsystem->CacheSetting->WindowMode = WindowMode;
		StorageSubsystem->SaveSetting();
	}
}

void UTabVideo::OnScreenResolutionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	FIntPoint Resolution = Resolutions[ScreenResolutionComboBox->FindOptionIndex(SelectedItem)];

	if (GameUserSettings == nullptr) GameUserSettings = GetMutableDefault<UGameUserSettings>();
	if (GameUserSettings)
	{
		GameUserSettings->SetScreenResolution(Resolution);
		GameUserSettings->ApplyResolutionSettings(false);
		GameUserSettings->SaveSettings();
	}

	if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
	if (StorageSubsystem && StorageSubsystem->CacheSetting)
	{
		StorageSubsystem->CacheSetting->ScreenResolution = Resolution;
		StorageSubsystem->SaveSetting();
	}
}

void UTabVideo::SetDefault()
{
	if (const UDefaultConfig* DefaultConfig = GetDefault<UDefaultConfig>())
	{
		if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
		if (StorageSubsystem && StorageSubsystem->CacheSetting)
		{
			BrightnessAnalogSlider->SetValue(DefaultConfig->Brightness);
			Brightness->SetText(FText::AsNumber(DefaultConfig->Brightness));
			
			switch (DefaultConfig->WindowMode)
			{
			case EWindowMode::Fullscreen:
				WindowModeComboBox->SetSelectedOption("Fullscreen");
				break;
			case EWindowMode::WindowedFullscreen:
				WindowModeComboBox->SetSelectedOption("WindowedFullscreen");
				break;
			case EWindowMode::Windowed:
				WindowModeComboBox->SetSelectedOption("Windowed");
				break;
			}

			ScreenResolutionComboBox->SetSelectedIndex(ScreenResolutionComboBox->GetOptionCount() - 1);

			if (GameUserSettings == nullptr) GameUserSettings = GetMutableDefault<UGameUserSettings>();
			if (GameUserSettings)
			{
				GameUserSettings->SetFullscreenMode(DefaultConfig->WindowMode);
				GameUserSettings->SetScreenResolution(DefaultConfig->ScreenResolution);
				GameUserSettings->ApplyResolutionSettings(false);
				GameUserSettings->SaveSettings();
			}

			StorageSubsystem->CacheSetting->Brightness = DefaultConfig->Brightness;
			StorageSubsystem->CacheSetting->WindowMode = DefaultConfig->WindowMode;
			StorageSubsystem->CacheSetting->ScreenResolution = DefaultConfig->ScreenResolution;
			StorageSubsystem->SaveSetting();
		}
	}
}
