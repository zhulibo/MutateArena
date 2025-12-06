#include "MutantSmoker.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "MutateArena/Characters/Data/CharacterType.h"

AMutantSmoker::AMutantSmoker()
{
	MutantCharacterName = EMutantCharacterName::Smoker;
	
	DefaultMaxWalkSpeed = 650.f;
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

void AMutantSmoker::OnRightHandCapsuleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnRightHandCapsuleOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
}

void AMutantSmoker::OnLeftHandCapsuleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnLeftHandCapsuleOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
}
