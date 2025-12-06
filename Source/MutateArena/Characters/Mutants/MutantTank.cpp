#include "MutantTank.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "MutateArena/Characters/Data/CharacterType.h"

AMutantTank::AMutantTank()
{
	MutantCharacterName = EMutantCharacterName::Tank;
	
	// 需保证与角色ASC中的MaxWalkSpeed一级速度一致
	DefaultMaxWalkSpeed = 650.f;
	GetCharacterMovement()->MaxWalkSpeed = DefaultMaxWalkSpeed;
	GetCharacterMovement()->MaxWalkSpeedCrouched = DefaultMaxWalkSpeed * 0.5f;
}

void AMutantTank::BeginPlay()
{
	Super::BeginPlay();
}

void AMutantTank::LightAttackButtonPressed(const FInputActionValue& Value)
{
	Super::LightAttackButtonPressed(Value);
}

void AMutantTank::LightAttackButtonReleased(const FInputActionValue& Value)
{
	Super::LightAttackButtonReleased(Value);
}

void AMutantTank::HeavyAttackButtonPressed(const FInputActionValue& Value)
{
	Super::HeavyAttackButtonPressed(Value);
}

void AMutantTank::HeavyAttackButtonReleased(const FInputActionValue& Value)
{
	Super::HeavyAttackButtonReleased(Value);
}

void AMutantTank::OnRightHandCapsuleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnRightHandCapsuleOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
}

void AMutantTank::OnLeftHandCapsuleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnLeftHandCapsuleOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
}
