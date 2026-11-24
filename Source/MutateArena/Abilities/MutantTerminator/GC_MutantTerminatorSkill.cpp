#include "GC_MutantTerminatorSkill.h"

#include "MetaSoundSource.h"
#include "Components/SkeletalMeshComponent.h"
#include "MutateArena/Characters/Mutants/MutantTerminator.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

AGC_MutantTerminatorSkill::AGC_MutantTerminatorSkill()
{
	PrimaryActorTick.bCanEverTick = false;
	bAutoDestroyOnRemove = true;
}

bool AGC_MutantTerminatorSkill::WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if (AMutantTerminator* MutantTerminator = Cast<AMutantTerminator>(MyTarget))
	{
		// 更改皮肤颜色
		if (MutantTerminator->GetMesh()) MutantTerminator->GetMesh()->SetOverlayMaterial(MI_Overlay_Skill);
		if (MutantTerminator->Mesh1P) MutantTerminator->Mesh1P->SetOverlayMaterial(MI_Overlay_Skill);
		
		if (MutantTerminator->IsLocallyControlled())
		{
			if (SkillActivationSound)
			{
				UGameplayStatics::PlaySound2D(MutantTerminator, SkillActivationSound);
			}
		}
	}

	return Super::WhileActive_Implementation(MyTarget, Parameters);
}

bool AGC_MutantTerminatorSkill::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if (AMutantTerminator* MutantTerminator = Cast<AMutantTerminator>(MyTarget))
	{
		if (MutantTerminator->GetMesh() && MutantTerminator->GetMesh()->GetOverlayMaterial() == MI_Overlay_Skill)
		{
			MutantTerminator->GetMesh()->SetOverlayMaterial(nullptr);
		}
		if (MutantTerminator->Mesh1P && MutantTerminator->Mesh1P->GetOverlayMaterial() == MI_Overlay_Skill)
		{
			MutantTerminator->Mesh1P->SetOverlayMaterial(nullptr);
		}
	}

	return Super::OnRemove_Implementation(MyTarget, Parameters);
}
