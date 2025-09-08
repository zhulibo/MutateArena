#pragma once

#include "CoreMinimal.h"
#include "Pickup.h"
#include "PickupAmmo.generated.h"

UCLASS()
class MUTATEARENA_API APickupAmmo : public APickup
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;

};
