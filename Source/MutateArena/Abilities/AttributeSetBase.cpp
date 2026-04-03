#include "AttributeSetBase.h"

#include "Kismet/GameplayStatics.h"
#include "MutateArena/Equipments/Data/DamageTypeBleed.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UAttributeSetBase::UAttributeSetBase()
{
	InitCharacterLevel(1.f);
}

void UAttributeSetBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UAttributeSetBase, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAttributeSetBase, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAttributeSetBase, DamageReceivedMul, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAttributeSetBase, RepelReceivedMul, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAttributeSetBase, CharacterLevel, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAttributeSetBase, MaxWalkSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAttributeSetBase, JumpZVelocity, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UAttributeSetBase, BodyResistance, COND_None, REPNOTIFY_Always);
}

void UAttributeSetBase::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	ClampAttr(Attribute, NewValue);
}

void UAttributeSetBase::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	ClampAttr(Attribute, NewValue);
}

void UAttributeSetBase::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	// if (Attribute == GetHealthAttribute())
	// {
	// 	UE_LOG(LogTemp, Warning, TEXT("Health OldValue %f NewValue %f"), OldValue, NewValue);
	// }

	// if (Attribute == GetMaxHealthAttribute())
	// {
	// 	UE_LOG(LogTemp, Warning, TEXT("MaxHealth OldValue %f NewValue %f"), OldValue, NewValue);
	// }
}

void UAttributeSetBase::ClampAttr(const FGameplayAttribute& Attribute, float& NewValue) const
{
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
}

void UAttributeSetBase::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// 拦截对 Damage 的修改
	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		// 1. 获取本次 GE 造成的伤害值
		float LocalDamageDone = GetDamage();
		
		// 2. 将 Damage 属性清零（非常重要，防止下一次 tick 累加）
		SetDamage(0.f);

		if (LocalDamageDone > 0.f)
		{
			// 3. 获取受击者
			AActor* TargetActor = GetOwningAbilitySystemComponent()->GetAvatarActor();
			
			// 4. 从 GE 的上下文中提取施法者（即 Cutter）
			AActor* Instigator = Data.EffectSpec.GetContext().GetInstigator();
			AController* InstigatorController = Instigator ? Instigator->GetInstigatorController() : nullptr;

			// 5. 触发原生伤害系统
			// 这将正确地路由到你的 AMutationMode::HumanReceiveDamage 或 MutantReceiveDamage
			UGameplayStatics::ApplyDamage(
				TargetActor,
				LocalDamageDone,
				InstigatorController,
				Instigator,
				UDamageTypeBleed::StaticClass()
			);
		}
	}
}

void UAttributeSetBase::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAttributeSetBase, MaxHealth, OldMaxHealth);
}

void UAttributeSetBase::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAttributeSetBase, Health, OldHealth);
}

void UAttributeSetBase::OnRep_DamageReceivedMul(const FGameplayAttributeData& OldDamageReceivedMul)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAttributeSetBase, DamageReceivedMul, OldDamageReceivedMul);
}

void UAttributeSetBase::OnRep_RepelReceivedMul(const FGameplayAttributeData& OldRepelReceivedMul)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAttributeSetBase, RepelReceivedMul, OldRepelReceivedMul);
}

void UAttributeSetBase::OnRep_CharacterLevel(const FGameplayAttributeData& OldCharacterLevel)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAttributeSetBase, CharacterLevel, OldCharacterLevel);
}

void UAttributeSetBase::OnRep_MaxWalkSpeed(const FGameplayAttributeData& OldMaxWalkSpeed)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAttributeSetBase, MaxWalkSpeed, OldMaxWalkSpeed);
}

void UAttributeSetBase::OnRep_JumpZVelocity(const FGameplayAttributeData& OldJumpZVelocity)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAttributeSetBase, JumpZVelocity, OldJumpZVelocity);
}

void UAttributeSetBase::OnRep_BodyResistance(const FGameplayAttributeData& OldBodyResistance)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAttributeSetBase, BodyResistance, OldBodyResistance);
}
