#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Abilities/GameplayAbilityBase.h"
#include "GameplayAbility_MutantChange.generated.h"

UCLASS()
class MUTATEARENA_API UGameplayAbility_MutantChange : public UGameplayAbilityBase
{
	GENERATED_BODY()

public:
	UGameplayAbility_MutantChange();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	
	UFUNCTION()
	void TimerEnd();

};
