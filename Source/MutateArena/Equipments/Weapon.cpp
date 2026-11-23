#include "Weapon.h"

#include "AnimInstEquipment.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Characters/Components/CombatComponent.h"
#include "MutateArena/PlayerControllers/BaseController.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Data/EquipmentAsset.h"
#include "Engine/GameInstance.h"
#include "Shells/Shell.h"
#include "Engine/SkeletalMeshSocket.h"
#include "MutateArena/System/AssetSubsystem.h"
#include "MutateArena/System/ObjectPoolSubsystem.h"
#include "MutateArena/System/UISubsystem.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

AWeapon::AWeapon()
{
	CollisionSphere->BodyInstance.bLockXRotation = true;
	CollisionSphere->BodyInstance.bLockYRotation = true;
	CollisionSphere->BodyInstance.bLockZRotation = true;
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();
	
	if (!AssetSubsystem) AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
	check(AssetSubsystem);

	FragWeapon = EquipmentAsset->FindFragmentByClass<UFragWeapon>();
	check(FragWeapon); // 强制判空，后续不再校验是否为空

	Ammo = FragWeapon->MagCapacity;
	CarriedAmmo = FragWeapon->MaxCarriedAmmo;

	// 提前同步预生成 20 个弹壳入池，且将该类弹壳的同屏硬上限锁死在 50 个
	if (FragWeapon && FragWeapon->ShellClass)
	{
		if (UObjectPoolSubsystem* PoolSubsystem = GetWorld()->GetSubsystem<UObjectPoolSubsystem>())
		{
			PoolSubsystem->PrewarmPool(FragWeapon->ShellClass, 20, 50);
		}
	}
}

void AWeapon::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelDelayedFireActions();

	Super::EndPlay(EndPlayReason);
}

void AWeapon::SetState(EEquipmentState NewState)
{
	Super::SetState(NewState);

	if (NewState != EEquipmentState::Equipped)
	{
		CancelDelayedFireActions();
	}
}

void AWeapon::OnRep_EquipmentState(EEquipmentState OldState)
{
	Super::OnRep_EquipmentState(OldState);

	if (EquipmentState != EEquipmentState::Equipped)
	{
		CancelDelayedFireActions();
	}
}

void AWeapon::HandleDrop()
{
	CancelDelayedFireActions();

	Super::HandleDrop();
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, Ammo);
	DOREPLIFETIME(ThisClass, CarriedAmmo);
}

FVector AWeapon::CalcFinalTarget(float RecoilVert, float RecoilHor, float SpreadPitch, float SpreadYaw) const
{
	const AHumanCharacter* HumanCharacter = Cast<AHumanCharacter>(GetOwner());
	if (!HumanCharacter || !HumanCharacter->Camera)
	{
		return FVector::ZeroVector;
	}

	const FVector CameraLoc = HumanCharacter->Camera->GetComponentLocation();
	FRotator TargetRotation = HumanCharacter->Camera->GetComponentRotation();
	TargetRotation.Pitch += RecoilVert + SpreadPitch;
	TargetRotation.Yaw += RecoilHor + SpreadYaw;

	return CameraLoc + TargetRotation.Vector() * 10000.f;
}

bool AWeapon::GetFireLocations(FVector& OutSpawnLocation, FVector& OutMuzzleLocation) const
{
	const AHumanCharacter* HumanCharacter = Cast<AHumanCharacter>(GetOwner());
	USkeletalMeshComponent* WeaponMesh = GetVisibleEquipmentMesh();
	if (!HumanCharacter || !HumanCharacter->Camera || !WeaponMesh)
	{
		return false;
	}

	const USkeletalMeshSocket* MuzzleSocket = WeaponMesh->GetSocketByName(Sockets::Muzzle);
	if (!MuzzleSocket)
	{
		return false;
	}

	// 物理生成起点：统一为角色摄像机世界位置
	OutSpawnLocation = HumanCharacter->Camera->GetComponentLocation();
	// 视觉表现起点：自动根据当前视角取 1P 或 3P 模型的枪口位置
	OutMuzzleLocation = MuzzleSocket->GetSocketTransform(WeaponMesh).GetLocation();
	return true;
}

float AWeapon::CalcVisualBlendDistance(const FVector& SpawnLocation, const FVector& ShotDirection, float MaxBlendDistance) const
{
	UWorld* World = GetWorld();
	if (!World) return MaxBlendDistance;

	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(GetOwner());

	const FVector TraceEnd = SpawnLocation + ShotDirection * MaxBlendDistance;
	if (World->LineTraceSingleByChannel(HitResult, SpawnLocation, TraceEnd, ECC_Visibility, Params))
	{
		return FMath::Clamp(HitResult.Distance, 50.f, MaxBlendDistance);
	}

	return MaxBlendDistance;
}

void AWeapon::Fire(const FVector& FinalTarget, int32 SpreadSeed)
{
	AHumanCharacter* HumanCharacter = Cast<AHumanCharacter>(GetOwner());
	const int32 ShotCount = HumanCharacter && HumanCharacter->CombatComp
		? HumanCharacter->CombatComp->CurShotCount
		: 1;
	SpawnProjectile(FinalTarget, ShotCount, SpreadSeed);
	SpendRound();
}

