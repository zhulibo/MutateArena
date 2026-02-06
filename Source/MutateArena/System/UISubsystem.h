#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UISubsystem.generated.h"

enum class EHUDState : uint8;
enum class EMsgType : uint8;
enum class ETeam : uint8;

class ABasePlayerState;

// Base
DECLARE_MULTICAST_DELEGATE_OneParam(FChangeCrosshairSpread, float Spread);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCrosshairHidden, bool bIsHidden);
DECLARE_MULTICAST_DELEGATE_OneParam(FChangeAnnouncement, FText Text);
DECLARE_MULTICAST_DELEGATE(FOnMatchEnd);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnMatchCountdownChange, int32 CountdownTime);
DECLARE_MULTICAST_DELEGATE_OneParam(FShowScoreboard, bool bIsShow);
DECLARE_MULTICAST_DELEGATE(FShowPauseMenu);
DECLARE_MULTICAST_DELEGATE_OneParam(FShowRadialMenu, bool bIsShow);
DECLARE_MULTICAST_DELEGATE(FSwitchRadialMenu);
DECLARE_MULTICAST_DELEGATE_TwoParams(FSelectRadialMenu, double X, double Y);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnKillStreakChange, int32 KillStreak);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnHumanHealthChange, float Health);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnMutantHealthChange, float Health);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnAmmoChange, int32 Ammo);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCarriedAmmoChange, int32 CarriedAmmo);
DECLARE_MULTICAST_DELEGATE(FShowTextChat);
DECLARE_MULTICAST_DELEGATE(FOnInteractStarted);
DECLARE_MULTICAST_DELEGATE(FOnInteractEnded);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnHUDStateChange, EHUDState HUDState);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnViewTargetChange, AActor* ViewTarget);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCauseDamage, float TempDamage);
DECLARE_MULTICAST_DELEGATE(FOnRoundStarted);
DECLARE_MULTICAST_DELEGATE(FOnRoundEnded);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnAddKillLog, ABasePlayerState* AttackerState, const FText& CauserName, ABasePlayerState* DamagedState);
DECLARE_MULTICAST_DELEGATE_FourParams(FOnReceiveMsg, EMsgType MsgType, ETeam Team, const FString& PlayerName, const FString& Msg);

// Mutation
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTeamChange, ETeam Team);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnTeamNumChange, int32 TeamNum, ETeam Team);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCurRoundChange, int32 Round);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTotalRoundChange, int32 Round);
DECLARE_MULTICAST_DELEGATE(FOnCause1000Damage);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnSkillChange, bool bIsShow);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnRageChange, float Rage);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnDamageMulChange, float DamageMul);
DECLARE_MULTICAST_DELEGATE(FOnBeImmune);

// TDM
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnTeamScoreChange, float Score, ETeam Team);

UCLASS()
class MUTATEARENA_API UUISubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
public:
	void RegisterGameLayout(class ULayoutBase* Layout);
	void UnregisterGameLayout(ULayoutBase* Layout);
	
	UFUNCTION()
	class UCommonActivatableWidgetStack* GetLayerStack(FGameplayTag LayerTag);
	
protected:
	TArray<TWeakObjectPtr<ULayoutBase>> ActiveLayouts;
	
public:
	// Base
	FChangeCrosshairSpread ChangeCrosshairSpread;
	FOnCrosshairHidden OnCrosshairHidden;
	FChangeAnnouncement ChangeAnnouncement;
	FOnMatchEnd OnMatchEnd;
	FOnMatchCountdownChange OnMatchCountdownChange;
	FShowScoreboard ShowScoreboard;
	FShowPauseMenu ShowPauseMenu;
	FShowRadialMenu ShowRadialMenu;
	FSwitchRadialMenu SwitchRadialMenu;
	FSelectRadialMenu SelectRadialMenu;
	FOnKillStreakChange OnKillStreakChange;
	FOnHumanHealthChange OnHumanHealthChange;
	FOnMutantHealthChange OnMutantHealthChange;
	FOnAmmoChange OnAmmoChange;
	FOnCarriedAmmoChange OnCarriedAmmoChange;
	FShowTextChat ShowTextChat;
	FOnInteractStarted OnInteractStarted;
	FOnInteractEnded OnInteractEnded;
	FOnHUDStateChange OnHUDStateChange;
	FOnViewTargetChange OnViewTargetChange;
	FOnCauseDamage OnCauseDamage;
	FOnRoundStarted OnRoundStarted;
	FOnRoundEnded OnRoundEnded;
	FOnAddKillLog OnAddKillLog;
	FOnReceiveMsg OnReceiveMsg;
	
	// Mutation
	FOnTeamChange OnTeamChange;
	FOnTeamNumChange OnTeamNumChange;
	FOnCurRoundChange OnCurRoundChange;
	FOnTotalRoundChange OnTotalRoundChange;
	FOnCause1000Damage OnCause1000Damage;
	FOnSkillChange OnSkillChange;
	FOnRageChange OnRageChange;
	FOnDamageMulChange OnDamageMulChange;
	FOnBeImmune OnBeImmune;
	
	// TDM
	FOnTeamScoreChange OnTeamScoreChange;
	
};
