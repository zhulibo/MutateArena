#include "MMC_MutantHealth.h"

#include "MutateArena/Abilities/AttributeSetBase.h"
#include "MutateArena/PlayerStates/MutationPlayerState.h"

UMMC_MutantHealth::UMMC_MutantHealth()
{
	MaxHealthDef.AttributeToCapture = UAttributeSetBase::GetMaxHealthAttribute();
	MaxHealthDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	MaxHealthDef.bSnapshot = false;

	HealthDef.AttributeToCapture = UAttributeSetBase::GetHealthAttribute();
	HealthDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	HealthDef.bSnapshot = false;

	RelevantAttributesToCapture.Add(MaxHealthDef);
	RelevantAttributesToCapture.Add(HealthDef);
}

float UMMC_MutantHealth::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	float MaxHealth = 0.f;
	GetCapturedAttributeMagnitude(MaxHealthDef, Spec, EvaluationParameters, MaxHealth); // TODO 此处捕获到的依然是升级前的最大血量，需要升级后的
	float Health = 0.f;
	GetCapturedAttributeMagnitude(HealthDef, Spec, EvaluationParameters, Health);
	// UE_LOG(LogTemp, Warning, TEXT("MaxHealth %f Health %f"), MaxHealth, Health);

	// float NewHealth = FMath::Clamp(Health + 500.f * Spec.GetLevel(), 0.f, MaxHealth); 
	float NewHealth = Health + 1000.f;
	// UE_LOG(LogTemp, Warning, TEXT("NewHealth %f"), NewHealth);
	
	return NewHealth;
}
