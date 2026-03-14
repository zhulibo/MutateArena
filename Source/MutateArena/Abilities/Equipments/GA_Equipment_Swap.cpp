#include "GA_Equipment_Swap.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Characters/Components/CombatComponent.h"
#include "MutateArena/Equipments/AnimInstEquipment.h"
#include "MutateArena/Equipments/Equipment.h"
#include "MutateArena/System/Tags/ProjectTags.h"

UGA_Equipment_Swap::UGA_Equipment_Swap()
{
	bRetriggerInstancedAbility = true; // 可被打断
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
	
	// 获取这次按键传进来的目标装备类型
	TargetEquipmentType = static_cast<EEquipmentType>(TriggerEventData->EventMagnitude);
	TargetEquipment = CombatComp->GetEquipmentByType(TargetEquipmentType);
    
	if (!TargetEquipment)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 更新切枪意图
	CombatComp->DesiredEquipmentType = TargetEquipmentType;

	// 如果想切的目标就是手里这把枪（切枪中途又切回）
	if (CombatComp->CurEquipmentType == TargetEquipmentType)
	{
		bool bIsPlayingSwapOut = false;

		// 检查角色当前是否正在播放切出动画
		if (UAnimInstance* HumanAnim = HumanChar->GetMesh()->GetAnimInstance())
		{
			if (HumanAnim->Montage_IsPlaying(TargetEquipment->SwapOutMontage_C))
			{
				bIsPlayingSwapOut = true;
			}
		}

		if (bIsPlayingSwapOut)
		{
			// 倒放切出动画
			FGameplayCueParameters CueParams;
			CueParams.Instigator = HumanChar;
			CueParams.SourceObject = TargetEquipment; 
			ASC->ExecuteGameplayCue(TAG_GC_EQUIPMENT_SWAPOUTREVERSE, CueParams);
          
			EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		}
		else
		{
			// 正常手持状态下按了相同的武器键，直接结束
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		}
		return;
	}

	if (AEquipment* CurEquipment = GetCurEquipment())
	{
		CurEquipment->OnStartSwapOut();

		// 监听角色切出动画里发出的通知
		UAbilityTask_WaitGameplayEvent* WaitSwapOutEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, TAG_EVENT_EQUIPMENT_SWAPOUT_END);
		WaitSwapOutEvent->EventReceived.AddDynamic(this, &ThisClass::OnSwapOutEventReceived);
		WaitSwapOutEvent->ReadyForActivation();

		bIsPlayingSwapIn = false;
		
		// 播放角色切出动画 bStopWhenAbilityEnds = false 保证逆向播放时的动画连续
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, CurEquipment->SwapOutMontage_C, 1.f, NAME_None, false);
		MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnSwapOutMontageFinished);
		MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnSwapOutMontageFinished);
		MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnSwapOutMontageFinished);
		MontageTask->ReadyForActivation();

		// 播放装备切出动画
		FGameplayCueParameters CueParams;
		CueParams.Instigator = HumanChar;
		CueParams.SourceObject = CurEquipment;
		ASC->ExecuteGameplayCue(TAG_GC_EQUIPMENT_SWAPOUT, CueParams);
	}
	else
	{
		// 投掷装备扔出后切换到上一个武器 / 开局赋予武器时，当前武器为空
		OnSwapOutEventReceived(FGameplayEventData());
	}
}

void UGA_Equipment_Swap::OnSwapOutEventReceived(FGameplayEventData Payload)
{
	// UE_LOG(LogTemp, Warning, TEXT("OnSwapOutEventReceived %d"), UE::GetPlayInEditorID());
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	AHumanCharacter* HumanChar = GetHumanCharacter();
	UCombatComponent* CombatComp = HumanChar ? HumanChar->CombatComp : nullptr;
	AEquipment* CurEquipment = GetCurEquipment();
	if (!ASC || !CombatComp || !TargetEquipment || !CurEquipment) return;
	
	// 切换装备
	CombatComp->InstantSwap(TargetEquipment->EquipmentType);

	if (TargetEquipment->SwapInMontage_C)
	{
		bIsPlayingSwapIn = true;

		// 监听角色切入动画里发出的通知 希望尽早开火 可能切入动画未完成
		UAbilityTask_WaitGameplayEvent* WaitSwapInEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
				this, TAG_EVENT_EQUIPMENT_SWAPIN_END);
		WaitSwapInEvent->EventReceived.AddDynamic(this, &ThisClass::OnSwapInEventReceived);
		WaitSwapInEvent->ReadyForActivation();
		
		// 播放角色切入动画 bStopWhenAbilityEnds = false 保证 OnSwapInEventReceived 结束技能时动画依然播放到末尾
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, TargetEquipment->SwapInMontage_C, 1.0f, NAME_None, false);
		MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnSwapInMontageFinished);
		MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnSwapInMontageFinished);
		MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnSwapInMontageFinished);
		MontageTask->ReadyForActivation();
		
		// 播放装备切入动画
		FScopedPredictionWindow PredictionWindow(ASC, HumanChar->IsLocallyControlled() && !HumanChar->HasAuthority());
		FGameplayCueParameters CueParams;
		CueParams.Instigator = HumanChar;
		if (HumanChar->HasAuthority())
		{
			CueParams.RawMagnitude = 1.f; // HACK 标记供GC区分
		}
		CueParams.SourceObject = TargetEquipment;
		ASC->ExecuteGameplayCue(TAG_GC_EQUIPMENT_SWAPIN, CueParams);
	}
	else
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}

void UGA_Equipment_Swap::OnSwapInEventReceived(FGameplayEventData Payload)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_Equipment_Swap::OnSwapOutMontageFinished()
{
	if (!bIsPlayingSwapIn)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}

void UGA_Equipment_Swap::OnSwapInMontageFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_Equipment_Swap::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
