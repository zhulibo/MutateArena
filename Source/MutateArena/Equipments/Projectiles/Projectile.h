#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

enum class EEquipmentName : uint8;
class UFragWeapon;

UCLASS()
class MUTATEARENA_API AProjectile : public AActor
{
	GENERATED_BODY()

public:
	AProjectile();

	UPROPERTY(VisibleAnywhere)
	class UBoxComponent* CollisionBox;

	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* ProjectileMesh;

	UPROPERTY(VisibleAnywhere)
	class UProjectileMovementComponent* ProjectileMovement;

	UPROPERTY()
	float InitialSpeed = 40000.f;

	UPROPERTY()
	FVector SpawnLocation;

	UFUNCTION()
	float GetDamage(float Distance);

	UFUNCTION()
	float GetImpulse(float DeclineDamage);

	EEquipmentName EquipmentParentName;

protected:
	virtual void PostActorCreated() override;
	virtual void BeginPlay() override;

	// 在空中留下的轨迹
	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* TrailEffect;

	UPROPERTY()
	class UNiagaraComponent* TrailEffectComponent;

	void SpawnTrailEffect();

	// 尾部效果
	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* TracerEffect;

	UPROPERTY()
	class UNiagaraComponent* TracerEffectComponent;

	void SpawnTracerEffect();

	// 击中效果
	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* ImpactEffect;

	// 弹孔
	UPROPERTY(EditAnywhere)
	TObjectPtr<UMaterialInterface> ImpactDecal;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UMaterialInterface> ImpactDecalExit;

	const UFragWeapon* CachedWeaponFrag = nullptr;
	bool bIsWeaponFragCached = false;

	void EnsureWeaponFragCached();
};
