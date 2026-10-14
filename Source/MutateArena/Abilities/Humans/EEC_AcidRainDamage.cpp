#include "EEC_AcidRainDamage.h"

#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/Abilities/AttributeSetBase.h"

struct AcidRainStatics
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(MaxHealth);

	AcidRainStatics()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(UAttributeSetBase, MaxHealth, Target, false);
	}
};

static const AcidRainStatics& GetAcidRainStatics()
{
	static AcidRainStatics Statics;
	return Statics;
}

UEEC_AcidRainDamage::UEEC_AcidRainDamage()
{
	RelevantAttributesToCapture.Add(GetAcidRainStatics().MaxHealthDef);
}

void UEEC_AcidRainDamage::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
                                                 FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
	AActor* TargetActor = TargetASC ? TargetASC->GetAvatarActor() : nullptr;

	if (!TargetActor || !TargetASC) return;

	// 1. 室外判定 (向上打射线检测遮挡)
	FVector StartLoc = TargetActor->GetActorLocation();
	FVector EndLoc = StartLoc + FVector(0.0f, 0.0f, GameConstants::MapMaxHeight);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(TargetActor);

	bool bIsIndoors = TargetActor->GetWorld()->LineTraceSingleByChannel(
		HitResult,
		StartLoc,
		EndLoc,
		ECC_Visibility,
		QueryParams
	);
	
	// 2. 如果射线被阻挡，说明在室内，不造成伤害
	if (bIsIndoors)
	{
		return;
	}

	float MaxHealth = 0.0f;
	FAggregatorEvaluateParameters EvaluationParameters;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(GetAcidRainStatics().MaxHealthDef, EvaluationParameters, MaxHealth);

	float DamageAmount = MaxHealth * 0.007f;

	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(UAttributeSetBase::GetDamageAttribute(), EGameplayModOp::Additive, DamageAmount));
}
