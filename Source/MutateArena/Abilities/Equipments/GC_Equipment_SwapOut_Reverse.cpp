#include "GC_Equipment_SwapOut_Reverse.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Equipments/Equipment.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "MutateArena/Equipments/AnimInstEquipment.h"

UGC_Equipment_SwapOut_Reverse::UGC_Equipment_SwapOut_Reverse()
{
}

bool UGC_Equipment_SwapOut_Reverse::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	AHumanCharacter* HumanChar = Cast<AHumanCharacter>(Parameters.Instigator.Get());
	AEquipment* CurEquipment = Cast<AEquipment>(const_cast<UObject*>(Parameters.SourceObject.Get()));

	if (HumanChar && CurEquipment)
	{
		// 倒放角色动画
		if (USkeletalMeshComponent* Mesh = HumanChar->GetMesh())
		{
			if (UAnimInstance* HumanAnimInst = Mesh->GetAnimInstance())
			{
				UAnimMontage* ActiveMontage = HumanAnimInst->GetCurrentActiveMontage();
				
				// 只有当前正在播放的确实是这把武器的切出动画时，才执行倒放
				if (ActiveMontage && ActiveMontage == CurEquipment->SwapOutMontage_C)
				{
					HumanAnimInst->Montage_SetPlayRate(ActiveMontage, -1.f);
				}
			}
		}

		// 倒放装备切出动画
		if (UAnimInstance* EquipAnimInst = CurEquipment->GetEquipmentAnimInst())
		{
			UAnimMontage* ActiveEquipMontage = EquipAnimInst->GetCurrentActiveMontage();
			
			if (ActiveEquipMontage && ActiveEquipMontage == CurEquipment->SwapOutMontage_E)
			{
				EquipAnimInst->Montage_SetPlayRate(ActiveEquipMontage, -1.f);
			}
		}
	}

	return Super::OnExecute_Implementation(MyTarget, Parameters);
}
