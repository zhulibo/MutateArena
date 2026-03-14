#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Equipments/Throwing.h"
#include "FireBottle.generated.h"

UCLASS()
class MUTATEARENA_API AFireBottle : public AThrowing
{
	GENERATED_BODY()

public:
	AFireBottle();

	virtual void ThrowOut() override;

protected:
	virtual void BeginPlay() override;
	virtual void Destroyed() override;

	UPROPERTY(VisibleAnywhere)
	class USphereComponent* FireSphere;
	UPROPERTY()
	float FireRadius = 200.f;

	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* ExplodeEffect;
	UPROPERTY(EditAnywhere)
	class UMetaSoundSource* ExplodeSound;
	UPROPERTY(EditAnywhere)
	class UMetaSoundSource* BurnSound;
	UPROPERTY(EditAnywhere)
	UMetaSoundSource* ExtinguishSound;
	
	UPROPERTY()
	class UNiagaraComponent* SpawnedFireEffect;
	UPROPERTY()
	UAudioComponent* SpawnedFireSound;

	// 监听碰撞
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	float Time = 10.f;
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
