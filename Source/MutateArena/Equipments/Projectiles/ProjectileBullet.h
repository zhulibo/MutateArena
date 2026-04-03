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
	
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	float LifeTime = 1.f;
public:
	UPROPERTY()
	float InitialPenetrationPower = 100.f;
protected:
	// 当前剩余穿甲力
	float CurrentPenetrationPower;
	// 木头材质单位阻力系数 (每穿透 1cm 扣除的穿透力)
	UPROPERTY()
	float Resistance1cm = 1.f; // TODO 目前只设置了木头可以穿透
	// 记录已经伤害过的角色，防止一枪穿过手和胸造成两次伤害
	UPROPERTY()
	TSet<AActor*> DamagedActors;
	// 延迟一帧恢复速度的辅助函数
	void RestoreVelocity(FVector NewVelocity);
	
};
