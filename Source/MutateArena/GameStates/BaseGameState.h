#pragma once

#include "CoreMinimal.h"
#include "ModularGameState.h"
#include "BaseGameState.generated.h"

UCLASS()
class MUTATEARENA_API ABaseGameState : public AModularGameState
{
	GENERATED_BODY()

public:

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	UPROPERTY()
	class ABaseController* BaseController;

public:
	virtual void OnRep_MatchState() override;
protected:
	virtual void HandleMatchHasStarted() override;
	virtual void HandleRoundHasEnded();

	UPROPERTY(ReplicatedUsing = OnRep_Team1PlayerStates)
	TArray<ABasePlayerState*> Team1PlayerStates;
	UPROPERTY(ReplicatedUsing = OnRep_Team2PlayerStates)
	TArray<ABasePlayerState*> Team2PlayerStates;
	UFUNCTION()
	virtual void OnRep_Team1PlayerStates() {}
	UFUNCTION()
	virtual void OnRep_Team2PlayerStates() {}
public:
	TArray<ABasePlayerState*> GetPlayerStates(TOptional<ETeam> Team);
	virtual void AddToPlayerStates(ABasePlayerState* BasePlayerState, ETeam Team);
	virtual void RemoveFromPlayerStates(ABasePlayerState* BasePlayerState, ETeam Team);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastAddKillLog(ABasePlayerState* AttackerState, const FText& CauserName, ABasePlayerState* DamagedState);
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastSendMsg(EMsgType MsgType, ETeam Team, const FString& PlayerName, const FString& Msg = FString());
	
	UPROPERTY()
	TArray<AActor*> AllEquipments;
protected:
	FTimerHandle SetAllEquipmentsTimerHandle;
	void SetAllEquipments();
	
};
