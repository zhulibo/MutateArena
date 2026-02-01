#include "AssetSubsystem.h"

#include "AssetThread.h"
#include "DataAssetManager.h"
#include "Engine/StreamableManager.h"
#include "MutateArena/Assets/Data/CommonAsset.h"
#include "MutateArena/Characters/Data/InputAsset.h"
#include "MutateArena/Characters/Data/CharacterAsset.h"
#include "MutateArena/Equipments/Data/EquipmentAsset.h"

void UAssetSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	// AssetThread* Runnable = new AssetThread(this);
	// FRunnableThread* Thread = FRunnableThread::Create(Runnable, TEXT("AssetThread"));

	double StartTime = FPlatformTime::Seconds();
	
	CommonAssetId = GenerateId(UCommonAsset::StaticClass());
	InputAssetId = GenerateId(UInputAsset::StaticClass());
	CharacterAssetId = GenerateId(UCharacterAsset::StaticClass());
	EquipmentAssetId = GenerateId(UEquipmentAsset::StaticClass());

	// Sync load asset
	FSoftObjectPath CommonAssetPath = UDataAssetManager::Get().GetPrimaryAssetPath(CommonAssetId);
	FSoftObjectPath InputAssetPath = UDataAssetManager::Get().GetPrimaryAssetPath(InputAssetId);

	CommonAsset = UDataAssetManager::Get().GetAsset(TSoftObjectPtr<UCommonAsset>(CommonAssetPath));
	InputAsset = UDataAssetManager::Get().GetAsset(TSoftObjectPtr<UInputAsset>(InputAssetPath));

	double EndTime = FPlatformTime::Seconds();
	UE_LOG(LogTemp, Warning, TEXT("Sync load asset time: %f seconds"), EndTime - StartTime);

	// Async load asset
	TArray<FPrimaryAssetId> AssetIds;
	AssetIds.Emplace(CharacterAssetId);
	AssetIds.Emplace(EquipmentAssetId);
	UDataAssetManager::Get().LoadPrimaryAssets(AssetIds, TArray<FName>(), FStreamableDelegate::CreateUObject(this, &ThisClass::LoadCompleted));
}

FPrimaryAssetId UAssetSubsystem::GenerateId(UClass* Class)
{
	return FPrimaryAssetId(FPrimaryAssetType(Class->GetFName()), FName(TEXT("DA_") + Class->GetFName().ToString()));
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
