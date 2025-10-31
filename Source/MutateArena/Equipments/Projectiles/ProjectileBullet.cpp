#include "ProjectileBullet.h"

#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "MutateArena/Characters/BaseCharacter.h"
#include "MutateArena/Equipments/Data/DamageTypeEquipment.h"
#include "Components/BoxComponent.h"
#include "Components/DecalComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "MetaSoundSource.h"
#include "MutateArena/Equipments/Data/EquipmentAsset.h"
#include "MutateArena/System/AssetSubsystem.h"

AProjectileBullet::AProjectileBullet()
{
	CollisionBox->SetBoxExtent(FVector(4.f, 2.f, 2.f));
	CollisionBox->bReturnMaterialOnMove = true;

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->SetUpdatedComponent(CollisionBox);
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->InitialSpeed = InitialSpeed;
	ProjectileMovement->MaxSpeed = InitialSpeed;
}

void AProjectileBullet::BeginPlay()
{
	Super::BeginPlay();

	SetLifeSpan(1.f);
	
	CollisionBox->OnComponentHit.AddUniqueDynamic(this, &ThisClass::OnHit);
}

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	// UE_LOG(LogTemp, Warning, TEXT("AProjectileBullet::OnHit Location: %s"), *Hit.ImpactPoint.ToString());
	// UE_LOG(LogTemp, Warning, TEXT("AProjectileBullet::OnHit Rotation: %s"), *Hit.ImpactNormal.Rotation().ToString());

	ABaseCharacter* DamagedActor = Cast<ABaseCharacter>(OtherActor);

	if (HasAuthority() && DamagedActor && GetInstigator())
	{
		UGameplayStatics::ApplyDamage(
			OtherActor,
			GetDamage(Hit.Distance),
			GetInstigator()->Controller,
			this,
			UDamageTypeEquipment::StaticClass()
		);
	}

	float LifeTime = FMath::FRandRange(4.f, 5.f);
	SetLifeSpan(LifeTime);

	if (TracerEffectComponent)
	{
		TracerEffectComponent->DestroyComponent();
	}

	if (DamagedActor == nullptr)
	{
		if (ImpactEffect)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(),
				ImpactEffect,
				GetActorLocation(),
				GetActorRotation()
			);
		}

		if (ImpactDecal)
		{
			auto DecalComponent = UGameplayStatics::SpawnDecalAttached(
				ImpactDecal,
				FVector(7.f),
				HitComp,
				NAME_None,
				Hit.ImpactPoint,
				Hit.ImpactNormal.Rotation(),
				EAttachLocation::KeepWorldPosition,
				LifeTime
			);

			DecalComponent->SetFadeOut(LifeTime - 1.f, 1.f, false);
			DecalComponent->SetFadeScreenSize(0.002f);
		}
	}

	// 播放击中音效
	if (AssetSubsystem == nullptr) AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
	if (AssetSubsystem && AssetSubsystem->EquipmentAsset)
	{
		UMetaSoundSource* ImpactSound = nullptr;
		if (DamagedActor)
		{
			ImpactSound = AssetSubsystem->EquipmentAsset->ImpactSound_Body;
		}
		else
		{
			if (Hit.PhysMaterial.IsValid())
			{
				switch (Hit.PhysMaterial->SurfaceType)
				{
				case EPhysicalSurface::SurfaceType1:
					ImpactSound = AssetSubsystem->EquipmentAsset->ImpactSound_Concrete;
					break;
				case EPhysicalSurface::SurfaceType2:
					ImpactSound = AssetSubsystem->EquipmentAsset->ImpactSound_Dirt;
					break;
				case EPhysicalSurface::SurfaceType3:
					ImpactSound = AssetSubsystem->EquipmentAsset->ImpactSound_Metal;
					break;
				case EPhysicalSurface::SurfaceType4:
					ImpactSound = AssetSubsystem->EquipmentAsset->ImpactSound_Wood;
					break;
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Hit.PhysMaterial == nullptr"));
			}
		}

		if (ImpactSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
		}
	}
}
