#include "GC_Weapon_Reload.h"
#include "MutateArena/Equipments/Weapon.h"
#include "MutateArena/Equipments/AnimInstEquipment.h"

UGC_Weapon_Reload::UGC_Weapon_Reload()
{
}

bool UGC_Weapon_Reload::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
    AWeapon* CurWeapon = Cast<AWeapon>(const_cast<UObject*>(Parameters.SourceObject.Get()));
	
	if (UAnimInstEquipment* EquipmentAnimInst = CurWeapon->GetEquipmentAnimInst())
	{
		EquipmentAnimInst->Montage_Play(CurWeapon->ReloadMontage_E);
	}
	
	return Super::OnExecute_Implementation(MyTarget, Parameters);
}