bool AWeapon::HasBoltAction() const
{
	return FragWeapon && (FragWeapon->BoltActionMontage1P_C || FragWeapon->BoltActionMontage1P_E
		|| FragWeapon->BoltActionMontage_C || FragWeapon->BoltActionMontage_E);
}

void AWeapon::TriggerShellEject()
{
	if (!FragWeapon || !FragWeapon->ShellClass) return;

	GetWorldTimerManager().ClearTimer(DelayedShellEjectTimerHandle);

	float ShellEjectDelay = 0.f;
	if (HasBoltAction() && FragWeapon->BoltActionShellEjectDelay > KINDA_SMALL_NUMBER)
	{
		// 栓动武器：开火后等待拉栓开始 + 蒙太奇拉栓至后死点时间
		ShellEjectDelay = FMath::Max(0.f, FragWeapon->BoltActionDelay) + FragWeapon->BoltActionShellEjectDelay;
	}
	else
	{
		// 泵动霰弹枪等程序化延迟退壳武器：优先检查 Pump，其次检查 Bolt
		const FWeaponFirePartMotion* DelayedMotion = FragWeapon->FindFirePartMotion(EWeaponMechBone::Pump);
		if (!DelayedMotion || DelayedMotion->StartDelay <= KINDA_SMALL_NUMBER)
		{
			DelayedMotion = FragWeapon->FindFirePartMotion(EWeaponMechBone::Bolt);
		}
		if (DelayedMotion && DelayedMotion->StartDelay > KINDA_SMALL_NUMBER)
		{
			// 护木/枪机拉到后死点时抛壳
			ShellEjectDelay = DelayedMotion->GetSafeStartDelay()
				+ DelayedMotion->GetSafeDuration() * DelayedMotion->GetClampedKickRatio();
		}
	}

	if (ShellEjectDelay > KINDA_SMALL_NUMBER)
	{
		GetWorldTimerManager().SetTimer(DelayedShellEjectTimerHandle, this, &ThisClass::EjectShell, ShellEjectDelay, false);
		return;
	}

	EjectShell();
}

void AWeapon::TriggerBoltAction()
{
	if (!HasBoltAction()) return;

	CancelBoltAction();

	const float Delay = FMath::Max(0.f, FragWeapon->BoltActionDelay);
	if (Delay > KINDA_SMALL_NUMBER)
	{
		GetWorldTimerManager().SetTimer(DelayedBoltActionTimerHandle, this, &ThisClass::PlayBoltActionMontage, Delay, false);
		return;
	}

	PlayBoltActionMontage();
}

void AWeapon::PlayBoltActionMontage()
{
	if (EquipmentState != EEquipmentState::Equipped || !FragWeapon) return;

	AHumanCharacter* HumanChar = Cast<AHumanCharacter>(GetOwner());
	if (!HumanChar) return;

	// 3P 角色与 3P 武器拉栓动画（所有端均播放）
	if (UAnimInstance* AnimInst3P = HumanChar->BodyMesh ? HumanChar->BodyMesh->GetAnimInstance() : nullptr; AnimInst3P && FragWeapon->BoltActionMontage_C)
	{
		AnimInst3P->Montage_Play(FragWeapon->BoltActionMontage_C);
	}
	if (UAnimInstEquipment* EquipAnimInst = GetEquipmentAnimInst(); EquipAnimInst && FragWeapon->BoltActionMontage_E)
	{
		EquipAnimInst->Montage_Play(FragWeapon->BoltActionMontage_E);
	}

	// 1P 手臂与 1P 武器拉栓动画（仅当前视角目标播放）
	if (HumanChar->bIsCurrentViewTarget)
	{
		if (UAnimInstance* AnimInst1P = HumanChar->Mesh1P ? HumanChar->Mesh1P->GetAnimInstance() : nullptr; AnimInst1P && FragWeapon->BoltActionMontage1P_C)
		{
			AnimInst1P->Montage_Play(FragWeapon->BoltActionMontage1P_C);
		}
		if (UAnimInstEquipment* EquipAnimInst1P = GetEquipmentAnimInst1P(); EquipAnimInst1P && FragWeapon->BoltActionMontage1P_E)
		{
			EquipAnimInst1P->Montage_Play(FragWeapon->BoltActionMontage1P_E);
		}
	}
}

void AWeapon::CancelDelayedFireActions()
{
	GetWorldTimerManager().ClearTimer(DelayedShellEjectTimerHandle);
	CancelBoltAction();
}

