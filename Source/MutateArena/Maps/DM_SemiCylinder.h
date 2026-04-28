#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DM_SemiCylinder.generated.h"

class UDynamicMeshComponent;
class UMaterialInterface;

UCLASS()
class MUTATEARENA_API ADM_SemiCylinder : public AActor
{
	GENERATED_BODY()
    
public:	
	ADM_SemiCylinder();

	UPROPERTY(VisibleAnywhere)
	UDynamicMeshComponent* DynamicMeshComponent;

	virtual void OnConstruction(const FTransform& Transform) override;

	// 半圆柱半径
	UPROPERTY(EditAnywhere)
	float Radius = 2200.0f;

	// 半圆柱高度
	UPROPERTY(EditAnywhere)
	float Height = 260.0f;

	// 段数
	UPROPERTY(EditAnywhere, meta = (ClampMin = "3", UIMin = "3"))
	int32 ArcSteps = 20;

	// 材质
	UPROPERTY(EditAnywhere)
	UMaterialInterface* Material;

private:
	void Generate();
	
};
