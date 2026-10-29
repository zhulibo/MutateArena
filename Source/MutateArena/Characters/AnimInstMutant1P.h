#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "AnimInstMutant1P.generated.h"

UCLASS(Abstract)
class MUTATEARENA_API UAnimInstMutant1P : public UAnimInstance
{
	GENERATED_BODY()

public:
	UAnimInstMutant1P();

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
	float AimPitch;
	
};
