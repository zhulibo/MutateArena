#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Pickup.generated.h"

UCLASS(Abstract)
class MUTATEARENA_API APickup : public AActor
{
	GENERATED_BODY()
	
public:
	APickup();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere)
	class USphereComponent* OverlapSphere;
	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* PickupMesh;

	UPROPERTY()
	class AMutationGameState* MutationGameState;

	UFUNCTION()
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) {}

	void OnRoundStarted();

};
