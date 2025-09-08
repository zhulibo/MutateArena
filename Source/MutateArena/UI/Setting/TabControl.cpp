#include "TabControl.h"
#include "AnalogSlider.h"
#include "CommonButtonBase.h"
#include "CommonHierarchicalScrollBox.h"
#include "CommonTextBlock.h"
#include "MutateArena/System/Storage/SaveGameSetting.h"
#include "MutateArena/System/Storage/DefaultConfig.h"
#include "MutateArena/System/Storage/StorageSubsystem.h"
#include "Components/ComboBoxString.h"
#include "Input/CommonUIInputTypes.h"

void UTabControl::NativeOnInitialized()
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
	if (const UDefaultConfig* DefaultConfig = GetDefault<UDefaultConfig>())
	{
		MouseSensitivityMin = DefaultConfig->MouseSensitivity / DefaultConfig->MouseSensitivityMaxMul;
		MouseSensitivityMiddle = DefaultConfig->MouseSensitivity;
		MouseSensitivityMax = DefaultConfig->MouseSensitivity * DefaultConfig->MouseSensitivityMaxMul;

		ControllerSensitivityMin = DefaultConfig->ControllerSensitivity / DefaultConfig->ControllerSensitivityMaxMul;
		ControllerSensitivityMiddle = DefaultConfig->ControllerSensitivity;
		ControllerSensitivityMax = DefaultConfig->ControllerSensitivity * DefaultConfig->ControllerSensitivityMaxMul;
	}

	SetUISavedValue();

	MouseSensitivityAnalogSlider->OnValueChanged.AddUniqueDynamic(this, &ThisClass::OnMouseSensitivityChanged);
	ControllerSensitivityAnalogSlider->OnValueChanged.AddUniqueDynamic(this, &ThisClass::OnControllerSensitivityChanged);

	SetDefaultHandle = RegisterUIActionBinding(FBindUIActionArgs(SetDefaultData, true, FSimpleDelegate::CreateUObject(this, &ThisClass::SetDefault)));
}

void UTabControl::NativeConstruct()
{
	Super::NativeConstruct();

	// 默认隐藏所有提示信息
	TArray<UWidget*> Contents = SettingRight->GetAllChildren();
	for (int i = 0; i < Contents.Num(); ++i)
	{
		Contents[i]->SetVisibility(ESlateVisibility::Collapsed);
	}
}

UWidget* UTabControl::NativeGetDesiredFocusTarget() const
{
	return MouseSensitivityAnalogSlider;
}

