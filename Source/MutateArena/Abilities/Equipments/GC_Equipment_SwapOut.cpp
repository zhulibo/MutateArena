#include "GC_Equipment_SwapOut.h"
#include "MutateArena/Equipments/Equipment.h"
#include "MutateArena/Equipments/AnimInstEquipment.h"

UGC_Equipment_SwapOut::UGC_Equipment_SwapOut()
{
}

bool UGC_Equipment_SwapOut::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	AEquipment* CurEquipment = Cast<AEquipment>(const_cast<UObject*>(Parameters.SourceObject.Get()));
	
	if (UAnimInstEquipment* EquipmentAnimInst = CurEquipment->GetEquipmentAnimInst())
	{
		EquipmentAnimInst->Montage_Play(CurEquipment->SwapOutMontage_E);
	}
	
	return Super::OnExecute_Implementation(MyTarget, Parameters);
}
