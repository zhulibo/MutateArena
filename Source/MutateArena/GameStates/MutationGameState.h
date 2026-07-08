#pragma once

#include "CoreMinimal.h"
#include "BaseGameState.h"
#include "MutationGameState.generated.h"

UCLASS()
class MUTATEARENA_API AMutationGameState : public ABaseGameState
{
	GENERATED_BODY()

public:
	AMutationGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaTime) override;

	virtual void OnRep_MatchState() override;
	virtual void HandleMatchHasStarted() override;
	virtual void HandleRoundHasEnded();
	
	UPROPERTY()
	class AMutationMode* MutationMode;
	UPROPERTY()
	class AMutationController* MutationController;

	void WatchGameState();

public:
	void EndRoundIfAllBeKilledByMelee();
	
	UPROPERTY(ReplicatedUsing = OnRep_DamageMul)
	float DamageMul = 1.f;
	UPROPERTY(ReplicatedUsing = OnRep_MeleeDamageMul)
	float MeleeDamageMul = 1.f;
protected:
	void AddDamageMul();
	UFUNCTION()
	void OnRep_DamageMul();
	void UpdateMeleeDamageMul();
	UFUNCTION()
	void OnRep_MeleeDamageMul();
public:
	void ResetRoundDamageModifiers();
	
	virtual void AddToPlayerStates(ABasePlayerState* BasePlayerState, ETeam Team) override;
	virtual void RemoveFromPlayerStates(ABasePlayerState* BasePlayerState, ETeam Team) override;
protected:
	virtual void OnRep_Team1PlayerStates() override;
	virtual void OnRep_Team2PlayerStates() override;

	void SetHUDTeamNum(int32 Team1Num, ETeam Team);

};
