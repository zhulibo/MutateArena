#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "TabGame.generated.h"

UCLASS()
class MUTATEARENA_API UTabGame : public UCommonActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	UPROPERTY()
	class UStorageSubsystem* StorageSubsystem;
	UPROPERTY()
	UGameUserSettings* GameUserSettings;

	UPROPERTY(meta = (BindWidget))
	class UCommonHierarchicalScrollBox* SettingLeft;
	UPROPERTY(meta = (BindWidget))
	UCommonHierarchicalScrollBox* SettingRight;
	void OnTabButtonHovered(int Index);

	void SetUISavedValue();

	UPROPERTY(meta = (BindWidget))
	class UComboBoxString* LanguageComboBox;
	UFUNCTION()
	void OnLanguageChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UPROPERTY(meta = (BindWidget))
	UComboBoxString* HideSkinsComboBox;
	UFUNCTION()
	void OnHideSkinsChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UPROPERTY(meta = (BindWidget))
	UComboBoxString* ObfuscateNameComboBox;
	UFUNCTION()
	void OnObfuscateNameChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UPROPERTY(meta = (BindWidget))
	UComboBoxString* ObfuscateTextChatComboBox;
	UFUNCTION()
	void OnObfuscateTextChatChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UPROPERTY(EditAnywhere)
	FDataTableRowHandle SetDefaultData;
	FUIActionBindingHandle SetDefaultHandle;
	void SetDefault();
	
};
