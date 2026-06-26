#include "DataAssetManager.h"

#include "Engine/Engine.h"

UDataAssetManager::UDataAssetManager()
{
}

void UDataAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();
}

UDataAssetManager& UDataAssetManager::Get()
{
	if (UDataAssetManager* Singleton = Cast<UDataAssetManager>(GEngine->AssetManager))
	{
		return *Singleton;
	}

	UE_LOG(LogTemp, Error, TEXT("Invalid AssetManagerClassName in DefaultEngine.ini. It must be set to UDataAssetManager!"));
	return *NewObject<UDataAssetManager>();
}

UObject* UDataAssetManager::SyncLoadAsset(const FSoftObjectPath& AssetPath)
{
	if (AssetPath.IsValid())
	{
		if (IsInitialized())
		{
			return GetStreamableManager().LoadSynchronous(AssetPath);
		}

		// Use LoadObject if asset manager isn't ready yet.
		return AssetPath.TryLoad();
	}

	return nullptr;
}

void UDataAssetManager::AddLoadedAsset(const UObject* Asset)
{
	if (ensureAlways(Asset))
	{
		FScopeLock LoadedAssetsLock(&LoadedAssetsCritical);
		LoadedAssets.Add(Asset);
	}
}

void UDataAssetManager::UnloadTrackedAsset(const UObject* Asset)
{
	if (Asset)
	{
		FScopeLock Lock(&LoadedAssetsCritical);
		LoadedAssets.Remove(Asset);
	}
}
