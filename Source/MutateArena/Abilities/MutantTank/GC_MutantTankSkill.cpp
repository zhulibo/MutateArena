#include "GC_MutantTankSkill.h"

#include "MetaSoundSource.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
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
		if (MutantTank->GetMesh()) MutantTank->GetMesh()->SetOverlayMaterial(MI_Overlay_Skill);
		if (MutantTank->Mesh1P) MutantTank->Mesh1P->SetOverlayMaterial(MI_Overlay_Skill);
		
		if (MutantTank->IsLocallyControlled())
		{
			if (SkillActivationSound)
			{
				UGameplayStatics::PlaySound2D(MutantTank, SkillActivationSound);
			}
		}
	}

	return Super::WhileActive_Implementation(MyTarget, Parameters);
}

bool AGC_MutantTankSkill::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if (AMutantTank* MutantTank = Cast<AMutantTank>(MyTarget))
	{
		if (MutantTank->GetMesh() && MutantTank->GetMesh()->GetOverlayMaterial() == MI_Overlay_Skill)
		{
			MutantTank->GetMesh()->SetOverlayMaterial(nullptr);
		}
		if (MutantTank->Mesh1P && MutantTank->Mesh1P->GetOverlayMaterial() == MI_Overlay_Skill)
		{
			MutantTank->Mesh1P->SetOverlayMaterial(nullptr);
		}
	}

	return Super::OnRemove_Implementation(MyTarget, Parameters);
}
