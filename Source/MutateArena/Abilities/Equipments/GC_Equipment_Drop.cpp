#include "GC_Equipment_Drop.h"
#include "MutateArena/Equipments/Equipment.h"
#include "MutateArena/Equipments/AnimInstEquipment.h"

UGC_Equipment_Drop::UGC_Equipment_Drop()
{
}

bool UGC_Equipment_Drop::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	if (AEquipment* Equipment = Cast<AEquipment>(Parameters.EffectCauser))
	{
		if (UAnimInstEquipment* EquipAnimInst = Equipment->GetEquipmentAnimInst())
		{
			EquipAnimInst->Montage_Stop(.1f);
		}
	}

	return Super::OnExecute_Implementation(MyTarget, Parameters);
}
