#include "GA_MutantAttack.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "MutateArena/Characters/MutantCharacter.h"
#include "MutateArena/Characters/Data/CharacterAsset.h"
#include "MutateArena/System/GameplayTags.h"

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
	bFirstSectionEnded = false;
	bMontageCompleted = false;
	bIsLightAttack = FMath::IsNearlyEqual(TriggerEventData->EventMagnitude, 1.f); // 1.f 轻击 | 2.f 重击
	MutantChar->bIsLightAttack = bIsLightAttack;

	// 轻击机制：一个蒙太奇内包含两段挥爪。
	// - 点按（第一段结束前松手）：播完第一段立即收手结束。
	// - 长按（一直按住不松手）：第一段 -> 第二段 -> 播完后无缝重头循环。
	if (bIsLightAttack)
	{
		// 1. 监听本机松开攻击键事件
		UAbilityTask_WaitGameplayEvent* WaitReleaseTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, TAG_EVENT_MUTANT_ATTACK_RELEASED);
		WaitReleaseTask->EventReceived.AddDynamic(this, &UGA_MutantAttack::OnAttackButtonReleased);
		WaitReleaseTask->ReadyForActivation();
		
		// 2. 监听蒙太奇第一段挥爪结束通知
		UAbilityTask_WaitGameplayEvent* WaitEndTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, TAG_EVENT_MUTANT_ATTACK_FIRSTSECTIONEND);
		WaitEndTask->EventReceived.AddDynamic(this, &UGA_MutantAttack::OnFirstSectionEnd);
		WaitEndTask->ReadyForActivation();

		// 3. 服务端注册监听：接收客机发来的松开攻击键 RPC
		if (HasAuthority(&ActivationInfo))
		{
			ASC->AbilityReplicatedEventDelegate(
				EAbilityGenericReplicatedEvent::InputReleased,
				Handle,
				ActivationInfo.GetActivationPredictionKey()
			).AddUObject(this, &ThisClass::OnServerAttackButtonReleased);
		}
	}

	PlayAttackMontage();
}

void UGA_MutantAttack::PlayAttackMontage()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	AMutantCharacter* MutantChar = Cast<AMutantCharacter>(CurrentActorInfo->AvatarActor.Get());

	if (!ASC || !MutantChar || !MutantChar->MutantAsset)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	// 每轮挥爪开始时重置状态标记（支持长按轻击无限循环）
	bFirstSectionEnded = false;
	bMontageCompleted = false;

	// 若处于长按循环连击中，先解绑并清理上一轮的蒙太奇任务，防止停止旧动画时误触发中断回调把技能自己结束掉
	if (CurrentMontageTask)
	{
		CurrentMontageTask->OnCompleted.RemoveAll(this);
		CurrentMontageTask->OnInterrupted.RemoveAll(this);
		CurrentMontageTask->OnCancelled.RemoveAll(this);
		CurrentMontageTask->EndTask();
		CurrentMontageTask = nullptr;
		ASC->CurrentMontageStop(0.f);
	}

	// 1. 播放 3P 攻击蒙太奇（若过渡期尚未配置 3P 蒙太奇，退回使用 1P 蒙太奇驱动技能生命周期）
	UAnimMontage* Montage3P = bIsLightAttack ? MutantChar->MutantAsset->LightAttackMontage : MutantChar->MutantAsset->HeavyAttackMontage;
	if (!Montage3P)
	{
		Montage3P = bIsLightAttack ? MutantChar->MutantAsset->LightAttackMontage1P : MutantChar->MutantAsset->HeavyAttackMontage1P;
	}
	if (!Montage3P)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	CurrentMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, Montage3P);
	CurrentMontageTask->OnCompleted.AddDynamic(this, &UGA_MutantAttack::OnMontageCompleted);
	CurrentMontageTask->OnInterrupted.AddDynamic(this, &UGA_MutantAttack::OnMontageInterruptedOrCancelled);
	CurrentMontageTask->OnCancelled.AddDynamic(this, &UGA_MutantAttack::OnMontageInterruptedOrCancelled);
	CurrentMontageTask->ReadyForActivation();

	// 2. 当前视角目标同步播放 1P 手臂攻击蒙太奇
	if (MutantChar->bIsCurrentViewTarget && MutantChar->Mesh1P)
	{
		UAnimMontage* Montage1P = bIsLightAttack ? MutantChar->MutantAsset->LightAttackMontage1P : MutantChar->MutantAsset->HeavyAttackMontage1P;
		if (Montage1P)
		{
			if (UAnimInstance* AnimInst1P = MutantChar->Mesh1P->GetAnimInstance())
			{
				AnimInst1P->Montage_Stop(0.f, Montage1P);
				AnimInst1P->Montage_Play(Montage1P);
			}
		}
	}
}

