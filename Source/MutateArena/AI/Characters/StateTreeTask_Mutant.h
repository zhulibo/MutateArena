#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "StateTreeTask_Mutant.generated.h"

USTRUCT()
struct MUTATEARENA_API FStateTreeTask_MutantChase_InstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	float AcceptanceRadius = 100.f;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	float RotationInterpSpeed = 5.f;
	
};

USTRUCT(meta = (DisplayName = "Chase", Category = "Mutant"))
struct MUTATEARENA_API FStateTreeTask_MutantChase : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeTask_MutantChase_InstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	
};

USTRUCT()
struct MUTATEARENA_API FStateTreeTask_MutantAttack_InstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	float RotationInterpSpeed = 5.f;
	
};

USTRUCT(meta = (DisplayName = "Attack", Category = "Mutant"))
struct MUTATEARENA_API FStateTreeTask_MutantAttack : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeTask_MutantAttack_InstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	
};

USTRUCT()
struct MUTATEARENA_API FStateTreeTask_MutantWander_InstanceData
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

USTRUCT(meta = (DisplayName = "Wander", Category = "Mutant"))
struct MUTATEARENA_API FStateTreeTask_MutantWander : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeTask_MutantWander_InstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	
};
