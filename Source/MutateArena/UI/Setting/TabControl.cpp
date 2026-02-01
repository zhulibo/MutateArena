#include "TabControl.h"
#include "AnalogSlider.h"
#include "CommonButtonBase.h"
#include "CommonHierarchicalScrollBox.h"
#include "CommonTextBlock.h"
#include "EnhancedActionKeyMapping.h"
#include "InputMappingContext.h"
#include "MutateArena/System/Storage/SaveGameSetting.h"
#include "MutateArena/System/Storage/DefaultConfig.h"
#include "MutateArena/System/Storage/StorageSubsystem.h"
#include "Components/ComboBoxString.h"
#include "Input/CommonUIInputTypes.h"
#include "MutateArena/System/AssetSubsystem.h"
#include "MutateArena/Utils/LibraryCommon.h"
#include "KeyBindingLineButton.h"
#include "Components/InputKeySelector.h"
#include "MutateArena/Characters/Data/InputAsset.h"
#include "CommonUserWidget.h"
#include "EnhancedInputSubsystems.h"
#include "Components/Border.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/PlayerControllers/BaseController.h"
#include "MutateArena/Utils/LibraryNotify.h"
#include "UserSettings/EnhancedInputUserSettings.h"

#define LOCTEXT_NAMESPACE "UTabControl"

void UTabControl::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	CreateKeyBindingWidgets();

	// 绑定提示信息切换菜单
	auto Tabs = ULibraryCommon::GetAllChildrenOfClass<UCommonButtonBase>(SettingLeft);
	for (int i = 0; i < Tabs.Num(); ++i)
	{
		Tabs[i]->OnHovered().AddUObject(this, &ThisClass::OnTabButtonHovered, i);
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
	
	// 重置键位绑定
	if (UserSettings)
	{
		const FString& ProfileId = UserSettings->GetActiveKeyProfileId();
		FGameplayTagContainer FailureReason;
		UserSettings->ResetKeyProfileIdToDefault(ProfileId, FailureReason);
		
		if (FailureReason.IsEmpty())
		{
			UserSettings->ApplySettings();
			UserSettings->SaveSettings();
			
			CreateKeyBindingWidgets();
		}
		else
		{
			NOTIFY(this, C_RED, LOCTEXT("ResetKeyBindingsFailed", "Reset Key Bindings Failed"));
			
			UE_LOG(LogTemp, Warning, TEXT("ResetKeyBindingsFailed: %s"), *FailureReason.ToString());
		}
	}
}

void UTabControl::CreateKeyBindingWidgets()
{
	if (AssetSubsystem == nullptr) AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
	if (AssetSubsystem == nullptr || AssetSubsystem->InputAsset == nullptr) return;

	SetWidgetSectionColor();
	
	if (EISubsystem == nullptr) EISubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetOwningLocalPlayer());
	if (EISubsystem == nullptr) return;

	if (UserSettings == nullptr) UserSettings = EISubsystem->GetUserSettings();
	if (UserSettings == nullptr) return;

	UserSettings->RegisterInputMappingContext(AssetSubsystem->InputAsset->BaseMappingContext);
	UserSettings->RegisterInputMappingContext(AssetSubsystem->InputAsset->HumanMappingContext);
	UserSettings->RegisterInputMappingContext(AssetSubsystem->InputAsset->MutantMappingContext);
	UserSettings->RegisterInputMappingContext(AssetSubsystem->InputAsset->SpectatorMappingContext);
	UserSettings->RegisterInputMappingContext(AssetSubsystem->InputAsset->RadialMenuMappingContext);

	BaseSection->ClearChildren();
	HumanSection->ClearChildren();
	MutantSection->ClearChildren();
	SpectatorSection->ClearChildren();
	
	CreateWidgetsForContext(AssetSubsystem->InputAsset->BaseMappingContext, BaseSection);
	CreateWidgetsForContext(AssetSubsystem->InputAsset->RadialMenuMappingContext, BaseSection);
	CreateWidgetsForContext(AssetSubsystem->InputAsset->HumanMappingContext, HumanSection);
	CreateWidgetsForContext(AssetSubsystem->InputAsset->MutantMappingContext, MutantSection);
	CreateWidgetsForContext(AssetSubsystem->InputAsset->SpectatorMappingContext, SpectatorSection);
}

void UTabControl::SetWidgetSectionColor()
{
	FColor White = C_WHITE;
	FColor Red = C_RED;
	FColor Green = C_GREEN;
	FColor Gray = C_BLACK;

	White.A = 30;
	Red.A = 40;
	Green.A = 40;
	Gray.A = 40;

	BaseSectionBG->SetBrushColor(White);
	HumanSectionBG->SetBrushColor(Red);
	MutantSectionBG->SetBrushColor(Green);
	SpectatorSectionBG->SetBrushColor(Gray);
}

void UTabControl::CreateWidgetsForContext(const UInputMappingContext* Context, UVerticalBox* Section)
{
    if (Context == nullptr) return;

    const TArray<FEnhancedActionKeyMapping>& Mappings = Context->GetMappings();
    TMap<const UInputAction*, UKeyBindingLineButton*> CreatedWidgetsMap;
	
    for (const FEnhancedActionKeyMapping& Mapping : Mappings)
    {
        const UInputAction* Action = Mapping.Action;

    	// wasd 键位暂时隐藏，UI不好处理
    	if (HiddenActions.Contains(Action)) continue;
    	
        UKeyBindingLineButton* RowWidget = nullptr;

        if (CreatedWidgetsMap.Contains(Action))
        {
            RowWidget = CreatedWidgetsMap[Action];
        }
        else
        {
            RowWidget = CreateWidget<UKeyBindingLineButton>(this, KeyBindingLineButtonClass);
            if (RowWidget)
            {
            	// 键鼠和手柄的DisplayName在UInputMappingContext中设置的一样
                RowWidget->ActionName->SetText(Mapping.GetDisplayName());

                if (UVerticalBoxSlot* NewSlot = Cast<UVerticalBoxSlot>(Section->AddChild(RowWidget)))
                {
                   NewSlot->SetPadding(FMargin(0, 10, 0, 10));
                }
            	
                CreatedWidgetsMap.Add(Action, RowWidget);
            }
        }
    	
    	if (RowWidget)
    	{
    		// 获取绑定后的键位
    		FKey MappedKey = Mapping.Key;
    		if (UserSettings)
    		{
			    if (const FPlayerKeyMapping* PlayerKeyMapping = UserSettings->FindCurrentMappingForSlot(Mapping.GetMappingName(), EPlayerMappableKeySlot::First))
    			{
    				MappedKey = PlayerKeyMapping->GetCurrentKey();
    			}
    		}
    		
    		if (Mapping.Key.IsGamepadKey())
    		{
    			RowWidget->SetKeySilent(MappedKey, true);
    			RowWidget->MappingName_Controller = Mapping.GetMappingName();
    		}
    		else if (Mapping.Key.IsTouch())
    		{
			    UE_LOG(LogTemp, Warning, TEXT("Mapping.Key.IsTouch()"));
    		}
    		else
    		{
    			RowWidget->SetKeySilent(MappedKey, false);
    			RowWidget->MappingName_KBM = Mapping.GetMappingName();
    		}
    	}
    	
    	// 禁用部分键位绑定
    	if (DisabledActions.Contains(Action))
    	{
    		RowWidget->KeySelector_KBM->SetIsEnabled(false);
    		RowWidget->KeySelector_Controller->SetIsEnabled(false);
    	}
    }
}

#undef LOCTEXT_NAMESPACE
