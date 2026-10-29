#include "WeaponShotgun.h"

#include "MutateArena/MutateArena.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/PlayerStates/TeamType.h"
#include "MutateArena/Utils/LibraryCommon.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Curves/CurveVector.h"
#include "Data/EquipmentAsset.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "MutateArena/Characters/Components/CombatComponent.h"
#include "Projectiles/ProjectileBullet.h"

AWeaponShotgun::AWeaponShotgun()
{
}

void AWeaponShotgun::Fire(const FVector& HitTarget, float RecoilVert, float RecoilHor, float SpreadPitch, float SpreadYaw)
{
	Super::Fire(HitTarget, RecoilVert, RecoilHor, SpreadPitch, SpreadYaw);

	AHumanCharacter* HumanCharacter = Cast<AHumanCharacter>(GetOwner());
	if (OwnerTeam == ETeam::NoTeam) SetOwnerTeam();
	const USkeletalMeshSocket* MuzzleSocket = EquipmentMesh->GetSocketByName(Sockets::Muzzle);

	if (!FragWeapon->ProjectileClass || !FragWeapon->RecoilCurve 
		|| !HumanCharacter || !HumanCharacter->CombatComp || OwnerTeam == ETeam::NoTeam || !MuzzleSocket) return;

	FTransform SocketTransform = MuzzleSocket->GetSocketTransform(EquipmentMesh);
	FRotator TargetRotation = (HitTarget - SocketTransform.GetLocation()).Rotation();

	// 添加后座
	TargetRotation.Pitch += RecoilVert;
	TargetRotation.Yaw += RecoilHor;

	// 霰弹枪每次射击的弹丸循环
	FVector RecoilKick = FragWeapon->RecoilCurve->GetVectorValue(HumanCharacter->CombatComp->CurShotCount);

	for (int32 i = 0; i < FragWeapon->PelletNum; ++i)
	{
		// 添加散布
		FVector ToTargetWithSpread = UKismetMathLibrary::RandomUnitVectorInConeInDegrees(
			TargetRotation.Vector(), RecoilKick.Z);

		// 构造当前这发弹丸的 Transform
		FTransform SpawnTransform(ToTargetWithSpread.Rotation(), SocketTransform.GetLocation());

		// 使用延迟生成
		AProjectileBullet* Projectile = GetWorld()->SpawnActorDeferred<AProjectileBullet>(
			FragWeapon->ProjectileClass,
			SpawnTransform,
			this,
			HumanCharacter,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn
		);

		if (Projectile)
		{
			Projectile->EquipmentParentName = EquipmentAsset->GetEquipmentParentName();
			Projectile->InitialPenetrationPower = FragWeapon->PenetrationPower;

			// 设置队伍碰撞免伤通道
			switch (OwnerTeam)
			{
			case ETeam::Team1:
				Projectile->CollisionBox->SetCollisionResponseToChannel(
					ECC::MeshTeam2, ECollisionResponse::ECR_Overlap);
				break;
			case ETeam::Team2:
				Projectile->CollisionBox->SetCollisionResponseToChannel(
					ECC::MeshTeam1, ECollisionResponse::ECR_Overlap);
				break;
			}

			// 结束生成过程，立即触发这颗弹丸的 BeginPlay
			UGameplayStatics::FinishSpawningActor(Projectile, SpawnTransform);
		}
	}
}
