#include "AssetSubsystem.h"
#include "DataAssetManager.h"
#include "Engine/StreamableManager.h"
#include "MutateArena/Assets/Data/CommonAsset.h"
#include "MutateArena/Characters/Data/InputAsset.h"
#include "MutateArena/Characters/Data/CharacterCommonAsset.h"
#include "MutateArena/Characters/Data/DNAAsset2.h"
#include "MutateArena/Equipments/Data/EquipmentAsset.h"
#include "MutateArena/Maps/Environment/EnvironmentAsset.h"

void UAssetSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	SyncLoadAsset();

	BuildEnvironmentCache();
	BuildHumanCharacterCache();
	BuildMutantCharacterCache();
	BuildEquipmentCache();
	BuildDNACache();
}

void UAssetSubsystem::Deinitialize()
{
	CachedEnvironmentAssets.Empty();
	CachedCharacterHumanAssets.Empty();
	CachedCharacterMutantAssets.Empty();
	CachedEquipmentAssets.Empty();
	CachedDNAAssets.Empty();

	Super::Deinitialize();
}

template <typename AssetType>
AssetType* UAssetSubsystem::LoadSinglePrimaryAsset(const FName& PrimaryAssetTypeName)
{
	UDataAssetManager& AssetManager = UDataAssetManager::Get();
	TArray<FPrimaryAssetId> AssetIds;
	AssetManager.GetPrimaryAssetIdList(FPrimaryAssetType(PrimaryAssetTypeName), AssetIds);

	for (const FPrimaryAssetId& AssetId : AssetIds)
	{
		if (AssetType* Asset = UDataAssetManager::GetAsset<AssetType>(AssetId))
		{
			return Asset;
		}
	}

	UE_LOG(LogTemp, Error, TEXT("Failed to load single primary asset of type [%s]"), *PrimaryAssetTypeName.ToString());
	return nullptr;
}

void UAssetSubsystem::SyncLoadAsset()
{
	double Time1 = FPlatformTime::Seconds();

	CommonAsset = LoadSinglePrimaryAsset<UCommonAsset>(DataAsset::Common);
	InputAsset = LoadSinglePrimaryAsset<UInputAsset>(DataAsset::Input);

	double Time2 = FPlatformTime::Seconds();

	CharacterCommonAsset = LoadSinglePrimaryAsset<UCharacterCommonAsset>(DataAsset::CharacterCommon);
	EquipmentCommonAsset = LoadSinglePrimaryAsset<UEquipmentCommonAsset>(DataAsset::EquipmentCommon);

	double Time3 = FPlatformTime::Seconds();
	UE_LOG(LogTemp, Warning, TEXT("Sync load time1 %f, time2 %f"), Time2 - Time1, Time3 - Time2);
}

template <typename AssetType, typename KeyType, typename KeyExtractorFunc>
void UAssetSubsystem::BuildPrimaryAssetCache(
	const FName& PrimaryAssetTypeName,
	TMap<KeyType, TObjectPtr<AssetType>>& OutCacheMap,
	KeyExtractorFunc KeyExtractor,
	const KeyType& InvalidKey)
{
	OutCacheMap.Empty();

	double Time1 = FPlatformTime::Seconds();

	UDataAssetManager& AssetManager = UDataAssetManager::Get();
	TArray<FPrimaryAssetId> AssetIds;
	AssetManager.GetPrimaryAssetIdList(FPrimaryAssetType(PrimaryAssetTypeName), AssetIds);
	if (AssetIds.Num() == 0) return;

	TSharedPtr<FStreamableHandle> Handle = AssetManager.LoadPrimaryAssets(AssetIds, TArray<FName>());
	if (Handle.IsValid())
	{
		Handle->WaitUntilComplete();
	}

	OutCacheMap.Reserve(AssetIds.Num());
	for (const FPrimaryAssetId& AssetId : AssetIds)
	{
		if (AssetType* Asset = AssetManager.GetPrimaryAssetObject<AssetType>(AssetId))
		{
			KeyType Key = KeyExtractor(Asset);
			if (Key != InvalidKey)
			{
				OutCacheMap.Add(Key, Asset);
			}
		}
	}

	double Time2 = FPlatformTime::Seconds();
	UE_LOG(LogTemp, Warning, TEXT("BuildCache [%s] Sync load time %f, Count: %d"), *PrimaryAssetTypeName.ToString(), Time2 - Time1, OutCacheMap.Num());
}

