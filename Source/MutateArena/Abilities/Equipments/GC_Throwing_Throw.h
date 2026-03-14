#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "GC_Throwing_Throw.generated.h"

UCLASS(Abstract)
class MUTATEARENA_API UGC_Throwing_Throw : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	UGC_Throwing_Throw();

	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
	
};
