#include "StateTreeEvaluator_Mutant.h"

#include "StateTreeExecutionContext.h"
#include "MutateArena/Characters/MutantCharacter.h"
#include "MutateArena/Characters/Components/AutoHostComponent.h"

// 查找目标
void FStateTreeTask_MutantFindTarget::TreeStart(FStateTreeExecutionContext& Context) const
{
	Tick(Context, 0.f);
}

void FStateTreeTask_MutantFindTarget::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData<FInstanceDataType>(*this);

	AMutantCharacter* MyHumanCharacter = Cast<AMutantCharacter>(Context.GetOwner());
	if (MyHumanCharacter == nullptr)
	{
		InstanceData.TargetActor = nullptr;
		return;
	}

	if (MyHumanCharacter->AutoHostComp)
	{
		if (AActor* FoundTarget = MyHumanCharacter->AutoHostComp->GetBestPerceivedTarget())
		{
			InstanceData.TargetActor = FoundTarget;
		}
		else
		{
			InstanceData.TargetActor = nullptr;
		}
	}
	else
	{
		InstanceData.TargetActor = nullptr;
	}
}
