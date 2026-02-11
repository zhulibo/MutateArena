#pragma once

#include "CoreMinimal.h"
#include "BaseController.h"
#include "MutationController.generated.h"

enum class ETeam : uint8;

UCLASS()
class MUTATEARENA_API AMutationController : public ABaseController
{
	GENERATED_BODY()
	
protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	UPROPERTY()
	class AMutationMode* MutationMode;
	UPROPERTY()
	class AMutationGameState* MutationGameState;
	UPROPERTY()
	class AMutationPlayerState* MutationPlayerState;

	UFUNCTION(Server, Reliable)
	void RequestServerMatchInfo();
	UFUNCTION(Client, Reliable)
	void ReturnServerMatchInfo(
		FName TempMatchState,
		int32 TempTotalRound,
		float TempLevelStartTime,
		float TempWarmupTime,
		float TempRoundStartTime,
		float TempRoundTime,
		float TempMutateTime,
		float TempRoundEndTime,
		float TempPostRoundTime,
		float TempCooldownTime
	);

	int32 TotalRound = 0;
	UPROPERTY(ReplicatedUsing = OnRep_CurRound)
	int32 CurRound = 1;

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
	void OnMatchStateSet(FName TempMatchState, int32 TempCurRound);
protected:
	UFUNCTION()
	void OnRep_MatchState();
	UFUNCTION()
	void HandleMatchStateChange();

	void HandleRoundHasEnded();
	UFUNCTION()
	void OnRep_CurRound();

	virtual void SetHUDTime() override;

	void InitHumanHUD();
	void InitMutantHUD();
	
public:
	virtual void InitHUD() override;
	virtual void SetHUDHealth(float Health) override;
	void SetHUDTeamNum(int32 TeamNum, ETeam Team);
	void SetHUDSkill(bool bIsReady);
	void SetHUDRage(float UpdateRageUI);
	void SetHUDDamageMul(float DamageMul);
protected:
	void SetHUDCurRound();
	void SetHUDTotalRound();

public:
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlaySpawnPickupSound();
	
};
