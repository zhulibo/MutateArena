#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MysticVeil.generated.h"

class UStaticMeshComponent;
class UBoxComponent;

UCLASS()
class MUTATEARENA_API AMysticVeil : public AActor
{
	GENERATED_BODY()
    
public:	
	AMysticVeil();

protected:
	UPROPERTY(VisibleAnywhere)
	USceneComponent* SceneRoot;
	
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* VeilMesh;

};
