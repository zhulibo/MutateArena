#pragma once

#include "CoreMinimal.h"
#include "Throwing.h"
#include "Smoke.generated.h"

UCLASS()
class MUTATEARENA_API ASmoke : public AThrowing
{
	GENERATED_BODY()

public:
	ASmoke();

	virtual void ThrowOut() override;

protected:
	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* ExplodeEffect;
	UPROPERTY(EditAnywhere)
	class UMetaSoundSource* ExplodeSound;

	void Explode();

};
