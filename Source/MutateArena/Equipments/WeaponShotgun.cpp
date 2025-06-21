#include "WeaponShotgun.h"

#include "MutateArena/Equipments/Data/EquipmentType.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/PlayerStates/TeamType.h"
#include "MutateArena/Utils/LibraryCommon.h"
#include "Components/BoxComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/KismetMathLibrary.h"
#include "Projectiles/ProjectileBullet.h"

AWeaponShotgun::AWeaponShotgun()
{
}

void AWeaponShotgun::Fire(const FVector& HitTarget, float RecoilVert, float RecoilHor)
{
	Super::Fire(HitTarget, RecoilVert, RecoilHor);

	if (HumanCharacter == nullptr) HumanCharacter = Cast<AHumanCharacter>(GetOwner());
	if (OwnerTeam == ETeam::NoTeam) SetOwnerTeam();
	const USkeletalMeshSocket* MuzzleSocket = EquipmentMesh->GetSocketByName(TEXT("Muzzle"));

	if (ProjectileClass && HumanCharacter && OwnerTeam != ETeam::NoTeam && MuzzleSocket)
	{
		FTransform SocketTransform = MuzzleSocket->GetSocketTransform(EquipmentMesh);
		FRotator TargetRotation = (HitTarget - SocketTransform.GetLocation()).Rotation();

		// 应用后坐力（gun kick）
		TargetRotation.Pitch += RecoilVert;
		TargetRotation.Yaw += RecoilHor;

		if (ProjectileClass)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			SpawnParams.Instigator = HumanCharacter;

			for (int32 i = 0; i < PelletNum; ++i)
			{
				// 添加散布
				FVector ToTargetWithSpread = UKismetMathLibrary::RandomUnitVectorInConeInDegrees(TargetRotation.Vector(), CenterSpread);

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
					Projectile->GetCollisionBox()->SetCollisionResponseToChannel(ECC_TEAM2_MESH, ECollisionResponse::ECR_Block);
					break;
				case ETeam::Team2:
					Projectile->GetCollisionBox()->SetCollisionResponseToChannel(ECC_TEAM1_MESH, ECollisionResponse::ECR_Block);
					break;
				}
			}
		}
	}
}
