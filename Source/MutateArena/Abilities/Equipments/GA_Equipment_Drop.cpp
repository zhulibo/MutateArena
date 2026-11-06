#include "GA_Equipment_Drop.h"

#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Characters/Components/CombatComponent.h"
#include "MutateArena/Equipments/AnimInstEquipment.h"
#include "MutateArena/Equipments/Equipment.h"
#include "MutateArena/System/GameplayTags.h"

UGA_Equipment_Drop::UGA_Equipment_Drop()
{
}

bool UGA_Equipment_Drop::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                            const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags,
                                            FGameplayTagContainer* OptionalRelevantTags) const
{
	// 跳过了 UGA_Equipment::CanActivateAbility
	if (!UGameplayAbilityBase::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	return true;
}

void UGA_Equipment_Drop::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                         const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	AHumanCharacter* HumanChar = GetHumanCharacter();
	UCombatComponent* CombatComp = HumanChar ? HumanChar->CombatComp : nullptr;

	if (!ASC || !HumanChar || !CombatComp || !TriggerEventData)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const EEquipmentSlot TargetEquipmentSlot = static_cast<EEquipmentSlot>(TriggerEventData->EventMagnitude);

	if (AEquipment* TargetEquipment = CombatComp->GetEquipmentBySlot(TargetEquipmentSlot))
	{
		TargetEquipment->OnStartSwapOut();

		// 立即清理本地 1P 手臂和武器动画，防止丢弃瞬间残留上一把枪的动画
		if (HumanChar->IsLocallyControlled())
		{
			if (UAnimInstance* AnimInst1P = HumanChar->Mesh1P ? HumanChar->Mesh1P->GetAnimInstance() : nullptr)
			{
				AnimInst1P->Montage_Stop(0.f);
			}
			if (UAnimInstEquipment* EquipAnimInst1P = TargetEquipment->GetEquipmentAnimInst1P())
			{
				EquipAnimInst1P->Montage_Stop(0.f);
			}
		}

		FGameplayCueParameters CueParams;
		CueParams.Instigator = HumanChar;
		CueParams.SourceObject = TargetEquipment;
		ASC->ExecuteGameplayCue(TAG_GC_EQUIPMENT_DROP, CueParams);

		// 丢弃前移除GA
		if (HasAuthority(&ActivationInfo))
		{
			TargetEquipment->ClearAbilities(ASC);
		}

		TargetEquipment->Drop();

		switch (TargetEquipmentSlot)
		{
		case EEquipmentSlot::Slot1:
			CombatComp->Slot1Equipment = nullptr;
			break;
		case EEquipmentSlot::Slot2:
			CombatComp->Slot2Equipment = nullptr;
			break;
		default:
			break;
		}
	}

	// 本地发起切枪事件
	if (HumanChar->IsLocallyControlled())
	{
		const EEquipmentSlot FallbackSlot = CombatComp->GetLastEquipment() ? CombatComp->LastEquipmentSlot : EEquipmentSlot::Slot3;
		HumanChar->SendSwapEquipmentEvent(FallbackSlot);
	}
	
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
