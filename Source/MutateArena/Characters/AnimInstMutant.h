#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "AnimInstMutant.generated.h"

UCLASS()
class MUTATEARENA_API UAnimInstMutant : public UAnimInstance
{
	GENERATED_BODY()

public:
	UAnimInstMutant();

	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	UPROPERTY()
	class AMutantCharacter* MutantChar;
	UPROPERTY()
	class UPlayerSubsystem* PlayerSubsystem;
	UPROPERTY()
	class UMAMovementComponent* MovementComp;
	
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
