#include "AnimInstance_Mutant.h"

#include "MutateArena/Characters/MutantCharacter.h"
#include "MutateArena/System/PlayerSubsystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

UAnimInstance_Mutant::UAnimInstance_Mutant()
{
	// 多线程动画开启且在服务端播放非本地角色动画蒙太奇时，动画通知会触发两次。
	// 性能影响较大，暂时在 UAN_ShellReload::Notify 中处理。
	// bUseMultiThreadedAnimationUpdate = false;
}

void UAnimInstance_Mutant::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (MutantCharacter == nullptr) MutantCharacter = Cast<AMutantCharacter>(TryGetPawnOwner());
	if (MutantCharacter == nullptr) return;

	FVector Velocity = MutantCharacter->GetVelocity();
	Velocity.Z = 0.f;
	Speed = Velocity.Size();

	WalkPlayRate = MutantCharacter->GetMaxWalkSpeed() / MutantCharacter->DefaultMaxWalkSpeed;
	// UE_LOG(LogTemp, Warning, TEXT("DefaultWalkSpeed %f GetMaxWalkSpeed %f WalkPlayRate: %f"), MutantCharacter->DefaultWalkSpeed, MutantCharacter->GetMaxWalkSpeed(), WalkPlayRate);

	bIsInAir = MutantCharacter->GetCharacterMovement()->IsFalling();
	bIsAccelerating = MutantCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f;
	bIsCrouched = MutantCharacter->bIsCrouched;

	// 计算瞄准方向与移动方向的偏移量，用于控制脚步朝向
	FRotator AimRotation = MutantCharacter->GetBaseAimRotation();
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(MutantCharacter->GetVelocity());
	FRotator DiffRotation = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	AimYaw = DiffRotation.Yaw;

	/**
	 * 人类被感染、开局被选为突变体、突变体切换角色等瞬间切换角色时，需要保持被销毁角色的AimPitch，但新角色生成后头几帧AimPitch为0，角色躯体闪动。
	 * 因为Character中的Controller未就绪，无法获取APawn::GetViewRotation > AController::GetControlRotation。
	 */
	AimPitch = MutantCharacter->AimPitch;

	// HACK 本地死亡时，直接获取GetFirstPlayerController中的GetControlRotation().Pitch
	if (AimPitch == 0.f)
	{
		if (PlayerSubsystem == nullptr) PlayerSubsystem = ULocalPlayer::GetSubsystem<UPlayerSubsystem>(GetWorld()->GetFirstLocalPlayerFromController());
		if (PlayerSubsystem && PlayerSubsystem->IsDead) // IsDead状态只持续很短时间，尽可能避免将本地AimPitch应用给非本地角色，同时也可避免影响到重生（3s）的本地角色
		{
			if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
			{
				AimPitch = MutantCharacter->MappingAimPitch(PlayerController->GetControlRotation().Pitch);
			}
		}
	}
	// TODO 非本地角色躯体闪动暂未处理

	// 根据AimPitch计算骨骼偏移量
	Spine_01_Rotator.Roll = UKismetMathLibrary::MapRangeClamped(AimPitch, -90.f, 90.f, 10.f, -10.f);
	Spine_02_Rotator.Roll = UKismetMathLibrary::MapRangeClamped(AimPitch, -90.f, 90.f, 10.f, -10.f);
	Spine_03_Rotator.Roll = UKismetMathLibrary::MapRangeClamped(AimPitch, -90.f, 90.f, 70.f, -70.f);
}
