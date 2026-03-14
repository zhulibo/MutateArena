#pragma once

#include "CoreMinimal.h"
#include "Pickup.h"
#include "PickupEquipment.generated.h"

UCLASS(Abstract)
class MUTATEARENA_API APickupEquipment : public APickup
{
	GENERATED_BODY()

public:
	UPROPERTY()
	class AEquipment* Equipment;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere)
	TSubclassOf<AEquipment> EquipmentClass;

	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;

};
