#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "GC_Equipment_SwapIn.generated.h"

UCLASS()
class MUTATEARENA_API UGC_Equipment_SwapIn : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	UGC_Equipment_SwapIn();

	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
	
};
