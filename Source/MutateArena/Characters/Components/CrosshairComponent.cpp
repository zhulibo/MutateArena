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

	// if (HumanCharacter)
	// {
	// 	WalkSpeedRange = FVector2D(0.f, HumanCharacter->GetCharacterMovement()->MaxWalkSpeed);
	// }
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

	AWeapon* Weapon = HumanCharacter->CombatComponent->GetUsingWeapon();
	if (Weapon == nullptr) return;

	// TODO 做各种动作时应关联射击精度与准星扩塞，不能只改变扩塞

	// 水平速度
	// FVector Velocity = HumanCharacter->GetVelocity();
	// Velocity.Z = 0.f;
	// VelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityFactorRange, Velocity.Size());

	// 跳跃
	// if (HumanCharacter->GetCharacterMovement()->IsFalling())
	// {
	// 	JumpFactor = FMath::FInterpTo(JumpFactor, 1.f, DeltaSeconds, 10.f);
	// }
	// else
	// {
	// 	JumpFactor = FMath::FInterpTo(JumpFactor, 0.f, DeltaSeconds, 20.f);
	// }

	// 瞄准
	// if (HumanCharacter->CombatComponent->bIsAiming)
	// {
	// 	AimFactor = FMath::FInterpTo(AimFactor, -0.2f, DeltaSeconds, 10.f);
	// }
	// else
	// {
	// 	AimFactor = FMath::FInterpTo(AimFactor, 0.f, DeltaSeconds, 20.f);
	// }

	// 射击
	if (URecoilComponent* RecoilComponent = HumanCharacter->RecoilComponent)
	{
		ShootFactor = (RecoilComponent->GetCurRecoilVert() + FMath::Abs(RecoilComponent->GetCurRecoilVert()))
		/ (Weapon->RecoilTotalVertLimit + Weapon->RecoilTotalHorLimit);
	}

	float TotalFactor = 1.f + VelocityFactor + JumpFactor + AimFactor + ShootFactor * 2.f;

	float BaseWeaponSpread = (Weapon->RecoilMaxVert + Weapon->RecoilMaxHor) * 4;

	BaseController->ChangeCrosshairSpread.Broadcast(2.f + BaseWeaponSpread * TotalFactor);
}
