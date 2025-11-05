#pragma once

#include "CoreMinimal.h"
#include "BaseMode.h"
#include "MutateArena/Characters/MutantCharacter.h"
#include "MutationMode.generated.h"

namespace MatchState
{
	extern MUTATEARENA_API const FName PostRound;
}

enum class ESpawnMutantReason : uint8;

UCLASS()
class MUTATEARENA_API AMutationMode : public ABaseMode
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	int32 TotalRound = 12;
	UPROPERTY(EditAnywhere)
	float WarmupTime = 10.f;
	UPROPERTY(EditAnywhere)
	float RoundTime = 180.f;
	UPROPERTY(EditAnywhere)
	float MutateTime = 25.f;
	UPROPERTY(EditAnywhere)
	float PostRoundTime = 7.f;
	UPROPERTY(EditAnywhere)
	float CooldownTime = 5.f;

	float RoundStartTime = 0.f;
	float RoundEndTime = 0.f;
	float MatchEndTime = 0.f;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnPostLogin(AController* NewPlayer) override;

	UPROPERTY()
	class AMutationGameState* MutationGameState;

	int32 CurRound = 1;
	float CountdownTime = 0.f;

	bool bIsEndingMatch = false;
	
public:
	virtual bool IsMatchInProgress() const override;

	bool bWatchRoundState = false;
	virtual void StartMatch() override;
	void EndRound();
	void StartNextRound();

	bool bWatchMatchState = false;
	virtual void EndMatch() override;
	
protected:
	virtual void OnMatchStateSet() override;
	virtual void HandleMatchHasStarted() override;
	
	UPROPERTY(EditAnywhere)
	TArray<TSubclassOf<class APickup>> PickupClasses;
	FTimerHandle SpawnPickupTimerHandle;
	UPROPERTY()
	TArray<APlayerStart*> PickupStartPoints;
	UFUNCTION()
	void SpawnPickups();
	
	UPROPERTY(EditAnywhere)
	TArray<TSubclassOf<class AHerbRage>> HerbRageClasses;
	FTimerHandle SpawnHerbRageTimerHandle;
	UPROPERTY()
	TArray<APlayerStart*> HerbRageStartPoints;
	UFUNCTION()
	void SpawnHerbRages();
	
	UPROPERTY(EditAnywhere)
	TArray<TSubclassOf<class AHerbRepelReceived>> HerbRepelReceivedClasses;
	FTimerHandle SpawnHerbRepelReceivedTimerHandle;
	UPROPERTY()
	TArray<APlayerStart*> HerbRepelReceivedStartPoints;
	UFUNCTION()
	void SpawnHerbRepelReceived();

	void RoundStartMutate();

public:
	virtual void HumanReceiveDamage(AHumanCharacter* DamagedCharacter, ABaseController* DamagedController,
		float Damage, const UDamageType* DamageType, AController* AttackerController, AActor* DamageCauser) override;

	void GetInfect(AHumanCharacter* DamagedCharacter, ABaseController* DamagedController,
		AMutantCharacter* AttackerCharacter, ABaseController* AttackerController, EMutantState MutantState);

	void MutantReceiveDamage(AMutantCharacter* DamagedCharacter, ABaseController* DamagedController,
		float Damage, const UDamageType* DamageType, AController* AttackerController, AActor* DamageCauser);

	void SelectMutant(ACharacter* Character, AController* Controller);
	void MutantRespawn(ACharacter* Character, ABaseController* BaseController, bool bKilledByMelee);
	void Mutate(ACharacter* Character, AController* Controller, ESpawnMutantReason SpawnMutantReason);

protected:
	UFUNCTION()
	void SpawnMutantCharacter(AController* Controller, ESpawnMutantReason SpawnMutantReason,
	FVector Location = FVector::ZeroVector, FRotator ActorRotation = FRotator::ZeroRotator, FRotator ViewRotation = FRotator::ZeroRotator);

};
