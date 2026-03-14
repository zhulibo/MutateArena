#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayAbilityBase.generated.h"

UCLASS(Abstract)
class MUTATEARENA_API UGameplayAbilityBase : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGameplayAbilityBase();
	
};
