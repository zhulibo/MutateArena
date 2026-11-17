#include "GC_Throwable_Throw.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Equipments/Throwable.h"
#include "MutateArena/Equipments/AnimInstEquipment.h"
#include "MutateArena/Equipments/Data/EquipmentAsset.h"

UGC_Throwable_Throw::UGC_Throwable_Throw()
{
}

bool UGC_Throwable_Throw::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	AThrowable* ThrowableEquip = Cast<AThrowable>(const_cast<UObject*>(Parameters.SourceObject.Get()));
	if (!ThrowableEquip) return Super::OnExecute_Implementation(MyTarget, Parameters);

	// 3P 投掷物动画（3P 角色投掷蒙太奇由 GA 的 PlayMontageAndWait 播放，并由 GAS 同步给其他端）
	UAnimInstEquipment* EquipAnimInst = ThrowableEquip->GetEquipmentAnimInst();
	if (EquipAnimInst && ThrowableEquip->FragThrowable->ThrowMontage_E)
	{
		EquipAnimInst->Montage_Play(ThrowableEquip->FragThrowable->ThrowMontage_E);
	}

	// 1P 投掷动画：只有当前视角目标（本地玩家或第一人称观战）才需要播放手臂和 1P 投掷物动画
	AHumanCharacter* HumanChar = Cast<AHumanCharacter>(Parameters.Instigator.Get());
	if (HumanChar && HumanChar->bIsCurrentViewTarget)
	{
		UAnimInstance* AnimInst1P = HumanChar->Mesh1P ? HumanChar->Mesh1P->GetAnimInstance() : nullptr;
		if (AnimInst1P && ThrowableEquip->FragThrowable->ThrowMontage1P_C)
		{
			AnimInst1P->Montage_Play(ThrowableEquip->FragThrowable->ThrowMontage1P_C);
		}

		UAnimInstEquipment* EquipAnimInst1P = ThrowableEquip->GetEquipmentAnimInst1P();
		if (EquipAnimInst1P && ThrowableEquip->FragThrowable->ThrowMontage1P_E)
		{
			EquipAnimInst1P->Montage_Play(ThrowableEquip->FragThrowable->ThrowMontage1P_E);
		}
	}

	return Super::OnExecute_Implementation(MyTarget, Parameters);
}
