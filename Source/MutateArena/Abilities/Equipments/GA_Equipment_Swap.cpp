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
#include "MutateArena/System/GameplayTags.h"

UGA_Equipment_Swap::UGA_Equipment_Swap()
{
	bRetriggerInstancedAbility = true; // 允许切枪中途再次按键重入打断
}

bool UGA_Equipment_Swap::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	// 跳过了 UGA_Equipment::CanActivateAbility
	if (!UGameplayAbilityBase::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	return true;
}

void UGA_Equipment_Swap::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
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

	TargetEquipmentSlot = static_cast<EEquipmentSlot>(TriggerEventData->EventMagnitude);
	TargetEquipment = CombatComp->GetEquipmentByType(TargetEquipmentSlot);

	if (!TargetEquipment || !TargetEquipment->EquipmentAsset)
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

	// 1. 通知旧装备开始切出（清理旧状态），并停止旧装备1P网格体上残留的动画
	if (AEquipment* CurEquipment = GetCurEquipment())
	{
		CurEquipment->OnStartSwapOut();

		if (HumanChar->IsLocallyControlled())
		{
			if (UAnimInstEquipment* CurEquipAnimInst1P = CurEquipment->GetEquipmentAnimInst1P())
			{
				CurEquipAnimInst1P->Montage_Stop(0.f);
			}
		}
	}

	// 2. 立即执行切枪（无切出动画）
	// 服务端与主控端立刻完成武器挂载与GA切换；3P模拟端通过 OnRep_CurEquipmentSlot 直接替换第三人称手部武器，不播放切入动画
	CombatComp->InstantSwap(TargetEquipmentSlot);

	// 3. 播放1P切入动画
	UAnimMontage* SwapInMontage_C = TargetEquipment->EquipmentAsset->GetSwapInMontage_C();
	if (!SwapInMontage_C)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	if (HumanChar->IsLocallyControlled())
	{
		// 监听1P角色切入动画里发出的通知（希望尽早开火，此时切入动画尾段可能尚未完全播完）
		UAbilityTask_WaitGameplayEvent* WaitSwapInEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, TAG_EVENT_EQUIPMENT_SWAPIN_END);
		WaitSwapInEvent->EventReceived.AddDynamic(this, &ThisClass::OnSwapInEventReceived);
		WaitSwapInEvent->ReadyForActivation();

		// 在1P手臂网格体（Mesh1P）上播放角色切入动画
		UAnimInstance* AnimInst1P = HumanChar->Mesh1P ? HumanChar->Mesh1P->GetAnimInstance() : nullptr;
		if (AnimInst1P)
		{
			CurrentSwapInMontage = SwapInMontage_C;
			const float PlayLength = AnimInst1P->Montage_Play(SwapInMontage_C, 1.0f);
			if (PlayLength > 0.f)
			{
				FOnMontageEnded EndDelegate;
				EndDelegate.BindUObject(this, &ThisClass::OnSwapInMontageEnded);
				AnimInst1P->Montage_SetEndDelegate(EndDelegate, SwapInMontage_C);
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
			if (UAnimMontage* SwapInMontage_E = TargetEquipment->EquipmentAsset->GetSwapInMontage_E())
			{
				EquipAnimInst1P->Montage_Play(SwapInMontage_E, 1.0f);
			}
		}
	}
	else if (HasAuthority(&ActivationInfo))
	{
		// 服务端处理远端客户端切枪：
		// 远端客户端的 Mesh1P 为 OnlyOwnerSee，在服务端不渲染且不触发 AnimNotify。
		// 正常流程下由客户端触发 OnSwapInEventReceived 调用 EndAbility(bReplicateEndAbility = true) 同步结束服务端技能；
		// 此处添加蒙太奇时长保底任务，防止客户端异常断线或丢包导致服务端卡在 Swapping Tag 状态。
		UAbilityTask_WaitDelay* WaitDelayTask = UAbilityTask_WaitDelay::WaitDelay(this, SwapInMontage_C->GetPlayLength() + 0.1f);
		WaitDelayTask->OnFinish.AddDynamic(this, &ThisClass::OnServerWaitDelayFinished);
		WaitDelayTask->ReadyForActivation();
	}
}

void UGA_Equipment_Swap::OnSwapInEventReceived(FGameplayEventData Payload)
{
	// bReplicateEndAbility = true 确保主控客户端结束技能时同步通知服务端结束
	// 此处不主动停止 Mesh1P 的切入蒙太奇，让动画尾段自然过渡完（若玩家立即开火，开火蒙太奇会自动覆盖）
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Equipment_Swap::OnSwapInMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != CurrentSwapInMontage) return;

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bInterrupted);
}

void UGA_Equipment_Swap::OnServerWaitDelayFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Equipment_Swap::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 解绑当前1P切入蒙太奇的结束委托，防止高频连切（bRetriggerInstancedAbility）重入时，
	// 上一段蒙太奇被新蒙太奇顶掉触发 OnMontageEnded 把新激活的切枪技能错误关闭
	if (CurrentSwapInMontage)
	{
		if (AHumanCharacter* HumanChar = GetHumanCharacter())
		{
			if (UAnimInstance* AnimInst1P = HumanChar->Mesh1P ? HumanChar->Mesh1P->GetAnimInstance() : nullptr)
			{
				if (FAnimMontageInstance* MontageInstance = AnimInst1P->GetActiveInstanceForMontage(CurrentSwapInMontage))
				{
					MontageInstance->OnMontageEnded.Unbind();
				}
			}
		}
		CurrentSwapInMontage = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}