#include "MutantCorpseDriver.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MutateArena/Characters/Data/CharacterType.h"

AMutantCorpseDriver::AMutantCorpseDriver(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	MutantCharacterName = EMutantCharacterName::CorpseDriver;
	
	DefaultMaxWalkSpeed = 550.f;
	GetCharacterMovement()->MaxWalkSpeed = DefaultMaxWalkSpeed;
	GetCharacterMovement()->MaxWalkSpeedCrouched = DefaultMaxWalkSpeed * 0.5f;
}

void AMutantCorpseDriver::BeginPlay()
{
	Super::BeginPlay();
}

void AMutantCorpseDriver::LightAttackButtonPressed(const FInputActionValue& Value)
{
	Super::LightAttackButtonPressed(Value);
}

void AMutantCorpseDriver::LightAttackButtonReleased(const FInputActionValue& Value)
{
	Super::LightAttackButtonReleased(Value);
}

void AMutantCorpseDriver::HeavyAttackButtonPressed(const FInputActionValue& Value)
{
	Super::HeavyAttackButtonPressed(Value);
}

void AMutantCorpseDriver::HeavyAttackButtonReleased(const FInputActionValue& Value)
{
	Super::HeavyAttackButtonReleased(Value);
}
