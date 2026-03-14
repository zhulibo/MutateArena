#include "GC_Weapon_Reload_End.h"

#include "MutateArena/MutateArena.h"
#include "MutateArena/Equipments/Weapon.h"
#include "MutateArena/Equipments/AnimInstEquipment.h"

UGC_Weapon_Reload_End::UGC_Weapon_Reload_End()
{
}

bool UGC_Weapon_Reload_End::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	AWeapon* CurWeapon = Cast<AWeapon>(const_cast<UObject*>(Parameters.SourceObject.Get()));
	
	if (CurWeapon && CurWeapon->ReloadMontage_E)
	{
		if (UAnimInstEquipment* EquipmentAnimInst = CurWeapon->GetEquipmentAnimInst())
		{
			// 让武器自身的动画实例跳转到对应的 Section
			EquipmentAnimInst->Montage_JumpToSection(SECTION_RELOAD_END, CurWeapon->ReloadMontage_E);
		}
	}
	
	return Super::OnExecute_Implementation(MyTarget, Parameters);
}
