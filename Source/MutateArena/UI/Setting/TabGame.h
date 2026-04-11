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
	virtual void NativeConstruct() override;
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

	UFUNCTION()
	UWidget* GenerateComboBoxWidget(FName ItemName);

	UPROPERTY(meta = (BindWidget))
	class UCommonComboBox2* LanguageComboBox;
	UFUNCTION()
	void OnLanguageChanged(FName SelectedItem, ESelectInfo::Type SelectionType);

	UPROPERTY(meta = (BindWidget))
	UCommonComboBox2* HideSkinsComboBox;
	UFUNCTION()
	void OnHideSkinsChanged(FName SelectedItem, ESelectInfo::Type SelectionType);

	UPROPERTY(meta = (BindWidget))
	UCommonComboBox2* ObfuscateNameComboBox;
	UFUNCTION()
	void OnObfuscateNameChanged(FName SelectedItem, ESelectInfo::Type SelectionType);

	UPROPERTY(meta = (BindWidget))
	UCommonComboBox2* ObfuscateTextChatComboBox;
	UFUNCTION()
	void OnObfuscateTextChatChanged(FName SelectedItem, ESelectInfo::Type SelectionType);

	UPROPERTY(EditAnywhere)
	FDataTableRowHandle SetDefaultData;
	FUIActionBindingHandle SetDefaultHandle;
	void SetDefault();
	
};
