#pragma once

#include "CoreMinimal.h"
#include "GA_Equipment.h"
#include "GA_Melee_Attack.generated.h"

class AMelee;
class UAbilityTask_PlayMontageAndWait;

UCLASS(Abstract)
class MUTATEARENA_API UGA_Melee_Attack : public UGA_Equipment
{
	GENERATED_BODY()

public:
	UGA_Melee_Attack();
	
protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UFUNCTION()
	void PlayMeleeMontage();

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterruptedOrCancelled();

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> CurrentMontageTask;

	bool bIsLightAttack = false;
	bool AttackButtonReleased = false;
	bool bFirstSectionEnded = false;
	bool bMontageCompleted = false;

	UFUNCTION()
	void OnAttackButtonReleased(FGameplayEventData Payload);

	void OnServerAttackButtonReleased();
	
	UFUNCTION()
	void OnFirstSectionEnd(FGameplayEventData Payload);
	
};
