#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "Engine/DataTable.h"
#include "TabAudio.generated.h"

UENUM(BlueprintType)
enum class ESoundClassType : uint8
{
	Master,
	Effects,
	Music,
	Dialogue,
};

UCLASS()
class MUTATEARENA_API UTabAudio : public UCommonActivatableWidget
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

	UPROPERTY(meta = (BindWidget))
	class UAnalogSlider* MasterVolumeAnalogSlider;
	UPROPERTY(meta = (BindWidget))
	class UCommonTextBlock* MasterVolume;
	UFUNCTION()
	void OnMasterVolumeChanged(float Value);

	UPROPERTY(meta = (BindWidget))
	UAnalogSlider* EffectsVolumeAnalogSlider;
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* EffectsVolume;
	UFUNCTION()
	void OnEffectsVolumeChanged(float Value);

	UPROPERTY(meta = (BindWidget))
	UAnalogSlider* MusicVolumeAnalogSlider;
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* MusicVolume;
	UFUNCTION()
	void OnMusicVolumeChanged(float Value);

	UPROPERTY(meta = (BindWidget))
	UAnalogSlider* DialogueVolumeAnalogSlider;
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* DialogueVolume;
	UFUNCTION()
	void OnDialogueVolumeChanged(float Value);

	UPROPERTY(EditAnywhere)
	FDataTableRowHandle SetDefaultData;
	FUIActionBindingHandle SetDefaultHandle;
	void SetDefault();

};
