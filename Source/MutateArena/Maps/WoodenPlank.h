#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WoodenPlank.generated.h"

class UGeometryCollectionComponent;
class UBoxComponent;

UCLASS()
class MUTATEARENA_API AWoodenPlank : public AActor
{
	GENERATED_BODY()
	
public:	
	AWoodenPlank();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
protected:
	virtual void BeginPlay() override;

public:	
	UPROPERTY(VisibleAnywhere)
	USceneComponent* DefaultSceneRoot;
	
	// 几何体集合组件
	UPROPERTY(VisibleAnywhere)
	UGeometryCollectionComponent* GeometryComp;

	// 用于服务器接收射线检测(子弹)的碰撞体 物理材质设置为了木头
	UPROPERTY(VisibleAnywhere)
	UBoxComponent* HitBox;

	UPROPERTY(EditAnywhere)
	float MaxHealth = 1.f;

	// 血量需要同步
	UPROPERTY(Replicated)
	float CurrentHealth;

	// 是否已经破碎
	bool bIsBroken = false;

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	// 多播 RPC：通知所有客户端触发破碎效果
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_TriggerBreak(FVector HitLocation, FVector HitDirection);
	
};
