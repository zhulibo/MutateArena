#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "AnimInstHuman1P_Layer.generated.h"

class AHumanCharacter;
class UCombatComponent;
class UMAMovementComponent;

UCLASS(Abstract)
class MUTATEARENA_API UAnimInstHuman1P_Layer : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

protected:
	UPROPERTY()
	TObjectPtr<AHumanCharacter> HumanChar;
	UPROPERTY()
	TObjectPtr<UCombatComponent> CombatComp;
	UPROPERTY()
	TObjectPtr<UMAMovementComponent> MovementComp;

};
