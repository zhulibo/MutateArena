#include "GA_Equipment_Drop.h"
#include "AbilitySystemComponent.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Characters/Components/CombatComponent.h"
#include "MutateArena/Equipments/Equipment.h"
#include "MutateArena/Equipments/Pickups/PickupEquipment.h"
#include "MutateArena/System/Tags/ProjectTags.h"

UGA_Equipment_Drop::UGA_Equipment_Drop()
{
}

bool UGA_Equipment_Drop::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	// 跳过了 UGA_Equipment::CanActivateAbility
	if (!UGameplayAbilityBase::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	return true;
}

void UGA_Equipment_Drop::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	AHumanCharacter* HumanChar = GetHumanCharacter();
	UCombatComponent* CombatComp = HumanChar ? HumanChar->CombatComp : nullptr;
	
	if (!ASC || !HumanChar || !CombatComp)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!TriggerEventData)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	EEquipmentType TargetEquipmentType = static_cast<EEquipmentType>(TriggerEventData->EventMagnitude);
	AEquipment* TargetEquipment = CombatComp->GetEquipmentByType(TargetEquipmentType);
	
	if (TargetEquipment)
	{
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

		switch (TargetEquipmentType)
		{
		case EEquipmentType::Primary:
			CombatComp->PrimaryEquipment = nullptr;
			break;
		case EEquipmentType::Secondary:
			CombatComp->SecondaryEquipment = nullptr;
			break;
		default:
			break;
		}
	}
	
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	
	// 补给箱强制替换 给予补给箱装备时传入了 OptionalObject
	if (TriggerEventData->OptionalObject)
	{
		APickupEquipment* PickupEquipment = const_cast<APickupEquipment*>(Cast<APickupEquipment>(TriggerEventData->OptionalObject));
		if (HasAuthority(&ActivationInfo) && PickupEquipment)
		{
			if (AEquipment* NewEquipment = PickupEquipment->Equipment)
			{
				bool bIsCurrent = CombatComp->CurEquipmentType == NewEquipment->EquipmentType;
				CombatComp->EquipEquipment(NewEquipment, bIsCurrent);
				
				if (!bIsCurrent)
				{
					CombatComp->ClientPlayEquipSound();
				}
			}
			PickupEquipment->Destroy();
		}
	}
	// 普通丢弃，切枪前如果脚下有同类型装备，直接捡起，如果没有则切枪
	else
	{
		// 检测脚下是否有同类型的装备
		AEquipment* OverlappingEquipment = nullptr;
		TArray<AActor*> OverlappingActors;
		HumanChar->GetOverlappingActors(OverlappingActors, AEquipment::StaticClass());
		for (AActor* Actor : OverlappingActors)
		{
			if (AEquipment* Equipment = Cast<AEquipment>(Actor))
			{
				if (Equipment->EquipmentType == TargetEquipmentType && Equipment != TargetEquipment) 
				{
					OverlappingEquipment = Equipment;
					break;
				}
			}
		}
		
		if (HasAuthority(&ActivationInfo))
		{
			if (OverlappingEquipment)
			{
				// 抢到了！服务器直接装备。这会触发客户端的 OnRep，让客户端自动拿枪
				CombatComp->EquipEquipment(OverlappingEquipment, true);
			}
			else
			{
				// 没抢到（或者根本没有）。通知客户端进行切枪。
				if (!HumanChar->IsLocallyControlled())
				{
					HumanChar->ClientSwapEquipmentWhenPickupFailed(CombatComp->GetLastEquipment() ? CombatComp->LastEquipmentType : EEquipmentType::Melee);
				}
			}
		}

		// 本地客户端的预测行为
		if (HumanChar->IsLocallyControlled())
		{
			if (!OverlappingEquipment)
			{
				// 本地发现脚下没东西：不干等，立刻发送切枪事件（无网络延迟手感）
				HumanChar->SendSwapEquipmentEvent(CombatComp->GetLastEquipment() ? CombatComp->LastEquipmentType : EEquipmentType::Melee);
			}
			// 如果本地发现脚下有东西，就什么也不做，静静等待服务器的 OnRep 同步（抢到了）或 RPC 通知（没抢到）
		}
	}
}
