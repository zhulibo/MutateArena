#pragma once

#include "CoreMinimal.h"
#include "BaseController.h"
#include "TeamDeadMatchController.generated.h"

enum class ETeam : uint8;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnTeamScoreChange, float Score, ETeam Team);

UCLASS()
class MUTATEARENA_API ATeamDeadMatchController : public ABaseController
{
	GENERATED_BODY()

public:
	// TODO Move to UISubsystem
	FOnTeamScoreChange OnTeamScoreChange;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	UPROPERTY()
	class ATeamDeadMatchMode* TeamDeadMatchMode;
	UPROPERTY()
	class ATeamDeadMatchGameState* TeamDeadMatchGameState;
	UPROPERTY()
	class AHumanCharacter* HumanCharacter;

	// Client join midgame, get match state from server.
	UFUNCTION(Server, Reliable)
	void RequestServerMatchInfo();
	UFUNCTION(Client, Reliable)
	void ReturnServerMatchInfo(
		FName TempMatchState,
		float TempLevelStartTime,
		float TempWarmupTime,
		float TempMatchTime,
		float TempCooldownTime
	);
	
public:
	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	FName MatchState;
	void OnMatchStateSet(FName TempMatchState);
protected:
	UFUNCTION()
	void OnRep_MatchState();
	void HandleMatchStateChange();

	float LevelStartTime = 0.f;
	float MatchTime = 0.f;
	float WarmupTime = 0.f;
	float CooldownTime = 0.f;

	int32 CountdownSeconds = 0;

	virtual void SetHUDTime() override;

	virtual void InitHUD() override;

	float HUDHealth;
	float HUDMaxHealth;
	float HUDScore;
	int32 HUDDefeat;
public:
	virtual void SetHUDHealth(float Health) override;
	void SetHUDTeamScore(float Score, ETeam Team);
	
};
