#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "MMC_MutantRestore.generated.h"

UCLASS()
class MUTATEARENA_API UMMC_MutantRestore : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UMMC_MutantRestore();

	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

private:
	// 用于捕获最大生命值的定义
	FGameplayEffectAttributeCaptureDefinition MaxHealthDef;
};
