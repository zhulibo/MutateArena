#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Abilities/GameplayAbilityBase.h"
#include "GA_MutantAttack.generated.h"

class UAbilityTask_PlayMontageAndWait;

// TAG_EVENT_MUTANT_ATTACK_FIRSTSECTIONEND 仅挂在 3P 蒙太奇，各端都能看到，第三人称 Mesh 在本地虽隐藏但仍在 Tick
// UANS_MutantLeftHandAttack / UANS_MutantRightHandAttack 除挂在 3P，还要挂在 1P 蒙太奇上，本机表现须与结果一致，取本地球体扫掠结果 ServerApplyDamage 扣血。
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

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> CurrentMontageTask;

	bool bIsLightAttack = true;
	bool bAttackButtonReleased = false;
	bool bFirstSectionEnded = false;
	bool bMontageCompleted = false;

	UFUNCTION()
	void OnAttackButtonReleased(FGameplayEventData Payload);

	void OnServerAttackButtonReleased();
	
	UFUNCTION()
	void OnFirstSectionEnd(FGameplayEventData Payload);
	
};
