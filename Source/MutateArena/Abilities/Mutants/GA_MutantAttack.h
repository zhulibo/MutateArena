#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Abilities/GameplayAbilityBase.h"
#include "GA_MutantAttack.generated.h"

UCLASS(Abstract)
class MUTATEARENA_API UGA_MutantAttack : public UGameplayAbilityBase
{
	GENERATED_BODY()

public:
	UGA_MutantAttack();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	UFUNCTION()
	void PlayAttackMontage();

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterruptedOrCancelled();

	bool bAttackButtonReleased = false;
	bool bIsLightAttack = true;

	UFUNCTION()
	void OnAttackButtonReleased(FGameplayEventData Payload);
	
	UFUNCTION()
	void OnFirstSectionEnd(FGameplayEventData Payload);
	
};
