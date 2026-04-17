#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Abilities/GameplayAbilityBase.h"
#include "GA_MutantCorpseDriverSkill.generated.h"

class ACorpseCharacter;

UCLASS()
class MUTATEARENA_API UGA_MutantCorpseDriverSkill : public UGameplayAbilityBase
{
	GENERATED_BODY()

public:
	UGA_MutantCorpseDriverSkill();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UPROPERTY(EditAnywhere)
	TSubclassOf<ACorpseCharacter> MinionClassToSpawn;

	UPROPERTY(EditAnywhere)
	UAnimMontage* SummonMontage;

	UPROPERTY(EditAnywhere)
	float SpawnForwardOffset = 150.0f;

private:
	UFUNCTION()
	void OnMontageCompleted();
	
	UFUNCTION()
	void OnMontageCancelled();

	void SpawnMinion(const FGameplayAbilityActorInfo* ActorInfo);
};