void UGA_MutantAttack::OnAttackButtonReleased(FGameplayEventData Payload)
{
	if (bIsLightAttack)
	{
		bAttackButtonReleased = true;

		// 客机本地松手后，发送 InputReleased RPC 通知服务端同步松手状态
		if (!HasAuthority(&CurrentActivationInfo))
		{
			if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
			{
				ASC->ServerSetReplicatedEvent(
					EAbilityGenericReplicatedEvent::InputReleased,
					CurrentSpecHandle,
					CurrentActivationInfo.GetActivationPredictionKey(),
					ASC->ScopedPredictionKey
				);
			}
		}
	}
}

void UGA_MutantAttack::OnServerAttackButtonReleased()
{
	if (bIsLightAttack)
	{
		bAttackButtonReleased = true;
	}
}

void UGA_MutantAttack::OnFirstSectionEnd(FGameplayEventData Payload)
{
	// 防止单轮挥爪内重复触发
	if (!bIsLightAttack || bFirstSectionEnded)
	{
		return;
	}

	bFirstSectionEnded = true;

	// 第一段挥爪结束，清空已命中列表，让接下来的第二段挥爪能再次对同一目标造成伤害和贴花
	if (AMutantCharacter* MutantChar = Cast<AMutantCharacter>(CurrentActorInfo->AvatarActor.Get()))
	{
		MutantChar->ClearHitEnemies();
	}

	// 若玩家在第一段期间就已经松开了左键（点按轻击），直接结束技能，打断后续的第二段动画
	if (bAttackButtonReleased)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UGA_MutantAttack::OnMontageCompleted()
{
	AMutantCharacter* MutantChar = Cast<AMutantCharacter>(CurrentActorInfo->AvatarActor.Get());

	// 两段轻击全部播完：若玩家仍按住左键不放，清空命中列表并无缝开启下一轮挥爪
	if (MutantChar && bIsLightAttack && !bAttackButtonReleased)
	{
		MutantChar->ClearHitEnemies();
		PlayAttackMontage(); 
	}
	else
	{
		// 动画已自然播放完毕，标记为 true，EndAbility 中无需再强制停止 1P 动画
		bMontageCompleted = true;
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UGA_MutantAttack::OnMontageInterruptedOrCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_MutantAttack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 解绑任务回调并置空指针（Super::EndAbility 会自动销毁挂载的 Task）
	if (CurrentMontageTask)
	{
		CurrentMontageTask->OnCompleted.RemoveAll(this);
		CurrentMontageTask->OnInterrupted.RemoveAll(this);
		CurrentMontageTask->OnCancelled.RemoveAll(this);
		CurrentMontageTask = nullptr;
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		// 技能结束时服务端解绑并清理本次激活的 InputReleased 委托缓存
		if (ASC->IsOwnerActorAuthoritative())
		{
			const FPredictionKey ActivationKey = ActivationInfo.GetActivationPredictionKey();
			ASC->AbilityReplicatedEventDelegate(EAbilityGenericReplicatedEvent::InputReleased, Handle, ActivationKey).RemoveAll(this);
			ASC->ConsumeAllReplicatedData(Handle, ActivationKey);
		}
	}

	if (AMutantCharacter* MutantChar = Cast<AMutantCharacter>(ActorInfo->AvatarActor.Get()))
	{
		// 仅在动画未自然播完就被中途打断（如点按轻击第一段结束收手）时，停止残留的 1P 攻击蒙太奇
		if (!bMontageCompleted && MutantChar->Mesh1P && MutantChar->MutantAsset)
		{
			UAnimMontage* Montage1P = bIsLightAttack ? MutantChar->MutantAsset->LightAttackMontage1P : MutantChar->MutantAsset->HeavyAttackMontage1P;
			if (UAnimInstance* AnimInst1P = MutantChar->Mesh1P->GetAnimInstance())
			{
				if (Montage1P && AnimInst1P->Montage_IsPlaying(Montage1P))
				{
					AnimInst1P->Montage_Stop(0.1f, Montage1P);
				}
			}
		}

		MutantChar->RightHandAttackEnd();
		MutantChar->LeftHandAttackEnd();
		MutantChar->ClearHitEnemies();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
