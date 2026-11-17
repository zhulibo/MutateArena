#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Abilities/Equipments/GA_Equipment.h"
#include "GA_Weapon_Reload.generated.h"

// TAG_EVENT_WEAPON_RELOAD_END 仅挂在 3P 蒙太奇，各端都能看到，第三人称 BodyMesh 在本地虽隐藏但仍在 Tick
UCLASS(Abstract)
class MUTATEARENA_API UGA_Weapon_Reload : public UGA_Equipment
{
	GENERATED_BODY()

public:
	UGA_Weapon_Reload();

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                                const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	// 监听蒙太奇播放完成或被打断
	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageCancelled();

	// 监听来自 AnimNotify (TAG_EVENT_WEAPON_RELOAD_END) 的事件
	UFUNCTION()
	void OnReloadEventReceived(FGameplayEventData Payload);

	virtual void ReloadAmmo();

private:
	bool bHasReloaded = false;
	
};
