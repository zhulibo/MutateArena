#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "GC_Weapon_Fire.generated.h"

UCLASS(Abstract)
class MUTATEARENA_API UGC_Weapon_Fire : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	UGC_Weapon_Fire();

protected:
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
	
};
