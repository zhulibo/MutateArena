#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "AnimInstMutant1P.generated.h"

class AMutantCharacter;
class UPlayerSubsystem;
class UMAMovementComponent;
class APlayerController;

UCLASS(Abstract)
class MUTATEARENA_API UAnimInstMutant1P : public UAnimInstance
{
	GENERATED_BODY()

public:
	UAnimInstMutant1P();

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

protected:
	UPROPERTY()
	TObjectPtr<AMutantCharacter> MutantChar;
	UPROPERTY()
	TObjectPtr<UMAMovementComponent> MovementComp;
	UPROPERTY()
	TObjectPtr<UPlayerSubsystem> PlayerSubsystem;
	UPROPERTY()
	TObjectPtr<APlayerController> PlayerController;
	
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