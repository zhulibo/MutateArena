#pragma once

#include "CoreMinimal.h"
#include "StateTreeEvaluatorBase.h"
#include "StateTreeEvaluator_Mutant.generated.h"

USTRUCT()
struct MUTATEARENA_API FStateTreeTask_MutantFindTarget_InstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Output")
	TObjectPtr<AActor> TargetActor = nullptr;
	
};

USTRUCT(meta = (DisplayName = "Find Target", Category = "Mutant"))
struct MUTATEARENA_API FStateTreeTask_MutantFindTarget : public FStateTreeEvaluatorCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeTask_MutantFindTarget_InstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	
	virtual void TreeStart(FStateTreeExecutionContext& Context) const override;
	virtual void Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	
};
