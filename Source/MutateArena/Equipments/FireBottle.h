#pragma once

#include "CoreMinimal.h"
#include "Throwing.h"
#include "FireBottle.generated.h"

UCLASS()
class MUTATEARENA_API AFireBottle : public AThrowing
{
	GENERATED_BODY()

public:
	AFireBottle();

	virtual void ThrowOut() override;

protected:
	virtual void Destroyed() override;

	UPROPERTY(VisibleAnywhere)
	class USphereComponent* FireSphere;
	UPROPERTY()
	float FireRadius = 200.f;
	
	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* ExplodeEffect;
	UPROPERTY(EditAnywhere)
	class UMetaSoundSource* ExplodeSound;
	UPROPERTY(EditAnywhere)
	class UMetaSoundSource* BurnSound;
	UPROPERTY(EditAnywhere)
	UMetaSoundSource* ExtinguishSound;

	UPROPERTY()
	UAudioComponent* AudioComponent;

	void Explode();

	UPROPERTY()
	FTimerHandle DetectTimerHandle;

	UFUNCTION()
	void DetectActors();
	
};
