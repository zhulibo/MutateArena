#include "Projectile.h"
#include "Components/BoxComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/StaticMeshComponent.h"
#include "Curves/CurveFloat.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/Equipments/Data/EquipmentAsset.h"
#include "MutateArena/System/AssetSubsystem.h"

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	SetRootComponent(CollisionBox);
	CollisionBox->SetCollisionObjectType(ECC::Projectile);
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Block);
	CollisionBox->SetUseCCD(true);

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
	SpawnLocation = GetActorLocation();
}

void AProjectile::SpawnTrailEffect()
{
	// if (TrailEffect)
	// {
	//     TrailEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
	//        TrailEffect,
	//        GetRootComponent(),
	//        TEXT(""),
	//        FVector::ZeroVector,
	//        FRotator::ZeroRotator,
	//        EAttachLocation::KeepRelativeOffset,
	//        true
	//     );
	// }
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

void AProjectile::EnsureWeaponFragCached()
{
	if (bIsWeaponFragCached) return;

	if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
	{
		if (UAssetSubsystem* AssetSubsystem = GameInstance->GetSubsystem<UAssetSubsystem>())
		{
			if (UEquipmentAsset* WeaponAsset = AssetSubsystem->GetEquipmentAssetByName(ParentEquipmentName))
			{
				CachedWeaponFrag = WeaponAsset->FindFragmentByClass<UFragWeapon>();
			}
		}
	}

	bIsWeaponFragCached = true;
}

float AProjectile::GetDamage(float Distance)
{
	EnsureWeaponFragCached();

	if (CachedWeaponFrag && CachedWeaponFrag->DamageCurve && CachedWeaponFrag->PelletNum != 0)
	{
		return CachedWeaponFrag->DamageCurve->GetFloatValue(Distance / 100.f) / CachedWeaponFrag->PelletNum;
	}

	return 0.f;
}

// 子弹伤害会受到受穿透力影响，施加伤害时已计算在内，击退力受伤害影响，所以间接地也受到穿透力影响
float AProjectile::GetImpulse(float DeclineDamage)
{
	EnsureWeaponFragCached();

	if (CachedWeaponFrag && CachedWeaponFrag->DamageCurve && CachedWeaponFrag->PelletNum != 0)
	{
		float MaxDamage = CachedWeaponFrag->DamageCurve->GetFloatValue(1.f);
		if (MaxDamage != 0.f)
		{
			return CachedWeaponFrag->Impulse * (DeclineDamage * CachedWeaponFrag->PelletNum / MaxDamage) / CachedWeaponFrag->PelletNum;
		}
	}

	return 0.f;
}
