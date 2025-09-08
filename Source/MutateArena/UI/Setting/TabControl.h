#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "TabControl.generated.h"

UCLASS()
class MUTATEARENA_API UTabControl : public UCommonActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	UPROPERTY()
	class UStorageSubsystem* StorageSubsystem;

	UPROPERTY(meta = (BindWidget))
	class UCommonHierarchicalScrollBox* SettingLeft;
	UPROPERTY(meta = (BindWidget))
	UCommonHierarchicalScrollBox* SettingRight;
	void OnTabButtonHovered(int Index);

	void SetUISavedValue();

	FVector2D Range_1_50 = FVector2D(1.f, 50.f);
	FVector2D Range_50_100 = FVector2D(50.f, 100.f);

	float MouseSensitivityMin;
	float MouseSensitivityMiddle;
	float MouseSensitivityMax;
	float MapMouseSensitivity(float Value);
	float InverseMapMouseSensitivity(float Value);

	float ControllerSensitivityMin;
	float ControllerSensitivityMiddle;
	float ControllerSensitivityMax;
	float MapControllerSensitivity(float Value);
	float InverseMapControllerSensitivity(float Value);

	// 键鼠
	UPROPERTY(meta = (BindWidget))
	class UAnalogSlider* MouseSensitivityAnalogSlider;
	UPROPERTY(meta = (BindWidget))
	class UCommonTextBlock* MouseSensitivity;
	UFUNCTION()
	void OnMouseSensitivityChanged(float Value);

	// 手柄
	UPROPERTY(meta = (BindWidget))
	UAnalogSlider* ControllerSensitivityAnalogSlider;
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* ControllerSensitivity;
	UFUNCTION()
	void OnControllerSensitivityChanged(float Value);

	UPROPERTY(EditAnywhere)
	FDataTableRowHandle SetDefaultData;
	FUIActionBindingHandle SetDefaultHandle;
	void SetDefault();

};
