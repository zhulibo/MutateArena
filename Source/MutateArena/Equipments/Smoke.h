#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Equipments/Throwing.h"
#include "Smoke.generated.h"

UCLASS()
class MUTATEARENA_API ASmoke : public AThrowing
{
	GENERATED_BODY()

public:
	ASmoke();

	virtual void ThrowOut() override;

protected:
	virtual void Destroyed() override;

	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* ExplodeEffect;
	UPROPERTY(EditAnywhere)
	class UMetaSoundSource* ExplodeSound;
	UPROPERTY()
	class UNiagaraComponent* SpawnedSmokeEffect;
	UPROPERTY()
	class UAudioComponent* SpawnedSmokeSound;
	UPROPERTY()
	class ASmokeBlocker* SpawnedSmokeBlocker;
	
	// 服务器执行爆炸逻辑
	UFUNCTION()
	void ServerExplode();
	
	float Time = 12.f;
	// 多播给客户端播放表现，并开启本地视线遮挡
	UFUNCTION(NetMulticast, Reliable)
	void MulticastExplodeEffects();
	
};
