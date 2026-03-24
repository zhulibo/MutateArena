#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Equipments/Throwing.h"
#include "Flashbang.generated.h"

UCLASS()
class MUTATEARENA_API AFlashbang : public AThrowing
{
	GENERATED_BODY()

public:
	AFlashbang();

protected:
	virtual void ThrowOut() override;

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
	// 闪光留存时间
	UPROPERTY()
	float MaxFlashTime = 6.f;
	// 残影留存时间
	UPROPERTY()
	float MaxCapTime = 8.f;

	UFUNCTION()
	void ServerExplode();

	UFUNCTION()
	void ServerPlaySound();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastExplode();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlaySound();
	
};
