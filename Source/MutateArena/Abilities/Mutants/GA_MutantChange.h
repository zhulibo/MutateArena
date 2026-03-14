#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Abilities/GameplayAbilityBase.h"
#include "GA_MutantChange.generated.h"

UCLASS(Abstract)
class MUTATEARENA_API UGA_MutantChange : public UGameplayAbilityBase
{
	GENERATED_BODY()

public:
	UGA_MutantChange();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	
	UFUNCTION()
	void TimerEnd();

};
