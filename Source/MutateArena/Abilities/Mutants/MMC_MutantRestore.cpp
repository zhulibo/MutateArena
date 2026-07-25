#include "MMC_MutantRestore.h"

#include "MutateArena/Abilities/AttributeSetBase.h"
#include "MutateArena/System/GameplayTags.h"

UMMC_MutantRestore::UMMC_MutantRestore()
{
	MaxHealthDef = FGameplayEffectAttributeCaptureDefinition(UAttributeSetBase::GetMaxHealthAttribute(), EGameplayEffectAttributeCaptureSource::Target, false);

	RelevantAttributesToCapture.Add(MaxHealthDef);
}

float UMMC_MutantRestore::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	// 提取聚合标签
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	// 获取捕获到的最大生命值
	float MaxHealth = 0.0f;
	GetCapturedAttributeMagnitude(MaxHealthDef, Spec, EvaluationParameters, MaxHealth);
	MaxHealth = FMath::Max<float>(MaxHealth, 0.0f);

	// 核心逻辑：检查目标身上是否有血月的 Tag
	float Coefficient = 0.1f; // 默认 10%
	if (TargetTags->HasTag(TAG_STATE_ENV_BLOODMOON))
	{
		Coefficient = 0.2f; // 血月期间 20%
	}

	return MaxHealth * Coefficient;
}
