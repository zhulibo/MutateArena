#include "AnimInstHuman1P.h"

#include "HumanCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/CombatComponent.h"
#include "Components/MAMovementComponent.h"
#include "MutateArena/Equipments/Equipment.h"
#include "MutateArena/Equipments/Weapon.h"

UAnimInstHuman1P::UAnimInstHuman1P()
{
}

void UAnimInstHuman1P::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (HumanChar == nullptr) HumanChar = Cast<AHumanCharacter>(TryGetPawnOwner());
	if (CombatComp == nullptr) CombatComp = HumanChar ? HumanChar->CombatComp : nullptr;
	if (MovementComp == nullptr) MovementComp = HumanChar ? HumanChar->MovementComp : nullptr;
	if (!HumanChar || !CombatComp || !MovementComp) return;
	
	bIsOnLadder = MovementComp->MovementMode == MOVE_Custom && MovementComp->CustomMovementMode == CMOVE_Ladder;
	
	FVector Velocity = HumanChar->GetVelocity();
	Speed = bIsOnLadder ? FMath::Abs(Velocity.Z) : Velocity.Size2D();
	
	bIsInAir = MovementComp->IsFalling() && !bIsOnLadder;
	bIsAccelerating = MovementComp->GetCurrentAcceleration().Size() > 0.f;
	bIsCrouched = HumanChar->bIsCrouched;
	
	if (AEquipment* CurEquipment = CombatComp->GetCurEquipment())
	{
		EquipmentName = CurEquipment->EquipmentName;
		// FString EnumString = UEnum::GetValueAsString(CurEquipment->EquipmentName);
		// UE_LOG(LogTemp, Warning, TEXT("EquipmentName = %s"), *EnumString);
		
		WalkPlayRate = HumanChar->GetMaxWalkSpeed() / HumanChar->DefaultMaxWalkSpeed;
		if (CombatComp->IsAiming())
		{
			WalkPlayRate *= CurEquipment->AimingWalkSpeedMul;
		}
		else
		{
			WalkPlayRate *= CurEquipment->WalkSpeedMul;
		}
	}
	else
	{
		WalkPlayRate = 1.f;
	}

	if (HumanChar->IsLocallyControlled()) // TODO 非本地瞄准动画暂时禁用了
	{
		if (AWeapon* CurWeapon = CombatComp->GetCurWeapon())
		{
			// if (!Montage_IsPlaying(CurWeapon->ADSMontage_C)) // 完全进入瞄准状态
			{
				bIsAiming = CombatComp->IsAiming();
			}
		}
	}

	AimPitch = HumanChar->AimPitch;
}
