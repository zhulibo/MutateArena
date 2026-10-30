#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "GC_Throwable_Throw.generated.h"

UCLASS(Abstract)
class MUTATEARENA_API UGC_Throwable_Throw : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	UGC_Throwable_Throw();

	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
	
};
