#include "GC_Throwing_Throw.h"
#include "MutateArena/Equipments/Throwing.h"
#include "MutateArena/Equipments/AnimInstEquipment.h"
#include "MutateArena/Equipments/Data/EquipmentAsset.h"

UGC_Throwing_Throw::UGC_Throwing_Throw()
{
}

bool UGC_Throwing_Throw::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	if (AThrowing* ThrowingEquip = Cast<AThrowing>(const_cast<UObject*>(Parameters.SourceObject.Get())))
	{
		if (UAnimInstEquipment* EquipmentAnimInst = ThrowingEquip->GetEquipmentAnimInst())
		{
			EquipmentAnimInst->Montage_Play(ThrowingEquip->FragThrowing->ThrowMontage_E);
		}
	}
	
	return Super::OnExecute_Implementation(MyTarget, Parameters);
}
