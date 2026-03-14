#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Abilities/Equipments/GA_Equipment.h"
#include "GA_Weapon_Reload.generated.h"

UCLASS(Abstract)
class MUTATEARENA_API UGA_Weapon_Reload : public UGA_Equipment
{
	GENERATED_BODY()

public:
	UGA_Weapon_Reload();

protected:
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	// 监听蒙太奇播放完成或被打断
	UFUNCTION()
	void OnMontageFinished();

	// 监听来自 AnimNotify (UAN_SendGameplayEvent) 的事件
	UFUNCTION()
	void OnReloadEventReceived(FGameplayEventData Payload);
	
};
