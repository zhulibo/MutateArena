#include "GA_Equipment_Swap.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Characters/Components/CombatComponent.h"
#include "MutateArena/Equipments/AnimInstEquipment.h"
#include "MutateArena/Equipments/Equipment.h"
#include "MutateArena/Equipments/Data/EquipmentAsset.h"
#include "MutateArena/Equipments/Data/EquipmentType.h"
#include "MutateArena/System/GameplayTags.h"

UGA_Equipment_Swap::UGA_Equipment_Swap()
{
	bRetriggerInstancedAbility = true; // 允许切枪中途再次按键重入打断
}

bool UGA_Equipment_Swap::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
                                            const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	// 跳过了 UGA_Equipment::CanActivateAbility
	if (!UGameplayAbilityBase::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	return true;
}

void UGA_Equipment_Swap::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                         const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
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

	if (!TargetEquipment || !TargetEquipment->EnsureEquipmentAsset())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 如果要切的目标就是当前手里的武器槽位，直接结束
	if (CombatComp->CurEquipmentSlot == TargetEquipmentSlot)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 通知旧装备开始切出（清理旧状态），并停止旧装备在 Mesh1P 和 EquipmentMesh1P 上残留的动画
	if (AEquipment* OldEquipment = GetCurEquipment())
	{
		OldEquipment->OnStartSwapOut();

		if (HumanChar->IsLocallyControlled())
		{
			if (UAnimInstance* AnimInst1P = HumanChar->Mesh1P ? HumanChar->Mesh1P->GetAnimInstance() : nullptr)
			{
				if (OldEquipment->EquipmentAsset)
				{
					if (UAnimMontage* OldEquipMontage_C = OldEquipment->EquipmentAsset->GetEquipMontage1P_C())
					{
						if (FAnimMontageInstance* OldMontageInst = AnimInst1P->GetActiveInstanceForMontage(OldEquipMontage_C))
						{
							OldMontageInst->OnMontageEnded.Unbind();
						}
						AnimInst1P->Montage_Stop(0.f, OldEquipMontage_C);
					}
				}
			}

			if (UAnimInstEquipment* OldEquipAnimInst1P = OldEquipment->GetEquipmentAnimInst1P())
			{
				OldEquipAnimInst1P->Montage_Stop(0.f);
			}
		}
	}

	// 执行切枪（无切出动画）
	CombatComp->InstantSwap(TargetEquipmentSlot);

	// 播放1P切入动画
	UAnimMontage* EquipMontage_C = TargetEquipment->EquipmentAsset->GetEquipMontage1P_C();
	if (!EquipMontage_C)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	if (HumanChar->IsLocallyControlled())
	{
		// 监听1P角色切入动画里发出的通知（希望尽早开火，此时切入动画尾段可能尚未完全播完）
		UAbilityTask_WaitGameplayEvent* WaitEquipEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, TAG_EVENT_EQUIPMENT_EQUIP_END);
		WaitEquipEvent->EventReceived.AddDynamic(this, &ThisClass::OnEquipEventReceived);
		WaitEquipEvent->ReadyForActivation();

		// 在1P手臂网格体（Mesh1P）上播放角色切入动画
		if (UAnimInstance* AnimInst1P = HumanChar->Mesh1P ? HumanChar->Mesh1P->GetAnimInstance() : nullptr)
		{
			if (FAnimMontageInstance* ExistingInst = AnimInst1P->GetActiveInstanceForMontage(EquipMontage_C))
			{
				ExistingInst->OnMontageEnded.Unbind();
				AnimInst1P->Montage_Stop(0.f, EquipMontage_C);
			}

			const float PlayLength = AnimInst1P->Montage_Play(EquipMontage_C, 1.0f);
			if (PlayLength > 0.f)
			{
				FOnMontageEnded EndDelegate;
				EndDelegate.BindUObject(this, &ThisClass::OnEquipMontageEnded);
				AnimInst1P->Montage_SetEndDelegate(EndDelegate, EquipMontage_C);
			}
			else
			{
				EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
				return;
			}
		}
		else
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
			return;
		}

		// 在1P武器网格体（EquipmentMesh1P）上播放装备切入动画
		if (UAnimInstEquipment* EquipAnimInst1P = TargetEquipment->GetEquipmentAnimInst1P())
		{
			if (UAnimMontage* EquipMontage_E = TargetEquipment->EquipmentAsset->GetEquipMontage1P_E())
			{
				EquipAnimInst1P->Montage_Stop(0.f);
				EquipAnimInst1P->Montage_Play(EquipMontage_E, 1.0f);
			}
		}
	}
	else if (HasAuthority(&ActivationInfo))
	{
		// 考虑 RateScale 对实际播放时长的影响
		const float RateScale = FMath::Max(KINDA_SMALL_NUMBER, EquipMontage_C->RateScale);
		const float ActualPlayLength = EquipMontage_C->GetPlayLength() / RateScale;

		UAbilityTask_WaitDelay* WaitDelayTask = UAbilityTask_WaitDelay::WaitDelay(this, ActualPlayLength + 0.1f);
		WaitDelayTask->OnFinish.AddDynamic(this, &ThisClass::OnServerWaitDelayFinished);
		WaitDelayTask->ReadyForActivation();
	}
}

void UGA_Equipment_Swap::OnEquipEventReceived(FGameplayEventData Payload)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Equipment_Swap::OnEquipMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	// 严格校验：只有当前手持武器的切入蒙太奇结束时，才结束当前 Ability（防止旧蒙太奇延迟触发回调误杀新切枪）
	if (AEquipment* CurEquipment = GetCurEquipment())
	{
		if (CurEquipment->EquipmentAsset && CurEquipment->EquipmentAsset->GetEquipMontage1P_C() == Montage)
		{
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bInterrupted);
		}
	}
}

void UGA_Equipment_Swap::OnServerWaitDelayFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Equipment_Swap::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                    const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (AEquipment* CurEquipment = GetCurEquipment())
	{
		if (CurEquipment->EquipmentAsset)
		{
			if (UAnimMontage* EquipMontage_C = CurEquipment->EquipmentAsset->GetEquipMontage1P_C())
			{
				if (AHumanCharacter* HumanChar = GetHumanCharacter())
				{
					if (UAnimInstance* AnimInst1P = HumanChar->Mesh1P ? HumanChar->Mesh1P->GetAnimInstance() : nullptr)
					{
						// 只有当前确实在播放该切入蒙太奇时才解绑与停止（避免前置检查失败提前退出时误停旧武器的其他动画）
						if (FAnimMontageInstance* MontageInstance = AnimInst1P->GetActiveInstanceForMontage(EquipMontage_C))
						{
							MontageInstance->OnMontageEnded.Unbind();

							if (bWasCancelled)
							{
								AnimInst1P->Montage_Stop(0.f, EquipMontage_C);

								if (UAnimInstEquipment* EquipAnimInst1P = CurEquipment->GetEquipmentAnimInst1P())
								{
									EquipAnimInst1P->Montage_Stop(0.f, CurEquipment->EquipmentAsset->GetEquipMontage1P_E());
								}
							}
						}
					}
				}
			}
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
