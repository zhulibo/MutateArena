#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Abilities/Equipments/GA_Equipment.h"
#include "MutateArena/Equipments/Data/EquipmentType.h"
#include "GA_Equipment_Swap.generated.h"

class UAnimMontage;
class AEquipment;

UCLASS(Abstract)
class MUTATEARENA_API UGA_Equipment_Swap : public UGA_Equipment
{
	GENERATED_BODY()

public:
	UGA_Equipment_Swap();

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                                const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
	                                FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
								 const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
							const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	
	// 监听来自切入动画 AN_SendGameplayEvent 的提前完成事件（允许切入动画尾段提前开火）
	UFUNCTION()
	void OnSwapInEventReceived(FGameplayEventData Payload);

	// 1P手臂切入蒙太奇自然结束回调
	void OnSwapInMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	// 服务端远端角色超时保底回调
	UFUNCTION()
	void OnServerWaitDelayFinished();

	UPROPERTY()
	EEquipmentSlot TargetEquipmentSlot = EEquipmentSlot::None;

	UPROPERTY()
	TObjectPtr<AEquipment> TargetEquipment;

	UPROPERTY()
	TObjectPtr<UAnimMontage> CurrentSwapInMontage;
};
