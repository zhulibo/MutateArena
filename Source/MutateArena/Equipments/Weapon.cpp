#include "Weapon.h"

#include "Camera/CameraComponent.h"
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
	check(FragWeapon);

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
	if (World->LineTraceSingleByChannel(HitResult, SpawnLocation, TraceEnd, ECC::Trace, Params))
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
