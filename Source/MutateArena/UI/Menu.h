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
	virtual void NativeDestruct() override;

	UPROPERTY()
	class AMenuController* MenuController;

	UPROPERTY()
	UEOSSubsystem* EOSSubsystem;
	UPROPERTY()
	class UUISubsystem* UISubsystem;
	UPROPERTY()
	class UAssetSubsystem* AssetSubsystem;
	
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

	UPROPERTY(meta = (BindWidget))
	class UHorizontalBox* MessageBox;
	UPROPERTY(meta = (BindWidget))
	class UCommonTextBlock* Message;
	FTimerHandle EnumerateTitleFileTimerHandle;
	UPROPERTY()
	FString TitleFile_Message = TEXT("Message");
	void OnEnumerateTitleFilesComplete(bool bWasSuccessful);
	void OnReadTitleFileComplete(bool bWasSuccessful, const FTitleFileContentsRef& FileContents, const FString& Filename);
	bool IsBeijingTimeInRange(const FString& StartStr, const FString& EndStr);
	
};
