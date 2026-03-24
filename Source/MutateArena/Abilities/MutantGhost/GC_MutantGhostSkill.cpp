#include "GC_MutantGhostSkill.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "MutateArena/Characters/Mutants/MutantGhost.h"
#include "MutateArena/System/Tags/ProjectTags.h"

AGC_MutantGhostSkill::AGC_MutantGhostSkill()
{
	PrimaryActorTick.bCanEverTick = false;
	bAutoDestroyOnRemove = true;
}

bool AGC_MutantGhostSkill::WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if (AMutantGhost* MutantGhost = Cast<AMutantGhost>(MyTarget))
	{
		// 更改材质透明度
		float TargetOpacity = .1f; 

		if (APlayerController* LocalPC = MyTarget->GetWorld()->GetFirstPlayerController())
		{
			if (APawn* LocalPawn = LocalPC->GetPawn())
			{
				if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(LocalPawn))
				{
					if (ASC->HasMatchingGameplayTag(TAG_STATE_DNA_EnhancedVision))
					{
						TargetOpacity = .2f; 
					}
				}
			}
		}

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
				DynamicMaterial->SetScalarParameterValue(FName("Opacity"), TargetOpacity);
			}
		}
		
		MutantGhost->ShowOverheadWidget(false);
	}

	return Super::WhileActive_Implementation(MyTarget, Parameters);
}

bool AGC_MutantGhostSkill::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
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
