#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "LoadoutSelect.generated.h"

UCLASS()
class MUTATEARENA_API ULoadoutSelect : public UCommonActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	UPROPERTY()
	class ABaseController* BaseController;
	UPROPERTY()
	mutable class UStorageSubsystem* StorageSubsystem;
	UPROPERTY()
	class UUISubsystem* UISubsystem;

	UPROPERTY(meta = (BindWidget))
	class UCommonHierarchicalScrollBox* LoadoutSelectButtonContainer;
	UFUNCTION()
	void OnLoadoutSelectButtonClicked(int32 LoadoutIndex);

	void CloseMenu(bool bClosePauseMenu);

};
