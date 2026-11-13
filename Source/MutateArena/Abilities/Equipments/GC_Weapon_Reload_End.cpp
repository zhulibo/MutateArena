#include "GC_Weapon_Reload_End.h"

#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Equipments/Weapon.h"
#include "MutateArena/Equipments/AnimInstEquipment.h"

UGC_Weapon_Reload_End::UGC_Weapon_Reload_End()
{
}

bool UGC_Weapon_Reload_End::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	AWeapon* CurWeapon = Cast<AWeapon>(const_cast<UObject*>(Parameters.SourceObject.Get()));
	if (!CurWeapon || !CurWeapon->FragWeapon) return Super::OnExecute_Implementation(MyTarget, Parameters);

	// 让 3P 武器自身的动画实例跳转到对应的 Section（3P 角色由 GA 的 MontageJumpToSection 处理）
	if (CurWeapon->FragWeapon->ReloadMontage_E)
	{
		if (UAnimInstEquipment* EquipmentAnimInst = CurWeapon->GetEquipmentAnimInst())
		{
			EquipmentAnimInst->Montage_JumpToSection(MontageSections::ReloadEnd, CurWeapon->FragWeapon->ReloadMontage_E);
		}
	}

	// 1P 手臂与 1P 武器同步跳转到换弹结束段
	AHumanCharacter* HumanChar = Cast<AHumanCharacter>(Parameters.Instigator.Get());
	if (HumanChar && HumanChar->bIsCurrentViewTarget)
	{
		UAnimInstance* AnimInst1P = HumanChar->Mesh1P ? HumanChar->Mesh1P->GetAnimInstance() : nullptr;
		if (AnimInst1P && CurWeapon->FragWeapon->ReloadMontage1P_C)
		{
			AnimInst1P->Montage_JumpToSection(MontageSections::ReloadEnd, CurWeapon->FragWeapon->ReloadMontage1P_C);
		}

		UAnimInstEquipment* EquipAnimInst1P = CurWeapon->GetEquipmentAnimInst1P();
		if (EquipAnimInst1P && CurWeapon->FragWeapon->ReloadMontage1P_E)
		{
			EquipAnimInst1P->Montage_JumpToSection(MontageSections::ReloadEnd, CurWeapon->FragWeapon->ReloadMontage1P_E);
		}
	}

	return Super::OnExecute_Implementation(MyTarget, Parameters);
}
