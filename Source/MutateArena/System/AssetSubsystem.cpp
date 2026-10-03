#include "AssetSubsystem.h"

#include "AssetThread.h"
#include "DataAssetManager.h"
#include "Engine/StreamableManager.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/Assets/Data/CommonAsset.h"
#include "MutateArena/Characters/Data/InputAsset.h"
#include "MutateArena/Characters/Data/CharacterAsset.h"
#include "MutateArena/Equipments/Data/EquipmentAsset.h"
#include "MutateArena/Maps/Environment/EnvironmentAsset.h"

void UAssetSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	// AssetThread* Runnable = new AssetThread(this);
	// FRunnableThread* Thread = FRunnableThread::Create(Runnable, TEXT("AssetThread"));
	
	SyncLoadAsset();
	
	BuildEnvironmentCacheAsync();
}

void UAssetSubsystem::Test()
{
	UE_LOG(LogTemp, Warning, TEXT("Test"));
}

void UAssetSubsystem::SyncLoadAsset()
{
	const FString Prefix = TEXT("DA_");
	CommonAssetId = FPrimaryAssetId(ASSET_COMMON, FName(Prefix + UCommonAsset::StaticClass()->GetFName().ToString()));
	InputAssetId = FPrimaryAssetId(ASSET_INPUT, FName(Prefix + UInputAsset::StaticClass()->GetFName().ToString()));
	CharacterAssetId = FPrimaryAssetId(ASSET_CHARACTER, FName(Prefix + UCharacterAsset::StaticClass()->GetFName().ToString()));
	EquipmentAssetId = FPrimaryAssetId(ASSET_EQUIPMENT, FName(Prefix + UEquipmentCommonAsset::StaticClass()->GetFName().ToString()));

	double Time1 = FPlatformTime::Seconds();
	
	CommonAsset = UDataAssetManager::Get().GetAsset<UCommonAsset>(CommonAssetId);
	InputAsset = UDataAssetManager::Get().GetAsset<UInputAsset>(InputAssetId);
	
	double Time2 = FPlatformTime::Seconds();
	
	CharacterAsset = UDataAssetManager::Get().GetAsset<UCharacterAsset>(CharacterAssetId);
	EquipmentAsset = UDataAssetManager::Get().GetAsset<UEquipmentCommonAsset>(EquipmentAssetId);

	double Time3 = FPlatformTime::Seconds();
	UE_LOG(LogTemp, Warning, TEXT("Sync load time1 %f, time2 %f"), Time2 - Time1, Time3 - Time2);
}

void UAssetSubsystem::BuildEnvironmentCacheAsync()
{
	CachedEnvironmentAssets.Empty();

	TArray<FPrimaryAssetId> EnvAssetIds;
	UDataAssetManager::Get().GetPrimaryAssetIdList(FPrimaryAssetType(ASSET_ENVIRONMENT), EnvAssetIds);
    
	if (EnvAssetIds.Num() == 0) return;

	UDataAssetManager::Get().LoadPrimaryAssets(
		EnvAssetIds,
		TArray<FName>(),
		FStreamableDelegate::CreateUObject(this, &ThisClass::OnEnvLoadCompleted, EnvAssetIds)
	);
}

void UAssetSubsystem::OnEnvLoadCompleted(TArray<FPrimaryAssetId> LoadedAssetIds)
{
	for (const FPrimaryAssetId& AssetId : LoadedAssetIds)
	{
		// 此时资产已被异步加载到内存，GetPrimaryAssetObject 是纯内存读取，绝对不会阻塞
		if (UEnvironmentAsset* EnvAsset = UDataAssetManager::Get().GetPrimaryAssetObject<UEnvironmentAsset>(AssetId))
		{
			if (EnvAsset->EnvironmentTag.IsValid())
			{
				CachedEnvironmentAssets.Add(EnvAsset->EnvironmentTag, EnvAsset);
			}
		}
	}
}

UEnvironmentAsset* UAssetSubsystem::GetEnvironmentAssetByTag(FGameplayTag InTag) const
{
	if (!InTag.IsValid())
	{
		return nullptr;
	}

	if (UEnvironmentAsset* const* FoundAsset = CachedEnvironmentAssets.Find(InTag))
	{
		return *FoundAsset;
	}
    
	return nullptr;
}
