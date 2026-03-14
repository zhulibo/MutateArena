#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "GC_Weapon_Reload_End.generated.h"

UCLASS(Abstract)
class MUTATEARENA_API UGC_Weapon_Reload_End : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	UGC_Weapon_Reload_End();

	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
	
};
