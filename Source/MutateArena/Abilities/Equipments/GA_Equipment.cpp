#include "GA_Equipment.h"

#include "AbilitySystemComponent.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Characters/Components/CombatComponent.h"
#include "MutateArena/Equipments/Equipment.h"

UGA_Equipment::UGA_Equipment()
{
}

bool UGA_Equipment::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}
	
	AHumanCharacter* HumanChar = GetHumanCharacter();
	UCombatComponent* CombatComp = HumanChar ? HumanChar->CombatComp : nullptr;
	AEquipment* CurEquipment = GetCurEquipment();

	if (!HumanChar || !CurEquipment || !CombatComp)
	{
		return false;
	}
	
	AEquipment* SourceEquipment = Cast<AEquipment>(ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle)->SourceObject);
	// 只有当这个技能所属的武器，就是玩家当前正拿在手里的武器时，才允许激活
	if (SourceEquipment && CurEquipment != SourceEquipment)
	{
		return false;
	}

	return true;
}

AHumanCharacter* UGA_Equipment::GetHumanCharacter() const
{
	return Cast<AHumanCharacter>(GetAvatarActorFromActorInfo());
}

AEquipment* UGA_Equipment::GetCurEquipment() const
{
	if (AHumanCharacter* HumanChar = GetHumanCharacter())
	{
		if (UCombatComponent* CombatComp = HumanChar->CombatComp)
		{
			return CombatComp->GetCurEquipment();
		}
	}
	
	return nullptr;
}

AEquipment* UGA_Equipment::GetLastEquipment() const
{
	if (AHumanCharacter* HumanChar = GetHumanCharacter())
	{
		if (UCombatComponent* CombatComp = HumanChar->CombatComp)
		{
			return CombatComp->GetLastEquipment();
		}
	}
	
	return nullptr;
}

AWeapon* UGA_Equipment::GetCurWeapon() const
{
	if (AHumanCharacter* HumanChar = GetHumanCharacter())
	{
		if (UCombatComponent* CombatComp = HumanChar->CombatComp)
		{
			return CombatComp->GetCurWeapon();
		}
	}
	
	return nullptr;
}

class AMelee* UGA_Equipment::GetCurMelee() const
{
	if (AHumanCharacter* HumanChar = GetHumanCharacter())
	{
		if (UCombatComponent* CombatComp = HumanChar->CombatComp)
		{
			return CombatComp->GetCurMelee();
		}
	}
	
	return nullptr;
}
