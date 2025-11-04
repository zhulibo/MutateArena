#include "CrosshairComponent.h"

#include "CombatComponent.h"
#include "RecoilComponent.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "TimerManager.h"
#include "MutateArena/Equipments/Weapon.h"
#include "MutateArena/PlayerControllers/BaseController.h"
#include "GameFramework/CharacterMovementComponent.h"

UCrosshairComponent::UCrosshairComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCrosshairComponent::BeginPlay()
{
	Super::BeginPlay();

	if (HumanCharacter)
	{
		WalkSpeedRange = FVector2D(0.f, HumanCharacter->GetCharacterMovement()->MaxWalkSpeed);
	}
}

void UCrosshairComponent::TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaSeconds, TickType, ThisTickFunction);

	if (HumanCharacter && HumanCharacter->IsLocallyControlled())
	{
		SetHUDCrosshair(DeltaSeconds);
	}
}

void UCrosshairComponent::SetHUDCrosshair(float DeltaSeconds)
{
	if (HumanCharacter == nullptr || HumanCharacter->CombatComponent == nullptr) return;
	if (BaseController == nullptr) BaseController = Cast<ABaseController>(HumanCharacter->Controller);
	if (BaseController == nullptr) return;

	AWeapon* Weapon = HumanCharacter->CombatComponent->GetCurWeapon();
	if (Weapon == nullptr) return;

	// 水平速度
	FVector Velocity = HumanCharacter->GetVelocity();
	Velocity.Z = 0.f;
	VelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, FVector2D(0.f, 1.f), Velocity.Size());

	// 跳跃
	if (HumanCharacter->GetCharacterMovement()->IsFalling())
	{
		JumpFactor = FMath::FInterpTo(JumpFactor, 1.f, DeltaSeconds, 10.f);
	}
	else
	{
		JumpFactor = FMath::FInterpTo(JumpFactor, 0.f, DeltaSeconds, 20.f);
	}

	// 射击
	if (URecoilComponent* RecoilComponent = HumanCharacter->RecoilComponent)
	{
		ShootFactor = FMath::Clamp(RecoilComponent->RecoilVertTotal / Weapon->RecoilVertRef_Crosshair, 0.f, 1.f);
	}

	float TotalFactor = 4.f + VelocityFactor * 1.f + JumpFactor * 2.f + ShootFactor * 2.f;

	BaseController->ChangeCrosshairSpread.Broadcast(Weapon->CrosshairBaseSpread * TotalFactor);
}
