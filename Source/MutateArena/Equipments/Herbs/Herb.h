#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Herb.generated.h"

UCLASS()
class MUTATEARENA_API AHerb : public AActor
{
	GENERATED_BODY()
	
public:
	AHerb();
	
	UFUNCTION(Server, Reliable)
	void ServerDestroy();

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere)
	class USphereComponent* OverlapSphere;
	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* HerbMesh;
	UPROPERTY(EditAnywhere)
	UStaticMesh* HerbMeshLevel2;

	UPROPERTY()
	class AMutationGameState* MutationGameState;

	UPROPERTY(ReplicatedUsing = OnRep_Level)
	int32 Level = 1;
	void SetLevel(int32 TempLevel);
	UFUNCTION()
	void OnRep_Level();

	UFUNCTION()
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) {}

	void OnRoundStarted();

};
