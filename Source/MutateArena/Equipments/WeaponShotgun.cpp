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
#include "Math/RandomStream.h"
#include "Kismet/GameplayStatics.h"
#include "MutateArena/Characters/Components/CombatComponent.h"
#include "Projectiles/ProjectileBullet.h"

AWeaponShotgun::AWeaponShotgun()
{
}

FVector AWeaponShotgun::CalcFinalTarget(const FVector& HitTarget, float RecoilVert, float RecoilHor, float, float) const
{
	// Shotgun pellets use the recoil curve for their cone spread; the generic random spread offsets are not applied.
	return Super::CalcFinalTarget(HitTarget, RecoilVert, RecoilHor, 0.f, 0.f);
}

void AWeaponShotgun::SpawnProjectile(const FVector& FinalTarget, int32 ShotCount)
{
	AHumanCharacter* HumanCharacter = Cast<AHumanCharacter>(GetOwner());
	USkeletalMeshComponent* WeaponMesh = GetVisibleEquipmentMesh();
	if (OwnerTeam == ETeam::NoTeam) SetOwnerTeam();
	if (!FragWeapon || !FragWeapon->ProjectileClass || !FragWeapon->RecoilCurve || !WeaponMesh
		|| !HumanCharacter || !HumanCharacter->CombatComp || OwnerTeam == ETeam::NoTeam)
	{
		return;
	}

	const USkeletalMeshSocket* MuzzleSocket = WeaponMesh->GetSocketByName(Sockets::Muzzle);

	if (!MuzzleSocket) return;

	FTransform SocketTransform = MuzzleSocket->GetSocketTransform(WeaponMesh);
	FRotator TargetRotation = (FinalTarget - SocketTransform.GetLocation()).Rotation();

	// 霰弹枪每次射击的弹丸循环
	FVector RecoilKick = FragWeapon->RecoilCurve->GetVectorValue(ShotCount);
	// 主控端、服务器和观察端使用同一 ShotCount，确保每颗弹丸采样相同的散射序列。
	FRandomStream PelletRandomStream(ShotCount);

	for (int32 i = 0; i < FragWeapon->PelletNum; ++i)
	{
		// 添加散布
		FVector ToTargetWithSpread = PelletRandomStream.VRandCone(
			TargetRotation.Vector(),
			FMath::DegreesToRadians(RecoilKick.Z)
		);

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
			Projectile->ParentEquipmentName = EquipmentAsset->GetParentEquipmentName();
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
