#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "GC_MutantGhostSkill.generated.h"

class UMetaSoundSource;

UCLASS()
class MUTATEARENA_API AGC_MutantGhostSkill : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	AGC_MutantGhostSkill();

	virtual bool WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;

protected:
	UPROPERTY(EditAnywhere)
	UMetaSoundSource* SkillActivationSound;
	
};
