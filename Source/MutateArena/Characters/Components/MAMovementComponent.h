#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MAMovementComponent.generated.h"

UENUM(BlueprintType)
enum ECustomMovementMode
{
	CMOVE_None      UMETA(DisplayName = "None"),
	CMOVE_Ladder    UMETA(DisplayName = "Ladder")
};

UCLASS()
class MUTATEARENA_API UMAMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UMAMovementComponent();

	// 爬梯子速度占玩家当前最大行走速度的比例
	UPROPERTY()
	float LadderSpeedMultiplier = .4f;
    
	// 梯子上横向移动的灵敏度 (默认 0.5，即左右速度是上下速度的一半)
	UPROPERTY()
	float LadderLateralSpeedMultiplier = .5f;

protected:
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;

	void PhysLadder(float deltaTime, int32 Iterations);
	
};
