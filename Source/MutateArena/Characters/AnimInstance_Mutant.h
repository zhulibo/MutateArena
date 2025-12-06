#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "AnimInstance_Mutant.generated.h"

UCLASS()
class MUTATEARENA_API UAnimInstance_Mutant : public UAnimInstance
{
	GENERATED_BODY()

public:
	UAnimInstance_Mutant();

protected:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UPROPERTY()
	class AMutantCharacter* MutantCharacter;
	UPROPERTY()
	class UPlayerSubsystem* PlayerSubsystem;
	
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
