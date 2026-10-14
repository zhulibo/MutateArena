#include "AnimInstMutant1P.h"

#include "Components/MAMovementComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "MutateArena/Characters/MutantCharacter.h"
#include "MutateArena/System/PlayerSubsystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

UAnimInstMutant1P::UAnimInstMutant1P()
{
}

void UAnimInstMutant1P::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (MutantChar == nullptr) MutantChar = Cast<AMutantCharacter>(TryGetPawnOwner());
	if (MovementComp == nullptr) MovementComp = MutantChar ? MutantChar->MovementComp : nullptr;
	if (!MutantChar || !MovementComp) return;

	bIsOnLadder = MovementComp->MovementMode == MOVE_Custom && MovementComp->CustomMovementMode == CMOVE_Ladder;
	
	FVector Velocity = MutantChar->GetVelocity();
	Speed = bIsOnLadder ? FMath::Abs(Velocity.Z) : Velocity.Size2D();

	WalkPlayRate = MutantChar->GetMaxWalkSpeed() / MutantChar->DefaultMaxWalkSpeed;
	// UE_LOG(LogTemp, Warning, TEXT("DefaultWalkSpeed %f GetMaxWalkSpeed %f WalkPlayRate: %f"), MutantCharacter->DefaultWalkSpeed, MutantCharacter->GetMaxWalkSpeed(), WalkPlayRate);
	
	bIsInAir = MovementComp->IsFalling() && !bIsOnLadder;
	bIsAccelerating = MovementComp->GetCurrentAcceleration().Size() > 0.f;
	bIsCrouched = MutantChar->bIsCrouched;

	/**
	 * 瞬时切换角色，需要保持被销毁角色的AimPitch，但新角色生成后头几帧AimPitch为0，角色躯体闪动。
	 * 因为Character中的Controller未就绪，无法获取 APawn::GetViewRotation > AController::GetControlRotation。
	 */
	AimPitch = MutantChar->AimPitch;

	// HACK 本地死亡时，直接获取GetFirstPlayerController中的GetControlRotation().Pitch
	if (AimPitch == 0.f)
	{
		// UE_LOG(LogTemp, Warning, TEXT("AimPitch == 0.f"));
		if (PlayerSubsystem == nullptr) PlayerSubsystem = ULocalPlayer::GetSubsystem<UPlayerSubsystem>(GetWorld()->GetFirstLocalPlayerFromController());
		if (PlayerSubsystem && PlayerSubsystem->IsDead) // IsDead状态只持续很短时间，尽可能避免将本地AimPitch应用给非本地角色，同时也可避免影响到重生（3s）的本地角色
		{
			if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
			{
				AimPitch = MutantChar->MappingAimPitch(PlayerController->GetControlRotation().Pitch);
			}
		}
	}
}
