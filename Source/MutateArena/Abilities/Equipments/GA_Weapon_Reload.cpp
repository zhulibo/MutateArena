#include "GA_Weapon_Reload.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Characters/Components/CombatComponent.h"
#include "MutateArena/Equipments/Weapon.h"
#include "MutateArena/System/Tags/ProjectTags.h"

UGA_Weapon_Reload::UGA_Weapon_Reload()
{
}

bool UGA_Weapon_Reload::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}
	
	AHumanCharacter* HumanChar = GetHumanCharacter();
	UCombatComponent* CombatComp = HumanChar->CombatComp;
	AWeapon* CurWeapon = GetCurWeapon();

	if (!HumanChar || !CurWeapon || !CombatComp)
	{
		return false;
	}

	if (CurWeapon->IsFull())
	{
		return false;
	}

	if (CurWeapon->CarriedAmmo <= 0)
	{
		return false;
	}

	return true;
}

void UGA_Weapon_Reload::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	AHumanCharacter* HumanChar = GetHumanCharacter();
	AWeapon* CurWeapon = GetCurWeapon();
	UCombatComponent* CombatComp = HumanChar ? HumanChar->CombatComp : nullptr;
	
	if (!ASC || !HumanChar || !CurWeapon || !CombatComp)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	if (CurWeapon->ReloadMontage_C)
	{
		// 监听换弹完成通知 希望尽早开火 可能换单动画未完成
		UAbilityTask_WaitGameplayEvent* WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, TAG_EVENT_WEAPON_RELOAD_END);
		WaitEventTask->EventReceived.AddDynamic(this, &ThisClass::OnReloadEventReceived);
		WaitEventTask->ReadyForActivation();
		
		// 播放角色换弹动画 bStopWhenAbilityEnds = false 保证 OnReloadEventReceived 结束技能时动画依然播放到末尾
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, CurWeapon->ReloadMontage_C, 1.f, NAME_None, false);
		MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageFinished);
		MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageFinished);
		MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageFinished);
		MontageTask->ReadyForActivation();
		
		// 播放武器换弹动画
		FGameplayCueParameters CueParams;
		CueParams.Instigator = HumanChar;
		CueParams.SourceObject = CurWeapon;
		ASC->ExecuteGameplayCue(TAG_GC_WEAPON_RELOAD, CueParams);
	}
	else
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

void UGA_Weapon_Reload::OnReloadEventReceived(FGameplayEventData Payload)
{
	AHumanCharacter* HumanChar = GetHumanCharacter();
	UCombatComponent* CombatComp = HumanChar ? HumanChar->CombatComp : nullptr;
	AWeapon* CurWeapon = GetCurWeapon();

	if (!HumanChar || !CombatComp || !CurWeapon)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	int32 Ammo = GetCurWeapon()->Ammo;
	int32 MagCapacity = GetCurWeapon()->MagCapacity;
	int32 CarriedAmmo = GetCurWeapon()->CarriedAmmo;
	int32 NeedAmmoNum = MagCapacity - Ammo;

	if (CarriedAmmo >= NeedAmmoNum)
	{
		GetCurWeapon()->SetAmmo(MagCapacity);
		GetCurWeapon()->SetCarriedAmmo(CarriedAmmo - NeedAmmoNum);
	}
	else
	{
		GetCurWeapon()->SetAmmo(Ammo + CarriedAmmo);
		GetCurWeapon()->SetCarriedAmmo(0);
	}
	
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Weapon_Reload::OnMontageFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Weapon_Reload::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
