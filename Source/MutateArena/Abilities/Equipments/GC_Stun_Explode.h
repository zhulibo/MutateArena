#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "GC_Stun_Explode.generated.h"

class UMetaSoundSource;

UCLASS()
class MUTATEARENA_API AGC_Stun_Explode : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	AGC_Stun_Explode();

protected:
	virtual bool OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	
	UPROPERTY(EditAnywhere)
	class UMaterialInterface* StunPostProcessMaterial;
	UPROPERTY()
	class UMaterialInstanceDynamic* StunMID;
	
	UPROPERTY(EditAnywhere)
	UMetaSoundSource* EarRingingSound;

	UPROPERTY()
	UAudioComponent* EarRingingAudioComponent;
	
};
