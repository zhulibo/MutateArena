#include "GC_Throwable_Throw.h"
#include "MutateArena/Equipments/Throwable.h"
#include "MutateArena/Equipments/AnimInstEquipment.h"
#include "MutateArena/Equipments/Data/EquipmentAsset.h"

UGC_Throwable_Throw::UGC_Throwable_Throw()
{
}

bool UGC_Throwable_Throw::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	if (AThrowable* ThrowableEquip = Cast<AThrowable>(const_cast<UObject*>(Parameters.SourceObject.Get())))
	{
		if (UAnimInstEquipment* EquipmentAnimInst = ThrowableEquip->GetEquipmentAnimInst())
		{
			EquipmentAnimInst->Montage_Play(ThrowableEquip->FragThrowable->ThrowMontage_E);
		}
	}
	
	return Super::OnExecute_Implementation(MyTarget, Parameters);
}