void UTabControl::OnTabButtonHovered(int Index)
{
	TArray<UWidget*> Contents = SettingRight->GetAllChildren();
	for (int i = 0; i < Contents.Num(); ++i)
	{
		Contents[i]->SetVisibility(i == Index ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UTabControl::SetUISavedValue()
{
	if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
	if (StorageSubsystem && StorageSubsystem->CacheSetting)
	{
		int32 MouseSensitivityInverse = FMath::RoundToInt(InverseMapMouseSensitivity(StorageSubsystem->CacheSetting->MouseSensitivity));
		MouseSensitivityAnalogSlider->SetValue(MouseSensitivityInverse);
		MouseSensitivity->SetText(FText::AsNumber(MouseSensitivityInverse));

		int32 ControllerSensitivityInverse = FMath::RoundToInt(InverseMapControllerSensitivity(StorageSubsystem->CacheSetting->ControllerSensitivity));
		ControllerSensitivityAnalogSlider->SetValue(ControllerSensitivityInverse);
		ControllerSensitivity->SetText(FText::AsNumber(ControllerSensitivityInverse));
	}
}

float UTabControl::MapMouseSensitivity(float Value)
{
	float Sensitivity;

	if (Value < 50.f)
	{
		FVector2D OutRange(MouseSensitivityMin, MouseSensitivityMiddle);
		Sensitivity = FMath::GetMappedRangeValueClamped(Range_1_50, OutRange, Value);
	}
	else if (Value > 50.f)
	{
		FVector2D OutRange(MouseSensitivityMiddle, MouseSensitivityMax);
		Sensitivity = FMath::GetMappedRangeValueClamped(Range_50_100, OutRange, Value);
	}
	else
	{
		Sensitivity = MouseSensitivityMiddle;
	}

	return Sensitivity;
}

float UTabControl::InverseMapMouseSensitivity(float Value)
{
	float Sensitivity;

	if (Value < MouseSensitivityMiddle)
	{
		FVector2D InRange(MouseSensitivityMin, MouseSensitivityMiddle);
		Sensitivity = FMath::GetMappedRangeValueClamped(InRange, Range_1_50, Value);
	}
	else if (Value > MouseSensitivityMiddle)
	{
		FVector2D InRange(MouseSensitivityMiddle, MouseSensitivityMax);
		Sensitivity = FMath::GetMappedRangeValueClamped(InRange, Range_50_100, Value);
	}
	else
	{
		Sensitivity = 50.f;
	}

	return Sensitivity;
}

float UTabControl::MapControllerSensitivity(float Value)
{
	float Sensitivity;

	if (Value < 50.f)
	{
		FVector2D OutRange(ControllerSensitivityMin, ControllerSensitivityMiddle);
		Sensitivity = FMath::GetMappedRangeValueClamped(Range_1_50, OutRange, Value);
	}
	else if (Value > 50.f)
	{
		FVector2D OutRange(ControllerSensitivityMiddle, ControllerSensitivityMax);
		Sensitivity = FMath::GetMappedRangeValueClamped(Range_50_100, OutRange, Value);
	}
	else
	{
		Sensitivity = ControllerSensitivityMiddle;
	}

	return Sensitivity;
}

float UTabControl::InverseMapControllerSensitivity(float Value)
{
	float Sensitivity;

	if (Value < ControllerSensitivityMiddle)
	{
		FVector2D InRange(ControllerSensitivityMin, ControllerSensitivityMiddle);
		Sensitivity = FMath::GetMappedRangeValueClamped(InRange, Range_1_50, Value);
	}
	else if (Value > ControllerSensitivityMiddle)
	{
		FVector2D InRange(ControllerSensitivityMiddle, ControllerSensitivityMax);
		Sensitivity = FMath::GetMappedRangeValueClamped(InRange, Range_50_100, Value);
	}
	else
	{
		Sensitivity = 50.f;
	}

	return Sensitivity;
}

void UTabControl::OnMouseSensitivityChanged(float Value)
{
	MouseSensitivity->SetText(FText::AsNumber(FMath::RoundToFloat(Value)));

	if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
	if (StorageSubsystem && StorageSubsystem->CacheSetting)
	{
		StorageSubsystem->CacheSetting->MouseSensitivity = MapMouseSensitivity(Value);
		StorageSubsystem->SaveSetting();
	}
}

void UTabControl::OnControllerSensitivityChanged(float Value)
{
	ControllerSensitivity->SetText(FText::AsNumber(FMath::RoundToFloat(Value)));

	if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
	if (StorageSubsystem && StorageSubsystem->CacheSetting)
	{
		StorageSubsystem->CacheSetting->ControllerSensitivity = MapControllerSensitivity(Value);
		StorageSubsystem->SaveSetting();
	}
}

void UTabControl::SetDefault()
{
	if (const UDefaultConfig* DefaultConfig = GetDefault<UDefaultConfig>())
	{
		if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
		if (StorageSubsystem && StorageSubsystem->CacheSetting)
		{
			int32 MouseSensitivityInverse = FMath::RoundToInt(InverseMapMouseSensitivity(DefaultConfig->MouseSensitivity));
			MouseSensitivityAnalogSlider->SetValue(MouseSensitivityInverse);
			MouseSensitivity->SetText(FText::AsNumber(MouseSensitivityInverse));

			int32 ControllerSensitivityInverse = FMath::RoundToInt(InverseMapControllerSensitivity(DefaultConfig->ControllerSensitivity));
			ControllerSensitivityAnalogSlider->SetValue(ControllerSensitivityInverse);
			ControllerSensitivity->SetText(FText::AsNumber(ControllerSensitivityInverse));

			StorageSubsystem->CacheSetting->MouseSensitivity = DefaultConfig->MouseSensitivity;
			StorageSubsystem->CacheSetting->ControllerSensitivity = DefaultConfig->ControllerSensitivity;
			StorageSubsystem->SaveSetting();
		}
	}
}
