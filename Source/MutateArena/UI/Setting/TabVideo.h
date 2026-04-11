#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "TabVideo.generated.h"

USTRUCT()
struct FResolutionOption
{
	GENERATED_BODY()

	UPROPERTY()
	FName Name;

	UPROPERTY()
	FIntPoint Point;

	FResolutionOption() {}
	FResolutionOption(FName InName, FIntPoint InPoint) : Name(InName), Point(InPoint) {}
};

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

	UFUNCTION()
	UWidget* GenerateComboBoxWidget(FName ItemName);

	UFUNCTION()
	UWidget* GenerateComboBoxWidget_NoTranslation(FName ItemName);

	UPROPERTY(meta = (BindWidget))
	class UCommonComboBox2* WindowModeComboBox;
	UFUNCTION()
	void OnWindowModeChanged(FName SelectedItem, ESelectInfo::Type SelectionType);

	UPROPERTY()
	TArray<FResolutionOption> Resolutions;

	FName FormatResolutionName(int32 Width, int32 Height) const;
	
	UPROPERTY(meta = (BindWidget))
	UCommonComboBox2* ScreenResolutionComboBox;
	UFUNCTION()
	void OnScreenResolutionChanged(FName SelectedItem, ESelectInfo::Type SelectionType);

	UPROPERTY(EditAnywhere)
	FDataTableRowHandle SetDefaultData;
	FUIActionBindingHandle SetDefaultHandle;
	void SetDefault();

};
