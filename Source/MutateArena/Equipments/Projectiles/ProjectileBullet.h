#pragma once

#include "CoreMinimal.h"
#include "Projectile.h"
#include "ProjectileBullet.generated.h"

/**
 * 子弹不从服务端复制到客户端，子弹在客户端服务器各生成一个，伤害在服务端计算，
 * 减少网络同步，但行为可能不一致。
 */
UCLASS()
class MUTATEARENA_API AProjectileBullet : public AProjectile
{
	GENERATED_BODY()

public:
	AProjectileBullet();

protected:
	virtual void BeginPlay() override;

	UPROPERTY()
	class UAssetSubsystem* AssetSubsystem;
	
	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		FVector NormalImpulse, const FHitResult& Hit);

	
};
