#include "AssetSubsystem.h"

#include "AssetThread.h"
#include "DataAssetManager.h"
#include "Engine/StreamableManager.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/Assets/Data/CommonAsset.h"
#include "MutateArena/Characters/Data/InputAsset.h"
#include "MutateArena/Characters/Data/CharacterAsset.h"
#include "MutateArena/Equipments/Data/EquipmentAsset.h"

void UAssetSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	// AssetThread* Runnable = new AssetThread(this);
	// FRunnableThread* Thread = FRunnableThread::Create(Runnable, TEXT("AssetThread"));

	
	const FString Prefix = TEXT("DA_");
	CommonAssetId = FPrimaryAssetId(ASSET_COMMON, FName(Prefix + UCommonAsset::StaticClass()->GetFName().ToString()));
	InputAssetId = FPrimaryAssetId(ASSET_INPUT, FName(Prefix + UInputAsset::StaticClass()->GetFName().ToString()));
	CharacterAssetId = FPrimaryAssetId(ASSET_CHARACTER, FName(Prefix + UCharacterAsset::StaticClass()->GetFName().ToString()));
	EquipmentAssetId = FPrimaryAssetId(ASSET_EQUIPMENT, FName(Prefix + UEquipmentAsset::StaticClass()->GetFName().ToString()));

	double Time1 = FPlatformTime::Seconds();
	
	// Sync load asset
	CommonAsset = UDataAssetManager::Get().GetAsset<UCommonAsset>(CommonAssetId);
	InputAsset = UDataAssetManager::Get().GetAsset<UInputAsset>(InputAssetId);
	
	double Time2 = FPlatformTime::Seconds();
	
	CharacterAsset = UDataAssetManager::Get().GetAsset<UCharacterAsset>(CharacterAssetId);
	EquipmentAsset = UDataAssetManager::Get().GetAsset<UEquipmentAsset>(EquipmentAssetId);

	double Time3 = FPlatformTime::Seconds();
	UE_LOG(LogTemp, Warning, TEXT("Sync load time1 %f, time2 %f"), Time2 - Time1, Time3 - Time2);

	// Async load asset
	// TArray<FPrimaryAssetId> AssetIds;
	// AssetIds.Emplace(CharacterAssetId);
	// AssetIds.Emplace(EquipmentAssetId);
	// UDataAssetManager::Get().LoadPrimaryAssets(AssetIds, TArray<FName>(), FStreamableDelegate::CreateUObject(this, &ThisClass::LoadCompleted));
}

void UAssetSubsystem::LoadCompleted()
{
	CharacterAsset = UDataAssetManager::Get().GetPrimaryAssetObject<UCharacterAsset>(CharacterAssetId);
	EquipmentAsset = UDataAssetManager::Get().GetPrimaryAssetObject<UEquipmentAsset>(EquipmentAssetId);
}

void UAssetSubsystem::Test()
{
	UE_LOG(LogTemp, Warning, TEXT("Test"));
}
