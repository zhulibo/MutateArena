#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DM_CubeSubCylinder.generated.h"

class UDynamicMeshComponent;

UCLASS()
class MUTATEARENA_API ADM_CubeSubCylinder : public AActor
{
	GENERATED_BODY()

public:
	ADM_CubeSubCylinder();

	// 核心组件：动态网格体
	UPROPERTY(VisibleAnywhere)
	UDynamicMeshComponent* DynamicMeshComponent;

	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintCallable)
	void Generate();
    
	// 主体材质 (ID 0)
	UPROPERTY(EditAnywhere)
	UMaterialInterface* BodyMaterial;

	// 切口内壁材质 (ID 1)
	UPROPERTY(EditAnywhere)
	UMaterialInterface* CutoutMaterial;

	// 矩形主体尺寸 (长X, 宽Y, 厚度Z)
	// 假设我们在 X/Y 平面上看，X是宽，Y是长
	UPROPERTY(EditAnywhere)
	FVector BodySize = FVector(4000.f, 7000.f, 260.f);

	// 边缘缺口(大刀)的半径
	UPROPERTY(EditAnywhere)
	float CutoutRadius = 1500.f;

	// 圆柱边缘的圆滑程度 (分段数)
	UPROPERTY(EditAnywhere, meta = (ClampMin = "3", UIMin = "3"))
	int32 RadialSteps = 20;
	
protected:
	virtual void BeginPlay() override;
	
};
