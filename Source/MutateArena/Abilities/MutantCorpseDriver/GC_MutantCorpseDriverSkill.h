#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "MetaSoundSource.h"
#include "GC_MutantCorpseDriverSkill.generated.h"

class UNiagaraSystem;

UCLASS()
class MUTATEARENA_API AGC_MutantCorpseDriverSkill : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	AGC_MutantCorpseDriverSkill();

	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;

protected:
	UPROPERTY(EditAnywhere)
	UNiagaraSystem* SummonEffect;

	UPROPERTY(EditAnywhere)
	UMetaSoundSource* SummonSound;
	
	UPROPERTY(EditAnywhere)
	UMetaSoundSource* SkillActivationSound;
	
};
