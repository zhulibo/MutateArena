#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "GC_Equipment_SwapOut_Reverse.generated.h"

UCLASS(Abstract)
class MUTATEARENA_API UGC_Equipment_SwapOut_Reverse : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	UGC_Equipment_SwapOut_Reverse();

protected:
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
	
};