#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "BaseMode.generated.h"

enum class ETeam : uint8;

UCLASS()
class MUTATEARENA_API ABaseMode : public AGameMode
{
	GENERATED_BODY()

public:
	ABaseMode();

	float LevelStartTime = 0.f;

	virtual void HumanReceiveDamage(class AHumanCharacter* DamagedCharacter, class ABaseController* DamagedController,
		float Damage, const UDamageType* DamageType, AController* AttackerController, AActor* DamageCauser) {}

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnPostLogin(AController* NewPlayer) override;
	
	virtual void HandleMatchHasEnded() override;

	virtual void Logout(AController* Exiting) override;
	virtual void HandleLeavingMap() override;

	UPROPERTY()
	class ABaseGameState* BaseGameState;
	UPROPERTY()
	class UEOSSubsystem* EOSSubsystem;

	UFUNCTION()
	void SpawnHumanCharacter(AController* Controller);

	UPROPERTY()
	TArray<class APlayerStart*> Team1PlayerStarts;
	UPROPERTY()
	TArray<APlayerStart*> Team2PlayerStarts;
	AActor* FindCharacterPlayerStart(ETeam Team);
	void AssignTeam(AController* Controller, ETeam Team);

	void AddKillLog(class ABasePlayerState* AttackerState, AActor* DamageCauser, const UDamageType* DamageType, ABasePlayerState* DamagedState);

	UFUNCTION()
	void ChangeLobbyStatus(int64 Status);
	
};
