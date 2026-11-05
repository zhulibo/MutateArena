// GA_Equipment_Drop.cpp
#include "GA_Equipment_Drop.h"

#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Characters/Components/CombatComponent.h"
#include "MutateArena/Equipments/AnimInstEquipment.h"
#include "MutateArena/Equipments/Equipment.h"
#include "MutateArena/Equipments/Data/EquipmentAsset.h"
#include "MutateArena/Equipments/Pickups/PickupEquipment.h"
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
	AEquipment* TargetEquipment = CombatComp->GetEquipmentBySlot(TargetEquipmentSlot);

	if (TargetEquipment)
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

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);

	// 1. 补给箱强制替换（给予补给箱装备时传入了 OptionalObject）
	if (TriggerEventData->OptionalObject)
	{
		APickupEquipment* PickupEquipment = const_cast<APickupEquipment*>(Cast<APickupEquipment>(TriggerEventData->OptionalObject));
		if (HasAuthority(&ActivationInfo) && PickupEquipment)
		{
			if (AEquipment* NewEquipment = PickupEquipment->Equipment)
			{
				if (const UEquipmentAsset* EquipAsset = NewEquipment->EnsureEquipmentAsset())
				{
					const EEquipmentSlot NewSlot = EquipAsset->GetEquipmentSlot();
					const bool bIsCurrent = (CombatComp->CurEquipmentSlot == NewSlot);

					CombatComp->GiveEquipment(NewEquipment);

					if (bIsCurrent)
					{
						// 替换的是当前手持槽位：触发切枪 GA 以进入切枪状态并在 1P 播放切入动画（客户端在 OnRep_SlotXEquipment 中同步触发）
						FGameplayEventData SwapPayload;
						SwapPayload.EventMagnitude = static_cast<float>(NewSlot);
						ASC->HandleGameplayEvent(TAG_EVENT_EQUIPMENT_SWAP, &SwapPayload);
					}
					else
					{
						CombatComp->ClientPlayEquipSound();
					}
				}
			}
			PickupEquipment->Destroy();
		}
	}
	// 2. 普通丢弃：如果脚下有同类型装备则直接捡起并播放 1P 切入动画，否则切换到上一把武器/近战武器
	else
	{
		AEquipment* OverlappingEquipment = nullptr;
		TArray<AActor*> OverlappingActors;
		HumanChar->GetOverlappingActors(OverlappingActors, AEquipment::StaticClass());
		for (AActor* Actor : OverlappingActors)
		{
			if (AEquipment* Equipment = Cast<AEquipment>(Actor))
			{
				if (Equipment != TargetEquipment && Equipment->EquipmentState == EEquipmentState::Dropped && Equipment->EnsureEquipmentAsset())
				{
					if (Equipment->EquipmentAsset->GetEquipmentSlot() == TargetEquipmentSlot)
					{
						OverlappingEquipment = Equipment;
						break;
					}
				}
			}
		}

		const EEquipmentSlot FallbackSlot = CombatComp->GetLastEquipment() ? CombatComp->LastEquipmentSlot : EEquipmentSlot::Slot3;

		if (HasAuthority(&ActivationInfo))
		{
			if (OverlappingEquipment)
			{
				// 抢到了地面同槽位武器：服务端直接装备，并激活切枪 GA（客户端在 OnRep_SlotXEquipment 收到新枪时自动触发切入动画）
				CombatComp->GiveEquipment(OverlappingEquipment);

				if (CombatComp->CurEquipmentSlot == TargetEquipmentSlot)
				{
					FGameplayEventData SwapPayload;
					SwapPayload.EventMagnitude = static_cast<float>(TargetEquipmentSlot);
					ASC->HandleGameplayEvent(TAG_EVENT_EQUIPMENT_SWAP, &SwapPayload);
				}
			}
			else
			{
				// 没抢到或脚下没有同类型武器：通知远端客户端回退切枪
				if (!HumanChar->IsLocallyControlled())
				{
					HumanChar->ClientSwapEquipmentWhenPickupFailed(FallbackSlot);
				}
			}
		}

		// 本地的切枪处理
		if (HumanChar->IsLocallyControlled())
		{
			if (!OverlappingEquipment)
			{
				// 本地检测到脚下没有可替换的同槽位武器：
				// 无论是房主还是客户端，都立即触发切换到备用槽位（客户端为本地预测，零延迟手感；房主为直接执行）
				HumanChar->SendSwapEquipmentEvent(FallbackSlot);
			}
			// 若脚下有同槽位武器：
			// 1. 房主（HasAuthority）：已在上方权威分支中执行了 GiveEquipment 并触发了切入动画，此处无需重复操作。
			// 2. 客户端（!HasAuthority）：保持等待，若在服务端成功抢到枪，将由 OnRep_SlotXEquipment 触发切入动画；若未抢到，将由 ClientSwapEquipmentWhenPickupFailed 通知回退切枪。
		}
	}
}
