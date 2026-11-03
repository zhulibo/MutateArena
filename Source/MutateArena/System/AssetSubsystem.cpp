#include "AssetSubsystem.h"
#include "DataAssetManager.h"
#include "Engine/StreamableManager.h"
#include "MutateArena/Assets/Data/CommonAsset.h"
#include "MutateArena/Characters/Data/InputAsset.h"
#include "MutateArena/Characters/Data/CharacterAsset.h"
#include "MutateArena/Characters/Data/DNAAsset2.h"
#include "MutateArena/Equipments/Data/EquipmentAsset.h"
#include "MutateArena/Maps/Environment/EnvironmentAsset.h"

void UAssetSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	SyncLoadAsset();

	BuildEnvironmentCache();
	BuildHumanCache();
	BuildMutantCache();
	BuildEquipmentCache();
	BuildDNACache();
}

void UAssetSubsystem::Deinitialize()
{
	CachedEnvironmentAssets.Empty();
	CachedHumanAssets.Empty();
	CachedMutantAssets.Empty();
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
		// 传入 false：因为返回后会直接赋给 UAssetSubsystem 的 UPROPERTY() 成员变量，不需要再往 DataAssetManager.LoadedAssets 里塞一份
		if (AssetType* Asset = UDataAssetManager::GetAsset<AssetType>(AssetId, false))
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

	// 一次性在底层把整个列表的所有主资产全部加载进内存
	TSharedPtr<FStreamableHandle> Handle = AssetManager.LoadPrimaryAssets(AssetIds, TArray<FName>());
	if (Handle.IsValid())
	{
		Handle->WaitUntilComplete();
	}

	OutCacheMap.Reserve(AssetIds.Num());
	for (const FPrimaryAssetId& AssetId : AssetIds)
	{
		// 从内存查找
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

void UAssetSubsystem::BuildHumanCache()
{
	BuildPrimaryAssetCache<UHumanAsset, EHumanName>(
		DataAsset::Human, CachedHumanAssets,
		[](const UHumanAsset* Asset) { return Asset->CharacterName; },
		EHumanName::None
	);
}

UHumanAsset* UAssetSubsystem::GetHumanAssetByName(EHumanName InName) const
{
	return GetCachedAsset(CachedHumanAssets, InName, EHumanName::None, TEXT("GetHumanAssetByName"));
}

void UAssetSubsystem::BuildMutantCache()
{
	BuildPrimaryAssetCache<UMutantAsset, EMutantName>(
		DataAsset::Mutant, CachedMutantAssets,
		[](const UMutantAsset* Asset) { return Asset->CharacterName; },
		EMutantName::None
	);
}

UMutantAsset* UAssetSubsystem::GetMutantAssetByName(EMutantName InName) const
{
	return GetCachedAsset(CachedMutantAssets, InName, EMutantName::None, TEXT("GetMutantAssetByName"));
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
