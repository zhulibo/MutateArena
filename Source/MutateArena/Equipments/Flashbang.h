#pragma once

#include "CoreMinimal.h"
#include "Throwing.h"
#include "Flashbang.generated.h"

UCLASS()
class MUTATEARENA_API AFlashbang : public AThrowing
{
	GENERATED_BODY()

public:
	AFlashbang();

	virtual void ThrowOut() override;

protected:
	UPROPERTY()
	class ABaseGameState* BaseGameState;

	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* ExplodeEffect;
	UPROPERTY()
	class UNiagaraComponent* ExplodeEffectComponent;
	UPROPERTY(EditAnywhere)
	class UMetaSoundSource* ExplodeSound;

	UPROPERTY()
	float Radius = 1000.f;
	UPROPERTY()
	float MaxFlashTime = 6.f;
	UPROPERTY()
	float MaxCapTime = 8.f;
	void Explode();
	void PlayExplodeSound();

};
