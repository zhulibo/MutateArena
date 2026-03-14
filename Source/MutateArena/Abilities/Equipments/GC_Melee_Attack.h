#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "GC_Melee_Attack.generated.h"

class UAnimMontage;

UCLASS(Abstract)
class MUTATEARENA_API UGC_Melee_Attack : public UGameplayCueNotify_Static
{
	GENERATED_BODY()
	
public:
	UGC_Melee_Attack();

	virtual bool OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
	
};