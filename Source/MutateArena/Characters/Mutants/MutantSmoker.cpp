#include "MutantSmoker.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "MutateArena/Characters/Data/CharacterType.h"

AMutantSmoker::AMutantSmoker(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	MutantCharacterName = EMutantCharacterName::Smoker;
	
	DefaultMaxWalkSpeed = 550.f;
	GetCharacterMovement()->MaxWalkSpeed = DefaultMaxWalkSpeed;
	GetCharacterMovement()->MaxWalkSpeedCrouched = DefaultMaxWalkSpeed * 0.5f;
}

void AMutantSmoker::BeginPlay()
{
	Super::BeginPlay();
}

void AMutantSmoker::LightAttackButtonPressed(const FInputActionValue& Value)
{
	Super::LightAttackButtonPressed(Value);
}

void AMutantSmoker::LightAttackButtonReleased(const FInputActionValue& Value)
{
	Super::LightAttackButtonReleased(Value);
}

void AMutantSmoker::HeavyAttackButtonPressed(const FInputActionValue& Value)
{
	Super::HeavyAttackButtonPressed(Value);
}

void AMutantSmoker::HeavyAttackButtonReleased(const FInputActionValue& Value)
{
	Super::HeavyAttackButtonReleased(Value);
}
