#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "MutateArena/System/EOSSubsystem.h"
#include "Lobby.generated.h"

UCLASS()
class MUTATEARENA_API ULobby : public UCommonActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	UPROPERTY()
	class AMenuController* MenuController;
	UPROPERTY()
	UEOSSubsystem* EOSSubsystem;
	UPROPERTY()
	class UUISubsystem* UISubsystem;

	void SetUIAttr();
	void SetUIButtonState();
	void UpdatePlayerList();
	void InitMapComboBox();
	int32 GetFocusedPlayerIndex(class UCommonHierarchicalScrollBox* Container);

	UPROPERTY(meta = (BindWidget))
	class UCommonHierarchicalScrollBox* Team1Container;
	UPROPERTY(meta = (BindWidget))
	UCommonHierarchicalScrollBox* Team2Container;
	UPROPERTY(EditAnywhere)
	TSubclassOf<class UPlayerLineButton> PlayerLineButtonClass;

	void OnLobbyMemberJoined(const FLobbyMemberJoined& LobbyMemberJoined);

	// 大厅属性
	UPROPERTY(meta = (BindWidget))
	class UEditableTextBox* ServerNameEditableTextBox;
	FText LastServerName;
	UFUNCTION()
	void OnServerNameCommitted(const FText& Text, ETextCommit::Type CommitMethod);
	UPROPERTY(meta = (BindWidget))
	class UCommonComboBox* ModeComboBox;
	UFUNCTION()
	void OnModeComboBoxChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
	UPROPERTY(meta = (BindWidget))
	UCommonComboBox* MapComboBox;
	UFUNCTION()
	void OnMapComboBoxChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	void OnModifyLobbyAttrComplete(bool bWasSuccessful);
	void OnLobbyAttrChanged(const FLobbyAttributesChanged& LobbyAttributesChanged);

	// 成员属性
	UPROPERTY(meta = (BindWidget))
	class UCommonButton* SwitchTeamButton;
	UFUNCTION()
	void OnSwitchTeamButtonClicked();
	UPROPERTY(meta = (BindWidget))
	UCommonButton* ReadyButton;
	UFUNCTION()
	void OnReadyButtonClicked();

	void OnModifyLobbyMemberAttrComplete(bool bWasSuccessful);
	void OnLobbyMemberAttrChanged(const FLobbyMemberAttributesChanged& LobbyMemberAttributesChanged);

	// 开始游戏
	UPROPERTY(meta = (BindWidget))
	UCommonButton* StartServerButton;
	UFUNCTION()
	void OnStartServerButtonClicked();
	bool CanStartServer();

	UPROPERTY(meta = (BindWidget))
	UCommonButton* JoinServerButton;
	UFUNCTION()
	void OnJoinServerButtonClicked();

	// 房主变更事件
	void OnLobbyLeaderChanged(const FLobbyLeaderChanged& LobbyLeaderChanged);

	// 离开大厅
	UPROPERTY(meta = (BindWidget))
	UCommonButton* BackButton;
	bool bIsExitingLobby = false;
	UFUNCTION()
	void OnBackButtonClicked();
	void OnLeaveLobbyComplete(bool bWasSuccessful);
	void OnLobbyMemberLeft(const FLobbyMemberLeft& LobbyMemberLeft);

	void OnLobbyLeft(const FLobbyLeft& LobbyLeft);

	UPROPERTY(meta = (BindWidget))
	class UTextChat* TextChat;

	bool bIsActioning = false;
	void OnTravelFailure(UWorld* World, ETravelFailure::Type Arg, const FString& String);

};
