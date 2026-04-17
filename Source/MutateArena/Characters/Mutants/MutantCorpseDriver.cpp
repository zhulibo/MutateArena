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

void AMutantCorpseDriver::OnRightHandCapsuleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnRightHandCapsuleOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
}

void AMutantCorpseDriver::OnLeftHandCapsuleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnLeftHandCapsuleOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
}
