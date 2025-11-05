#include "Projectile.h"

#include "DataRegistrySubsystem.h"
#include "Components/BoxComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/Equipments/Data/EquipmentType.h"

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	SetRootComponent(CollisionBox);
	CollisionBox->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AProjectile::PostActorCreated()
{
	Super::PostActorCreated();

	// 尽早生成，避免OnHit时Component还未生成
	SpawnTrailEffect();
	SpawnTracerEffect();
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();
}

void AProjectile::SpawnTrailEffect()
{
	if (TrailEffect)
	{
		TrailEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			TrailEffect,
			GetRootComponent(),
			TEXT(""),
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::KeepRelativeOffset,
			true
		);
	}
}

void AProjectile::SpawnTracerEffect()
{
	if (TracerEffect)
	{
		TracerEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			TracerEffect,
			GetRootComponent(),
			TEXT(""),
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::KeepRelativeOffset,
			true
		);
	}
}

float AProjectile::GetDamage(float Distance)
{
	float Damage = 0.f;

	FDataRegistryId DataRegistryId(DR_WEAPON_DATA, OwnerName);
	if (const FWeaponData* WeaponData = UDataRegistrySubsystem::Get()->GetCachedItem<FWeaponData>(DataRegistryId))
	{
		if (UCurveFloat* DamageCurve = WeaponData->DamageCurve)
		{
			Damage = DamageCurve->GetFloatValue(Distance / 100) / WeaponData->PelletNum;
		}
	}

	return Damage;
}

float AProjectile::GetImpulse(float DeclineDamage)
{
	float Impulse = 0.f;

	FDataRegistryId DataRegistryId(DR_WEAPON_DATA, OwnerName);
	if (const FWeaponData* WeaponData = UDataRegistrySubsystem::Get()->GetCachedItem<FWeaponData>(DataRegistryId))
	{
		if (UCurveFloat* DamageCurve = WeaponData->DamageCurve)
		{
			float MaxDamage = DamageCurve->GetFloatValue(1.f);
			if (MaxDamage != 0 && WeaponData->PelletNum != 0)
			{
				Impulse = WeaponData->Impulse * (DeclineDamage * WeaponData->PelletNum / MaxDamage) / WeaponData->PelletNum;
			}
		}
	}

	return Impulse;
}
