#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "TabVideo.generated.h"

// TODO FullscreenMode无法保存，退出游戏后会自动恢复默认
UCLASS()
class MUTATEARENA_API UTabVideo : public UCommonActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	UPROPERTY()
	class UStorageSubsystem* StorageSubsystem;
	UPROPERTY()
	UGameUserSettings* GameUserSettings;

	void SetUISavedValue();

	UPROPERTY(meta = (BindWidget))
	class UCommonHierarchicalScrollBox* SettingLeft;
	UPROPERTY(meta = (BindWidget))
	UCommonHierarchicalScrollBox* SettingRight;
	void OnTabButtonHovered(int Index);

	UPROPERTY(meta = (BindWidget))
	class UAnalogSlider* BrightnessAnalogSlider;
	UPROPERTY(meta = (BindWidget))
	class UCommonTextBlock* Brightness;
	UFUNCTION()
	void OnBrightnessChanged(float Value);

	UPROPERTY(meta = (BindWidget))
	class UComboBoxString* WindowModeComboBox;
	UFUNCTION()
	void OnWindowModeChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UPROPERTY()
	TArray<FIntPoint> Resolutions;
	UPROPERTY(meta = (BindWidget))
	UComboBoxString* ScreenResolutionComboBox;
	UFUNCTION()
	void OnScreenResolutionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UPROPERTY(EditAnywhere)
	FDataTableRowHandle SetDefaultData;
	FUIActionBindingHandle SetDefaultHandle;
	void SetDefault();

};
