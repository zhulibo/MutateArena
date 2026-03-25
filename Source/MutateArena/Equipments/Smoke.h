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
	
	UFUNCTION()
	void ServerExplode();
	float Time = 12.f;
	UFUNCTION(NetMulticast, Reliable)
	void MulticastExplodeEffects();
	
};
