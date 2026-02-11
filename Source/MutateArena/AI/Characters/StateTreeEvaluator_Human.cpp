#include "StateTreeEvaluator_Human.h"

#include "StateTreeExecutionContext.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Characters/Components/CombatComponent.h"
#include "MutateArena/Equipments/Weapon.h"

// 查找目标
void FStateTreeTask_HumanFindTarget::TreeStart(FStateTreeExecutionContext& Context) const
{
	Tick(Context, 0.f);
}

void FStateTreeTask_HumanFindTarget::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData<FInstanceDataType>(*this);

	AHumanCharacter* MyHumanCharacter = Cast<AHumanCharacter>(Context.GetOwner());
	if (MyHumanCharacter == nullptr)
	{
		InstanceData.TargetActor = nullptr;
		return;
	}

	if (AActor* FoundTarget = MyHumanCharacter->GetBestPerceivedTarget())
	{
		InstanceData.TargetActor = FoundTarget;
	}
	else
	{
		InstanceData.TargetActor = nullptr;
	}
}

// 弹药
void FStateTreeEvaluator_HumanAmmo::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData<FInstanceDataType>(*this);
	
	AHumanCharacter* HumanCharacter = Cast<AHumanCharacter>(Context.GetOwner());
	if (HumanCharacter == nullptr || HumanCharacter->CombatComponent == nullptr) return;

	if (AWeapon* Weapon = HumanCharacter->CombatComponent->GetCurWeapon())
	{
		InstanceData.Ammo = Weapon->Ammo;
		InstanceData.CarriedAmmo = Weapon->CarriedAmmo;
	}
}
