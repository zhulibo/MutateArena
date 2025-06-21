#pragma once

#include "CoreMinimal.h"
#include "BaseController.h"
#include "MutationController.generated.h"

enum class ETeam : uint8;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnTeamChange, ETeam Team);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnTeamNumChange, int32 TeamNum, ETeam Team);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCurRoundChange, int32 Round);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTotalRoundChange, int32 Round);
DECLARE_MULTICAST_DELEGATE(FOnCause1000Damage);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnSkillChange, bool bIsShow);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnRageChange, float Rage);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnDamageMulChange, float DamageMul);
DECLARE_MULTICAST_DELEGATE(FOnBeImmune);

UCLASS()
class MUTATEARENA_API AMutationController : public ABaseController
{
	GENERATED_BODY()

public:
	FOnTeamChange OnTeamChange;
	FOnTeamNumChange OnTeamNumChange;
	FOnCurRoundChange OnCurRoundChange;
	FOnTotalRoundChange OnTotalRoundChange;
	FOnCause1000Damage OnCause1000Damage;
	FOnSkillChange OnSkillChange;
	FOnRageChange OnRageChange;
	FOnDamageMulChange OnDamageMulChange;
	FOnBeImmune OnBeImmune;
	
protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	UPROPERTY()
	class AMutationMode* MutationMode;
	UPROPERTY()
	class AMutationGameState* MutationGameState;
	UPROPERTY()
	class AMutationPlayerState* MutationPlayerState;
	UPROPERTY()
	class UAssetSubsystem* AssetSubsystem;

	UFUNCTION(Server, Reliable)
	void RequestServerMatchInfo();
	UFUNCTION(Client, Reliable)
	void ReturnServerMatchInfo(
		FName TempMatchState,
		int32 TempTotalRound,
		float TempLevelStartTime,
		float TempWarmupTime,
		float TempRoundTime,
		float TempMutateTime,
		float TempPostRoundTime,
		float TempCooldownTime
	);

	int32 TotalRound = 0;
	UPROPERTY(ReplicatedUsing = OnRep_CurrentRound)
	int32 CurrentRound = 1;

	float RoundStartTime = 0.f;
	float RoundEndTime = 0.f;

	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	FName MatchState;
	float LevelStartTime = 0.f;
	float WarmupTime = 0.f;
	float RoundTime = 0.f;
	float MutateTime = 0.f;
	float PostRoundTime = 0.f;
	float CooldownTime = 0.f;

	int32 CountdownSeconds = 0;
	
public:
	void OnMatchStateSet(FName TempMatchState, int32 TempCurrentRound);
protected:
	UFUNCTION()
	void OnRep_MatchState();
	UFUNCTION()
	void HandleMatchStateChange();

	void HandleRoundHasEnded();
	UFUNCTION()
	void OnRep_CurrentRound();

	virtual void SetHUDTime() override;

	void InitHumanHUD();
	void InitMutantHUD();
	
public:
	virtual void InitHUD() override;
	virtual void SetHUDHealth(float Health) override;
	void SetHUDTeamNum(int32 TeamNum, ETeam Team);
	void ShowHUDSkill(bool bIsShow);
	void SetHUDRage(float UpdateRageUI);
	void SetHUDDamageMul(float DamageMul);
protected:
	void SetHUDCurrentRound();
	void SetHUDTotalRound();

public:
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlaySpawnPickupSound();
	
};
