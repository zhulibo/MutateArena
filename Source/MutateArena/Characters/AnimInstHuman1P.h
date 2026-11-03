#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "MutateArena/Equipments/Data/EquipmentType.h"
#include "AnimInstHuman1P.generated.h"

class AHumanCharacter;
class UCombatComponent;
class UMAMovementComponent;

UCLASS(Abstract)
class MUTATEARENA_API UAnimInstHuman1P : public UAnimInstance
{
	GENERATED_BODY()

public:
	UAnimInstHuman1P();

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

	UPROPERTY(BlueprintReadOnly)
	bool bIsOnLadder;
	UPROPERTY(BlueprintReadOnly)
	float Speed;
	UPROPERTY(BlueprintReadOnly)
	float WalkPlayRate = 1.f;
	UPROPERTY(BlueprintReadOnly)
	bool bIsInAir;
	UPROPERTY(BlueprintReadOnly)
	bool bIsAccelerating;
	UPROPERTY(BlueprintReadOnly)
	bool bIsCrouched;
	UPROPERTY(BlueprintReadOnly)
	bool bIsAiming = false;
	UPROPERTY(BlueprintReadOnly)
	EEquipmentName EquipmentName = EEquipmentName::None;
	UPROPERTY(BlueprintReadOnly)
	float AimYaw;
	UPROPERTY(BlueprintReadOnly)
	float AimPitch;
	UPROPERTY(BlueprintReadOnly)
	FRotator Spine_01_Rotator;
	UPROPERTY(BlueprintReadOnly)
	FRotator Spine_02_Rotator;
	UPROPERTY(BlueprintReadOnly)
	FRotator Spine_03_Rotator;
};
