#pragma once

#include "CoreMinimal.h"
#include "BaseGameState.h"
#include "MeleeGameState.generated.h"

UCLASS()
class MUTATEARENA_API AMeleeGameState : public ABaseGameState
{
	GENERATED_BODY()

public:
	AMeleeGameState();

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY()
	class AMeleeMode* MeleeMode;
	UPROPERTY()
	class AMeleeController* MeleeController;

	void WatchGameState();

	UPROPERTY(ReplicatedUsing = OnRep_Team1Score)
	float Team1Score = 0.f;
	UPROPERTY(ReplicatedUsing = OnRep_Team2Score)
	float Team2Score = 0.f;
public:
	float GetTeamScore(ETeam Team);
	void AddTeamScore(ETeam Team);
protected:
	UFUNCTION()
	void OnRep_Team1Score();
	UFUNCTION()
	void OnRep_Team2Score();
	void SetHUDTeamScore(float Score, ETeam Team);

};
