#include "GameplayCue_MutantTankSkill.h"

#include "MutateArena/Characters/Mutants/MutantTank.h"

AGameplayCue_MutantTankSkill::AGameplayCue_MutantTankSkill()
{
	bAutoDestroyOnRemove = true;
}

bool AGameplayCue_MutantTankSkill::WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if (AMutantTank* MutantTank = Cast<AMutantTank>(MyTarget))
	{
		// 更改皮肤颜色
		MutantTank->GetMesh()->SetOverlayMaterial(MI_Overlay_Skill);
	}

	return Super::WhileActive_Implementation(MyTarget, Parameters);
}

bool AGameplayCue_MutantTankSkill::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
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