void AWeapon::CancelBoltAction()
{
	GetWorldTimerManager().ClearTimer(DelayedBoltActionTimerHandle);
	if (!FragWeapon) return;

	// Montage_Stop 传入具体蒙太奇指针时仅停止该拉栓蒙太奇，不会误停换弹或切入动画
	if (UAnimInstEquipment* EquipAnimInst = GetEquipmentAnimInst(); EquipAnimInst && FragWeapon->BoltActionMontage_E)
	{
		EquipAnimInst->Montage_Stop(0.1f, FragWeapon->BoltActionMontage_E);
	}
	if (UAnimInstEquipment* EquipAnimInst1P = GetEquipmentAnimInst1P(); EquipAnimInst1P && FragWeapon->BoltActionMontage1P_E)
	{
		EquipAnimInst1P->Montage_Stop(0.1f, FragWeapon->BoltActionMontage1P_E);
	}

	if (AHumanCharacter* HumanChar = Cast<AHumanCharacter>(GetOwner()))
	{
		if (UAnimInstance* AnimInst3P = HumanChar->BodyMesh ? HumanChar->BodyMesh->GetAnimInstance() : nullptr; AnimInst3P && FragWeapon->BoltActionMontage_C)
		{
			AnimInst3P->Montage_Stop(0.1f, FragWeapon->BoltActionMontage_C);
		}
		if (UAnimInstance* AnimInst1P = HumanChar->Mesh1P ? HumanChar->Mesh1P->GetAnimInstance() : nullptr; AnimInst1P && FragWeapon->BoltActionMontage1P_C)
		{
			AnimInst1P->Montage_Stop(0.1f, FragWeapon->BoltActionMontage1P_C);
		}

		// 若拉栓中途被打断且 3P 武器仍临时挂在左手 _L 插槽，归位回右手 _R 插槽
		if (EquipmentState == EEquipmentState::Equipped && EquipmentMesh && HumanChar->CombatComp
			&& EquipmentMesh->GetAttachSocketName().ToString().EndsWith(Sockets::SuffixL.ToString()))
		{
			HumanChar->CombatComp->AttachToHand(this, Sockets::SuffixR);
		}
	}
}

void AWeapon::EjectShell()
{
	USkeletalMeshComponent* WeaponMesh = GetVisibleEquipmentMesh();
	if (!FragWeapon || !FragWeapon->ShellClass || !WeaponMesh)
	{
		return;
	}

	if (const USkeletalMeshSocket* ShellEjectSocket = WeaponMesh->GetSocketByName(Sockets::ShellEject))
	{
		const FTransform SocketTransform = ShellEjectSocket->GetSocketTransform(WeaponMesh);
		if (UObjectPoolSubsystem* PoolSubsystem = GetWorld()->GetSubsystem<UObjectPoolSubsystem>())
		{
			AActor* SpawnedActor = PoolSubsystem->AcquireObject(FragWeapon->ShellClass, SocketTransform);
			if (AShell* Shell = Cast<AShell>(SpawnedActor))
			{
				if (AHumanCharacter* HumanCharacter = Cast<AHumanCharacter>(GetOwner()))
				{
					Shell->LaunchShell(HumanCharacter->GetVelocity());
				}
			}
		}
	}
}

void AWeapon::SpendRound()
{
	Ammo = FMath::Clamp(Ammo - 1, 0, FragWeapon->MagCapacity);

	AHumanCharacter* HumanCharacter = Cast<AHumanCharacter>(GetOwner());
	if (HumanCharacter && HumanCharacter->IsLocallyControlled())
	{
		SetHUDAmmo();
	}
}

void AWeapon::OnRep_Ammo()
{
	SetHUDAmmo();
}

// 新增：实现 CarriedAmmo 的同步回调
void AWeapon::OnRep_CarriedAmmo()
{
	SetHUDCarriedAmmo();
}

void AWeapon::SetAmmo(int32 AmmoNum)
{
	Ammo = AmmoNum;

	AHumanCharacter* HumanCharacter = Cast<AHumanCharacter>(GetOwner());
	if (HumanCharacter && HumanCharacter->IsLocallyControlled())
	{
		SetHUDAmmo();
	}
}

void AWeapon::SetCarriedAmmo(int32 AmmoNum)
{
	CarriedAmmo = AmmoNum;

	AHumanCharacter* HumanCharacter = Cast<AHumanCharacter>(GetOwner());
	if (HumanCharacter && HumanCharacter->IsLocallyControlled())
	{
		SetHUDCarriedAmmo();
	}
}

void AWeapon::SetHUDAmmo()
{
	if (AHumanCharacter* HumanCharacter = Cast<AHumanCharacter>(GetOwner()))
	{
		if (ABaseController* BaseController = Cast<ABaseController>(HumanCharacter->Controller))
		{
			if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(BaseController->GetLocalPlayer()))
			{
				UISubsystem->OnAmmoChange.Broadcast(Ammo);
			}
		}
	}
}

void AWeapon::SetHUDCarriedAmmo()
{
	if (AHumanCharacter* HumanCharacter = Cast<AHumanCharacter>(GetOwner()))
	{
		if (ABaseController* BaseController = Cast<ABaseController>(HumanCharacter->Controller))
		{
			if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(BaseController->GetLocalPlayer()))
			{
				UISubsystem->OnCarriedAmmoChange.Broadcast(CarriedAmmo);
			}
		}
	}
}
