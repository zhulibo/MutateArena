#include "MutantGhost.h"

#include "MutateArena/Abilities/AttributeSetBase.h"
#include "MutateArena/Abilities/MAAbilitySystemComponent.h"
#include "MutateArena/Characters/Data/CharacterType.h"
#include "MutateArena/Characters/Components/OverheadWidget.h"
#include "GameFramework/CharacterMovementComponent.h"

AMutantGhost::AMutantGhost(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	MutantCharacterName = EMutantCharacterName::Ghost;
	
	// 需保证与角色ASC中的MaxWalkSpeed一级速度一致
	DefaultMaxWalkSpeed = 550.f;
	GetCharacterMovement()->MaxWalkSpeed = DefaultMaxWalkSpeed;
	GetCharacterMovement()->MaxWalkSpeedCrouched = DefaultMaxWalkSpeed * 0.5f;
}

void AMutantGhost::BeginPlay()
{
	Super::BeginPlay();
}

void AMutantGhost::LightAttackButtonPressed(const FInputActionValue& Value)
{
	Super::LightAttackButtonPressed(Value);
}

void AMutantGhost::LightAttackButtonReleased(const FInputActionValue& Value)
{
	Super::LightAttackButtonReleased(Value);
}

void AMutantGhost::HeavyAttackButtonPressed(const FInputActionValue& Value)
{
	Super::HeavyAttackButtonPressed(Value);
}

void AMutantGhost::HeavyAttackButtonReleased(const FInputActionValue& Value)
{
	Super::HeavyAttackButtonReleased(Value);
}

void AMutantGhost::ShowOverheadWidget(bool bIsShow)
{
	if (OverheadWidgetClass)
	{
		OverheadWidgetClass->ShowOverheadWidget(bIsShow);
	}
}
