#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "TextChat.generated.h"

enum class ETeam : uint8;

UENUM(BlueprintType)
enum class EMsgType : uint8
{
	Msg,
	Radio,
	Join,
	Left,
	Kicked,
	Disconnected,
	Closed,
	Start,
	HostChange,
	ServerNameChange,
	ModeNameChange,
	MapNameChange,
	MatchRoundChange,
	MatchTimeChange,
};

UCLASS()
class MUTATEARENA_API UTextChat : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	void ShowMsg(EMsgType MsgType, ETeam Team, const FString& PlayerName, const FString& Msg = FString());

	UPROPERTY(meta = (BindWidget))
	class UCommonHierarchicalScrollBox* MsgContainer;
	UPROPERTY(meta = (BindWidget))
	class UEditableTextBox* MsgEditableTextBox;

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY()
	class UEOSSubsystem* EOSSubsystem;
	UPROPERTY()
	class ABaseGameState* BaseGameState;
	UPROPERTY()
	class ABaseController* BaseController;
	UPROPERTY()
	class ABasePlayerState* BasePlayerState;

	UPROPERTY(meta = (BindWidget))
	class UCommonButtonBase* MsgContainerHoverButton;

	void ShowTextChat();

	UPROPERTY(EditAnywhere)
	TSubclassOf<class UMsgLineButton> MsgLineButtonClass;
	UFUNCTION()
	void OnMsgCommitted(const FText& Text, ETextCommit::Type CommitMethod);
	FString LastMsg;
	void SendMsg();

};