template <typename AssetType, typename KeyType>
AssetType* UAssetSubsystem::GetCachedAsset(
	const TMap<KeyType, TObjectPtr<AssetType>>& CacheMap,
	const KeyType& Key,
	const KeyType& InvalidKey,
	const TCHAR* CallerName) const
{
	if (Key == InvalidKey) return nullptr;
	if (const TObjectPtr<AssetType>* Found = CacheMap.Find(Key))
	{
		return Found->Get();
	}

	UE_LOG(LogTemp, Warning, TEXT("%s: Asset not found! CacheMap.Num() = %d"), CallerName, CacheMap.Num());
	return nullptr;
}

void UAssetSubsystem::BuildEnvironmentCache()
{
	BuildPrimaryAssetCache<UEnvironmentAsset, FGameplayTag>(
		DataAsset::Environment, CachedEnvironmentAssets,
		[](const UEnvironmentAsset* Asset) { return Asset->EnvironmentTag; },
		FGameplayTag::EmptyTag
	);
}

UEnvironmentAsset* UAssetSubsystem::GetEnvironmentAssetByTag(FGameplayTag InTag) const
{
	return GetCachedAsset(CachedEnvironmentAssets, InTag, FGameplayTag::EmptyTag, TEXT("GetEnvironmentAssetByTag"));
}

void UAssetSubsystem::BuildHumanCharacterCache()
{
	BuildPrimaryAssetCache<UCharacterHumanAsset, EHumanCharacterName>(
		DataAsset::CharacterHuman, CachedCharacterHumanAssets,
		[](const UCharacterHumanAsset* Asset) { return Asset->CharacterName; },
		EHumanCharacterName::None
	);
}

UCharacterHumanAsset* UAssetSubsystem::GetCharacterHumanAssetByName(EHumanCharacterName InName) const
{
	return GetCachedAsset(CachedCharacterHumanAssets, InName, EHumanCharacterName::None, TEXT("GetCharacterHumanAssetByName"));
}

void UAssetSubsystem::BuildMutantCharacterCache()
{
	BuildPrimaryAssetCache<UCharacterMutantAsset, EMutantCharacterName>(
		DataAsset::CharacterMutant, CachedCharacterMutantAssets,
		[](const UCharacterMutantAsset* Asset) { return Asset->CharacterName; },
		EMutantCharacterName::None
	);
}

UCharacterMutantAsset* UAssetSubsystem::GetCharacterMutantAssetByName(EMutantCharacterName InName) const
{
	return GetCachedAsset(CachedCharacterMutantAssets, InName, EMutantCharacterName::None, TEXT("GetCharacterMutantAssetByName"));
}

void UAssetSubsystem::BuildEquipmentCache()
{
	BuildPrimaryAssetCache<UEquipmentAsset, EEquipmentName>(
		DataAsset::Equipment, CachedEquipmentAssets,
		[](const UEquipmentAsset* Asset) { return Asset->EquipmentName; },
		EEquipmentName::None
	);
}

UEquipmentAsset* UAssetSubsystem::GetEquipmentAssetByName(EEquipmentName InName) const
{
	return GetCachedAsset(CachedEquipmentAssets, InName, EEquipmentName::None, TEXT("GetEquipmentAssetByName"));
}

void UAssetSubsystem::BuildDNACache()
{
	BuildPrimaryAssetCache<UDNAAsset2, EDNA2>(
		DataAsset::DNA, CachedDNAAssets,
		[](const UDNAAsset2* Asset) { return Asset->DNA; },
		EDNA2::None
	);
}

UDNAAsset2* UAssetSubsystem::GetDNAAssetByType(EDNA2 InDNA) const
{
	return GetCachedAsset(CachedDNAAssets, InDNA, EDNA2::None, TEXT("GetDNAAssetByType"));
}
