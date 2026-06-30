#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BounceFan.generated.h"

class UMetaSoundSource;
class UAudioComponent;
class UStaticMeshComponent;
class UNiagaraComponent;
class UCapsuleComponent;
class UArrowComponent;

UCLASS()
class MUTATEARENA_API ABounceFan : public AActor
{
	GENERATED_BODY()
    
public:
	ABounceFan();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

public:
	UPROPERTY(VisibleAnywhere)
	USceneComponent* DefaultRootComp;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* FanMesh;

	UPROPERTY(VisibleAnywhere)
	UArrowComponent* LaunchDirectionArrow;

	UPROPERTY(VisibleAnywhere)
	UNiagaraComponent* NS_Wind;

	UPROPERTY(VisibleAnywhere)
	UCapsuleComponent* TriggerCapsule;

	UPROPERTY(EditAnywhere)
	float LaunchSpeed = 1500.f;

	UPROPERTY(EditAnywhere)
	bool bXYOverride = true;

	UPROPERTY(EditAnywhere)
	bool bZOverride = true;
	
	UPROPERTY(VisibleAnywhere)
	UAudioComponent* FanLoopAudioComp;

	UPROPERTY(EditAnywhere)
	UMetaSoundSource* FanLoopSound;
	
};
