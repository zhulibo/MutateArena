#include "GA_Throwable_Throw.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Characters/Components/CombatComponent.h"
#include "MutateArena/Equipments/Throwable.h"
#include "MutateArena/Equipments/Data/EquipmentAsset.h"
#include "MutateArena/System/GameplayTags.h"

UGA_Throwable_Throw::UGA_Throwable_Throw()
{
}

bool UGA_Throwable_Throw::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	// 确保当前拿在手里的是投掷物
	AThrowable* ThrowableEquip = Cast<AThrowable>(GetCurEquipment());
	if (!ThrowableEquip)
	{
		return false;
	}

	return true;
}

void UGA_Throwable_Throw::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	AHumanCharacter* HumanChar = GetHumanCharacter();
	AThrowable* ThrowableEquip = Cast<AThrowable>(GetCurEquipment());

	if (!ASC || !HumanChar || !ThrowableEquip || !ThrowableEquip->FragThrowable)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	bHasThrownOut = false;

	if (ThrowableEquip->FragThrowable->ThrowMontage_C)
	{
		// 出手时机统一由 3P 角色蒙太奇（ThrowMontage_C）的 Notify 发出 THROWOUT 事件驱动，
		// 3P BodyMesh 在第一人称下虽隐藏但仍在 Tick，所以各端时机一致；1P 蒙太奇需与其出手帧对齐，仅做表现。
		UAbilityTask_WaitGameplayEvent* WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, TAG_EVENT_THROWABLE_THROWOUT);
		WaitEventTask->EventReceived.AddDynamic(this, &ThisClass::OnThrowOutEventReceived);
		WaitEventTask->ReadyForActivation();

		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, ThrowableEquip->FragThrowable->ThrowMontage_C, 1.f, NAME_None, false);
		MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageFinished);
		MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageFinished);
		MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageFinished);
		MontageTask->ReadyForActivation();

		// GC 播放其他网格体动画（3P 投掷物 + 1P 手臂 + 1P 投掷物）
		FGameplayCueParameters CueParams;
		CueParams.Instigator = HumanChar;
		CueParams.SourceObject = ThrowableEquip;
		ASC->ExecuteGameplayCue(TAG_GC_THROWABLE_THROW, CueParams);
	}
	else
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

void UGA_Throwable_Throw::OnThrowOutEventReceived(FGameplayEventData Payload)
{
	AHumanCharacter* HumanChar = GetHumanCharacter();
	UCombatComponent* CombatComp = HumanChar ? HumanChar->CombatComp : nullptr;
	AThrowable* ThrowableEquip = Cast<AThrowable>(GetCurEquipment());

	if (ThrowableEquip && CombatComp)
	{
		ThrowableEquip->ThrowOut();

		CombatComp->Slot4Equipment = nullptr;
		bHasThrownOut = true;
	}
}

void UGA_Throwable_Throw::OnMontageFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Throwable_Throw::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	AHumanCharacter* HumanChar = GetHumanCharacter();
	UCombatComponent* CombatComp = HumanChar ? HumanChar->CombatComp : nullptr;

	// 必须加上 !bWasCancelled 切换装备会取消投掷造成死循环
	if (!bWasCancelled && bHasThrownOut && HumanChar && HumanChar->IsLocallyControlled() && CombatComp)
	{
		HumanChar->SendSwapEquipmentEvent(CombatComp->LastEquipmentSlot);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
