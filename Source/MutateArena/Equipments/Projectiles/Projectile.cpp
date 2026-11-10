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

	VisualRootComp = CreateDefaultSubobject<USceneComponent>(TEXT("VisualRootComp"));
	VisualRootComp->SetupAttachment(RootComponent);

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(VisualRootComp);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();
	
	SpawnLocation = GetActorLocation();
	LastTickLocation = SpawnLocation;

	if (VisualRootComp)
	{
		InitialRelativeOffset = GetActorTransform().InverseTransformPosition(MuzzleLocation);
		// 让弹头和特效在合并阶段朝向汇聚点，消除侧面平移感
		InitialRelativeRotation = (FVector(BlendDistance, 0.f, 0.f) - InitialRelativeOffset).Rotation();

		VisualRootComp->SetRelativeLocationAndRotation(
			InitialRelativeOffset,
			InitialRelativeRotation,
			false,
			nullptr,
			ETeleportType::TeleportPhysics
		);
		bNeedsVisualBlend = true;
	}

	SpawnTrailEffect();
	SpawnTracerEffect();
}

void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateVisualOffset();
}

void AProjectile::UpdateVisualOffset()
{
	if (!bNeedsVisualBlend || !VisualRootComp) return;

	const FVector CurrentLocation = GetActorLocation();
	TraveledDistance += FVector::Distance(LastTickLocation, CurrentLocation);
	LastTickLocation = CurrentLocation;

	// 已会聚距离除以总会聚距离算出插值参数Alpha
	const float Alpha = FMath::Clamp(TraveledDistance / BlendDistance, 0.f, 1.f);
	const FVector NewRelLoc = FMath::Lerp(InitialRelativeOffset, FVector::ZeroVector, Alpha);
	const FRotator NewRelRot = FMath::Lerp(InitialRelativeRotation, FRotator::ZeroRotator, Alpha);
	VisualRootComp->SetRelativeLocationAndRotation(NewRelLoc, NewRelRot);

	if (Alpha >= 1.f)
	{
		bNeedsVisualBlend = false;
	}
}

void AProjectile::CompleteVisualBlend()
{
	if (VisualRootComp)
	{
		VisualRootComp->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
	}
	bNeedsVisualBlend = false;
}

void AProjectile::SpawnTrailEffect()
{
	// if (TrailEffect && VisualRootComp)
	// {
	//     TrailEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
	//        TrailEffect,
	//        VisualRootComp,
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
	if (TracerEffect && VisualRootComp)
	{
		TracerEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			TracerEffect,
			VisualRootComp,
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
