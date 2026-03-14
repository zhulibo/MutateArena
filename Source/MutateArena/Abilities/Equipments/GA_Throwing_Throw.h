#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Abilities/Equipments/GA_Equipment.h"
#include "GA_Throwing_Throw.generated.h"

UCLASS(Abstract)
class MUTATEARENA_API UGA_Throwing_Throw : public UGA_Equipment
{
	GENERATED_BODY()

public:
	UGA_Throwing_Throw();

protected:
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UFUNCTION()
	void OnMontageFinished();

	UFUNCTION()
	void OnThrowOutEventReceived(FGameplayEventData Payload);

private:
	bool bHasThrownOut = false;
	
};
