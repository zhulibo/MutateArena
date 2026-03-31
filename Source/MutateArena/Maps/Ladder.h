#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Ladder.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

UCLASS()
class MUTATEARENA_API ALadder : public AActor
{
	GENERATED_BODY()
	
public:	
	ALadder();

protected:
	virtual void BeginPlay() override;

public:	
	UPROPERTY(VisibleAnywhere)
	USceneComponent* RootAnchor;
	
	UPROPERTY(VisibleAnywhere)
	UBoxComponent* TriggerBox;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* LadderMesh;
	
};
