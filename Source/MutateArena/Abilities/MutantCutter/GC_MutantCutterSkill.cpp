#include "GC_MutantCutterSkill.h"

#include "MetaSoundSource.h"
#include "MutateArena/Characters/Mutants/MutantCutter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

AGC_MutantCutterSkill::AGC_MutantCutterSkill()
{
	PrimaryActorTick.bCanEverTick = false;
	bAutoDestroyOnRemove = true;
}

bool AGC_MutantCutterSkill::WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if (AMutantCutter* MutantCutter = Cast<AMutantCutter>(MyTarget))
	{
		// 更改皮肤颜色
		MutantCutter->GetMesh()->SetOverlayMaterial(MI_Overlay_Skill);
		
		if (MutantCutter->IsLocallyControlled())
		{
			if (SkillActivationSound)
			{
				UGameplayStatics::PlaySound2D(MutantCutter, SkillActivationSound);
			}
		}
	}

	return Super::WhileActive_Implementation(MyTarget, Parameters);
}

bool AGC_MutantCutterSkill::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if (AMutantCutter* MutantCutter = Cast<AMutantCutter>(MyTarget))
	{
		if (MutantCutter->GetMesh()->GetOverlayMaterial() == MI_Overlay_Skill)
		{
			MutantCutter->GetMesh()->SetOverlayMaterial(nullptr);
		}
	}

	return Super::OnRemove_Implementation(MyTarget, Parameters);
}
