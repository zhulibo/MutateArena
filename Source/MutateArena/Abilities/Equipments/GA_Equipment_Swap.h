#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Abilities/Equipments/GA_Equipment.h"
#include "MutateArena/Equipments/Data/EquipmentType.h"
#include "GA_Equipment_Swap.generated.h"

UCLASS(Abstract)
class MUTATEARENA_API UGA_Equipment_Swap : public UGA_Equipment
{
	GENERATED_BODY()

public:
	UGA_Equipment_Swap();
	
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	// 监听来自 AN_SwapOutEnd 的切出完成事件
	UFUNCTION()
	void OnSwapOutEventReceived(FGameplayEventData Payload);
	
	UFUNCTION()
	void OnSwapInEventReceived(FGameplayEventData Payload);

	bool bIsPlayingSwapIn = false;
	
	UFUNCTION()
	void OnSwapOutMontageFinished();
	UFUNCTION()
	void OnSwapInMontageFinished();

	// 缓存要切换到的目标装备类型
	UPROPERTY()
	EEquipmentType TargetEquipmentType;

	UPROPERTY()
	class AEquipment* TargetEquipment;
	
};
