#include "GA_Weapon_Reload_Shotgun.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Equipments/Weapon.h"
#include "MutateArena/System/Tags/ProjectTags.h"

UGA_Weapon_Reload_Shotgun::UGA_Weapon_Reload_Shotgun()
{
}

void UGA_Weapon_Reload_Shotgun::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	AHumanCharacter* HumanChar = GetHumanCharacter();
	AWeapon* CurWeapon = GetCurWeapon();

	if (!ASC || !HumanChar || !CurWeapon)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (CurWeapon->ReloadMontage_C)
	{
		// 监听换弹通知
		UAbilityTask_WaitGameplayEvent* WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, TAG_EVENT_WEAPON_RELOAD_INSERT);
		WaitEventTask->EventReceived.AddDynamic(this, &ThisClass::OnShellInsertEventReceived);
		WaitEventTask->ReadyForActivation();
		
		// 播放角色换弹动画
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

void UGA_Weapon_Reload_Shotgun::OnShellInsertEventReceived(FGameplayEventData Payload)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	AHumanCharacter* HumanChar = GetHumanCharacter();
	AWeapon* CurWeapon = GetCurWeapon();

	if (!ASC || !HumanChar || !CurWeapon)
	{
		return;
	}

	int32 Ammo = CurWeapon->Ammo;
	int32 MagCapacity = CurWeapon->MagCapacity;
	int32 CarriedAmmo = CurWeapon->CarriedAmmo;

	// 执行单发装弹逻辑
	if (CarriedAmmo > 0 && Ammo < MagCapacity)
	{
		CurWeapon->SetAmmo(Ammo + 1);
		CurWeapon->SetCarriedAmmo(CarriedAmmo - 1);
	}

	if (CurWeapon->IsFull() || CurWeapon->CarriedAmmo <= 0)
	{
		MontageJumpToSection(SECTION_RELOAD_END);
		
		// 让枪械蒙太奇跳到最后
		FGameplayCueParameters CueParams;
		CueParams.Instigator = HumanChar;
		CueParams.SourceObject = CurWeapon;
		ASC->ExecuteGameplayCue(TAG_GC_WEAPON_RELOADEND, CueParams);
	}
}
