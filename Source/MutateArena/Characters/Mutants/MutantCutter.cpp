#include "MutantCutter.h"

#include "MutateArena/Characters/Data/CharacterType.h"
#include "GameFramework/CharacterMovementComponent.h"

AMutantCutter::AMutantCutter()
{
	MutantCharacterName = EMutantCharacterName::Cutter;
	
	// 需保证与角色ASC中的MaxWalkSpeed一级速度一致
	DefaultMaxWalkSpeed = 650.f;
	GetCharacterMovement()->MaxWalkSpeed = DefaultMaxWalkSpeed;
	GetCharacterMovement()->MaxWalkSpeedCrouched = DefaultMaxWalkSpeed * 0.5f;
}

void AMutantCutter::BeginPlay()
{
	Super::BeginPlay();
}

void AMutantCutter::LightAttackButtonPressed(const FInputActionValue& Value)
{
	Super::LightAttackButtonPressed(Value);
}

void AMutantCutter::LightAttackButtonReleased(const FInputActionValue& Value)
{
	Super::LightAttackButtonReleased(Value);
}

void AMutantCutter::HeavyAttackButtonPressed(const FInputActionValue& Value)
{
	Super::HeavyAttackButtonPressed(Value);
}

void AMutantCutter::HeavyAttackButtonReleased(const FInputActionValue& Value)
{
	Super::HeavyAttackButtonReleased(Value);
}

void AMutantCutter::OnRightHandCapsuleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnRightHandCapsuleOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
}

void AMutantCutter::OnLeftHandCapsuleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnLeftHandCapsuleOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
}
