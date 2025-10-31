#include "WeaponGeneric.h"

#include "MutateArena/MutateArena.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "..\PlayerStates\TeamType.h"
#include "MutateArena/Characters/Components/CombatComponent.h"
#include "MutateArena/Utils/LibraryCommon.h"
#include "Components/BoxComponent.h"
#include "Data/EquipmentType.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/KismetMathLibrary.h"
#include "Projectiles/ProjectileBullet.h"

AWeaponGeneric::AWeaponGeneric()
{
}

void AWeaponGeneric::Fire(const FVector& HitTarget, float RecoilVert, float RecoilHor, float SpreadPitch, float SpreadYaw)
{
	Super::Fire(HitTarget, RecoilVert, RecoilHor, SpreadPitch, SpreadYaw);

	if (HumanCharacter == nullptr) HumanCharacter = Cast<AHumanCharacter>(GetOwner());
	if (OwnerTeam == ETeam::NoTeam) SetOwnerTeam();
	const USkeletalMeshSocket* MuzzleSocket = EquipmentMesh->GetSocketByName(SOCKET_MUZZLE);

	if (HumanCharacter == nullptr || ProjectileClass == nullptr|| OwnerTeam == ETeam::NoTeam || MuzzleSocket == nullptr) return;

	FTransform SocketTransform = MuzzleSocket->GetSocketTransform(EquipmentMesh);
	FRotator TargetRotation = (HitTarget - SocketTransform.GetLocation()).Rotation();

	// 添加后座
	TargetRotation.Pitch += RecoilVert;
	TargetRotation.Yaw += RecoilHor;

	// 添加散布
	TargetRotation.Pitch += SpreadPitch;
	TargetRotation.Yaw += SpreadYaw;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = HumanCharacter;

	AProjectileBullet* Projectile = GetWorld()->SpawnActor<AProjectileBullet>(
		ProjectileClass,
		SocketTransform.GetLocation(),
		TargetRotation,
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
