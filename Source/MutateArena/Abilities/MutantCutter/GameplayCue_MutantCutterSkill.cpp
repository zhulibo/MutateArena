#include "GameplayCue_MutantCutterSkill.h"

#include "MutateArena/Characters/Mutants/MutantCutter.h"
#include "GameFramework/CharacterMovementComponent.h"

AGameplayCue_MutantCutterSkill::AGameplayCue_MutantCutterSkill()
{
	bAutoDestroyOnRemove = true;
}

bool AGameplayCue_MutantCutterSkill::WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if (MutantCutter == nullptr) MutantCutter = Cast<AMutantCutter>(MyTarget);
	if (MutantCutter)
	{
		// 更改皮肤颜色
		MutantCutter->GetMesh()->SetOverlayMaterial(MutantCutter->MI_Overlay_Skill);
	}

	return Super::WhileActive_Implementation(MyTarget, Parameters);
}

bool AGameplayCue_MutantCutterSkill::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if (MutantCutter == nullptr) MutantCutter = Cast<AMutantCutter>(MyTarget);
	if (MutantCutter)
	{
		if (MutantCutter->GetMesh()->GetOverlayMaterial() == MutantCutter->MI_Overlay_Skill)
		{
			MutantCutter->GetMesh()->SetOverlayMaterial(nullptr);
		}
	}

	return Super::OnExecute_Implementation(MyTarget, Parameters);
}
