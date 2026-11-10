#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

class UProjectileMovementComponent;
class UBoxComponent;
class UNiagaraSystem;
class UNiagaraComponent;
enum class EEquipmentName : uint8;
class UFragWeapon;

UCLASS()
class MUTATEARENA_API AProjectile : public AActor
{
	GENERATED_BODY()

public:
	AProjectile();

	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere)
	UBoxComponent* CollisionBox;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> VisualRootComp;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* ProjectileMesh;

	UPROPERTY(VisibleAnywhere)
	UProjectileMovementComponent* ProjectileMovement;

	UPROPERTY()
	float InitialSpeed = 40000.f;

	UPROPERTY()
	FVector SpawnLocation;

	UFUNCTION()
	float GetDamage(float Distance);

	UFUNCTION()
	float GetImpulse(float DeclineDamage);

	EEquipmentName ParentEquipmentName;

	// 根据当前飞行距离更新视觉偏移
	void UpdateVisualOffset();

	// 立即完成视觉合并（用于进入传送门或终止碰撞时）
	void CompleteVisualBlend();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere)
	UNiagaraSystem* TrailEffect;
	UPROPERTY()
	UNiagaraComponent* TrailEffectComponent;
	void SpawnTrailEffect();

	UPROPERTY(EditAnywhere)
	UNiagaraSystem* TracerEffect;
	UPROPERTY()
	UNiagaraComponent* TracerEffectComponent;
	void SpawnTracerEffect();

	UPROPERTY(EditAnywhere)
	UNiagaraSystem* ImpactEffect;
	UPROPERTY(EditAnywhere)
	TObjectPtr<UMaterialInterface> ImpactDecal;
	UPROPERTY(EditAnywhere)
	TObjectPtr<UMaterialInterface> ImpactDecalExit;

	UPROPERTY()
	const UFragWeapon* CachedWeaponFrag = nullptr;
	bool bIsWeaponFragCached = false;

	void EnsureWeaponFragCached();

public:
	FVector MuzzleLocation = FVector::ZeroVector;
	float BlendDistance = 1000.f;
protected:
	FVector InitialRelativeOffset = FVector::ZeroVector;
	FRotator InitialRelativeRotation = FRotator::ZeroRotator;
	FVector LastTickLocation = FVector::ZeroVector;
	float TraveledDistance = 0.f;
	bool bNeedsVisualBlend = false;
};