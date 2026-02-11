#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "StateTreeTask_Human.generated.h"

USTRUCT()
struct MUTATEARENA_API FStateTreeTask_HumanChase_InstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	float AcceptanceRadius = 1000.f;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	float RotationInterpSpeed = 5.f;

};

USTRUCT(meta = (DisplayName = "Chase", Category = "Human"))
struct MUTATEARENA_API FStateTreeTask_HumanChase : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeTask_HumanChase_InstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	
};

USTRUCT()
struct MUTATEARENA_API FStateTreeTask_HumanFire_InstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	float RotationInterpSpeed = 5.f;

	// 射击或停顿已运行的时间
	UPROPERTY()
	float CurrentStateTimer = 0.f;
	// 总目标时间
	UPROPERTY()
	float CurrentTargetDuration = 0.f;

	UPROPERTY()
	bool bIsShooting = false; // 当前是否处于扣动扳机的状态
	
};

USTRUCT(meta = (DisplayName = "Fire", Category = "Human"))
struct MUTATEARENA_API FStateTreeTask_HumanFire : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeTask_HumanFire_InstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	
};

USTRUCT()
struct MUTATEARENA_API FStateTreeTask_HumanReload_InstanceData
{
	GENERATED_BODY()

	// UPROPERTY(EditAnywhere, Category = "Input")
	// TObjectPtr<AActor> TargetActor = nullptr;

};

USTRUCT(meta = (DisplayName = "Reload", Category = "Human"))
struct FStateTreeTask_HumanReload : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeTask_HumanReload_InstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	
};

USTRUCT()
struct MUTATEARENA_API FStateTreeTask_HumanWander_InstanceData
{
	GENERATED_BODY()

	// 游荡的半径范围
	UPROPERTY(EditAnywhere, Category = "Parameter")
	float WanderRadius = 2000.0f;

	// 停止距离
	UPROPERTY(EditAnywhere, Category = "Parameter")
	float AcceptanceRadius = 100.0f;

	// 转向速度
	UPROPERTY(EditAnywhere, Category = "Parameter")
	float RotationInterpSpeed = 5.f;

	// 内部使用的变量：记录本次随机选中的目标位置
	UPROPERTY()
	FVector TargetLocation = FVector::ZeroVector;

};

USTRUCT(meta = (DisplayName = "Wander", Category = "Human"))
struct MUTATEARENA_API FStateTreeTask_HumanWander : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeTask_HumanWander_InstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	
};
