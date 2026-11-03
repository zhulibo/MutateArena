#include "GC_MutantReaperSkill.h"

#include "MetaSoundSource.h"
#include "Components/SkeletalMeshComponent.h"
#include "MutateArena/Characters/Mutants/MutantReaper.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

AGC_MutantReaperSkill::AGC_MutantReaperSkill()
{
	PrimaryActorTick.bCanEverTick = false;
	bAutoDestroyOnRemove = true;
}

bool AGC_MutantReaperSkill::WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if (AMutantReaper* MutantReaper = Cast<AMutantReaper>(MyTarget))
	{
		// 更改皮肤颜色
		MutantReaper->GetMesh()->SetOverlayMaterial(MI_Overlay_Skill);
		
		if (MutantReaper->IsLocallyControlled())
		{
			if (SkillActivationSound)
			{
				UGameplayStatics::PlaySound2D(MutantReaper, SkillActivationSound);
			}
		}
	}

	return Super::WhileActive_Implementation(MyTarget, Parameters);
}

bool AGC_MutantReaperSkill::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if (AMutantReaper* MutantReaper = Cast<AMutantReaper>(MyTarget))
	{
		if (MutantReaper->GetMesh()->GetOverlayMaterial() == MI_Overlay_Skill)
		{
			MutantReaper->GetMesh()->SetOverlayMaterial(nullptr);
		}
	}

	return Super::OnRemove_Implementation(MyTarget, Parameters);
}
