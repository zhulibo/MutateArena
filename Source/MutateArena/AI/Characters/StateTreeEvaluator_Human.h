#pragma once

#include "CoreMinimal.h"
#include "StateTreeEvaluatorBase.h"
#include "StateTreeEvaluator_Human.generated.h"

USTRUCT()
struct MUTATEARENA_API FStateTreeTask_HumanFindTarget_InstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Output")
	TObjectPtr<AActor> TargetActor = nullptr;
	
};

USTRUCT(meta = (DisplayName = "Find Target", Category = "Human"))
struct MUTATEARENA_API FStateTreeTask_HumanFindTarget : public FStateTreeEvaluatorCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeTask_HumanFindTarget_InstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	
	virtual void TreeStart(FStateTreeExecutionContext& Context) const override;
	virtual void Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	
};

USTRUCT()
struct MUTATEARENA_API FStateTreeEvaluator_HumanAmmo_InstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Output")
	int32 Ammo = 0;

	UPROPERTY(EditAnywhere, Category = "Output")
	int32 CarriedAmmo = 0;
};

USTRUCT(meta = (DisplayName = "Ammo", Category = "Human"))
struct MUTATEARENA_API FStateTreeEvaluator_HumanAmmo : public FStateTreeEvaluatorCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeEvaluator_HumanAmmo_InstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual void Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	
};
