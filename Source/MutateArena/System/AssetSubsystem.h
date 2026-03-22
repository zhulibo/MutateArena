#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AssetSubsystem.generated.h"

UCLASS()
class MUTATEARENA_API UAssetSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY()
	class UCommonAsset* CommonAsset;
	UPROPERTY()
	class UInputAsset* InputAsset;
	UPROPERTY()
	class UCharacterAsset* CharacterAsset;
	UPROPERTY()
	class UEquipmentAsset* EquipmentAsset;

	void LoadCompleted();

	void Test();

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// 编辑器中资源命名必须是 Prefix + 类名
	FPrimaryAssetId CommonAssetId;
	FPrimaryAssetId InputAssetId;
	FPrimaryAssetId CharacterAssetId;
	FPrimaryAssetId EquipmentAssetId;

};
