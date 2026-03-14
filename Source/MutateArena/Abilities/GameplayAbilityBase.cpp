#include "GameplayAbilityBase.h"

UGameplayAbilityBase::UGameplayAbilityBase()
{
	// 对于每一个被赋予的技能Spec，只生成一个实例
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}
