#include "GA_Equipment_Aim.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Characters/Components/CombatComponent.h"

UGA_Equipment_Aim::UGA_Equipment_Aim()
{
}

bool UGA_Equipment_Aim::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	// 跳过了 UGA_Equipment::CanActivateAbility
	if (!UGameplayAbilityBase::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}
	
	AHumanCharacter* HumanChar = GetHumanCharacter();
	UCombatComponent* CombatComp = HumanChar->CombatComp;
	AWeapon* CurWeapon = GetCurWeapon();

	// 目前限制只有主副武器可瞄准
	if (!HumanChar || !CurWeapon || !CombatComp)
	{
		return false;
	}
	
	return true;
}

void UGA_Equipment_Aim::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	AHumanCharacter* HumanChar = GetHumanCharacter();
	AEquipment* CurEquipment = GetCurEquipment();
	UCombatComponent* CombatComp = HumanChar->CombatComp;
	
	if (!ASC || !HumanChar || !CurEquipment || !CombatComp)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
}

void UGA_Equipment_Aim::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
