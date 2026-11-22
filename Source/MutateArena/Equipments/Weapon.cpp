#include "Weapon.h"

#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "MetaSoundSource.h"
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
	CancelDelayedFireEffects();

	Super::EndPlay(EndPlayReason);
}

void AWeapon::SetState(EEquipmentState NewState)
{
	Super::SetState(NewState);

	if (NewState != EEquipmentState::Equipped)
	{
		CancelDelayedFireEffects();
	}
}

void AWeapon::OnRep_EquipmentState(EEquipmentState OldState)
{
	Super::OnRep_EquipmentState(OldState);

	if (EquipmentState != EEquipmentState::Equipped)
	{
		CancelDelayedFireEffects();
	}
}

void AWeapon::HandleDrop()
{
	CancelDelayedFireEffects();

	Super::HandleDrop();
}

void AWeapon::OnStartSwapOut()
{
	CancelDelayedFireEffects();

	Super::OnStartSwapOut();
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

void AWeapon::TriggerFireMechEffects()
{
	if (!FragWeapon) return;

	CancelDelayedFireEffects();

	// 优先检查 Bolt（如栓动狙击枪），若未配置延迟则检查 Pump（如泵动霰弹枪）
	const FWeaponFirePartMotion* DelayedMotion = FragWeapon->FindFirePartMotion(EWeaponMechBone::Bolt);
	if (!DelayedMotion || DelayedMotion->StartDelay <= KINDA_SMALL_NUMBER)
	{
		DelayedMotion = FragWeapon->FindFirePartMotion(EWeaponMechBone::Pump);
	}

	if (DelayedMotion && DelayedMotion->StartDelay > KINDA_SMALL_NUMBER)
	{
		if (UWorld* World = GetWorld())
		{
			const float MechSoundDelay = DelayedMotion->GetSafeStartDelay();
			// 护木/枪机拉到后死点时抛壳
			const float ShellEjectDelay = MechSoundDelay + DelayedMotion->GetSafeDuration() * DelayedMotion->GetEffectiveKickRatio();

			if (FragWeapon->MechSound)
			{
				World->GetTimerManager().SetTimer(DelayedMechSoundTimerHandle, this, &ThisClass::PlayMechSound, MechSoundDelay, false);
			}
			if (FragWeapon->ShellClass)
			{
				World->GetTimerManager().SetTimer(DelayedShellEjectTimerHandle, this, &ThisClass::EjectShell, ShellEjectDelay, false);
			}
			return;
		}
	}

	PlayMechSound();
	EjectShell();
}

void AWeapon::CancelDelayedFireEffects()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DelayedMechSoundTimerHandle);
		World->GetTimerManager().ClearTimer(DelayedShellEjectTimerHandle);
	}
}

void AWeapon::PlayMechSound()
{
	if (!FragWeapon || !FragWeapon->MechSound) return;

	// 延迟触发时重新查询当前可见网格体，避免期间观战视角切换导致音效挂错网格体
	if (USkeletalMeshComponent* WeaponMesh = GetVisibleEquipmentMesh())
	{
		UGameplayStatics::SpawnSoundAttached(FragWeapon->MechSound, WeaponMesh, Sockets::Muzzle);
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
