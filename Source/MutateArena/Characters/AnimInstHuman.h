#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "MutateArena/Equipments/Data/EquipmentType.h"
#include "AnimInstHuman.generated.h"

UCLASS()
class MUTATEARENA_API UAnimInstHuman : public UAnimInstance
{
	GENERATED_BODY()

public:
	UAnimInstHuman();

protected:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UPROPERTY()
	class AHumanCharacter* HumanChar;
	UPROPERTY()
	class UCombatComponent* CombatComp;

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
