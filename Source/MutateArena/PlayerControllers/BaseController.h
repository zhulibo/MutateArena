#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BaseController.generated.h"

enum class EHUDState : uint8;
enum class EMsgType : uint8;
enum class ETeam : uint8;

DECLARE_MULTICAST_DELEGATE_OneParam(FChangeCrosshairSpread, float Spread);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCrosshairHidden, bool bIsHidden);
DECLARE_MULTICAST_DELEGATE_OneParam(FChangeAnnouncement, FText Text);
DECLARE_MULTICAST_DELEGATE(FOnMatchEnd);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnMatchCountdownChange, int32 CountdownTime);
DECLARE_MULTICAST_DELEGATE_OneParam(FShowScoreboard, bool bIsShow);
DECLARE_MULTICAST_DELEGATE(FShowPauseMenu);
DECLARE_MULTICAST_DELEGATE_OneParam(FShowRadialMenu, bool bIsShow);
DECLARE_MULTICAST_DELEGATE(FChangeRadialMenu);
DECLARE_MULTICAST_DELEGATE_TwoParams(FSelectRadialMenu, double X, double Y);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnKillStreakChange, int32 KillStreak);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnHumanHealthChange, float Health);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnMutantHealthChange, float Health);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnAmmoChange, int32 Ammo);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCarriedAmmoChange, int32 CarriedAmmo);
DECLARE_MULTICAST_DELEGATE(FShowTextChat);
DECLARE_MULTICAST_DELEGATE_FourParams(FSendRadioMsg, EMsgType MsgType, ETeam Team, const FString& PlayerName, const FString& Msg);
DECLARE_MULTICAST_DELEGATE(FOnInteractStarted);
DECLARE_MULTICAST_DELEGATE(FOnInteractEnded);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnHUDStateChange, EHUDState HUDState);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnViewTargetChange, AActor* ViewTarget);

UCLASS()
class MUTATEARENA_API ABaseController : public APlayerController
{
	GENERATED_BODY()

public:
	// TODO 将UI部分转移到子系统中
	UPROPERTY()
	class UGameLayout* GameLayout;

	FChangeCrosshairSpread ChangeCrosshairSpread;
	FOnCrosshairHidden OnCrosshairHidden;
	FChangeAnnouncement ChangeAnnouncement;
	FOnMatchEnd OnMatchEnd;
	FOnMatchCountdownChange OnMatchCountdownChange;
	FShowScoreboard ShowScoreboard;
	FShowPauseMenu ShowPauseMenu;
	FShowRadialMenu ShowRadialMenu;
	FChangeRadialMenu ChangeRadialMenu;
	FSelectRadialMenu SelectRadialMenu;
	FOnKillStreakChange OnKillStreakChange;
	FOnHumanHealthChange OnHumanHealthChange;
	FOnMutantHealthChange OnMutantHealthChange;
	FOnAmmoChange OnAmmoChange;
	FOnCarriedAmmoChange OnCarriedAmmoChange;
	FShowTextChat ShowTextChat;
	FSendRadioMsg SendRadioMsg;
	FOnInteractStarted OnInteractStarted;
	FOnInteractEnded OnInteractEnded;
	FOnHUDStateChange OnHUDStateChange;
	FOnViewTargetChange OnViewTargetChange;
	
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnUnPossess() override;
	virtual void OnRep_Pawn() override;
	virtual void SetViewTarget(class AActor* NewViewTarget, FViewTargetTransitionParams TransitionParams) override;
	
	UPROPERTY()
	class ABaseMode* BaseMode;
	UPROPERTY()
	class ABaseGameState* BaseGameState;
	UPROPERTY()
	class UAssetSubsystem* AssetSubsystem;
	// Diff between client and server WORLD TIME, not network delay.
	float ServerClientDeltaTime = 0.f;
	void HandleServerClientDeltaTime();
	UFUNCTION(Server, Reliable)
	void RequestServerTime(float ClientTime);
	UFUNCTION(Client, Reliable)
	void ReturnServerTime(float ClientTime, float ServerTime);
	bool bServerClientDeltaTimeHasInit = false;
	float GetServerTime();

	UPROPERTY(EditAnywhere)
	TSubclassOf<UGameLayout> GameLayoutClass;
	void AddGameLayout();

public:
	void FocusGame();
	void FocusUI();

	// 观战
	UFUNCTION()
	void SetPlayerSpectate();
	UFUNCTION()
	void SetPlayerPlay();
	UFUNCTION(Client, Reliable)
	void ClientHUDStateChanged(EHUDState HUDState);

protected:
	void SetHUDWarmupCountdown(int32 CountdownTime);

	virtual void HandleMatchHasStarted();
	virtual void HandleMatchHasEnded();
	virtual void HandleLeavingMap();

public:
	virtual void SetHUDHealth(float Health) {}

	virtual void SetHUDAmmo(int32 Ammo);
	virtual void SetHUDCarriedAmmo(int32 CarriedAmmo);
public:
	virtual void InitHUD() {}
protected:
	virtual void SetHUDTime() {}

public:
	UFUNCTION(Server, Reliable)
	void ServerSendMsg(EMsgType MsgType, ETeam Team, const FString& PlayerName, const FString& Msg = FString());
	
};
