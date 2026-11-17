#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Equipments/Throwable.h"
#include "FireBottle.generated.h"

class UAudioComponent;
class UFragFireBottle;

UCLASS()
class MUTATEARENA_API AFireBottle : public AThrowable
{
	GENERATED_BODY()

public:
	AFireBottle();

	virtual void ThrowOut() override;

	virtual void BeginPlay() override;
	virtual void Destroyed() override;

	UPROPERTY(Transient)
	TObjectPtr<const UFragFireBottle> FragFireBottle = nullptr;

	UPROPERTY(VisibleAnywhere)
	class USphereComponent* FireSphere;
	
	UPROPERTY()
	class UNiagaraComponent* SpawnedFireEffect;
	UPROPERTY()
	UAudioComponent* SpawnedFireSound;

	// 监听碰撞
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	// 多播爆炸表现
	UFUNCTION(NetMulticast, Reliable)
	void MulticastExplodeEffects();

	// 多播灭火表现
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastExtinguishEffects();

	UPROPERTY()
	FTimerHandle DetectTimerHandle;

	// 服务器专属的伤害与环境检测逻辑
	UFUNCTION()
	void ServerDetectActors();

private:
	// 防止单帧多次碰撞导致重复爆炸
	bool bHasExploded = false;
	
};
