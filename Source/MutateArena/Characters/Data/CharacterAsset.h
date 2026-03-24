#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MutateArena/MutateArena.h"
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
	TArray<TSubclassOf<class UGameplayAbilityBase>> HumanDefaultAbilities;
	
	UPROPERTY(EditAnywhere, Category = "GAS")
	TArray<TSubclassOf<UGameplayAbilityBase>> MutantDefaultAbilities;

	UPROPERTY(EditAnywhere, Category = "GAS")
	TSubclassOf<UGameplayEffect> GE_DNA_EnhancedVision;
	UPROPERTY(EditAnywhere, Category = "GAS")
	TSubclassOf<UGameplayEffect> GE_DNA_EnhancedHearing;
	UPROPERTY(EditAnywhere, Category = "GAS")
	TSubclassOf<UGameplayEffect> GE_DNA_EnhancedSmell;
	UPROPERTY(EditAnywhere, Category = "GAS")
	TSubclassOf<UGameplayEffect> GE_DNA_SubconsciousAwareness;
	UPROPERTY(EditAnywhere, Category = "GAS")
	TSubclassOf<UGameplayEffect> GE_DNA_HighBoneDensity;
	UPROPERTY(EditAnywhere, Category = "GAS")
	TSubclassOf<UGameplayEffect> GE_DNA_AcceleratedClotting;
	UPROPERTY(EditAnywhere, Category = "GAS")
	TSubclassOf<UGameplayEffect> GE_DNA_AcceleratedMetabolism;
	UPROPERTY(EditAnywhere, Category = "GAS")
	TSubclassOf<UGameplayEffect> GE_DNA_ThermalRegulation;
	UPROPERTY(EditAnywhere, Category = "GAS")
	TSubclassOf<UGameplayEffect> GE_DNA_ToxicityImmunity;
	UPROPERTY(EditAnywhere, Category = "GAS")
	TSubclassOf<UGameplayEffect> GE_DNA_PainModulation;
	UPROPERTY(EditAnywhere, Category = "GAS")
	TSubclassOf<UGameplayEffect> GE_DNA_CoreMaintenance;

	// Character
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
	
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(ASSET_CHARACTER, GetFName());
	}

};
