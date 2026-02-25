#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "GameplayCue_MutantGhostSkill.generated.h"

UCLASS()
class MUTATEARENA_API AGameplayCue_MutantGhostSkill : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	AGameplayCue_MutantGhostSkill();

	virtual bool WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;

protected:

};
