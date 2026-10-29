#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MutateArena/Characters/Data/CharacterType.h"
#include "MutateArena/MutateArena.h"
#include "CharacterCommonAsset.generated.h"

class UTextureRenderTarget2D;
class UMaterialParameterCollection;
class UMaterialInterface;

UCLASS()
class MUTATEARENA_API UCharacterCommonAsset : public UPrimaryDataAsset
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
	
	// Character
	UPROPERTY(EditAnywhere, Category = "Character")
	UMaterialInterface* MI_Overlay_Dead;
	UPROPERTY(EditAnywhere, Category = "Audio")
	
	class USoundAttenuation* Atten_Footstep_Normal;
	UPROPERTY(EditAnywhere, Category = "Audio")
	class USoundAttenuation* Atten_Footstep_Enhanced;

	// 受伤
	UPROPERTY(EditAnywhere, Category = "Hurt")
	UMaterialInterface* MI_Hurt;

	// 闪光弹
	UPROPERTY(EditAnywhere, Category = "Flashbang")
	UTextureRenderTarget2D* RT_Flashbang;
	UPROPERTY(EditAnywhere, Category = "Flashbang")
	UMaterialInterface* MI_Flashbang;
	
	// DNA
	UPROPERTY(EditAnywhere, Category = "DNA")
	UMaterialParameterCollection* MPC_EnhancedVision;
	
	// 脚印贴花
	UPROPERTY(EditAnywhere)
	TObjectPtr<UMaterialInterface> MI_Footprint;

	// AMutantCharacter击墙
	UPROPERTY(EditAnywhere, Category = "HitWall")
	UMaterialInterface* WallMarkDecal;
	UPROPERTY(EditAnywhere, Category = "HitWall")
	UMetaSoundSource* HitWall_Concrete;
	UPROPERTY(EditAnywhere, Category = "HitWall")
	UMetaSoundSource* HitWall_Dirt;
	UPROPERTY(EditAnywhere, Category = "HitWall")
	UMetaSoundSource* HitWall_Metal;
	UPROPERTY(EditAnywhere, Category = "HitWall")
	UMetaSoundSource* HitWall_Wood;
	
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(DataAsset::CharacterCommon, GetFName());
	}

};

UCLASS()
class MUTATEARENA_API UCharacterHumanAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	FText ShowName;

	UPROPERTY(EditAnywhere)
	EHumanCharacterName CharacterName = EHumanCharacterName::None;
    
	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UTexture2D> ShowImg;
    
	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UTexture2D> ShowImgFullBody;

	UPROPERTY(EditAnywhere)
	TSoftClassPtr<AHumanCharacter> CharacterClass;

	/**
	 * Must be the same as the OfferId in dev portal,
	 * Used to determine the button type in the UShop::OnQueryOffersComplete.
	 */
	UPROPERTY(EditAnywhere)
	FString OfferId;

	/**
	 * Must be same as the audience item ID in dev portal,
	 * Used to determine whether player's account owns the product.
	 */
	UPROPERTY(EditAnywhere)
	FString AudienceItemId;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(DataAsset::CharacterHuman, GetFName());
	}
	
};

UCLASS()
class MUTATEARENA_API UCharacterMutantAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	FText ShowName;

	UPROPERTY(EditAnywhere)
	EMutantCharacterName CharacterName = EMutantCharacterName::None;
	
	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UTexture2D> ShowImg;
	
	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UTexture2D> ShowImgFullBody;

	UPROPERTY(EditAnywhere)
	TSoftClassPtr<class AMutantCharacter> CharacterClass;

	UPROPERTY(EditAnywhere)
	float LightAttackDamage = 0.f;
	UPROPERTY(EditAnywhere)
	float HeavyAttackDamage = 0.f;

	UPROPERTY(EditAnywhere)
	FText Attribute;
	UPROPERTY(EditAnywhere)
	FText Skill;
	UPROPERTY(EditAnywhere)
	FText Desc;

	UPROPERTY(EditAnywhere)
	TArray<FName> TraceSockets_R;
	UPROPERTY(EditAnywhere)
	float TraceRadius_R = 10.f;
	UPROPERTY(EditAnywhere)
	TArray<FName> TraceSockets_L;
	UPROPERTY(EditAnywhere)
	float TraceRadius_L = 10.f;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(DataAsset::CharacterMutant, GetFName());
	}
	
};
