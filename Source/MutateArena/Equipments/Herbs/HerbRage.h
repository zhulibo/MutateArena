#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Equipments/Herbs/Herb.h"
#include "HerbRage.generated.h"

UCLASS()
class MUTATEARENA_API AHerbRage : public AHerb
{
	GENERATED_BODY()

public:
	AHerbRage();

protected:
	virtual void BeginPlay() override;

	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;

};
