#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Equipments/Throwable.h"
#include "Stun.generated.h"

class UFragStun;

UCLASS()
class MUTATEARENA_API AStun : public AThrowable
{
	GENERATED_BODY()

public:
	AStun();

	virtual void BeginPlay() override;
	virtual void ThrowOut() override;

	UPROPERTY(Transient)
	TObjectPtr<const UFragStun> FragStun = nullptr;

protected:
	UPROPERTY()
	class UNiagaraComponent* ExplodeEffectComponent;

	UFUNCTION()
	void ServerExplode();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastExplodeEffects();
	
};
