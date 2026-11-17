#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Equipments/Throwable.h"
#include "Smoke.generated.h"

class UFragSmoke;

UCLASS()
class MUTATEARENA_API ASmoke : public AThrowable
{
	GENERATED_BODY()

public:
	ASmoke();

	virtual void BeginPlay() override;
	virtual void ThrowOut() override;

	UPROPERTY(Transient)
	TObjectPtr<const UFragSmoke> FragSmoke = nullptr;

protected:
	virtual void Destroyed() override;

	UPROPERTY()
	class UNiagaraComponent* SpawnedSmokeEffect;
	UPROPERTY()
	class UAudioComponent* SpawnedSmokeSound;
	UPROPERTY()
	class ASmokeBlocker* SpawnedSmokeBlocker;
	
	UFUNCTION()
	void ServerExplode();
	UFUNCTION(NetMulticast, Reliable)
	void MulticastExplodeEffects();
	
};
