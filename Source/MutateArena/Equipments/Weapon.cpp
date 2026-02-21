#include "Weapon.h"

#include "AudioModulationStatics.h"
#include "DataRegistryId.h"
#include "DataRegistrySubsystem.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/PlayerControllers/BaseController.h"
#include "MutateArena/Utils/LibraryCommon.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SphereComponent.h"
#include "Data/EquipmentAsset.h"
#include "Shells/Shell.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "MutateArena/System/AssetSubsystem.h"
#include "MutateArena/Assets/Data/CommonAsset.h"
#include "MutateArena/System/Interfaces/ObjectPoolSubsystem.h"

AWeapon::AWeapon()
{
	CollisionSphere->BodyInstance.bLockXRotation = true;
	CollisionSphere->BodyInstance.bLockYRotation = true;
	CollisionSphere->BodyInstance.bLockZRotation = true;
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();
	
	InitData();

	if (bIsPIP)
	{
		if (AssetSubsystem == nullptr) AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
		if (AssetSubsystem && AssetSubsystem->EquipmentAsset)
		{
			ScopeCapture = NewObject<USceneCaptureComponent2D>(this, TEXT("ScopeCapture"));
			ScopeCapture->RegisterComponent();
			ScopeCapture->AttachToComponent(EquipmentMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SOCKET_SCOPE_START);
			ScopeCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorHDR;
			ScopeCapture->FOVAngle = ScopeFOV;
			ScopeCapture->TextureTarget = AssetSubsystem->EquipmentAsset->RT_Scope;
			ScopeCapture->Deactivate();
		}
	}
}

// 初始化数据
void AWeapon::InitData()
{
	FString EnumValue = ULibraryCommon::GetEnumValue(UEnum::GetValueAsString(EquipmentParentName));

	{
		FDataRegistryId DataRegistryId(DR_WEAPON_DATA, FName(EnumValue));
		if (const FWeaponData* WeaponData = UDataRegistrySubsystem::Get()->GetCachedItem<FWeaponData>(DataRegistryId))
		{
			AimingFOVMul = WeaponData->AimingFOVMul;
			AimingWalkSpeedMul = WeaponData->AimingWalkSpeedMul;
			bIsPIP = WeaponData->bIsPIP;
			ScopeFOV = WeaponData->ScopeFOV;
			CarriedAmmo = WeaponData->MaxCarriedAmmo;
			MaxCarriedAmmo = WeaponData->MaxCarriedAmmo;
			Ammo = WeaponData->MagCapacity;
			MagCapacity = WeaponData->MagCapacity;
			FireRate = WeaponData->FireRate;
			bIsAutomatic = WeaponData->bIsAutomatic;
			WalkSpeedMul = WeaponData->WalkSpeedMul;
			PelletNum = WeaponData->PelletNum;
			
			// 补给箱中的装备 TODO
			if (EquipmentName == EEquipmentName::AK47_Cyclone)
			{
				MaxCarriedAmmo = 180;
				CarriedAmmo = 180;
				MagCapacity = 60;
				Ammo = 60;
			}
		}
	}

	{
		FDataRegistryId DataRegistryId(DR_EQUIPMENT_RECOIL, FName(EnumValue));
		if (const FWeaponRecoil* WeaponRecoil = UDataRegistrySubsystem::Get()->GetCachedItem<FWeaponRecoil>(DataRegistryId))
		{
			RecoilCurve = WeaponRecoil->RecoilCurve;
			RecoilCurveRandVert = WeaponRecoil->RecoilCurveRandVert;
			RecoilCurveRandHor = WeaponRecoil->RecoilCurveRandHor;
			RecoilIncTime = WeaponRecoil->RecoilIncTime;
			RecoilMaxDecTime = WeaponRecoil->RecoilMaxDecTime;
			RecoilMinDecTime = WeaponRecoil->RecoilMinDecTime;
			RecoilVertRef_DecTime = WeaponRecoil->RecoilVertRef_DecTime;
			RecoilVertRef_Crosshair = WeaponRecoil->RecoilVertRef_Crosshair;
			CrosshairBaseSpread = WeaponRecoil->CrosshairBaseSpread;
		}
	}
}

