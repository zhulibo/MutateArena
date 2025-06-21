#include "Weapon.h"

#include "DataRegistryId.h"
#include "DataRegistrySubsystem.h"
#include "MetasoundSource.h"
#include "MutateArena/Equipments/Data/EquipmentType.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/PlayerControllers/BaseController.h"
#include "MutateArena/Utils/LibraryCommon.h"
#include "Components/AudioComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SphereComponent.h"
#include "Data/EquipmentAsset.h"
#include "Shells/Shell.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "MutateArena/System/AssetSubsystem.h"

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

	if (bHasScope)
	{
		if (AssetSubsystem == nullptr) AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
		if (AssetSubsystem && AssetSubsystem->EquipmentAsset)
		{
			ScopeCapture = NewObject<USceneCaptureComponent2D>(this, TEXT("ScopeCapture"));
			ScopeCapture->RegisterComponent();
			ScopeCapture->AttachToComponent(EquipmentMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("ScopeSocket"));
			ScopeCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorHDR;
			// ScopeCapture->ShowFlags.SetDynamicShadows(true);
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
			AimSpeed = WeaponData->AimSpeed;
			AimMoveSpeedMul = WeaponData->AimMoveSpeedMul;
			ScopeFOV = WeaponData->ScopeFOV;
			MaxCarriedAmmo = WeaponData->MaxCarriedAmmo;
			MagCapacity = WeaponData->MagCapacity;
			FireRate = WeaponData->FireRate;
			bIsAutomatic = WeaponData->bIsAutomatic;
			MoveSpeedMul = WeaponData->MoveSpeedMul;
			PelletNum = WeaponData->PelletNum;

			CarriedAmmo = MaxCarriedAmmo;
			Ammo = MagCapacity;
		}
	}

	{
		FDataRegistryId DataRegistryId(DR_EQUIPMENT_RECOIL, FName(EnumValue));
		if (const FEquipmentRecoil* EquipmentData = UDataRegistrySubsystem::Get()->GetCachedItem<FEquipmentRecoil>(DataRegistryId))
		{
			RecoilMaxVert = EquipmentData->RecoilMaxVert;
			RecoilMinVert = EquipmentData->RecoilMinVert;
			RecoilMaxHor = EquipmentData->RecoilMaxHor;
			RecoilMinHor = EquipmentData->RecoilMinHor;
			FirstShotRecoilMul = EquipmentData->FirstShotRecoilMul;
			RecoilIncTime = EquipmentData->RecoilIncTime;
			RecoilTotalVertLimit = EquipmentData->RecoilTotalVertLimit;
			RecoilTotalHorLimit = EquipmentData->RecoilTotalHorLimit;
			RecoilDecSpeed = EquipmentData->RecoilDecSpeed;
			CenterSpread = EquipmentData->CenterSpread;
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

void AWeapon::Fire(const FVector& HitTarget, float RecoilVert, float RecoilHor)
{
	if (ShellClass)
	{
		if (const USkeletalMeshSocket* ShellEjectSocket = EquipmentMesh->GetSocketByName(TEXT("ShellEject")))
		{
			FTransform SocketTransform = ShellEjectSocket->GetSocketTransform(EquipmentMesh);

			FActorSpawnParameters SpawnParams;
			SpawnParams.bDeferConstruction = true;

			AShell* Shell = GetWorld()->SpawnActor<AShell>(
				ShellClass,
				SocketTransform.GetLocation(),
				SocketTransform.GetRotation().Rotator(),
				SpawnParams
			);

			if (HumanCharacter == nullptr) HumanCharacter = Cast<AHumanCharacter>(GetOwner());
			if (HumanCharacter)
			{
				Shell->InitVelocity = HumanCharacter->GetVelocity();
			}

			UGameplayStatics::FinishSpawningActor(Shell, SocketTransform);
		}
	}

	SpendRound();

	// 播放开火机械层声音
	if (UAudioComponent* AudioComponent = UGameplayStatics::SpawnSoundAttached(MechSound, EquipmentMesh))
	{
		float Volume = 1.f;
		if (Ammo * (60 / FireRate) < 1.f)
		{
			Volume = 3.f;
		}
		AudioComponent->SetFloatParameter(TEXT("Volume"), Volume);
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
