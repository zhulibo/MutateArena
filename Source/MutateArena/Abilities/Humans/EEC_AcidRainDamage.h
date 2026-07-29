#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "EEC_AcidRainDamage.generated.h"

UCLASS()
class MUTATEARENA_API UEEC_AcidRainDamage : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UEEC_AcidRainDamage();

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	                                    FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
