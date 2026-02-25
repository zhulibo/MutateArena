#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Abilities/GameplayAbilityBase.h"
#include "GameplayAbility_MutantSmokerSkill.generated.h"

UCLASS()
class MUTATEARENA_API UGameplayAbility_MutantSmokerSkill : public UGameplayAbilityBase
{
	GENERATED_BODY()

public:
	UGameplayAbility_MutantSmokerSkill();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UGameplayEffect> SkillEffectClass;
	
	FActiveGameplayEffectHandle ActiveEffectHandle;
	
};
