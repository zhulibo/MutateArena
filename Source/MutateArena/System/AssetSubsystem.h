#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AssetSubsystem.generated.h"

class UEnvironmentAsset;

UCLASS()
class MUTATEARENA_API UAssetSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	void Test();
	
	UPROPERTY()
	class UCommonAsset* CommonAsset;
	UPROPERTY()
	class UInputAsset* InputAsset;
	UPROPERTY()
	class UCharacterAsset* CharacterAsset;
	UPROPERTY()
	class UEquipmentAsset* EquipmentAsset;

protected:
	// 编辑器中资源命名必须是 Prefix + 类名
	FPrimaryAssetId CommonAssetId;
	FPrimaryAssetId InputAssetId;
	FPrimaryAssetId CharacterAssetId;
	FPrimaryAssetId EquipmentAssetId;

	void SyncLoadAsset();
	
	// 环境资产缓存
public:
	void BuildEnvironmentCacheAsync();
	void OnEnvLoadCompleted(TArray<FPrimaryAssetId> LoadedAssetIds);
	UEnvironmentAsset* GetEnvironmentAssetByTag(FGameplayTag InTag) const;

	UPROPERTY(Transient)
	TMap<FGameplayTag, UEnvironmentAsset*> CachedEnvironmentAssets;

};
