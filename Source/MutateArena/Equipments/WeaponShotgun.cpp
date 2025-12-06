#include "WeaponShotgun.h"

#include "MutateArena/MutateArena.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/PlayerStates/TeamType.h"
#include "MutateArena/Utils/LibraryCommon.h"
#include "Components/BoxComponent.h"
#include "Curves/CurveVector.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/KismetMathLibrary.h"
#include "MutateArena/Characters/Components/CombatComponent.h"
#include "Projectiles/ProjectileBullet.h"

AWeaponShotgun::AWeaponShotgun()
{
}

void AWeaponShotgun::Fire(const FVector& HitTarget, float RecoilVert, float RecoilHor, float SpreadPitch, float SpreadYaw)
{
	Super::Fire(HitTarget, RecoilVert, RecoilHor, SpreadPitch, SpreadYaw);

	if (HumanCharacter == nullptr) HumanCharacter = Cast<AHumanCharacter>(GetOwner());
	if (OwnerTeam == ETeam::NoTeam) SetOwnerTeam();
	const USkeletalMeshSocket* MuzzleSocket = EquipmentMesh->GetSocketByName(SOCKET_MUZZLE);

	if (ProjectileClass && HumanCharacter && HumanCharacter->CombatComponent && RecoilCurve && OwnerTeam != ETeam::NoTeam && MuzzleSocket)
	{
		FTransform SocketTransform = MuzzleSocket->GetSocketTransform(EquipmentMesh);
		FRotator TargetRotation = (HitTarget - SocketTransform.GetLocation()).Rotation();

		// 添加后座
		TargetRotation.Pitch += RecoilVert;
		TargetRotation.Yaw += RecoilHor;

		if (ProjectileClass)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			SpawnParams.Instigator = HumanCharacter;
			
			FVector RecoilKick = RecoilCurve->GetVectorValue(HumanCharacter->CombatComponent->CurShotCount);

			for (int32 i = 0; i < PelletNum; ++i)
			{
				// 添加散布
				FVector ToTargetWithSpread = UKismetMathLibrary::RandomUnitVectorInConeInDegrees(TargetRotation.Vector(), RecoilKick.Z);

				AProjectileBullet* Projectile = GetWorld()->SpawnActor<AProjectileBullet>(
					ProjectileClass,
					SocketTransform.GetLocation(),
					ToTargetWithSpread.Rotation(),
					SpawnParams
				);

				Projectile->OwnerName = FName(ULibraryCommon::GetEnumValue(UEnum::GetValueAsString(EquipmentParentName)));

				switch (OwnerTeam)
				{
				case ETeam::Team1:
					Projectile->GetCollisionBox()->SetCollisionResponseToChannel(ECC_MESH_TEAM2, ECollisionResponse::ECR_Block);
					break;
				case ETeam::Team2:
					Projectile->GetCollisionBox()->SetCollisionResponseToChannel(ECC_MESH_TEAM1, ECollisionResponse::ECR_Block);
					break;
				}
			}
		}
	}
}
