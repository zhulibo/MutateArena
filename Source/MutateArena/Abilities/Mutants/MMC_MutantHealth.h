#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "MMC_MutantHealth.generated.h"

UCLASS()
class MUTATEARENA_API UMMC_MutantHealth : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UMMC_MutantHealth();
	
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

protected:
	FGameplayEffectAttributeCaptureDefinition MaxHealthDef;
	FGameplayEffectAttributeCaptureDefinition HealthDef;

};
