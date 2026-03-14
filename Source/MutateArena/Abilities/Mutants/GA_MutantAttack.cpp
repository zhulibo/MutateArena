#include "GA_MutantAttack.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "MutateArena/Characters/MutantCharacter.h"
#include "MutateArena/System/Tags/ProjectTags.h"

UGA_MutantAttack::UGA_MutantAttack()
{
}

void UGA_MutantAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	AMutantCharacter* MutantChar = Cast<AMutantCharacter>(ActorInfo->AvatarActor.Get());

	if (!ASC || !MutantChar || !TriggerEventData)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	bAttackButtonReleased = false;
	bIsLightAttack = TriggerEventData->EventMagnitude == 1.f; // 1.f 轻击 2.f 重击
	MutantChar->bIsLightAttack = bIsLightAttack;

	if (bIsLightAttack)
	{
		UAbilityTask_WaitGameplayEvent* WaitReleaseTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, TAG_EVENT_MUTANT_ATTACK_RELEASED);
		WaitReleaseTask->EventReceived.AddDynamic(this, &UGA_MutantAttack::OnAttackButtonReleased);
		WaitReleaseTask->ReadyForActivation();
		
		UAbilityTask_WaitGameplayEvent* WaitEndTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, TAG_EVENT_MUTANT_ATTACK_FIRSTSECTIONEND);
		WaitEndTask->EventReceived.AddDynamic(this, &UGA_MutantAttack::OnFirstSectionEnd);
		WaitEndTask->ReadyForActivation();
	}

	PlayAttackMontage();
}

void UGA_MutantAttack::PlayAttackMontage()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	AMutantCharacter* MutantChar = Cast<AMutantCharacter>(CurrentActorInfo->AvatarActor.Get());

	if (!ASC || !MutantChar)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, bIsLightAttack ? MutantChar->LightAttackMontage : MutantChar->HeavyAttackMontage);
	MontageTask->OnCompleted.AddDynamic(this, &UGA_MutantAttack::OnMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &UGA_MutantAttack::OnMontageInterruptedOrCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &UGA_MutantAttack::OnMontageInterruptedOrCancelled);
	MontageTask->ReadyForActivation();
}

void UGA_MutantAttack::OnAttackButtonReleased(FGameplayEventData Payload)
{
	if (bIsLightAttack)
	{
		bAttackButtonReleased = true;
	}
}

void UGA_MutantAttack::OnFirstSectionEnd(FGameplayEventData Payload)
{
	AMutantCharacter* MutantChar = Cast<AMutantCharacter>(CurrentActorInfo->AvatarActor.Get());
	if (MutantChar && bIsLightAttack)
	{
		MutantChar->RightHandHitEnemies.Empty();
		MutantChar->LeftHandHitEnemies.Empty();

		if (bAttackButtonReleased)
		{
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		}
	}
}

void UGA_MutantAttack::OnMontageCompleted()
{
	AMutantCharacter* MutantChar = Cast<AMutantCharacter>(CurrentActorInfo->AvatarActor.Get());

	if (MutantChar && bIsLightAttack && !bAttackButtonReleased)
	{
		// 轻击连击
		MutantChar->RightHandHitEnemies.Empty();
		MutantChar->LeftHandHitEnemies.Empty();
		PlayAttackMontage(); 
	}
	else
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UGA_MutantAttack::OnMontageInterruptedOrCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_MutantAttack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (AMutantCharacter* MutantChar = Cast<AMutantCharacter>(ActorInfo->AvatarActor.Get()))
	{
		MutantChar->RightHandAttackEnd();
		MutantChar->LeftHandAttackEnd();
		MutantChar->RightHandHitEnemies.Empty();
		MutantChar->LeftHandHitEnemies.Empty();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
