#pragma once

#include "CoreMinimal.h"
#include "Throwable.h"
#include "Grenade.generated.h"

class UFragGrenade;

UCLASS()
class MUTATEARENA_API AGrenade : public AThrowable
{
	GENERATED_BODY()

public:
	AGrenade();

	virtual void BeginPlay() override;
	virtual void ThrowOut() override;

	UPROPERTY(Transient)
	TObjectPtr<const UFragGrenade> FragGrenade = nullptr;

protected:
	UPROPERTY()
	class UNiagaraComponent* ExplodeEffectComponent;

	UFUNCTION()
	void ServerExplode();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastExplodeEffects();
	
};
