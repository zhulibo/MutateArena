#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "GameplayCue_MutantSmokerSkill.generated.h"

UCLASS()
class MUTATEARENA_API AGameplayCue_MutantSmokerSkill : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	AGameplayCue_MutantSmokerSkill();
	
	virtual bool WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;

protected:
	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* SmokerEffect;
	
	UPROPERTY()
	class UNiagaraComponent* SmokeEffectComponent;

};
