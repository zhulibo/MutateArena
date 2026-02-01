#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EquipmentAsset.generated.h"

UCLASS()
class MUTATEARENA_API UEquipmentAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 捡到装备的声音
	UPROPERTY(EditAnywhere, Category = "EquipSound")
	class UMetaSoundSource* EquipSound;
	// 无子弹时扣动扳机的声音
	UPROPERTY(EditAnywhere, Category = "DryFireSound")
	UMetaSoundSource* DryFireSound;

	UPROPERTY(EditAnywhere, Category = "ImpactSound")
	UMetaSoundSource* ImpactSound_Body;
	UPROPERTY(EditAnywhere, Category = "ImpactSound")
	UMetaSoundSource* ImpactSound_Concrete;
	UPROPERTY(EditAnywhere, Category = "ImpactSound")
	UMetaSoundSource* ImpactSound_Dirt;
	UPROPERTY(EditAnywhere, Category = "ImpactSound")
	UMetaSoundSource* ImpactSound_Metal;
	UPROPERTY(EditAnywhere, Category = "ImpactSound")
	UMetaSoundSource* ImpactSound_Wood;

	// 武器镜子
	UPROPERTY(EditAnywhere, Category = "Scope")
	UTextureRenderTarget2D* RT_Scope;
	UPROPERTY(EditAnywhere, Category = "Scope")
	UMaterialParameterCollection* MPC_Scope;

};
