#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "PauseMenu.generated.h"

enum class EMsgResult : uint8;

UCLASS()
class MUTATEARENA_API UPauseMenu : public UCommonActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	UPROPERTY()
	class ABaseController* BaseController;
	UPROPERTY()
	class UEOSSubsystem* EOSSubsystem;
	UPROPERTY()
	class UUISubsystem* UISubsystem;
	UPROPERTY()
	class UAssetSubsystem* AssetSubsystem;
	
	UPROPERTY()
	bool bWantToBack = true;
	UFUNCTION()
	void OnDeactivatedInternal();

	UPROPERTY(meta = (BindWidget))
	class UCommonButton* LoadoutSelectButton;
	UPROPERTY(EditAnywhere)
	TSubclassOf<class ULoadoutSelect> LoadoutSelectClass;
	void OnLoadoutSelectClicked();

	UPROPERTY(meta = (BindWidget))
	UCommonButton* MutantSelectButton;
	UPROPERTY(EditAnywhere)
	TSubclassOf<class UMutantSelect> MutantSelectClass;
	void OnMutantSelectButtonClicked();

	UPROPERTY(meta = (BindWidget))
	UCommonButton* SettingButton;
	UPROPERTY(EditAnywhere)
	TSubclassOf<class USetting> SettingClass;
	void OnSettingButtonClicked();

	UPROPERTY(meta = (BindWidget))
	UCommonButton* VoteButton;
	void OnVoteButtonClicked();

	UPROPERTY(meta = (BindWidget))
	UCommonButton* QuitButton;
	void OnQuitButtonClicked();
	void Quit(EMsgResult MsgResult);

};
