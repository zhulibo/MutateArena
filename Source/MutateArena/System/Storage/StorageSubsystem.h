#pragma once

#include "CoreMinimal.h"
#include "MutateArena/System/EOSSubsystem.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "StorageSubsystem.generated.h"

enum class ESoundClassType : uint8;

UCLASS()
class MUTATEARENA_API UStorageSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FString SlotSetting = TEXT("Slot1");
	UPROPERTY()
	FString SlotLoadout = TEXT("Slot2");

	UPROPERTY()
	class USaveGameSetting* CacheSetting;
	UPROPERTY()
	class USaveGameLoadout* CacheLoadout;
	void SaveSetting();
	void SaveLoadouts();

	void ApplySetting();
	void SetAudio(float Value, ESoundClassType SoundClassType);

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UPROPERTY()
	UEOSSubsystem* EOSSubsystem;
	UPROPERTY()
	uint32 UserIndex = 0;

	void CreateSaveGameSetting();
	void CreateSaveGameLoadout();
	
	FTimerHandle SaveSettingToCloudTimerHandle;
	void SaveSettingToCloud();
	
	FTimerHandle SaveLoadoutToCloudTimerHandle;
	void SaveLoadoutToCloud();
	
	void OnWriteFileComplete(bool bWasSuccessful);

	UPROPERTY()
	class UAssetSubsystem* AssetSubsystem;

};
