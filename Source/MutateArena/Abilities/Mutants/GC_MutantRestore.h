#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "GC_MutantRestore.generated.h"

class UMetaSoundSource;

UCLASS()
class MUTATEARENA_API UGC_MutantRestore : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	UGC_MutantRestore();

protected:
	UPROPERTY(EditAnywhere)
	UMetaSoundSource* RestoreHealthSound;

	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
	
};