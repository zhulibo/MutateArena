#include "WeaponGeneric.h"

#include "MutateArena/MutateArena.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "..\PlayerStates\TeamType.h"
#include "MutateArena/Characters/Components/CombatComponent.h"
#include "MutateArena/Utils/LibraryCommon.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/EquipmentAsset.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Projectiles/ProjectileBullet.h"

AWeaponGeneric::AWeaponGeneric()
{
}

void AWeaponGeneric::SpawnProjectile(const FVector& FinalTarget, int32)
{
	AHumanCharacter* HumanCharacter = Cast<AHumanCharacter>(GetOwner());
	USkeletalMeshComponent* WeaponMesh = GetVisibleEquipmentMesh();
	if (OwnerTeam == ETeam::NoTeam) SetOwnerTeam();
	if (!HumanCharacter || !FragWeapon || !FragWeapon->ProjectileClass || !WeaponMesh)
	{
		return;
	}

	const USkeletalMeshSocket* MuzzleSocket = WeaponMesh->GetSocketByName(Sockets::Muzzle);

	if (OwnerTeam == ETeam::NoTeam || !MuzzleSocket) return;
	
	// 子弹是从枪口发出，射向准星在世界场景的投射点，所以穿透后的弹着点会有视差，这是正常的
	FTransform SocketTransform = MuzzleSocket->GetSocketTransform(WeaponMesh);
	FRotator TargetRotation = (FinalTarget - SocketTransform.GetLocation()).Rotation();

	// 构造生成的 Transform
	FTransform SpawnTransform(TargetRotation, SocketTransform.GetLocation());

	AProjectileBullet* Projectile = GetWorld()->SpawnActorDeferred<AProjectileBullet>(
		FragWeapon->ProjectileClass,
		SpawnTransform,
		this,
		HumanCharacter,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn
	);

	if (Projectile)
	{
		Projectile->ParentEquipmentName = EquipmentAsset->GetParentEquipmentName();
		Projectile->InitialPenetrationPower = FragWeapon->PenetrationPower;

		// 设置队伍碰撞通道
		switch (OwnerTeam)
		{
		case ETeam::Team1:
			Projectile->CollisionBox->SetCollisionResponseToChannel(ECC::MeshTeam2, ECollisionResponse::ECR_Overlap);
			break;
		case ETeam::Team2:
			Projectile->CollisionBox->SetCollisionResponseToChannel(ECC::MeshTeam1, ECollisionResponse::ECR_Overlap);
			break;
		}

		UGameplayStatics::FinishSpawningActor(Projectile, SpawnTransform);
	}
}
