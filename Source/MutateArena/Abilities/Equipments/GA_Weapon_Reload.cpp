#include "GA_Weapon_Reload.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Characters/Components/CombatComponent.h"
#include "MutateArena/Equipments/Weapon.h"
#include "MutateArena/Equipments/Data/EquipmentAsset.h"
#include "MutateArena/System/GameplayTags.h"

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
	UCombatComponent* CombatComp = HumanChar ? HumanChar->CombatComp : nullptr;
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
	bHasReloaded = false;

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
	
	if (CurWeapon->FragWeapon->ReloadMontage_C)
	{
		// 监听换弹完成通知 希望尽早开火 可能换单动画未完成
		UAbilityTask_WaitGameplayEvent* WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, TAG_EVENT_WEAPON_RELOAD_END);
		WaitEventTask->EventReceived.AddDynamic(this, &ThisClass::OnReloadEventReceived);
		WaitEventTask->ReadyForActivation();
		
		// 播放角色换弹动画 bStopWhenAbilityEnds = false 保证 OnReloadEventReceived 结束技能时动画依然播放到末尾
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, CurWeapon->FragWeapon->ReloadMontage_C, 1.f, NAME_None, false);
		MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
		MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageCancelled);
		MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageCancelled);
		MontageTask->ReadyForActivation();
		
		// 播放武器换弹动画
		FGameplayCueParameters CueParams;
		CueParams.Instigator = HumanChar;
		CueParams.SourceObject = CurWeapon;
		ASC->ExecuteGameplayCue(TAG_GC_WEAPON_RELOAD, CueParams);
	}
	else
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

void UGA_Weapon_Reload::OnReloadEventReceived(FGameplayEventData Payload)
{
	ReloadAmmo();
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Weapon_Reload::OnMontageCompleted()
{
	ReloadAmmo();
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Weapon_Reload::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_Weapon_Reload::ReloadAmmo()
{
	if (bHasReloaded)
	{
		return;
	}

	AHumanCharacter* HumanChar = GetHumanCharacter();
	UCombatComponent* CombatComp = HumanChar ? HumanChar->CombatComp : nullptr;
	AWeapon* CurWeapon = GetCurWeapon();

	if (!HumanChar || !CombatComp || !CurWeapon || !CurWeapon->FragWeapon)
	{
		return;
	}

	bHasReloaded = true;

	const int32 Ammo = CurWeapon->Ammo;
	const int32 MagCapacity = CurWeapon->FragWeapon->MagCapacity;
	const int32 CarriedAmmo = CurWeapon->CarriedAmmo;
	const int32 NeedAmmoNum = MagCapacity - Ammo;

	if (NeedAmmoNum <= 0 || CarriedAmmo <= 0)
	{
		return;
	}

	if (CarriedAmmo >= NeedAmmoNum)
	{
		CurWeapon->SetAmmo(MagCapacity);
		CurWeapon->SetCarriedAmmo(CarriedAmmo - NeedAmmoNum);
	}
	else
	{
		CurWeapon->SetAmmo(Ammo + CarriedAmmo);
		CurWeapon->SetCarriedAmmo(0);
	}
}

void UGA_Weapon_Reload::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 换弹完成通知（Event.Weapon.Reload.End）挂在 1P 手臂蒙太奇（ReloadMontage1P_C）上，仅在本地控制端播放并触发；
	// 当非服务端的客户端收到通知并调用 EndAbility(bWasCancelled = false) 同步到服务端时，服务端在此处完成权威弹药补充。
	if (!bWasCancelled && !bHasReloaded)
	{
		ReloadAmmo();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
