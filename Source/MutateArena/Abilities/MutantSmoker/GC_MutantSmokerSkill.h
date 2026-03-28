#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "GC_MutantSmokerSkill.generated.h"

class UMetaSoundSource;

UCLASS()
class MUTATEARENA_API AGC_MutantSmokerSkill : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	AGC_MutantSmokerSkill();
	
	virtual bool WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;

protected:
	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* SmokerEffect;
	
	UPROPERTY()
	class UNiagaraComponent* SmokeEffectComponent;
	
	UPROPERTY()
	AActor* SpawnedSmokeActor;
	
	UPROPERTY(EditAnywhere)
	UMetaSoundSource* SkillActivationSound;
	
};
