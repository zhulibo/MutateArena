#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Equipments/Throwable.h"
#include "Stun.generated.h"

class UGameplayEffect;
class UCameraShakeBase;

UCLASS()
class MUTATEARENA_API AStun : public AThrowable
{
	GENERATED_BODY()

public:
	AStun();

	virtual void ThrowOut() override;

protected:
	// 爆炸特效
	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* ExplodeEffect;

	UPROPERTY()
	class UNiagaraComponent* ExplodeEffectComponent;

	UPROPERTY(EditAnywhere)
	class UMetaSoundSource* ExplodeSound;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UCameraShakeBase> CameraShakeClass;

	UPROPERTY()
	float Radius = 800.f;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UGameplayEffect> StunEffectClass;

	UFUNCTION()
	void ServerExplode();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastExplodeEffects();
	
};
