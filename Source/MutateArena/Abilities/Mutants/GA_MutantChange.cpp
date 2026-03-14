#include "GA_MutantChange.h"

#include "Abilities/Tasks/AbilityTask_WaitDelay.h"

UGA_MutantChange::UGA_MutantChange()
{
}

void UGA_MutantChange::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (UAbilityTask_WaitDelay* DelayTask = UAbilityTask_WaitDelay::WaitDelay(this, 10.f))
	{
		DelayTask->OnFinish.AddDynamic(this, &ThisClass::TimerEnd);
		DelayTask->ReadyForActivation();
	}
}

void UGA_MutantChange::TimerEnd()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
