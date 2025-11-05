#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Equipments/Herbs/Herb.h"
#include "HerbRepelReceived.generated.h"

UCLASS()
class MUTATEARENA_API AHerbRepelReceived : public AHerb
{
	GENERATED_BODY()

public:
	AHerbRepelReceived();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class UGameplayEffect> HerbRepelReceivedEffect;
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;

};
