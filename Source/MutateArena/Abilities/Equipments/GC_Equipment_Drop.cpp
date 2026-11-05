// GC_Equipment_Drop.cpp
#include "GC_Equipment_Drop.h"

#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Equipments/Equipment.h"
#include "MutateArena/Equipments/AnimInstEquipment.h"

UGC_Equipment_Drop::UGC_Equipment_Drop()
{
}

bool UGC_Equipment_Drop::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	if (AEquipment* Equipment = Cast<AEquipment>(const_cast<UObject*>(Parameters.SourceObject.Get())))
	{
		// 停止 3P 武器自身动画（如换弹时弹匣的运动）
		if (UAnimInstEquipment* EquipAnimInst = Equipment->GetEquipmentAnimInst())
		{
			EquipAnimInst->Montage_Stop(0.f);
		}

		// 停止 1P 武器自身动画
		if (UAnimInstEquipment* EquipAnimInst1P = Equipment->GetEquipmentAnimInst1P())
		{
			EquipAnimInst1P->Montage_Stop(0.f);
		}
	}

	AHumanCharacter* HumanChar = Cast<AHumanCharacter>(MyTarget);
	if (!HumanChar)
	{
		HumanChar = Cast<AHumanCharacter>(Parameters.Instigator.Get());
	}

	if (HumanChar)
	{
		// 停止角色 1P 手臂网格体上残留的动作（如换弹、切入、射击动画等）
		if (HumanChar->Mesh1P)
		{
			if (UAnimInstance* AnimInst1P = HumanChar->Mesh1P->GetAnimInstance())
			{
				AnimInst1P->Montage_Stop(0.f);
			}
		}

		// 停止角色 3P 身体网格体上残留的动作（如换弹中途丢枪，防止 3P 继续空手播换弹动画）
		if (HumanChar->BodyMesh)
		{
			if (UAnimInstance* AnimInst3P = HumanChar->BodyMesh->GetAnimInstance())
			{
				AnimInst3P->Montage_Stop(0.1f);
			}
		}
	}

	return Super::OnExecute_Implementation(MyTarget, Parameters);
}