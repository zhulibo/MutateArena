#include "GA_MutantGhostSkill.h"
#include "AbilitySystemComponent.h"
#include "MutateArena/Characters/Mutants/MutantGhost.h"

UGA_MutantGhostSkill::UGA_MutantGhostSkill()
{
}

void UGA_MutantGhostSkill::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	AMutantGhost* MutantGhost = Cast<AMutantGhost>(GetAvatarActorFromActorInfo());
	if (!ASC || !MutantGhost || !SkillEffectClass)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(SkillEffectClass, MutantGhost->GetCharacterLevel());
	if (SpecHandle.IsValid())
	{
		ActiveEffectHandle = ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
}

void UGA_MutantGhostSkill::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
