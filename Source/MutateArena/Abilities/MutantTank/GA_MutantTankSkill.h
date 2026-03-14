#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Abilities/GameplayAbilityBase.h"
#include "GA_MutantTankSkill.generated.h"

UCLASS(Abstract)
class MUTATEARENA_API UGA_MutantTankSkill : public UGameplayAbilityBase
{
	GENERATED_BODY()

public:
	UGA_MutantTankSkill();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UGameplayEffect> SkillEffectClass;
	
	FActiveGameplayEffectHandle ActiveEffectHandle;
	
};
