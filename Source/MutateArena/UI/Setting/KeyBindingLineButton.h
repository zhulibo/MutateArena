#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "KeyBindingLineButton.generated.h"

UCLASS()
class MUTATEARENA_API UKeyBindingLineButton : public UCommonButtonBase
{
	GENERATED_BODY()

protected:
	UFUNCTION()
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	
	UPROPERTY()
	class ABaseController* BaseController;
	UPROPERTY()
	class UEnhancedInputLocalPlayerSubsystem* EISubsystem;
	UPROPERTY()
	class UEnhancedInputUserSettings* UserSettings;
	
	UFUNCTION()
	void OnKBMKeySelected(FInputChord SelectedKey);
	UFUNCTION()
	void OnControllerKeySelected(FInputChord SelectedKey);
	UFUNCTION()
	void RebindKey(FName MappingName, FKey NewKey);

public:
	UPROPERTY()
	FName MappingName_KBM;
	UPROPERTY()
	FName MappingName_Controller;
	
	UPROPERTY(meta = (BindWidget))
	class UCommonTextBlock* ActionName;
	
	UPROPERTY(meta = (BindWidget))
	class UInputKeySelector* KeySelector_KBM;
	UPROPERTY(meta = (BindWidget))
	UInputKeySelector* KeySelector_Controller;
	void SetKeySilent(const FKey& Key, bool bIsGamepad);
	
};
