#pragma once

#include "CoreMinimal.h"
#include "BaseMode.h"
#include "TeamDeadMatchMode.generated.h"

UCLASS()
class MUTATEARENA_API ATeamDeadMatchMode : public ABaseMode
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	float WarmupTime = 10.f;
	UPROPERTY(EditAnywhere)
	float MatchTime = 600.f;
	UPROPERTY(EditAnywhere)
	float CooldownTime = 5.f;
	
	bool bWatchMatchState = false;
	virtual void EndMatch() override;

	virtual void HumanReceiveDamage(AHumanCharacter* DamagedCharacter, ABaseController* DamagedController,
		float Damage, const UDamageType* DamageType, AController* AttackerController, AActor* DamageCauser) override;

	void HumanRespawn(ACharacter* Character, AController* Controller);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnPostLogin(AController* NewPlayer) override;

	UPROPERTY()
	class ATeamDeadMatchGameState* TeamDeadMatchGameState;

	float MatchEndTime = 0.f;

	float CountdownTime = 0.f;

	bool bIsEndingMatch = false;

	virtual void OnMatchStateSet() override;
	virtual void HandleMatchHasStarted() override;
	void HandleSpawn(AController* Controller);

	FTimerHandle ChangeLobbyStatusTimerHandle;
	void HandleChangeLobbyStatus();

};
