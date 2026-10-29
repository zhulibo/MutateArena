#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Abilities/GameplayAbilityBase.h"
#include "GA_Equipment.generated.h"

UCLASS(Abstract)
class MUTATEARENA_API UGA_Equipment : public UGameplayAbilityBase
{
	GENERATED_BODY()

public:
	UGA_Equipment();

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

protected:
	class AHumanCharacter* GetHumanCharacter() const;
	
	class AEquipment* GetCurEquipment() const;
	AEquipment* GetLastEquipment() const;
	class AWeapon* GetCurWeapon() const;
	class AMelee* GetCurMelee() const;

};
