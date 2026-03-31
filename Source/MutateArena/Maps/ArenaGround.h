#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArenaGround.generated.h"

class UDynamicMeshComponent;

UCLASS()
class MUTATEARENA_API AArenaGround : public AActor
{
	GENERATED_BODY()

public:
	AArenaGround();

	// 核心组件：动态网格体
	UPROPERTY(VisibleAnywhere)
	UDynamicMeshComponent* DynamicMeshComponent;

	UFUNCTION(BlueprintCallable)
	void Generate();
    
	// 地面材质 (ID 0)
	UPROPERTY(EditAnywhere)
	UMaterialInterface* GroundMaterial;

	// 平台材质 (ID 1)
	UPROPERTY(EditAnywhere)
	UMaterialInterface* PlatformMaterial;

	// 通道内壁材质 (ID 2)
	UPROPERTY(EditAnywhere)
	UMaterialInterface* HoleMaterial;
    
	// 【新增】：下方管道外壁材质 (ID 3)
	UPROPERTY(EditAnywhere)
	UMaterialInterface* PipeMaterial;

	// 地面尺寸 (长X, 宽Y, 厚度Z)
	UPROPERTY(EditAnywhere)
	FVector GroundSize = FVector(8000.f, 8000.f, 100.f);

	// 平台(顶部)半径
	UPROPERTY(EditAnywhere)
	float PlatformRadius = 600.f;

	// 平台(底部)半径 —— 产生平缓坡度
	UPROPERTY(EditAnywhere)
	float PlatformBottomRadius = 700.f;

	// 平台高度
	UPROPERTY(EditAnywhere)
	float PlatformHeight = 20.f;

	// 中央通道(洞口)的半径
	UPROPERTY(EditAnywhere)
	float HoleRadius = 500.f;

	// 【新增】：下方管道的长度 (向下延伸多深)
	UPROPERTY(EditAnywhere)
	float PipeLength = 1300.f;

	// 【新增】：下方管道的管壁厚度
	UPROPERTY(EditAnywhere)
	float PipeThickness = 800.f;

	// 圆柱边缘的圆滑程度 (分段数)
	UPROPERTY(EditAnywhere)
	int32 RadialSteps = 64;

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	
};
