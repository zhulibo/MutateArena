#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "GC_MutantTankSkill.generated.h"

class UMetaSoundSource;

UCLASS()
class MUTATEARENA_API AGC_MutantTankSkill : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	AGC_MutantTankSkill();

	virtual bool WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;

protected:
	UPROPERTY(EditAnywhere)
	UMaterialInterface* MI_Overlay_Skill;
	
	UPROPERTY(EditAnywhere)
	UMetaSoundSource* SkillActivationSound;
	
};
