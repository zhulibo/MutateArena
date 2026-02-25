#include "GameplayCue_MutantGhostSkill.h"

#include "MutateArena/Characters/Mutants/MutantGhost.h"

AGameplayCue_MutantGhostSkill::AGameplayCue_MutantGhostSkill()
{
	bAutoDestroyOnRemove = true;
}

bool AGameplayCue_MutantGhostSkill::WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if (AMutantGhost* MutantGhost = Cast<AMutantGhost>(MyTarget))
	{
		// 更改材质透明度
		TArray<UMaterialInterface*> MaterialInterfaces = MutantGhost->GetMesh()->GetMaterials();
		for (int32 i = 0; i < MaterialInterfaces.Num(); ++i)
		{
			UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(MaterialInterfaces[i]);
			if (DynamicMaterial == nullptr)
			{
				DynamicMaterial = MutantGhost->GetMesh()->CreateAndSetMaterialInstanceDynamic(i);
			}
			if (DynamicMaterial)
			{
				DynamicMaterial->SetScalarParameterValue(FName("Opacity"), 0.1f);
			}
		}
		
		MutantGhost->ShowOverheadWidget(false);
	}

	return Super::WhileActive_Implementation(MyTarget, Parameters);
}

bool AGameplayCue_MutantGhostSkill::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if (AMutantGhost* MutantGhost = Cast<AMutantGhost>(MyTarget))
	{
		TArray<UMaterialInterface*> MaterialInterfaces = MutantGhost->GetMesh()->GetMaterials();
		for (int32 i = 0; i < MaterialInterfaces.Num(); ++i)
		{
			UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(MaterialInterfaces[i]);
			if (MID == nullptr)
			{
				MID = MutantGhost->GetMesh()->CreateAndSetMaterialInstanceDynamic(i);
			}
			if (MID)
			{
				MID->SetScalarParameterValue(FName("Opacity"), .8f);
			}
		}
		
		MutantGhost->ShowOverheadWidget(true);
	}

	return Super::OnRemove_Implementation(MyTarget, Parameters);
}
