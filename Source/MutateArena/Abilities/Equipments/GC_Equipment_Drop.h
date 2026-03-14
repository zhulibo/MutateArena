#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "GC_Equipment_Drop.generated.h"

UCLASS(Abstract)
class MUTATEARENA_API UGC_Equipment_Drop : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	UGC_Equipment_Drop();

protected:
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
	
};
