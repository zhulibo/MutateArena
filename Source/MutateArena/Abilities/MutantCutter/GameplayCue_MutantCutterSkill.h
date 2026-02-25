#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "GameplayCue_MutantCutterSkill.generated.h"

UCLASS()
class MUTATEARENA_API AGameplayCue_MutantCutterSkill : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	AGameplayCue_MutantCutterSkill();

	virtual bool WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;

protected:
	UPROPERTY(EditAnywhere)
	UMaterialInterface* MI_Overlay_Skill;

};
