#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Equipments/Throwable.h"
#include "Flashbang.generated.h"

class UFragFlashbang;

UCLASS()
class MUTATEARENA_API AFlashbang : public AThrowable
{
	GENERATED_BODY()

public:
	AFlashbang();

	virtual void BeginPlay() override;

	UPROPERTY(Transient)
	TObjectPtr<const UFragFlashbang> FragFlashbang = nullptr;

protected:
	virtual void ThrowOut() override;

	UPROPERTY()
	class UNiagaraComponent* ExplodeEffectComponent;

	UFUNCTION()
	void ServerExplode();

	UFUNCTION()
	void ServerPlaySound();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastExplode();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlaySound();
	
};
