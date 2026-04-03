#include "WeaponShotgun.h"

#include "MutateArena/MutateArena.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/PlayerStates/TeamType.h"
#include "MutateArena/Utils/LibraryCommon.h"
#include "Components/BoxComponent.h"
#include "Curves/CurveVector.h"
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
	const USkeletalMeshSocket* MuzzleSocket = EquipmentMesh->GetSocketByName(SOCKET_MUZZLE);

	if (ProjectileClass && HumanCharacter && HumanCharacter->CombatComp && RecoilCurve && OwnerTeam != ETeam::NoTeam && MuzzleSocket)
	{
		FTransform SocketTransform = MuzzleSocket->GetSocketTransform(EquipmentMesh);
		FRotator TargetRotation = (HitTarget - SocketTransform.GetLocation()).Rotation();

		// 添加后座
		TargetRotation.Pitch += RecoilVert;
		TargetRotation.Yaw += RecoilHor;

		// 霰弹枪每次射击的弹丸循环
		FVector RecoilKick = RecoilCurve->GetVectorValue(HumanCharacter->CombatComp->CurShotCount);

		for (int32 i = 0; i < PelletNum; ++i)
		{
			// 添加散布
			FVector ToTargetWithSpread = UKismetMathLibrary::RandomUnitVectorInConeInDegrees(
				TargetRotation.Vector(), RecoilKick.Z);

			// 构造当前这发弹丸的 Transform
			FTransform SpawnTransform(ToTargetWithSpread.Rotation(), SocketTransform.GetLocation());

			// 使用延迟生成
			AProjectileBullet* Projectile = GetWorld()->SpawnActorDeferred<AProjectileBullet>(
				ProjectileClass,
				SpawnTransform,
				this,
				HumanCharacter,
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn
			);

			if (Projectile)
			{
				// 设置所属武器名称
				Projectile->OwnerName = FName(
					ULibraryCommon::GetEnumValue(UEnum::GetValueAsString(EquipmentParentName)));

				Projectile->InitialPenetrationPower = PenetrationPower;

				// 设置队伍碰撞免伤通道
				switch (OwnerTeam)
				{
				case ETeam::Team1:
					Projectile->CollisionBox->SetCollisionResponseToChannel(
						ECC_MESH_TEAM2, ECollisionResponse::ECR_Overlap);
					break;
				case ETeam::Team2:
					Projectile->CollisionBox->SetCollisionResponseToChannel(
						ECC_MESH_TEAM1, ECollisionResponse::ECR_Overlap);
					break;
				}

				// 结束生成过程，立即触发这颗弹丸的 BeginPlay
				UGameplayStatics::FinishSpawningActor(Projectile, SpawnTransform);
			}
		}
	}
}
