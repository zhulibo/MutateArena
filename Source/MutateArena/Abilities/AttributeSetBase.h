#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AttributeSetBase.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class MUTATEARENA_API UAttributeSetBase : public UAttributeSet
{
	GENERATED_BODY()

public:
	UAttributeSetBase();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue)const  override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

	UPROPERTY(ReplicatedUsing = OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UAttributeSetBase, MaxHealth)

	UPROPERTY(ReplicatedUsing = OnRep_Health)
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UAttributeSetBase, Health)

	UPROPERTY(ReplicatedUsing = OnRep_DamageReceivedMul)
	FGameplayAttributeData DamageReceivedMul;
	ATTRIBUTE_ACCESSORS(UAttributeSetBase, DamageReceivedMul)

	UPROPERTY(ReplicatedUsing = OnRep_RepelReceivedMul)
	FGameplayAttributeData RepelReceivedMul;
	ATTRIBUTE_ACCESSORS(UAttributeSetBase, RepelReceivedMul)

	UPROPERTY(ReplicatedUsing = OnRep_CharacterLevel)
	FGameplayAttributeData CharacterLevel;
	ATTRIBUTE_ACCESSORS(UAttributeSetBase, CharacterLevel)

	UPROPERTY(ReplicatedUsing = OnRep_MaxWalkSpeed)
	FGameplayAttributeData MaxWalkSpeed;
	ATTRIBUTE_ACCESSORS(UAttributeSetBase, MaxWalkSpeed)

	UPROPERTY(ReplicatedUsing = OnRep_JumpZVelocity)
	FGameplayAttributeData JumpZVelocity;
	ATTRIBUTE_ACCESSORS(UAttributeSetBase, JumpZVelocity)

protected:
	UFUNCTION()
	virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);
	UFUNCTION()
	virtual void OnRep_Health(const FGameplayAttributeData& OldHealth);
	UFUNCTION()
	virtual void OnRep_DamageReceivedMul(const FGameplayAttributeData& OldDamageReceivedMul);
	UFUNCTION()
	virtual void OnRep_RepelReceivedMul(const FGameplayAttributeData& OldRepelReceivedMul);
	UFUNCTION()
	virtual void OnRep_CharacterLevel(const FGameplayAttributeData& OldCharacterLevel);
	UFUNCTION()
	virtual void OnRep_MaxWalkSpeed(const FGameplayAttributeData& OldMaxWalkSpeed);
	UFUNCTION()
	virtual void OnRep_JumpZVelocity(const FGameplayAttributeData& OldJumpZVelocity);

	void ClampAttr(const FGameplayAttribute& Attribute, float& NewValue) const;

};
