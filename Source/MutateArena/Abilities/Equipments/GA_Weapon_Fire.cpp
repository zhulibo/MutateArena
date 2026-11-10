#include "GA_Weapon_Fire.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Engine/World.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Characters/Components/CombatComponent.h"
#include "MutateArena/Characters/Components/RecoilComponent.h"
#include "MutateArena/Equipments/Weapon.h"
#include "MutateArena/Characters/AnimInstHuman.h"
#include "MutateArena/System/GameplayTags.h"

UGA_Weapon_Fire::UGA_Weapon_Fire()
{
}

bool UGA_Weapon_Fire::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
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
	
	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - CurWeapon->LastFireTime < CurWeapon->GetFireDelay())
	{
		return false;
	}
	
	return true;
}

void UGA_Weapon_Fire::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	AHumanCharacter* HumanChar = GetHumanCharacter();
	AWeapon* CurWeapon = GetCurWeapon();
	UCombatComponent* CombatComp = HumanChar->CombatComp;
	
	if (!ASC || !HumanChar || !CurWeapon || !CombatComp)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	CurWeapon->LastFireTime = GetWorld()->GetTimeSeconds();
	
	// 没子弹了播放击锤声音
	if (CurWeapon->IsEmpty())
	{
		FGameplayCueParameters CueParams;
		CueParams.Instigator = HumanChar;
		CueParams.SourceObject = CurWeapon;
		ASC->ExecuteGameplayCue(TAG_GC_WEAPON_DRYFIRE, CueParams);
        
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}
	
	// 让服务端准备好接收客户端的开火信息
	if (HumanChar->HasAuthority())
	{
		ASC->AbilityTargetDataSetDelegate(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey()).AddUObject(this, &UGA_Weapon_Fire::OnTargetDataReceived);
	}
	
	CombatComp->CurShotCount = 1;
	
	// 只允许本地控制的角色开启循环定时器，服务端只等开火信息发过来
	if (HumanChar->IsLocallyControlled())
	{
		StartFireLoop();
	}
}

void UGA_Weapon_Fire::StartFireLoop()
{
    AWeapon* CurWeapon = GetCurWeapon();
    if (!CurWeapon) return;

	LocalFireOnce();

    UAbilityTask_WaitDelay* DelayTask = UAbilityTask_WaitDelay::WaitDelay(this, CurWeapon->GetFireDelay());
    DelayTask->OnFinish.AddDynamic(this, &ThisClass::OnFireIntervalFinished);
    DelayTask->ReadyForActivation();
}

void UGA_Weapon_Fire::OnFireIntervalFinished()
{
    AWeapon* CurWeapon = GetCurWeapon();
	AHumanCharacter* HumanChar = GetHumanCharacter();
	UCombatComponent* CombatComp = HumanChar ? HumanChar->CombatComp : nullptr;
	if (!HumanChar || !CurWeapon || !CombatComp)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}
	
	if (CurWeapon->IsEmpty())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}
	
    if (CurWeapon->FragWeapon->bIsAutomatic) 
    {
		CombatComp->CurShotCount++;
    	
        StartFireLoop(); 
    }
    else
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
    }
}

void UGA_Weapon_Fire::LocalFireOnce()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	AHumanCharacter* HumanChar = GetHumanCharacter();
	AWeapon* CurWeapon = GetCurWeapon();
	URecoilComponent* RecoilComp = HumanChar ? HumanChar->RecoilComp : nullptr;
	UCombatComponent* CombatComp = HumanChar ? HumanChar->CombatComp : nullptr;

	if (!ASC || !HumanChar || !CurWeapon || !RecoilComp || !CombatComp) return;
	
	if (HumanChar->IsLocallyControlled())
	{
		CurWeapon->LastFireTime = GetWorld()->GetTimeSeconds();
		
		if (HumanChar->RecoilComp)
		{
			HumanChar->RecoilComp->IncRecoil();
		}
		
		// 构造开火信息
		const float RecoilVert = RecoilComp->GetCurRecoilVert();
		const float RecoilHor = RecoilComp->GetCurRecoilHor();
		const FVector2D Spread = RecoilComp->GetCurSpreadVector();
		const FVector_NetQuantize FinalTarget = CurWeapon->CalcFinalTarget(
			RecoilVert,
			RecoilHor,
			Spread.X,
			Spread.Y
		);
		const int32 SpreadSeed = CombatComp->GenerateSpreadSeed();

		FGameplayAbilityTargetData_FireInfo* FireData = new FGameplayAbilityTargetData_FireInfo();
		FireData->FinalTarget = FinalTarget;
		FireData->SpreadSeed = SpreadSeed;

		FGameplayAbilityTargetDataHandle TargetDataHandle;
		TargetDataHandle.Add(FireData);

		// 把开火信息传给服务端
		if (!HumanChar->HasAuthority())
		{
			ASC->CallServerSetReplicatedTargetData(
				CurrentSpecHandle,
				CurrentActivationInfo.GetActivationPredictionKey(),
				TargetDataHandle,
				FGameplayTag(),
				ASC->ScopedPredictionKey
			);
		}

		// 本地开火
		FScopedPredictionWindow ScopedPrediction(ASC, true);
		FGameplayCueParameters CueParams;
		CueParams.Instigator = HumanChar;
		CueParams.SourceObject = CurWeapon;
		CueParams.Location = FinalTarget;
		CueParams.RawMagnitude = static_cast<float>(FMath::Max(1, CombatComp->CurShotCount));
		CueParams.AbilityLevel = SpreadSeed; // 种子通过 AbilityLevel 传递
		ASC->ExecuteGameplayCue(TAG_GC_WEAPON_FIRE, CueParams);

		CurWeapon->Fire(FinalTarget, SpreadSeed);
	}
}

void UGA_Weapon_Fire::OnTargetDataReceived(const FGameplayAbilityTargetDataHandle& Data, FGameplayTag ActivationTag)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	AHumanCharacter* HumanChar = GetHumanCharacter();
	AWeapon* CurWeapon = GetCurWeapon();
    
	if (!ASC || !HumanChar || !CurWeapon) return;

	// 服务器收到数据后清理缓存，防止内存泄漏或重复处理
	ASC->ConsumeClientReplicatedTargetData(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());
	
	if (CurWeapon->IsEmpty()) return;

	if (Data.Num() > 0)
	{
		if (const FGameplayAbilityTargetData_FireInfo* FireData = static_cast<const FGameplayAbilityTargetData_FireInfo*>(Data.Get(0)))
		{
			UCombatComponent* CombatComp = HumanChar->CombatComp;
			const int32 ShotCount = CombatComp ? FMath::Max(1, CombatComp->CurShotCount) : 1;
			if (!HumanChar->IsLocallyControlled())
			{
				CurWeapon->Fire(FireData->FinalTarget, FireData->SpreadSeed);

				if (CurWeapon->FragWeapon && CurWeapon->FragWeapon->bIsAutomatic)
				{
					if (CombatComp)
					{
						CombatComp->CurShotCount++;
					}
				}
			}

			FGameplayCueParameters CueParams;
			CueParams.Instigator = HumanChar;
			CueParams.SourceObject = CurWeapon;
			CueParams.Location = FireData->FinalTarget;
			CueParams.RawMagnitude = static_cast<float>(ShotCount);
			CueParams.AbilityLevel = FireData->SpreadSeed;
			ASC->ExecuteGameplayCue(TAG_GC_WEAPON_FIRE, CueParams);
		}
	}
}

void UGA_Weapon_Fire::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
