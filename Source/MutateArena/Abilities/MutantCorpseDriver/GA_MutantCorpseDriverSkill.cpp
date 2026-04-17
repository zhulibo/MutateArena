#include "GA_MutantCorpseDriverSkill.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GameFramework/Character.h"
#include "MutateArena/Characters/Mutants/Corpse/CorpseCharacter.h"
#include "MutateArena/System/Tags/ProjectTags.h"

UGA_MutantCorpseDriverSkill::UGA_MutantCorpseDriverSkill()
{
}

void UGA_MutantCorpseDriverSkill::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 1. 提交技能消耗和冷却 (GAS 内部逻辑)
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 2. 播放召唤蒙太奇
	if (SummonMontage)
	{
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, SummonMontage);
		
		MontageTask->OnBlendOut.AddDynamic(this, &UGA_MutantCorpseDriverSkill::OnMontageCompleted);
		MontageTask->OnCompleted.AddDynamic(this, &UGA_MutantCorpseDriverSkill::OnMontageCompleted);
		MontageTask->OnInterrupted.AddDynamic(this, &UGA_MutantCorpseDriverSkill::OnMontageCancelled);
		MontageTask->OnCancelled.AddDynamic(this, &UGA_MutantCorpseDriverSkill::OnMontageCancelled);
		
		MontageTask->ReadyForActivation();

		// 【可选补充】：如果想在动画的特定帧生成小兵，可以用 UAbilityTask_WaitGameplayEvent 监听动画里的 AnimNotify
	}
	else
	{
		// 如果没有蒙太奇，直接生成
		SpawnMinion(ActorInfo);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

void UGA_MutantCorpseDriverSkill::SpawnMinion(const FGameplayAbilityActorInfo* ActorInfo)
{
	// 【核心：P2P架构下的权限控制】只允许服务器端生成小兵实体！
	if (HasAuthority(&CurrentActivationInfo) && MinionClassToSpawn)
	{
		if (ACharacter* AvatarCharacter = Cast<ACharacter>(ActorInfo->AvatarActor.Get()))
		{
			UWorld* World = GetWorld();
			if (World)
			{
				// 计算生成位置：玩家正前方一段距离
				FVector SpawnLocation = AvatarCharacter->GetActorLocation() + (AvatarCharacter->GetActorForwardVector() * SpawnForwardOffset);
				FRotator SpawnRotation = AvatarCharacter->GetActorRotation();
				
				// 略微抬高Z轴，防止穿模卡在地板里
				SpawnLocation.Z += 50.0f; 

				FActorSpawnParameters SpawnParams;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

				// 生成小兵，由于你在 CorpseCharacter 构造函数中写了 AutoPossessAI，这里它会自动被 AI 控制并同步给客户端
				World->SpawnActor<ACharacter>(MinionClassToSpawn, SpawnLocation, SpawnRotation, SpawnParams);
				
				// 手动构建参数，精确传递召唤位置
				FGameplayCueParameters CueParams;
				CueParams.Location = SpawnLocation;
				// 如果需要，还可以传召唤者的朝向
				CueParams.Normal = AvatarCharacter->GetActorForwardVector(); 
				// 手动触发（注意：由于我们在 Authority 下，这里会通过 RPC 广播给所有客户端播放）
				ActorInfo->AbilitySystemComponent->ExecuteGameplayCue(TAG_GC_MUTANT_CORPSEDRIVER_SKILL, CueParams);
			}
		}
	}
}

void UGA_MutantCorpseDriverSkill::OnMontageCompleted()
{
	// 动画播放完毕后生成小兵（如果是等待 Notify，这里的逻辑就要转移）
	SpawnMinion(CurrentActorInfo);
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_MutantCorpseDriverSkill::OnMontageCancelled()
{
	// 动画被打断（如被击晕），技能取消
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_MutantCorpseDriverSkill::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}