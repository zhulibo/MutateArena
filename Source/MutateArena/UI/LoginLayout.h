#pragma once

#include "CoreMinimal.h"
#include "LayoutBase.h"
#include "LoginLayout.generated.h"

class UVersion;
class UEOSSubsystem;
enum class EMsgResult : uint8;
enum class ECoolLoginType : uint8;

UCLASS()
class MUTATEARENA_API ULoginLayout : public ULayoutBase
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	
	// 模态层
	UPROPERTY(meta = (BindWidget))
	UCommonActivatableWidgetStack* ModalStack;
	
	UPROPERTY()
	UEOSSubsystem* EOSSubsystem;
	UPROPERTY()
	class UUISubsystem* UISubsystem;
	UPROPERTY()
	class UAssetSubsystem* AssetSubsystem;
	
	UPROPERTY()
	class ALoginController* LoginController;
	
	UPROPERTY(meta = (BindWidget))
	class UCommonTextBlock* LoginStatus;
	UPROPERTY(meta = (BindWidget))
	class UCommonButton* LoginButton;
	UFUNCTION()
	void OnLoginButtonClicked(ECoolLoginType LoginType, FString Id, FString Token);
	void OnLoginComplete(bool bWasSuccessful, FString ErrorMessage);

	UPROPERTY(meta = (BindWidget))
	class USizeBox* DevContainer;
	UPROPERTY(meta = (BindWidget))
	UCommonButton* Login1Button;
	UPROPERTY(meta = (BindWidget))
	UCommonButton* Login2Button;
	UPROPERTY(meta = (BindWidget))
	UCommonButton* Login3Button;
	UPROPERTY(meta = (BindWidget))
	UCommonButton* Login4Button;
	UPROPERTY(meta = (BindWidget))
	UCommonButton* DevButton;
	void ShowDevContainer();
	void OnDevButtonClicked();
	
	UPROPERTY(meta = (BindWidget))
	UCommonButton* QuitButton;
	void OnQuitButtonClicked();
	void Quit(EMsgResult MsgResult);

	UPROPERTY(meta = (BindWidget))
	UCommonButton* StatusLinkButton;
	void OnStatusLinkButtonClicked();
	
	UPROPERTY(meta = (BindWidget))
	UVersion* Version;
	
};
