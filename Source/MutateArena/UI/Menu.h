#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "MutateArena/System/EOSSubsystem.h"
#include "Menu.generated.h"

enum class EMsgResult : uint8;

UCLASS()
class MUTATEARENA_API UMenu : public UCommonActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY()
	class AMenuController* MenuController;

	UPROPERTY()
	UEOSSubsystem* EOSSubsystem;

	UPROPERTY(meta = (BindWidget))
	class UCommonButton* SettingButton;
	UPROPERTY(EditAnywhere)
	TSubclassOf<class USetting> SettingClass;
	void OnSettingButtonClicked();

	UPROPERTY(meta = (BindWidget))
	UCommonButton* QuitButton;
	void OnQuitButtonClicked();
	void Quit(EMsgResult MsgResult);

	UPROPERTY(meta = (BindWidget))
	class UCommonActivatableWidgetSwitcher* MenuSwitcher;
	UPROPERTY(meta = (BindWidget))
	class UMenuTab* MenuTab;
	UPROPERTY(meta = (BindWidget))
	class UServerContainer* Server;
	UPROPERTY(meta = (BindWidget))
	class UStorage* Storage;
	UPROPERTY(meta = (BindWidget))
	class UShop* Shop;
	UPROPERTY(meta = (BindWidget))
	class UDev* Dev;

	UPROPERTY()
	FString TitleFile_Notice = TEXT("Notice");
	void OnEnumerateTitleFilesComplete(bool bWasSuccessful);
	void OnReadTitleFileComplete(bool bWasSuccessful, const FTitleFileContentsRef& FileContents);
	bool IsBeijingTimeInRange(const FString& StartStr, const FString& EndStr);
	
};
