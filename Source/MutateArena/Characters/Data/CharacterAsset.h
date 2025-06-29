#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CharacterAsset.generated.h"

UCLASS()
class MUTATEARENA_API UCharacterAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 脚步声
	UPROPERTY(EditAnywhere, Category = "FootSound")
	class UMetaSoundSource* FootSound_Concrete;
	UPROPERTY(EditAnywhere, Category = "FootSound")
	UMetaSoundSource* FootSound_Dirt;
	UPROPERTY(EditAnywhere, Category = "FootSound")
	UMetaSoundSource* FootSound_Metal;
	UPROPERTY(EditAnywhere, Category = "FootSound")
	UMetaSoundSource* FootSound_Wood;
	UPROPERTY(EditAnywhere, Category = "FootSound")
	UMetaSoundSource* FootLandSound_Concrete;
	UPROPERTY(EditAnywhere, Category = "FootSound")
	UMetaSoundSource* FootLandSound_Dirt;
	UPROPERTY(EditAnywhere, Category = "FootSound")
	UMetaSoundSource* FootLandSound_Metal;
	UPROPERTY(EditAnywhere, Category = "FootSound")
	UMetaSoundSource* FootLandSound_Wood;

	// GAS
	UPROPERTY(EditAnywhere, Category = "GAS")
	TSubclassOf<class UGameplayAbilityBase> MutantChangeAbility;
	
	UPROPERTY(EditAnywhere, Category = "GAS")
	TSubclassOf<class UGameplayAbilityBase> MutantRestoreAbility;
	UPROPERTY(EditAnywhere, Category = "GAS")
	TSubclassOf<class UGameplayEffect> MutantRestoreEffect;

	UPROPERTY(EditAnywhere, Category = "Character")
	UMaterialInterface* MI_BloodDecal;
	UPROPERTY(EditAnywhere, Category = "Character")
	UMaterialInterface* MI_Overlay_Dead;

	// 低血量
	UPROPERTY(EditAnywhere, Category = "LowHealth")
	UMaterialParameterCollection* MPC_LowHealth;

	// 闪光弹
	UPROPERTY(EditAnywhere, Category = "Flashbang")
	UTextureRenderTarget2D* RT_Flashbang;
	UPROPERTY(EditAnywhere, Category = "Flashbang")
	UMaterialInterface* MI_Flashbang;
	UPROPERTY(EditAnywhere, Category = "Flashbang")
	UMaterialParameterCollection* MPC_Flashbang;

	// 无线电文本
	UPROPERTY(EditAnywhere, Category = "Radio")
	TArray<FString> RadioTexts;

};
