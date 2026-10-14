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

	BuildEnvironmentCacheAsync();
	BuildHumanCharacterCacheAsync();
	BuildMutantCharacterCacheAsync();
	BuildEquipmentCacheAsync();
	BuildEquipmentSkinCacheAsync();
	BuildDNACacheAsync();
}

void UAssetSubsystem::Test()
{
	UE_LOG(LogTemp, Warning, TEXT("Test"));
}

void UAssetSubsystem::SyncLoadAsset()
{
	const FString Prefix = TEXT("DA_");

	CommonAssetId = FPrimaryAssetId(DataAsset::Common, FName(*(Prefix + UCommonAsset::StaticClass()->GetName())));
	InputAssetId = FPrimaryAssetId(DataAsset::Input, FName(*(Prefix + UInputAsset::StaticClass()->GetName())));
	CharacterAssetId = FPrimaryAssetId(DataAsset::Character, FName(*(Prefix + UCharacterAsset::StaticClass()->GetName())));
	EquipmentCommonAssetId = FPrimaryAssetId(DataAsset::Equipment, FName(*(Prefix + UEquipmentCommonAsset::StaticClass()->GetName())));

	double Time1 = FPlatformTime::Seconds();

	CommonAsset = UDataAssetManager::Get().GetAsset<UCommonAsset>(CommonAssetId);
	InputAsset = UDataAssetManager::Get().GetAsset<UInputAsset>(InputAssetId);

	double Time2 = FPlatformTime::Seconds();

	CharacterAsset = UDataAssetManager::Get().GetAsset<UCharacterAsset>(CharacterAssetId);
	EquipmentCommonAsset = UDataAssetManager::Get().GetAsset<UEquipmentCommonAsset>(EquipmentCommonAssetId);

	double Time3 = FPlatformTime::Seconds();
	UE_LOG(LogTemp, Warning, TEXT("Sync load time1 %f, time2 %f"), Time2 - Time1, Time3 - Time2);
}

void UAssetSubsystem::BuildEnvironmentCacheAsync()
{
	CachedEnvironmentAssets.Empty();

	TArray<FPrimaryAssetId> AssetIds;
	UDataAssetManager::Get().GetPrimaryAssetIdList(FPrimaryAssetType(DataAsset::Environment), AssetIds);
	if (AssetIds.Num() == 0) return;

	UDataAssetManager::Get().LoadPrimaryAssets(
		AssetIds, TArray<FName>(),
		FStreamableDelegate::CreateWeakLambda(this, [this, AssetIds]()
		{
			for (const FPrimaryAssetId& AssetId : AssetIds)
			{
				if (UEnvironmentAsset* Asset = UDataAssetManager::Get().GetPrimaryAssetObject<UEnvironmentAsset>(AssetId))
				{
					if (Asset->EnvironmentTag.IsValid())
					{
						CachedEnvironmentAssets.Add(Asset->EnvironmentTag, Asset);
					}
				}
			}
		})
	);
}

UEnvironmentAsset* UAssetSubsystem::GetEnvironmentAssetByTag(FGameplayTag InTag) const
{
	if (!InTag.IsValid()) return nullptr;
	if (const TObjectPtr<UEnvironmentAsset>* Found = CachedEnvironmentAssets.Find(InTag))
	{
		return Found->Get();
	}
	return nullptr;
}

void UAssetSubsystem::BuildHumanCharacterCacheAsync()
{
	CachedHumanCharacterAssets.Empty();

	TArray<FPrimaryAssetId> AssetIds;
	UDataAssetManager::Get().GetPrimaryAssetIdList(FPrimaryAssetType(DataAsset::CharacterHuman), AssetIds);
	if (AssetIds.Num() == 0) return;

	UDataAssetManager::Get().LoadPrimaryAssets(
		AssetIds, TArray<FName>(),
		FStreamableDelegate::CreateWeakLambda(this, [this, AssetIds]()
		{
			for (const FPrimaryAssetId& AssetId : AssetIds)
			{
				if (UCharacterHumanAsset* Asset = UDataAssetManager::Get().GetPrimaryAssetObject<UCharacterHumanAsset>(AssetId))
				{
					if (Asset->CharacterName != EHumanCharacterName::None)
					{
						CachedHumanCharacterAssets.Add(Asset->CharacterName, Asset);
					}
				}
			}
		})
	);
}

UCharacterHumanAsset* UAssetSubsystem::GetCharacterHumanAssetByName(EHumanCharacterName InName) const
{
	if (InName == EHumanCharacterName::None) return nullptr;
	if (const TObjectPtr<UCharacterHumanAsset>* Found = CachedHumanCharacterAssets.Find(InName))
	{
		return Found->Get();
	}
	return nullptr;
}

void UAssetSubsystem::BuildMutantCharacterCacheAsync()
{
	CachedMutantCharacterAssets.Empty();

	TArray<FPrimaryAssetId> AssetIds;
	UDataAssetManager::Get().GetPrimaryAssetIdList(FPrimaryAssetType(DataAsset::CharacterMutant), AssetIds);
	if (AssetIds.Num() == 0) return;

	UDataAssetManager::Get().LoadPrimaryAssets(
		AssetIds, TArray<FName>(),
		FStreamableDelegate::CreateWeakLambda(this, [this, AssetIds]()
		{
			for (const FPrimaryAssetId& AssetId : AssetIds)
			{
				if (UCharacterMutantAsset* Asset = UDataAssetManager::Get().GetPrimaryAssetObject<UCharacterMutantAsset>(AssetId))
				{
					if (Asset->CharacterName != EMutantCharacterName::None)
					{
						CachedMutantCharacterAssets.Add(Asset->CharacterName, Asset);
					}
				}
			}
		})
	);
}

