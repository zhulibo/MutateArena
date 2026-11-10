#include "WeaponGeneric.h"

#include "MutateArena/MutateArena.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "..\PlayerStates\TeamType.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/EquipmentAsset.h"
#include "Kismet/GameplayStatics.h"
#include "Projectiles/ProjectileBullet.h"

AWeaponGeneric::AWeaponGeneric()
{
}

void AWeaponGeneric::SpawnProjectile(const FVector& FinalTarget, int32, int32)
{
	AHumanCharacter* HumanCharacter = Cast<AHumanCharacter>(GetOwner());
	if (OwnerTeam == ETeam::NoTeam) SetOwnerTeam();
	if (!HumanCharacter || !FragWeapon || !FragWeapon->ProjectileClass || OwnerTeam == ETeam::NoTeam)
	{
		return;
	}

	FVector SpawnLocation;
	FVector MuzzleLocation;
	if (!GetFireLocations(SpawnLocation, MuzzleLocation))
	{
		return;
	}

	const FVector ShotDirection = (FinalTarget - SpawnLocation).GetSafeNormal();
	const FRotator TargetRotation = ShotDirection.Rotation();
	const float BlendDistance = CalcVisualBlendDistance(SpawnLocation, ShotDirection);

	// 构造生成的 Transform（起点在摄像机，方向直指准星目标点）
	const FTransform SpawnTransform(TargetRotation, SpawnLocation);

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
		Projectile->MuzzleLocation = MuzzleLocation;
		Projectile->BlendDistance = BlendDistance;
		
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
