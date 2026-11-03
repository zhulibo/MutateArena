#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MAMovementComponent.generated.h"

UENUM(BlueprintType)
enum ECustomMovementMode
{
	CMOVE_None UMETA(DisplayName = "None"),
	CMOVE_Ladder UMETA(DisplayName = "Ladder")
};

UCLASS()
class MUTATEARENA_API UMAMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UMAMovementComponent();
	
	virtual bool CanAttemptJump() const override;
	
	// 记录最后一次落地的时间
	double LastLandedTime = -100.f;
	// 是否处于刚落地的可触发连跳状态
	bool bCanTriggerBHop = false;
	// 是否通过预输入（落地前0.1s）预约了连跳
	bool bBufferedBHop = false;
	// 连跳/鬼跳：落地前允许跳跃预输入的时间窗口
	UPROPERTY()
	float BHopPreLandBufferTime = 0.1f;
	// 连跳/鬼跳：落地后允许触发连跳判定及地面免摩擦的宽限时间
	UPROPERTY()
	float BHopPostLandGraceTime = 0.1f;
	virtual void ProcessLanded(const FHitResult& Hit, float remainingTime, int32 Iterations) override;
	virtual bool DoJump(bool bReplayingMoves, float DeltaTime) override;
	virtual float GetMaxSpeed() const override;
	
	UPROPERTY()
	float MaxStepUpHeight = 45.f;
	virtual bool StepUp(const FVector& GravDir, const FVector& Delta, const FHitResult &InHit, FStepDownResult* OutStepDownResult = nullptr) override;

	// 爬梯子速度占玩家当前最大行走速度的比例
	UPROPERTY()
	float LadderSpeedMultiplier = .4f;

	// 梯子上横向移动的灵敏度 (默认 0.5，即左右速度是上下速度的一半)
	UPROPERTY()
	float LadderLateralSpeedMultiplier = .5f;

protected:
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;

	void PhysLadder(float deltaTime, int32 Iterations);

	// 加速跳
public:
	// 期望达到的最大空中速度（用于限制点乘投影的上限） 在输入方向上投影的最大速度
	UPROPERTY(EditAnywhere)
	float MaxAirSpeed = 200.0f;
	// 空中加速度
	UPROPERTY(EditAnywhere)
	float AirAcceleration = 2000.0f;
	// 重写速度计算核心逻辑
	virtual void CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration) override;
protected:
	// 空中加速算法
	void ApplyQuakeAirAccelerate(float DeltaTime, FVector WishDir, float WishSpeed);

};
