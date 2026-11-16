#include "GA_Melee_Attack.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Equipments/Melee.h"
#include "MutateArena/Equipments/Data/EquipmentAsset.h"
#include "MutateArena/System/GameplayTags.h"

UGA_Melee_Attack::UGA_Melee_Attack()
{
}

void UGA_Melee_Attack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                       const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	AHumanCharacter* HumanChar = GetHumanCharacter();
	AMelee* CurMelee = GetCurMelee();

	if (!ASC || !HumanChar || !CurMelee || !CurMelee->FragMelee || !TriggerEventData)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AttackButtonReleased = false;
	bFirstSectionEnded = false;
	bIsLightAttack = FMath::IsNearlyEqual(TriggerEventData->EventMagnitude, 1.f); // 1.f 轻击 | 2.f 重击
	CurMelee->bIsLightAttack = bIsLightAttack;

	// 轻击机制：一个蒙太奇内包含两段挥刀。
	// - 点按（第一段结束前松手）：播完第一段立即收刀结束。
	// - 长按（一直按住不松手）：第一段 -> 第二段 -> 播完后无缝重头循环。
	if (bIsLightAttack)
	{
		// 1. 监听本机松开攻击键事件
		UAbilityTask_WaitGameplayEvent* WaitReleaseTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, TAG_EVENT_MELEE_RELEASED);
		WaitReleaseTask->EventReceived.AddDynamic(this, &UGA_Melee_Attack::OnAttackButtonReleased);
		WaitReleaseTask->ReadyForActivation();

		// 2. 监听蒙太奇第一段挥刀结束通知（由动画 Notify 触发）
		UAbilityTask_WaitGameplayEvent* WaitEndTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, TAG_EVENT_MELEE_FIRSTSECTIONEND);
		WaitEndTask->EventReceived.AddDynamic(this, &UGA_Melee_Attack::OnFirstSectionEnd);
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

	PlayMeleeMontage();
}

void UGA_Melee_Attack::PlayMeleeMontage()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	AHumanCharacter* HumanChar = GetHumanCharacter();
	AMelee* CurMelee = GetCurMelee();

	if (!ASC || !HumanChar || !CurMelee || !CurMelee->FragMelee)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	// 每轮挥刀开始时重置状态标记（支持长按轻击无限循环）
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

	UAnimMontage* CharMontage3P = bIsLightAttack ? CurMelee->FragMelee->LightAttackMontage_C : CurMelee->FragMelee->HeavyAttackMontage_C;
	if (!CharMontage3P)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	// 1. 播放 3P 角色身体蒙太奇（由 GAS 自动同步给其他客户端）
	CurrentMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, CharMontage3P);
	CurrentMontageTask->OnCompleted.AddDynamic(this, &UGA_Melee_Attack::OnMontageCompleted);
	CurrentMontageTask->OnInterrupted.AddDynamic(this, &UGA_Melee_Attack::OnMontageInterruptedOrCancelled);
	CurrentMontageTask->OnCancelled.AddDynamic(this, &UGA_Melee_Attack::OnMontageInterruptedOrCancelled);
	CurrentMontageTask->ReadyForActivation();

	// 2. 触发 GC 播放其他网格体动画（3P 刀 + 1P 手臂 + 1P 刀）
	// RawMagnitude 正数表示播放：1.f 轻击 | 2.f 重击
	FGameplayCueParameters CueParams;
	CueParams.Instigator = HumanChar;
	CueParams.SourceObject = CurMelee;
	CueParams.RawMagnitude = bIsLightAttack ? 1.f : 2.f;

	// 【为什么不直接调一行 ExecuteGameplayCue？】
	// 因为长按循环连击（OnMontageCompleted）和第一段提前收刀（OnFirstSectionEnd）都是异步回调，此时早已脱离 GAS 初始预测窗口（ScopedPredictionKey 为空）。
	// 若客机在无 PredictionKey 时直接调 ExecuteGameplayCue，会被引擎底层静默丢弃（导致本地不播动画或停不掉动画）。
	// 因此分流处理：服务端负责多播给别人看，主控客机直接调本地接口立即表现！
	if (HumanChar->HasAuthority())
	{
		// 服务端：通过 NetMulticast 多播给所有客户端（供旁观者同步 3P 刀动画，Listen Server 房主自己也会在此触发）
		ASC->ExecuteGameplayCue(TAG_GC_MELEE_ATTACK, CueParams);
	}
	else if (HumanChar->IsLocallyControlled())
	{
		// 主控客机：贴上本地防伪标记（NormalizedMagnitude = 1.f）后纯本地直接调用，0 延迟播放且不受 PredictionKey 限制
		CueParams.NormalizedMagnitude = 1.f;
		ASC->InvokeGameplayCueEvent(TAG_GC_MELEE_ATTACK, EGameplayCueEvent::Executed, CueParams);
	}
}

