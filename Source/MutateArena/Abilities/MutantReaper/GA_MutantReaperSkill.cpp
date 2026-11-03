#include "GA_MutantReaperSkill.h"
#include "AbilitySystemComponent.h"
#include "MutateArena/Characters/Mutants/MutantReaper.h"

UGA_MutantReaperSkill::UGA_MutantReaperSkill()
{
}

void UGA_MutantReaperSkill::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	AMutantReaper* MutantReaper = Cast<AMutantReaper>(GetAvatarActorFromActorInfo());
	if (!ASC || !MutantReaper || !SkillEffectClass)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(SkillEffectClass, MutantReaper->GetCharacterLevel());
	if (SpecHandle.IsValid())
	{
		ActiveEffectHandle = ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
}

void UGA_MutantReaperSkill::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
