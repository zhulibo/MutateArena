#include "GC_MutantTankSkill.h"

#include "MutateArena/Characters/Mutants/MutantTank.h"

AGC_MutantTankSkill::AGC_MutantTankSkill()
{
	PrimaryActorTick.bCanEverTick = false;
	
	bAutoDestroyOnRemove = true;
}

bool AGC_MutantTankSkill::WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if (AMutantTank* MutantTank = Cast<AMutantTank>(MyTarget))
	{
		// 更改皮肤颜色
		MutantTank->GetMesh()->SetOverlayMaterial(MI_Overlay_Skill);
	}

	return Super::WhileActive_Implementation(MyTarget, Parameters);
}

bool AGC_MutantTankSkill::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if (AMutantTank* MutantTank = Cast<AMutantTank>(MyTarget))
	{
		if (MutantTank->GetMesh()->GetOverlayMaterial() == MI_Overlay_Skill)
		{
			MutantTank->GetMesh()->SetOverlayMaterial(nullptr);
		}
	}

	return Super::OnRemove_Implementation(MyTarget, Parameters);
}
