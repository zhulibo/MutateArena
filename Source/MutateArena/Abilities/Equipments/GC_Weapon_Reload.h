#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "GC_Weapon_Reload.generated.h"

UCLASS(Abstract)
class MUTATEARENA_API UGC_Weapon_Reload : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	UGC_Weapon_Reload();

	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
	
};
