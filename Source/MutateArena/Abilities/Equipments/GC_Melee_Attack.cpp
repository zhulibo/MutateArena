#include "GC_Melee_Attack.h"
#include "Animation/AnimInstance.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Equipments/AnimInstEquipment.h"
#include "MutateArena/Equipments/Melee.h"

UGC_Melee_Attack::UGC_Melee_Attack()
{
}

bool UGC_Melee_Attack::OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	AHumanCharacter* HumanChar = Cast<AHumanCharacter>(Parameters.Instigator.Get());
	AMelee* CurMelee = Cast<AMelee>(const_cast<UObject*>(Parameters.SourceObject.Get()));
	
	if (HumanChar && CurMelee)
	{
		if (UAnimMontage* MontageToPlay = Parameters.RawMagnitude == 1.f ? CurMelee->LightAttackMontage_E : CurMelee->HeavyAttackMontage_E)
		{
			if (UAnimInstEquipment* EquipAnimInst = CurMelee->GetEquipmentAnimInst())
			{
				EquipAnimInst->Montage_Play(MontageToPlay);
			}
		}
	}

	return Super::OnActive_Implementation(MyTarget, Parameters);
}

bool UGC_Melee_Attack::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	AHumanCharacter* HumanChar = Cast<AHumanCharacter>(Parameters.Instigator.Get());
	AMelee* CurMelee = Cast<AMelee>(const_cast<UObject*>(Parameters.SourceObject.Get()));
	
	if (HumanChar && CurMelee)
	{
		if (UAnimMontage* MontagePlayed = Parameters.RawMagnitude == 1.f ? CurMelee->LightAttackMontage_E : CurMelee->HeavyAttackMontage_E)
		{
			if (UAnimInstEquipment* EquipAnimInst = CurMelee->GetEquipmentAnimInst())
			{
				if (EquipAnimInst->GetCurrentActiveMontage() == MontagePlayed)
				{
					EquipAnimInst->Montage_Stop(.1f, EquipAnimInst->GetCurrentActiveMontage());
				}
			}
		}
	}

	return Super::OnRemove_Implementation(MyTarget, Parameters);
}
