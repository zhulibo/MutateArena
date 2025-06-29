#include "AttributeSetBase.h"

#include "Net/UnrealNetwork.h"

UAttributeSetBase::UAttributeSetBase()
{
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
