#include "Weapon.h"

#include "MutateArena/MutateArena.h"
#include "MutateArena/Characters/HumanCharacter.h"
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
	
	if (AssetSubsystem == nullptr) AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
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

void AWeapon::Fire(const FVector& HitTarget, float RecoilVert, float RecoilHor, float SpreadPitch, float SpreadYaw)
{
	if (FragWeapon->ShellClass)
	{
		if (const USkeletalMeshSocket* ShellEjectSocket = EquipmentMesh->GetSocketByName(Sockets::ShellEject))
		{
			FTransform SocketTransform = ShellEjectSocket->GetSocketTransform(EquipmentMesh);

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

	SpendRound();
}

void AWeapon::MulticastFire_Implementation(FVector HitTarget, float RecoilVert, float RecoilHor, float SpreadX, float SpreadY)
{
	AHumanCharacter* HumanChar = Cast<AHumanCharacter>(GetOwner());
	if (HumanChar && !HumanChar->IsLocallyControlled())
	{
		Fire(HitTarget, RecoilVert, RecoilHor, SpreadX, SpreadY);
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
