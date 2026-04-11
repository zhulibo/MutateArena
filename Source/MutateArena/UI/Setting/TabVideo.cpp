#include "TabVideo.h"
#include "AnalogSlider.h"
#include "CommonButtonBase.h"
#include "CommonHierarchicalScrollBox.h"
#include "CommonTextBlock.h"
#include "MutateArena/System/Storage/SaveGameSetting.h"
#include "MutateArena/System/Storage/DefaultConfig.h"
#include "MutateArena/System/Storage/StorageSubsystem.h"
#include "GameFramework/GameUserSettings.h"
#include "Input/CommonUIInputTypes.h"
#include "MutateArena/Utils/LibraryCommon.h"
#include "MutateArena/UI/Common/CommonComboBox2.h"
#include "MutateArena/UI/Common/ComboBoxItem.h"
#include "MutateArena/System/AssetSubsystem.h"
#include "MutateArena/Assets/Data/CommonAsset.h"
#include "Internationalization/StringTable.h"

void UTabVideo::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 绑定提示信息切换菜单
	auto Tabs = ULibraryCommon::GetAllChildrenOfClass<UCommonButtonBase>(SettingLeft);
	for (int i = 0; i < Tabs.Num(); ++i)
	{
		Tabs[i]->OnHovered().AddUObject(this, &ThisClass::OnTabButtonHovered, i);
	}

	// 绑定 ComboBox 生成事件
	WindowModeComboBox->OnGenerateItemWidget.BindDynamic(this, &ThisClass::GenerateComboBoxWidget);
	WindowModeComboBox->OnGenerateContentWidget.BindDynamic(this, &ThisClass::GenerateComboBoxWidget);
	
	// 分辨率框绑定无需翻译的生成事件
	ScreenResolutionComboBox->OnGenerateItemWidget.BindDynamic(this, &ThisClass::GenerateComboBoxWidget_NoTranslation);
	ScreenResolutionComboBox->OnGenerateContentWidget.BindDynamic(this, &ThisClass::GenerateComboBoxWidget_NoTranslation);

	// 设置窗口模式下拉项
	WindowModeComboBox->AddOption(FULLSCREEN);
	WindowModeComboBox->AddOption(WIDOWED_FULLSCREEN);
	WindowModeComboBox->AddOption(WIDOWED);

	// 设置分辨率下拉项
	// 添加默认分辨率
	FIntPoint DefaultResolution = FIntPoint(1280, 720);
	if (const UDefaultConfig* DefaultConfig = GetDefault<UDefaultConfig>())
	{
		DefaultResolution = DefaultConfig->ScreenResolution;
	}
	
	FName DefaultResName = FormatResolutionName(DefaultResolution.X, DefaultResolution.Y);
	Resolutions.Add(FResolutionOption(DefaultResName, DefaultResolution));
	ScreenResolutionComboBox->AddOption(DefaultResName);
	
	// 添加显示器支持的分辨率（需大于默认分辨率）
	FScreenResolutionArray TempResolutions;
	RHIGetAvailableResolutions(TempResolutions, true);
	for (const FScreenResolutionRHI& Resolution : TempResolutions)
	{
		if (Resolution.Width > static_cast<uint32>(DefaultResolution.X) && Resolution.Height > static_cast<uint32>(DefaultResolution.Y))
		{
			FName ResName = FormatResolutionName(Resolution.Width, Resolution.Height);
			Resolutions.Add(FResolutionOption(ResName, FIntPoint(Resolution.Width, Resolution.Height)));
			ScreenResolutionComboBox->AddOption(ResName);
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

// 包含多语言查询逻辑的生成函数
UWidget* UTabVideo::GenerateComboBoxWidget(FName ItemName)
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

// 专用于分辨率等数字字段的生成函数（跳过 StringTable 检索）
UWidget* UTabVideo::GenerateComboBoxWidget_NoTranslation(FName ItemName)
{
	UAssetSubsystem* AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
	if (AssetSubsystem == nullptr || AssetSubsystem->CommonAsset == nullptr) return nullptr;
	
	UComboBoxItem* ItemWidget = CreateWidget<UComboBoxItem>(this, AssetSubsystem->CommonAsset->ComboBoxItemClass);
	if (!ItemWidget) return nullptr;

	// 直接使用 FName 作为显示文本，不走翻译查询
	ItemWidget->ItemText->SetText(FText::FromName(ItemName));

	return ItemWidget;
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
				WindowModeComboBox->SetSelectedOption(FULLSCREEN);
				break;
			case EWindowMode::WindowedFullscreen:
				WindowModeComboBox->SetSelectedOption(WIDOWED_FULLSCREEN);
				break;
			case EWindowMode::Windowed:
				WindowModeComboBox->SetSelectedOption(WIDOWED);
				break;
			}

			FIntPoint CurResolution = GameUserSettings->GetScreenResolution();
			FName CurResName = FormatResolutionName(CurResolution.X, CurResolution.Y);
			
			if (Resolutions.ContainsByPredicate([&](const FResolutionOption& Opt) { return Opt.Name == CurResName; }))
			{
				ScreenResolutionComboBox->SetSelectedOption(CurResName);
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

void UTabVideo::OnWindowModeChanged(FName SelectedItem, ESelectInfo::Type SelectionType)
{
	EWindowMode::Type WindowMode = EWindowMode::WindowedFullscreen;

	if (SelectedItem == FULLSCREEN)
	{
		WindowMode = EWindowMode::Fullscreen;
	}
	else if (SelectedItem == WIDOWED_FULLSCREEN)
	{
		WindowMode = EWindowMode::WindowedFullscreen;
	}
	else if (SelectedItem == WIDOWED)
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

FName UTabVideo::FormatResolutionName(int32 Width, int32 Height) const
{
	return FName(*FString::Printf(TEXT("%d x %d"), Width, Height));
}

void UTabVideo::OnScreenResolutionChanged(FName SelectedItem, ESelectInfo::Type SelectionType)
{
	if (const FResolutionOption* FoundOption = Resolutions.FindByPredicate([&](const FResolutionOption& Opt) { return Opt.Name == SelectedItem; }))
	{
		FIntPoint Resolution = FoundOption->Point;

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
				WindowModeComboBox->SetSelectedOption(FULLSCREEN);
				break;
			case EWindowMode::WindowedFullscreen:
				WindowModeComboBox->SetSelectedOption(WIDOWED_FULLSCREEN);
				break;
			case EWindowMode::Windowed:
				WindowModeComboBox->SetSelectedOption(WIDOWED);
				break;
			}

			if (Resolutions.Num() > 0)
			{
				ScreenResolutionComboBox->SetSelectedOption(Resolutions.Last().Name);
			}

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
