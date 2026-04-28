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
	
// 	// 加速跳 待测试
// public:
// 	// 期望达到的最大空中速度（用于限制点乘投影的上限）
// 	UPROPERTY(EditAnywhere)
// 	float MaxAirSpeed = 600.0f;
// 	// 空中加速度
// 	UPROPERTY(EditAnywhere)
// 	float AirAcceleration = 2000.0f;
// 	// 重写速度计算核心逻辑
// 	virtual void CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration) override;
// protected:
// 	// 空中加速算法
// 	void ApplyQuakeAirAccelerate(float DeltaTime, FVector WishDir, float WishSpeed);
// 	
// 	// 连跳 待测试
// public:
// 	// 落地后允许免受地表摩擦力减速的宽恕时间
// 	UPROPERTY(EditAnywhere)
// 	float BhopFrictionToleranceTime = 0.1f;
// protected:
// 	// 重写落地逻辑，用于记录落地时间
// 	virtual void ProcessLanded(const FHitResult& Hit, float remainingTime, int32 Iterations) override;
// 	// 重写减速应用，用于在宽恕期内豁免刹车
// 	virtual void ApplyVelocityBraking(float DeltaTime, float Friction, float BrakingDeceleration) override;
// 	// 记录最后一次真正落地的时间戳
// 	float LastLandedTime = 0.0f;
	
};
