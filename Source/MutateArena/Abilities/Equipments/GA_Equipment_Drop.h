#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Abilities/Equipments/GA_Equipment.h"
#include "GA_Equipment_Drop.generated.h"

UCLASS(Abstract)
class MUTATEARENA_API UGA_Equipment_Drop : public UGA_Equipment
{
	GENERATED_BODY()

public:
	UGA_Equipment_Drop();

protected:
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	
};
