#include "CrosshairComponent.h"

#include "CombatComponent.h"
#include "RecoilComponent.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "TimerManager.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "MutateArena/Equipments/Weapon.h"
#include "MutateArena/PlayerControllers/BaseController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MutateArena/System/UISubsystem.h"

UCrosshairComponent::UCrosshairComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCrosshairComponent::BeginPlay()
{
	Super::BeginPlay();
	
	HumanCharacter = Cast<AHumanCharacter>(GetOwner());

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
	if (HumanCharacter == nullptr || HumanCharacter->CombatComp == nullptr) return;
	
	if (UISubsystem == nullptr)
	{
		if (APlayerController* PlayerController = Cast<APlayerController>(HumanCharacter->GetController()))
		{
			UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(PlayerController->GetLocalPlayer());
		}
	}
	if (UISubsystem == nullptr) return;
	
	AWeapon* Weapon = HumanCharacter->CombatComp->GetCurWeapon();
	if (Weapon == nullptr)
	{
		UISubsystem->ChangeCrosshairSpread.Broadcast(0.04); // 需与材质节点的默认 Spread 一致
		return;
	}

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
	if (URecoilComponent* RecoilComponent = HumanCharacter->RecoilComp)
	{
		ShootFactor = FMath::Clamp(RecoilComponent->RecoilVertTotal / Weapon->RecoilVertRef_Crosshair, 0.f, 1.f);
	}

	float TotalFactor = 3.f + VelocityFactor * 1.f + JumpFactor * 2.f + ShootFactor * 3.f;
	
	// 计算原始的像素级扩散值
	float PixelSpread = Weapon->CrosshairBaseSpread * TotalFactor;

	// 转换为 UV 空间的扩散值 UV 0（中心） 到 0.5（边缘）
	float UVSpread = PixelSpread * 0.01f;
	
	// UE_LOG(LogTemp, Warning, TEXT("PixelSpread: %f UVSpread: %f"), PixelSpread, UVSpread);
	
	// float DPIScale = UWidgetLayoutLibrary::GetViewportScale(GetWorld());
	
	if (UVSpread > 0.4) UVSpread = 0.4;
	
	UISubsystem->ChangeCrosshairSpread.Broadcast(UVSpread);
}