void UGA_Melee_Attack::OnAttackButtonReleased(FGameplayEventData Payload)
{
	if (bIsLightAttack)
	{
		AttackButtonReleased = true;

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

void UGA_Melee_Attack::OnServerAttackButtonReleased()
{
	if (bIsLightAttack)
	{
		AttackButtonReleased = true;
	}
}

void UGA_Melee_Attack::OnFirstSectionEnd(FGameplayEventData Payload)
{
	// 防止单轮挥刀内重复触发
	if (!bIsLightAttack || bFirstSectionEnded)
	{
		return;
	}

	bFirstSectionEnded = true;

	// 第一段挥刀结束，清空已命中列表，让接下来的第二段挥刀能再次对同一目标造成伤害和贴花
	if (AMelee* CurMelee = GetCurMelee())
	{
		CurMelee->ClearHitEnemies();
	}

	// 若玩家在第一段期间就已经松开了左键（点按轻击），直接结束技能，打断后续的第二段动画
	if (AttackButtonReleased)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UGA_Melee_Attack::OnMontageCompleted()
{
	AMelee* CurMelee = GetCurMelee();

	// 两段轻击全部播完：若玩家仍按住左键不放，清空命中列表并无缝开启下一轮挥刀
	if (CurMelee && bIsLightAttack && !AttackButtonReleased)
	{
		CurMelee->ClearHitEnemies();
		PlayMeleeMontage();
	}
	else
	{
		// 动画已自然播放完毕，标记为 true，EndAbility 中无需再发 GC 去强制停止动画
		bMontageCompleted = true;
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UGA_Melee_Attack::OnMontageInterruptedOrCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_Melee_Attack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                  bool bReplicateEndAbility, bool bWasCancelled)
{
	// 解绑任务回调并置空指针（Super::EndAbility 会自动销毁挂载的 Task）
	if (CurrentMontageTask)
	{
		CurrentMontageTask->OnCompleted.RemoveAll(this);
		CurrentMontageTask->OnInterrupted.RemoveAll(this);
		CurrentMontageTask->OnCancelled.RemoveAll(this);
		CurrentMontageTask = nullptr;
	}

	// 优先从 Spec->SourceObject 获取该技能绑定的刀（防止切枪打断技能时，GetCurMelee() 已经变成了新切出来的枪）
	AMelee* MeleeToClean = Cast<AMelee>(GetSourceObject(Handle, ActorInfo));
	if (!MeleeToClean)
	{
		MeleeToClean = GetCurMelee();
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		// 技能为 InstancedPerActor 复用模式，结束时服务端必须解绑并清理本次激活的 InputReleased 委托缓存
		if (ASC->IsOwnerActorAuthoritative())
		{
			const FPredictionKey ActivationKey = ActivationInfo.GetActivationPredictionKey();
			ASC->AbilityReplicatedEventDelegate(EAbilityGenericReplicatedEvent::InputReleased, Handle, ActivationKey).RemoveAll(this);
			ASC->ConsumeAllReplicatedData(Handle, ActivationKey);
		}

		// 仅在“动画未自然播完就被中途打断”（如点按轻击第一段结束收刀、或切枪打断）时，才触发 GC 停止残留动画。
		// RawMagnitude 负数表示停止：-1.f 停止轻击 | -2.f 停止重击
		if (MeleeToClean && !bMontageCompleted)
		{
			AHumanCharacter* HumanChar = GetHumanCharacter();
			FGameplayCueParameters CueParams;
			CueParams.Instigator = HumanChar;
			CueParams.SourceObject = MeleeToClean;
			CueParams.RawMagnitude = bIsLightAttack ? -1.f : -2.f;

			if (HumanChar && HumanChar->HasAuthority())
			{
				// 服务端：多播通知其他客户端停止 3P 刀动画
				ASC->ExecuteGameplayCue(TAG_GC_MELEE_ATTACK, CueParams);
			}
			else if (HumanChar && HumanChar->IsLocallyControlled())
			{
				// 主控客机：贴上本地标记（NormalizedMagnitude = 1.f）立即停止 1P 动画
				CueParams.NormalizedMagnitude = 1.f;
				ASC->InvokeGameplayCueEvent(TAG_GC_MELEE_ATTACK, EGameplayCueEvent::Executed, CueParams);
			}
		}
	}

	// 保底关闭刀刃碰撞检测并清空命中记录
	if (MeleeToClean)
	{
		MeleeToClean->SetAttackCollisionEnabled(false);
		MeleeToClean->ClearHitEnemies();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