void AWeapon::SetScopeActive(bool bIsActive)
{
	if (ScopeCapture)
	{
		ScopeCapture->SetActive(bIsActive);
	}
}

void AWeapon::Fire(const FVector& HitTarget, float RecoilVert, float RecoilHor, float SpreadPitch, float SpreadYaw)
{
	if (ShellClass)
	{
		if (const USkeletalMeshSocket* ShellEjectSocket = EquipmentMesh->GetSocketByName(SOCKET_SHELL_EJECT))
		{
			FTransform SocketTransform = ShellEjectSocket->GetSocketTransform(EquipmentMesh);

			if (UObjectPoolSubsystem* PoolSubsystem = GetWorld()->GetSubsystem<UObjectPoolSubsystem>())
			{
				AActor* SpawnedActor = PoolSubsystem->AcquireObject(ShellClass, SocketTransform);

				if (AShell* Shell = Cast<AShell>(SpawnedActor))
				{
					if (HumanCharacter == nullptr)
					{
						HumanCharacter = Cast<AHumanCharacter>(GetOwner());
					}

					FVector CharVelocity = HumanCharacter ? HumanCharacter->GetVelocity() : FVector::ZeroVector;

					Shell->LaunchShell(CharVelocity);
				}
			}
		}
	}

	SpendRound();

	// 即将耗尽弹药时增大机械层声音
	float Volume = 0.6f;
	float LastTime = bIsAutomatic ? 1.2f : 0.5f;
	if (Ammo <= FireRate / 60.f * LastTime)
	{
		Volume = 2.f;
	}
	if (AssetSubsystem == nullptr) AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
	if (AssetSubsystem && AssetSubsystem->CommonAsset)
	{
		UAudioModulationStatics::SetGlobalBusMixValue(
			this, 
			AssetSubsystem->CommonAsset->CB_EquipmentMech, 
			Volume, 
			0.f
		);
	}
}

void AWeapon::SpendRound()
{
	Ammo = FMath::Clamp(Ammo - 1, 0, MagCapacity);

	if (HumanCharacter == nullptr) HumanCharacter = Cast<AHumanCharacter>(GetOwner());
	if (HumanCharacter && HumanCharacter->IsLocallyControlled())
	{
		SetHUDAmmo();
	}
}

void AWeapon::SetAmmo(int32 AmmoNum)
{
	Ammo = AmmoNum;

	if (HumanCharacter == nullptr) HumanCharacter = Cast<AHumanCharacter>(GetOwner());
	if (HumanCharacter && HumanCharacter->IsLocallyControlled())
	{
		SetHUDAmmo();
	}
}

void AWeapon::SetCarriedAmmo(int32 AmmoNum)
{
	CarriedAmmo = AmmoNum;

	if (HumanCharacter == nullptr) HumanCharacter = Cast<AHumanCharacter>(GetOwner());
	if (HumanCharacter && HumanCharacter->IsLocallyControlled())
	{
		SetHUDCarriedAmmo();
	}
}

void AWeapon::SetHUDAmmo()
{
	if (HumanCharacter == nullptr) HumanCharacter = Cast<AHumanCharacter>(GetOwner());
	if (HumanCharacter)
	{
		if (BaseController == nullptr) BaseController = Cast<ABaseController>(HumanCharacter->Controller);
		if (BaseController)
		{
			BaseController->SetHUDAmmo(Ammo);
		}
	}
}

void AWeapon::SetHUDCarriedAmmo()
{
	if (HumanCharacter == nullptr) HumanCharacter = Cast<AHumanCharacter>(GetOwner());
	if (HumanCharacter)
	{
		if (BaseController == nullptr) BaseController = Cast<ABaseController>(HumanCharacter->Controller);
		if (BaseController)
		{
			BaseController->SetHUDCarriedAmmo(CarriedAmmo);
		}
	}
}

void AWeapon::MulticastSetFullAmmo_Implementation()
{
	SetAmmo(MagCapacity);
	SetCarriedAmmo(MaxCarriedAmmo);
}
