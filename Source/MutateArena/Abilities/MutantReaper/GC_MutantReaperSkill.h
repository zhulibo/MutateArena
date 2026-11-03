#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "GC_MutantReaperSkill.generated.h"

class UMetaSoundSource;

UCLASS()
class MUTATEARENA_API AGC_MutantReaperSkill : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	AGC_MutantReaperSkill();

	virtual bool WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;

protected:
	UPROPERTY(EditAnywhere)
	UMaterialInterface* MI_Overlay_Skill;

	UPROPERTY(EditAnywhere)
	UMetaSoundSource* SkillActivationSound;
	
};
