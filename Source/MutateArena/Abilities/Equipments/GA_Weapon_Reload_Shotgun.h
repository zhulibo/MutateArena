#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Abilities/Equipments/GA_Weapon_Reload.h"
#include "GA_Weapon_Reload_Shotgun.generated.h"

UCLASS(Abstract)
class MUTATEARENA_API UGA_Weapon_Reload_Shotgun : public UGA_Weapon_Reload
{
	GENERATED_BODY()

public:
	UGA_Weapon_Reload_Shotgun();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, 
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnShellInsertEventReceived(FGameplayEventData Payload);
	
};
