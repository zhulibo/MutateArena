#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "AssetSubsystem.generated.h"

class UCommonAsset;
class UInputAsset;
class UCharacterCommonAsset;
class UCharacterHumanAsset;
class UCharacterMutantAsset;
class UEquipmentCommonAsset;
class UEquipmentAsset;
class UDNAAsset2;
class UEnvironmentAsset;

enum class EHumanCharacterName : uint8;
enum class EMutantCharacterName : uint8;
enum class EEquipmentName : uint8;
enum class EDNA2 : uint8;

UCLASS()
class MUTATEARENA_API UAssetSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void SyncLoadAsset();

	UPROPERTY()
	TObjectPtr<UCommonAsset> CommonAsset;
	UPROPERTY()
	TObjectPtr<UInputAsset> InputAsset;
	UPROPERTY()
	TObjectPtr<UCharacterCommonAsset> CharacterCommonAsset;
	UPROPERTY()
	TObjectPtr<UEquipmentCommonAsset> EquipmentCommonAsset;

	void BuildEnvironmentCache();
	void BuildHumanCharacterCache();
	void BuildMutantCharacterCache();
	void BuildEquipmentCache();
	void BuildDNACache();

	UEnvironmentAsset* GetEnvironmentAssetByTag(FGameplayTag InTag) const;
	UCharacterHumanAsset* GetCharacterHumanAssetByName(EHumanCharacterName InName) const;
	UCharacterMutantAsset* GetCharacterMutantAssetByName(EMutantCharacterName InName) const;
	UEquipmentAsset* GetEquipmentAssetByName(EEquipmentName InName) const;
	UDNAAsset2* GetDNAAssetByType(EDNA2 InDNA) const;

	// 同步加载唯一全局配置资产
	template <typename AssetType>
	AssetType* LoadSinglePrimaryAsset(const FName& PrimaryAssetTypeName);

	// 批量同步加载并构建缓存 Map
	template <typename AssetType, typename KeyType, typename KeyExtractorFunc>
	void BuildPrimaryAssetCache(
		const FName& PrimaryAssetTypeName,
		TMap<KeyType, TObjectPtr<AssetType>>& OutCacheMap,
		KeyExtractorFunc KeyExtractor,
		const KeyType& InvalidKey
	);

	// 统一从缓存 Map 中查询资产
	template <typename AssetType, typename KeyType>
	FORCEINLINE AssetType* GetCachedAsset(
		const TMap<KeyType, TObjectPtr<AssetType>>& CacheMap,
		const KeyType& Key,
		const KeyType& InvalidKey,
		const TCHAR* CallerName
	) const;

	UPROPERTY()
	TMap<FGameplayTag, TObjectPtr<UEnvironmentAsset>> CachedEnvironmentAssets;

	UPROPERTY()
	TMap<EHumanCharacterName, TObjectPtr<UCharacterHumanAsset>> CachedCharacterHumanAssets;

	UPROPERTY()
	TMap<EMutantCharacterName, TObjectPtr<UCharacterMutantAsset>> CachedCharacterMutantAssets;

	UPROPERTY()
	TMap<EEquipmentName, TObjectPtr<UEquipmentAsset>> CachedEquipmentAssets;

	UPROPERTY()
	TMap<EDNA2, TObjectPtr<UDNAAsset2>> CachedDNAAssets;
};
