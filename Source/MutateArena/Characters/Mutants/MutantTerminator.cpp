#include "MutantTerminator.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "MutateArena/Characters/Data/CharacterType.h"
#include "GameFramework/CharacterMovementComponent.h"

AMutantTerminator::AMutantTerminator(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void AMutantTerminator::BeginPlay()
{
	Super::BeginPlay();
}

void AMutantTerminator::LightAttackButtonPressed(const FInputActionValue& Value)
{
	Super::LightAttackButtonPressed(Value);
}

void AMutantTerminator::LightAttackButtonReleased(const FInputActionValue& Value)
{
	Super::LightAttackButtonReleased(Value);
}

void AMutantTerminator::HeavyAttackButtonPressed(const FInputActionValue& Value)
{
	Super::HeavyAttackButtonPressed(Value);
}

void AMutantTerminator::HeavyAttackButtonReleased(const FInputActionValue& Value)
{
	Super::HeavyAttackButtonReleased(Value);
}

void AMutantTerminator::ServerApplyDamage_Implementation(AActor* OtherActor, float Damage)
{
	Super::ServerApplyDamage_Implementation(OtherActor, Damage);
	
	if (OtherActor && BleedEffectClass)
	{
		// 获取双方的 ASC (Ability System Component)
		UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OtherActor);
		UAbilitySystemComponent* InstigatorASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(this);

		if (TargetASC && InstigatorASC)
		{
			// 创建 GE 上下文
			FGameplayEffectContextHandle ContextHandle = InstigatorASC->MakeEffectContext();
			
			// 【非常重要】将 Terminator (this) 注册为伤害的 Instigator 和 Causer
			// 这是后续在 AttributeSet 里面能拿到 InstigatorController 的关键
			ContextHandle.AddInstigator(this, this);

			// 创建规范句柄 (SpecHandle)
			FGameplayEffectSpecHandle SpecHandle = InstigatorASC->MakeOutgoingSpec(BleedEffectClass, 1.0f, ContextHandle);

			if (SpecHandle.IsValid())
			{
				// 将 GE 应用给对方
				TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			}
		}
	}
}
