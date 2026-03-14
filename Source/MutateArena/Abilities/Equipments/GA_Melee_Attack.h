#pragma once

#include "CoreMinimal.h"
#include "GA_Equipment.h"
#include "GA_Melee_Attack.generated.h"

class AMelee;

UCLASS(Abstract)
class MUTATEARENA_API UGA_Melee_Attack : public UGA_Equipment
{
	GENERATED_BODY()

public:
	UGA_Melee_Attack();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	UFUNCTION()
	void PlayMeleeMontage();

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterruptedOrCancelled();

	bool AttackButtonReleased = false;

	UFUNCTION()
	void OnAttackButtonReleased(FGameplayEventData Payload);
	
	UFUNCTION()
	void OnFirstSectionEnd(FGameplayEventData Payload);
	
};
