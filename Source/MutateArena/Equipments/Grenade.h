#pragma once

#include "CoreMinimal.h"
#include "Throwing.h"
#include "Grenade.generated.h"

UCLASS()
class MUTATEARENA_API AGrenade : public AThrowing
{
	GENERATED_BODY()

public:
	AGrenade();

	virtual void ThrowOut() override;

protected:
	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* ExplodeEffect;
	UPROPERTY()
	class UNiagaraComponent* ExplodeEffectComponent;
	UPROPERTY(EditAnywhere)
	class UMetaSoundSource* ExplodeSound;

	UPROPERTY()
	float Damage = 500.f;
	UPROPERTY()
	float DamageInnerRadius = 100.f;
	UPROPERTY()
	float DamageOuterRadius = 400.f;

	UFUNCTION()
	void ServerExplode();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastExplodeEffects();
	
};
