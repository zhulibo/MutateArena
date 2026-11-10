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

FVector AWeaponShotgun::CalcFinalTarget(float RecoilVert, float RecoilHor, float, float) const
{
	// 单颗弹丸有各自的散布，整组弹丸不应该有散布
	return Super::CalcFinalTarget(RecoilVert, RecoilHor, 0.f, 0.f);
}

void AWeaponShotgun::SpawnProjectile(const FVector& FinalTarget, int32 ShotCount, int32 SpreadSeed)
{
	AHumanCharacter* HumanCharacter = Cast<AHumanCharacter>(GetOwner());
	if (OwnerTeam == ETeam::NoTeam) SetOwnerTeam();
	if (!FragWeapon || !FragWeapon->ProjectileClass || !FragWeapon->RecoilCurve
		|| !HumanCharacter || !HumanCharacter->CombatComp || OwnerTeam == ETeam::NoTeam)
	{
		return;
	}

	FVector SpawnLocation;
	FVector MuzzleLocation;
	if (!GetFireLocations(SpawnLocation, MuzzleLocation))
	{
		return;
	}

	const FVector BaseShotDir = (FinalTarget - SpawnLocation).GetSafeNormal();
	const FRotator TargetRotation = BaseShotDir.Rotation();

	// 霰弹枪每次射击的弹丸循环
	FVector RecoilKick = FragWeapon->RecoilCurve->GetVectorValue(ShotCount);
	// 主控端、服务器和观察端使用同一 SpreadSeed，确保每颗弹丸采样相同的散射序列。
	FRandomStream PelletRandomStream(SpreadSeed);

	for (int32 i = 0; i < FragWeapon->PelletNum; ++i)
	{
		// 添加散布
		FVector ToTargetWithSpread = PelletRandomStream.VRandCone(
			TargetRotation.Vector(),
			FMath::DegreesToRadians(RecoilKick.Z)
		);

		// 每颗弹丸按自身散布方向独立计算障碍物合并距离，避免擦掩体边缘射击时飞向远处的弹丸提前折线
		const float BlendDistance = CalcVisualBlendDistance(SpawnLocation, ToTargetWithSpread);

		// 构造当前这发弹丸的 Transform（起点为摄像机）
		FTransform SpawnTransform(ToTargetWithSpread.Rotation(), SpawnLocation);

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
			Projectile->MuzzleLocation = MuzzleLocation;
			Projectile->BlendDistance = BlendDistance;

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
