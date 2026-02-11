#include "StateTreeEvaluator_Mutant.h"

#include "StateTreeExecutionContext.h"
#include "MutateArena/Characters/MutantCharacter.h"

// 查找目标
void FStateTreeTask_MutantFindTarget::TreeStart(FStateTreeExecutionContext& Context) const
{
	Tick(Context, 0.f);
}

void FStateTreeTask_MutantFindTarget::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData<FInstanceDataType>(*this);

	AMutantCharacter* MyMutantCharacter = Cast<AMutantCharacter>(Context.GetOwner());
	if (MyMutantCharacter == nullptr)
	{
		InstanceData.TargetActor = nullptr;
		return;
	}

	if (AActor* FoundTarget = MyMutantCharacter->GetBestPerceivedTarget())
	{
		InstanceData.TargetActor = FoundTarget;
	}
	else
	{
		InstanceData.TargetActor = nullptr;
	}
}
