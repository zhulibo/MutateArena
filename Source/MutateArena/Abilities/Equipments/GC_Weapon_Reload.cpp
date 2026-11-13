#include "GC_Weapon_Reload.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Equipments/Weapon.h"
#include "MutateArena/Equipments/AnimInstEquipment.h"

UGC_Weapon_Reload::UGC_Weapon_Reload()
{
}

bool UGC_Weapon_Reload::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	AWeapon* CurWeapon = Cast<AWeapon>(const_cast<UObject*>(Parameters.SourceObject.Get()));
	if (!CurWeapon || !CurWeapon->FragWeapon) return Super::OnExecute_Implementation(MyTarget, Parameters);

	// 3P 武器换弹动画（3P 角色换弹蒙太奇由 GA 的 PlayMontageAndWait 播放，并由 GAS 同步给其他端）
	if (UAnimInstEquipment* EquipmentAnimInst = CurWeapon->GetEquipmentAnimInst())
	{
		EquipmentAnimInst->Montage_Play(CurWeapon->FragWeapon->ReloadMontage_E);
	}

	// 1P 换弹动画：只有当前视角目标（本地玩家或第一人称观战）才需要播放手臂和 1P 武器动画
	AHumanCharacter* HumanChar = Cast<AHumanCharacter>(Parameters.Instigator.Get());
	if (HumanChar && HumanChar->bIsCurrentViewTarget)
	{
		UAnimInstance* AnimInst1P = HumanChar->Mesh1P ? HumanChar->Mesh1P->GetAnimInstance() : nullptr;
		if (AnimInst1P && CurWeapon->FragWeapon->ReloadMontage1P_C)
		{
			AnimInst1P->Montage_Play(CurWeapon->FragWeapon->ReloadMontage1P_C);
		}

		UAnimInstEquipment* EquipAnimInst1P = CurWeapon->GetEquipmentAnimInst1P();
		if (EquipAnimInst1P && CurWeapon->FragWeapon->ReloadMontage1P_E)
		{
			EquipAnimInst1P->Montage_Play(CurWeapon->FragWeapon->ReloadMontage1P_E);
		}
	}

	return Super::OnExecute_Implementation(MyTarget, Parameters);
}
