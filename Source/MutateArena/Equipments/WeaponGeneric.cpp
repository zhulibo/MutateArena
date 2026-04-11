#include "WeaponGeneric.h"

#include "MutateArena/MutateArena.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "..\PlayerStates\TeamType.h"
#include "MutateArena/Characters/Components/CombatComponent.h"
#include "MutateArena/Utils/LibraryCommon.h"
#include "Components/BoxComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Projectiles/ProjectileBullet.h"

AWeaponGeneric::AWeaponGeneric()
{
}

void AWeaponGeneric::Fire(const FVector& HitTarget, float RecoilVert, float RecoilHor, float SpreadPitch, float SpreadYaw)
{
	Super::Fire(HitTarget, RecoilVert, RecoilHor, SpreadPitch, SpreadYaw);

	AHumanCharacter* HumanCharacter = Cast<AHumanCharacter>(GetOwner());
	if (OwnerTeam == ETeam::NoTeam) SetOwnerTeam();
	const USkeletalMeshSocket* MuzzleSocket = EquipmentMesh->GetSocketByName(SOCKET_MUZZLE);

	if (HumanCharacter == nullptr || ProjectileClass == nullptr || OwnerTeam == ETeam::NoTeam || MuzzleSocket == nullptr) return;
	
	// 子弹是从枪口发出，射向准星在世界场景的投射点，所以穿透后的弹着点会有视差，这是正常的
	FTransform SocketTransform = MuzzleSocket->GetSocketTransform(EquipmentMesh);
	FRotator TargetRotation = (HitTarget - SocketTransform.GetLocation()).Rotation();

	// 添加后座
	TargetRotation.Pitch += RecoilVert;
	TargetRotation.Yaw += RecoilHor;

	// 添加散布
	TargetRotation.Pitch += SpreadPitch;
	TargetRotation.Yaw += SpreadYaw;

	// 构造生成的 Transform
	FTransform SpawnTransform(TargetRotation, SocketTransform.GetLocation());

	AProjectileBullet* Projectile = GetWorld()->SpawnActorDeferred<AProjectileBullet>(
		ProjectileClass,
		SpawnTransform,
		this,
		HumanCharacter,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn
	);

	if (Projectile)
	{
		// 设置 OwnerName
		Projectile->OwnerName = StaticEnum<EEquipmentName>()->GetNameStringByValue(static_cast<int64>(EquipmentParentName));

		Projectile->InitialPenetrationPower = PenetrationPower;

		// 设置队伍碰撞通道
		switch (OwnerTeam)
		{
		case ETeam::Team1:
			Projectile->CollisionBox->SetCollisionResponseToChannel(ECC_MESH_TEAM2, ECollisionResponse::ECR_Overlap);
			break;
		case ETeam::Team2:
			Projectile->CollisionBox->SetCollisionResponseToChannel(ECC_MESH_TEAM1, ECollisionResponse::ECR_Overlap);
			break;
		}

		UGameplayStatics::FinishSpawningActor(Projectile, SpawnTransform);
	}
}
