#pragma once

#include "CoreMinimal.h"
#include "Pickup.h"
#include "PickupHerb.generated.h"

UCLASS()
class MUTATEARENA_API APickupHerb : public APickup
{
	GENERATED_BODY()

public:
	APickupHerb();
	
	UFUNCTION(Server, Reliable)
	void ServerDestroy();

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere)
	UStaticMesh* PickupMeshLevel2;

	UPROPERTY(ReplicatedUsing = OnRep_Level)
	int32 Level = 1;
	void SetLevel(int32 TempLevel);
	UFUNCTION()
	void OnRep_Level();

	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;

};
