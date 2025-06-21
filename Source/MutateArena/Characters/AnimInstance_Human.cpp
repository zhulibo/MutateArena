#include "AnimInstance_Human.h"

#include "HumanCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/CombatComponent.h"
#include "MutateArena/Equipments/Equipment.h"
#include "MutateArena/Equipments/Data/EquipmentType.h"

UAnimInstance_Human::UAnimInstance_Human()
{
	// 多线程动画更新开启时，在服务端播放非本地角色动画蒙太奇时，动画通知会触发两次。
	// 性能影响较大，暂时在 UAN_ShellReload::Notify 中处理。
	// bUseMultiThreadedAnimationUpdate = false;
}

void UAnimInstance_Human::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (HumanCharacter == nullptr) HumanCharacter = Cast<AHumanCharacter>(TryGetPawnOwner());
	if (HumanCharacter == nullptr || HumanCharacter->CombatComponent == nullptr) return;

	FVector Velocity = HumanCharacter->GetVelocity();
	Velocity.Z = 0.f;
	Speed = Velocity.Size();

	bIsInAir = HumanCharacter->GetCharacterMovement()->IsFalling();
	bIsAccelerating = HumanCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f;
	bIsCrouched = HumanCharacter->bIsCrouched;
	bIsAiming = HumanCharacter->CombatComponent->bIsAiming;
	if (HumanCharacter->CombatComponent->GetCurEquipment())
	{
		EquipmentName = HumanCharacter->CombatComponent->GetCurEquipment()->EquipmentName;
	}
	else
	{
		EquipmentName = EEquipmentName::None;
	}

	// 计算瞄准方向与移动方向的偏移量，用于控制脚步朝向
	FRotator AimRotation = HumanCharacter->GetBaseAimRotation();
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(HumanCharacter->GetVelocity());
	FRotator DiffRotation = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	AimYaw = DiffRotation.Yaw;

	// 根据AimPitch计算骨骼偏移量
	AimPitch = HumanCharacter->AimPitch;
	Spine_01_Rotator.Roll = UKismetMathLibrary::MapRangeClamped(AimPitch, -90.f, 90.f, 20.f, -20.f);
	Spine_02_Rotator.Roll = UKismetMathLibrary::MapRangeClamped(AimPitch, -90.f, 90.f, 10.f, -10.f);
	Spine_03_Rotator.Roll = UKismetMathLibrary::MapRangeClamped(AimPitch, -90.f, 90.f, 60.f, -60.f);
}
