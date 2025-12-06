#include "MutantGhost.h"

#include "MutateArena/Abilities/AttributeSetBase.h"
#include "MutateArena/Abilities/MAAbilitySystemComponent.h"
#include "MutateArena/Characters/Data/CharacterType.h"
#include "MutateArena/Characters/Components/OverheadWidget.h"
#include "GameFramework/CharacterMovementComponent.h"

AMutantGhost::AMutantGhost()
{
	MutantCharacterName = EMutantCharacterName::Ghost;
	
	// 需保证与角色ASC中的MaxWalkSpeed一级速度一致
	DefaultMaxWalkSpeed = 650.f;
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

void AMutantGhost::OnRightHandCapsuleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnRightHandCapsuleOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
}

void AMutantGhost::OnLeftHandCapsuleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnLeftHandCapsuleOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
}

void AMutantGhost::OnASCInit()
{
	Super::OnASCInit();

	if (ASC && AttributeSetBase)
	{
		ASC->GetGameplayAttributeValueChangeDelegate(AttributeSetBase->GetJumpZVelocityAttribute()).AddUObject(this, &ThisClass::OnJumpZVelocityChanged);
	}
}

void AMutantGhost::OnJumpZVelocityChanged(const FOnAttributeChangeData& Data)
{
	GetCharacterMovement()->JumpZVelocity = Data.NewValue;
}

void AMutantGhost::ShowOverheadWidget(bool bIsShow)
{
	if (OverheadWidgetClass)
	{
		OverheadWidgetClass->ShowOverheadWidget(bIsShow);
	}
}