UCharacterMutantAsset* UAssetSubsystem::GetCharacterMutantAssetByName(EMutantCharacterName InName) const
{
	if (InName == EMutantCharacterName::None) return nullptr;
	if (const TObjectPtr<UCharacterMutantAsset>* Found = CachedMutantCharacterAssets.Find(InName))
	{
		return Found->Get();
	}
	return nullptr;
}

void UAssetSubsystem::BuildEquipmentCacheAsync()
{
	CachedEquipmentAssets.Empty();

	TArray<FPrimaryAssetId> AssetIds;
	UDataAssetManager::Get().GetPrimaryAssetIdList(FPrimaryAssetType(DataAsset::Equipment), AssetIds);
	if (AssetIds.Num() == 0) return;

	UDataAssetManager::Get().LoadPrimaryAssets(
		AssetIds, TArray<FName>(),
		FStreamableDelegate::CreateWeakLambda(this, [this, AssetIds]()
		{
			for (const FPrimaryAssetId& AssetId : AssetIds)
			{
				if (UEquipmentAsset* Asset = UDataAssetManager::Get().GetPrimaryAssetObject<UEquipmentAsset>(AssetId))
				{
					if (Asset->EquipmentName != EEquipmentName::None)
					{
						CachedEquipmentAssets.Add(Asset->EquipmentName, Asset);
					}
				}
			}
		})
	);
}

UEquipmentAsset* UAssetSubsystem::GetEquipmentAssetByName(EEquipmentName InName) const
{
	if (InName == EEquipmentName::None) return nullptr;
	if (const TObjectPtr<UEquipmentAsset>* Found = CachedEquipmentAssets.Find(InName))
	{
		return Found->Get();
	}
	return nullptr;
}

void UAssetSubsystem::BuildEquipmentSkinCacheAsync()
{
	CachedEquipmentSkinAssets.Empty();

	TArray<FPrimaryAssetId> AssetIds;
	UDataAssetManager::Get().GetPrimaryAssetIdList(FPrimaryAssetType(DataAsset::EquipmentSkin), AssetIds);
	if (AssetIds.Num() == 0) return;

	UDataAssetManager::Get().LoadPrimaryAssets(
		AssetIds, TArray<FName>(),
		FStreamableDelegate::CreateWeakLambda(this, [this, AssetIds]()
		{
			for (const FPrimaryAssetId& AssetId : AssetIds)
			{
				if (UEquipmentSkinAsset* Asset = UDataAssetManager::Get().GetPrimaryAssetObject<UEquipmentSkinAsset>(AssetId))
				{
					if (Asset->EquipmentName != EEquipmentName::None)
					{
						CachedEquipmentSkinAssets.Add(Asset->EquipmentName, Asset);
					}
				}
			}
		})
	);
}

UEquipmentSkinAsset* UAssetSubsystem::GetEquipmentSkinAssetByName(EEquipmentName InName) const
{
	if (InName == EEquipmentName::None) return nullptr;
	if (const TObjectPtr<UEquipmentSkinAsset>* Found = CachedEquipmentSkinAssets.Find(InName))
	{
		return Found->Get();
	}
	return nullptr;
}

void UAssetSubsystem::BuildDNACacheAsync()
{
	CachedDNAAssets.Empty();

	TArray<FPrimaryAssetId> AssetIds;
	UDataAssetManager::Get().GetPrimaryAssetIdList(FPrimaryAssetType(DataAsset::DNA), AssetIds);
	if (AssetIds.Num() == 0) return;

	UDataAssetManager::Get().LoadPrimaryAssets(
		AssetIds, TArray<FName>(),
		FStreamableDelegate::CreateWeakLambda(this, [this, AssetIds]()
		{
			for (const FPrimaryAssetId& AssetId : AssetIds)
			{
				if (UDNAAsset2* Asset = UDataAssetManager::Get().GetPrimaryAssetObject<UDNAAsset2>(AssetId))
				{
					if (Asset->DNA != EDNA2::None)
					{
						CachedDNAAssets.Add(Asset->DNA, Asset);
					}
				}
			}
		})
	);
}

void UAssetSubsystem::ResolveEquipmentAssets(EEquipmentName InName, const UEquipmentAsset*& OutBaseAsset, const UEquipmentSkinAsset*& OutSkinAsset) const
{
	OutBaseAsset = nullptr;
	OutSkinAsset = nullptr;

	if (InName == EEquipmentName::None) return;

	// 1. 先尝试作为原版基础武器查找
	OutBaseAsset = GetEquipmentAssetByName(InName);
	if (OutBaseAsset)
	{
		return; // 找到了，说明它是默认原版武器，没有皮肤资产
	}

	// 2. 如果不是基础武器，说明它是皮肤/补给箱变体，去查 SkinAsset
	OutSkinAsset = GetEquipmentSkinAssetByName(InName);
	if (OutSkinAsset)
	{
		// 通过皮肤里配好的 EquipmentParentName 拿到基础武器资产
		OutBaseAsset = GetEquipmentAssetByName(OutSkinAsset->EquipmentParentName);
	}
}

UDNAAsset2* UAssetSubsystem::GetDNAAssetByType(EDNA2 InDNA) const
{
	if (InDNA == EDNA2::None) return nullptr;
	if (const TObjectPtr<UDNAAsset2>* Found = CachedDNAAssets.Find(InDNA))
	{
		return Found->Get();
	}
	return nullptr;
}
