#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

enum class EEquipmentName : uint8;

UCLASS()
class MUTATEARENA_API AProjectile : public AActor
{
	GENERATED_BODY()

public:
	AProjectile();

	UPROPERTY()
	float InitialSpeed = 40000.f;

	UFUNCTION()
	float GetDamage(float Distance);
	UFUNCTION()
	float GetImpulse(float DeclineDamage);

	UPROPERTY()
	FName OwnerName;

protected:
	virtual void PostActorCreated() override;
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere)
	class UBoxComponent* CollisionBox;
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* ProjectileMesh;
	UPROPERTY(VisibleAnywhere)
	class UProjectileMovementComponent* ProjectileMovement;

	// 在空中留下的轨迹
	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* TrailEffect;
	UPROPERTY()
	class UNiagaraComponent* TrailEffectComponent;
	void SpawnTrailEffect();

	// 尾部效果
	UPROPERTY(EditAnywhere)
	UNiagaraSystem* TracerEffect;
	UPROPERTY()
	UNiagaraComponent* TracerEffectComponent;
	void SpawnTracerEffect();

	// 击中效果
	UPROPERTY(EditAnywhere)
	UNiagaraSystem* ImpactEffect;

	// 弹孔
	UPROPERTY(EditAnywhere)
	TObjectPtr<UMaterialInterface> ImpactDecal;

public:
	FORCEINLINE UBoxComponent* GetCollisionBox() const { return CollisionBox; }

};
