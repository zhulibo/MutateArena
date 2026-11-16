#include "GC_Melee_Attack.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Equipments/AnimInstEquipment.h"
#include "MutateArena/Equipments/Melee.h"
#include "MutateArena/Equipments/Data/EquipmentAsset.h"

UGC_Melee_Attack::UGC_Melee_Attack()
{
}

bool UGC_Melee_Attack::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	AHumanCharacter* HumanChar = Cast<AHumanCharacter>(Parameters.Instigator.Get());
	if (!HumanChar)
	{
		HumanChar = Cast<AHumanCharacter>(MyTarget);
	}
	AMelee* CurMelee = Cast<AMelee>(const_cast<UObject*>(Parameters.SourceObject.Get()));
	
	if (!HumanChar || !CurMelee || !CurMelee->FragMelee)
	{
		return Super::OnExecute_Implementation(MyTarget, Parameters);
	}

	// 本地主控客机：只执行本地主动发起的 GC（NormalizedMagnitude == 1.f），100% 过滤服务端多播回传的包，防止延迟包回拉或误停下一刀动画
	if (HumanChar->IsLocallyControlled() && !HumanChar->HasAuthority())
	{
		if (!FMath::IsNearlyEqual(Parameters.NormalizedMagnitude, 1.f))
		{
			return Super::OnExecute_Implementation(MyTarget, Parameters);
		}
	}

	const bool bIsStop = Parameters.RawMagnitude < 0.f;
	const bool bIsLight = FMath::IsNearlyEqual(FMath::Abs(Parameters.RawMagnitude), 1.f);

	if (!bIsStop)
	{
		// 1. 播放 3P 武器攻击蒙太奇（3P 角色攻击蒙太奇由 GA 的 PlayMontageAndWait 播放，并由 GAS 同步给其他端）
		if (UAnimMontage* MontageToPlay_E = bIsLight ? CurMelee->FragMelee->LightAttackMontage_E : CurMelee->FragMelee->HeavyAttackMontage_E)
		{
			if (UAnimInstEquipment* EquipAnimInst = CurMelee->GetEquipmentAnimInst())
			{
				EquipAnimInst->Montage_Stop(0.f, MontageToPlay_E);
				EquipAnimInst->Montage_Play(MontageToPlay_E);
			}
		}

		// 2. 播放 1P 攻击动画：只有当前视角目标（本地玩家或第一人称观战）才需要播放 1P 手臂和 1P 武器动画
		if (HumanChar->bIsCurrentViewTarget)
		{
			if (UAnimMontage* MontageToPlay1P_C = bIsLight ? CurMelee->FragMelee->LightAttackMontage1P_C : CurMelee->FragMelee->HeavyAttackMontage1P_C)
			{
				if (UAnimInstance* AnimInst1P = HumanChar->Mesh1P ? HumanChar->Mesh1P->GetAnimInstance() : nullptr)
				{
					AnimInst1P->Montage_Stop(0.f, MontageToPlay1P_C);
					AnimInst1P->Montage_Play(MontageToPlay1P_C);
				}
			}

			if (UAnimMontage* MontageToPlay1P_E = bIsLight ? CurMelee->FragMelee->LightAttackMontage1P_E : CurMelee->FragMelee->HeavyAttackMontage1P_E)
			{
				if (UAnimInstEquipment* EquipAnimInst1P = CurMelee->GetEquipmentAnimInst1P())
				{
					EquipAnimInst1P->Montage_Stop(0.f, MontageToPlay1P_E);
					EquipAnimInst1P->Montage_Play(MontageToPlay1P_E);
				}
			}
		}
	}
	else
	{
		// 停止 3P 武器攻击蒙太奇
		if (UAnimMontage* MontagePlayed_E = bIsLight ? CurMelee->FragMelee->LightAttackMontage_E : CurMelee->FragMelee->HeavyAttackMontage_E)
		{
			if (UAnimInstEquipment* EquipAnimInst = CurMelee->GetEquipmentAnimInst())
			{
				if (EquipAnimInst->Montage_IsPlaying(MontagePlayed_E))
				{
					EquipAnimInst->Montage_Stop(0.1f, MontagePlayed_E);
				}
			}
		}

		// 停止 1P 手臂与 1P 武器攻击蒙太奇（仅针对当前攻击蒙太奇停止，避免切枪打断时误停新武器的切入动画）
		if (UAnimMontage* MontagePlayed1P_C = bIsLight ? CurMelee->FragMelee->LightAttackMontage1P_C : CurMelee->FragMelee->HeavyAttackMontage1P_C)
		{
			if (UAnimInstance* AnimInst1P = HumanChar->Mesh1P ? HumanChar->Mesh1P->GetAnimInstance() : nullptr)
			{
				if (AnimInst1P->Montage_IsPlaying(MontagePlayed1P_C))
				{
					AnimInst1P->Montage_Stop(0.1f, MontagePlayed1P_C);
				}
			}
		}

		if (UAnimMontage* MontagePlayed1P_E = bIsLight ? CurMelee->FragMelee->LightAttackMontage1P_E : CurMelee->FragMelee->HeavyAttackMontage1P_E)
		{
			if (UAnimInstEquipment* EquipAnimInst1P = CurMelee->GetEquipmentAnimInst1P())
			{
				if (EquipAnimInst1P->Montage_IsPlaying(MontagePlayed1P_E))
				{
					EquipAnimInst1P->Montage_Stop(0.1f, MontagePlayed1P_E);
				}
			}
		}
	}

	return Super::OnExecute_Implementation(MyTarget, Parameters);
}
