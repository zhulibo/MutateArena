#include "GA_Melee_Attack.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Equipments/Melee.h"
#include "MutateArena/System/Tags/ProjectTags.h"

UGA_Melee_Attack::UGA_Melee_Attack()
{
}

void UGA_Melee_Attack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	AMelee* CurMelee = GetCurMelee();

	if (!ASC || !CurMelee || !TriggerEventData)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AttackButtonReleased = false;
	CurMelee->bIsLightAttack = TriggerEventData->EventMagnitude == 1.f; // 1.f 轻击 2.f 重击

	if (CurMelee->bIsLightAttack)
	{
		UAbilityTask_WaitGameplayEvent* WaitReleaseTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, TAG_EVENT_MELEE_RELEASED);
		WaitReleaseTask->EventReceived.AddDynamic(this, &UGA_Melee_Attack::OnAttackButtonReleased);
		WaitReleaseTask->ReadyForActivation();
		
		UAbilityTask_WaitGameplayEvent* WaitEndTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, TAG_EVENT_MELEE_FIRSTSECTIONEND);
		WaitEndTask->EventReceived.AddDynamic(this, &UGA_Melee_Attack::OnFirstSectionEnd);
		WaitEndTask->ReadyForActivation();
	}

	PlayMeleeMontage();
}

void UGA_Melee_Attack::PlayMeleeMontage()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	AHumanCharacter* HumanChar = GetHumanCharacter();
	AMelee* CurMelee = GetCurMelee();

	if (!ASC || !HumanChar || !CurMelee)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	// 播放角色蒙太奇
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, CurMelee->bIsLightAttack ? CurMelee->LightAttackMontage_C : CurMelee->HeavyAttackMontage_C);
	MontageTask->OnCompleted.AddDynamic(this, &UGA_Melee_Attack::OnMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &UGA_Melee_Attack::OnMontageInterruptedOrCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &UGA_Melee_Attack::OnMontageInterruptedOrCancelled);
	MontageTask->ReadyForActivation();

	// 播放武器蒙太奇
	FGameplayCueParameters CueParams;
	CueParams.Instigator = HumanChar;
	CueParams.SourceObject = CurMelee;
	CueParams.RawMagnitude = CurMelee->bIsLightAttack ? 1.f : 2.f;

	// 注意：因为是循环播放，如果要让 GC 的 OnActive 再次触发，须先 Remove 再 Add
	ASC->RemoveGameplayCue(TAG_GC_MELEE_ATTACK);
	ASC->AddGameplayCue(TAG_GC_MELEE_ATTACK, CueParams);
}

void UGA_Melee_Attack::OnAttackButtonReleased(FGameplayEventData Payload)
{
	AMelee* CurMelee = GetCurMelee();
	if (CurMelee && CurMelee->bIsLightAttack)
	{
		AttackButtonReleased = true;
	}
}

void UGA_Melee_Attack::OnFirstSectionEnd(FGameplayEventData Payload)
{
	AMelee* CurMelee = GetCurMelee();
	if (CurMelee && CurMelee->bIsLightAttack)
	{
		CurMelee->ClearHitEnemies();

		if (AttackButtonReleased)
		{
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		}
	}
}

void UGA_Melee_Attack::OnMontageCompleted()
{
	AMelee* CurMelee = GetCurMelee();

	if (CurMelee && CurMelee->bIsLightAttack && !AttackButtonReleased)
	{
		CurMelee->ClearHitEnemies();
		PlayMeleeMontage(); // 再次播放
	}
	else
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UGA_Melee_Attack::OnMontageInterruptedOrCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_Melee_Attack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveGameplayCue(TAG_GC_MELEE_ATTACK);
	}

	if (AMelee* CurMelee = GetCurMelee())
	{
		CurMelee->SetAttackCollisionEnabled(false);
		CurMelee->ClearHitEnemies();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
