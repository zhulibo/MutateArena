#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MutateArena/Characters/Data/CharacterType.h"
#include "MutateArena/Equipments/Data/EquipmentType.h"
#include "AssetSubsystem.generated.h"

class UEnvironmentAsset;
class UCharacterHumanAsset;
class UCharacterMutantAsset;
class UEquipmentAsset;
class UEquipmentSkinAsset;
enum class EDNA2 : uint8;
class UDNAAsset2;

UCLASS()
class MUTATEARENA_API UAssetSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	void Test();

	UPROPERTY()
	TObjectPtr<class UCommonAsset> CommonAsset;

	UPROPERTY()
	TObjectPtr<class UInputAsset> InputAsset;

	UPROPERTY()
	TObjectPtr<class UCharacterAsset> CharacterAsset;

	UPROPERTY()
	TObjectPtr<class UEquipmentCommonAsset> EquipmentCommonAsset;

protected:
	// 编辑器中资源命名必须是 Prefix + 类名
	FPrimaryAssetId CommonAssetId;
	FPrimaryAssetId InputAssetId;
	FPrimaryAssetId CharacterAssetId;
	FPrimaryAssetId EquipmentCommonAssetId;

	void SyncLoadAsset();

public:
	void BuildEnvironmentCacheAsync();
	void BuildHumanCharacterCacheAsync();
	void BuildMutantCharacterCacheAsync();
	void BuildEquipmentCacheAsync();
	void BuildEquipmentSkinCacheAsync();
	void BuildDNACacheAsync();

	UEnvironmentAsset* GetEnvironmentAssetByTag(FGameplayTag InTag) const;
	UCharacterHumanAsset* GetCharacterHumanAssetByName(EHumanCharacterName InName) const;
	UCharacterMutantAsset* GetCharacterMutantAssetByName(EMutantCharacterName InName) const;
	UEquipmentAsset* GetEquipmentAssetByName(EEquipmentName InName) const;
	UEquipmentSkinAsset* GetEquipmentSkinAssetByName(EEquipmentName InName) const;
	void ResolveEquipmentAssets(EEquipmentName InName, const UEquipmentAsset*& OutBaseAsset, const UEquipmentSkinAsset*& OutSkinAsset) const;
	UDNAAsset2* GetDNAAssetByType(EDNA2 InDNA) const;

	UPROPERTY(Transient)
	TMap<FGameplayTag, TObjectPtr<UEnvironmentAsset>> CachedEnvironmentAssets;

	UPROPERTY(Transient)
	TMap<EHumanCharacterName, TObjectPtr<UCharacterHumanAsset>> CachedHumanCharacterAssets;

	UPROPERTY(Transient)
	TMap<EMutantCharacterName, TObjectPtr<UCharacterMutantAsset>> CachedMutantCharacterAssets;

	UPROPERTY(Transient)
	TMap<EEquipmentName, TObjectPtr<UEquipmentAsset>> CachedEquipmentAssets;

	UPROPERTY(Transient)
	TMap<EEquipmentName, TObjectPtr<UEquipmentSkinAsset>> CachedEquipmentSkinAssets;

	UPROPERTY(Transient)
	TMap<EDNA2, TObjectPtr<UDNAAsset2>> CachedDNAAssets;
};
