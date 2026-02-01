#include "StorageSubsystem.h"

#include "MutateArena/System/AssetSubsystem.h"
#include "SaveGameLoadout.h"
#include "SaveGameSetting.h"
#include "MutateArena/UI/Setting/TabAudio.h"
#include "MutateArena/Assets/Data/CommonAsset.h"
#include "Kismet/GameplayStatics.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

void UStorageSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	EOSSubsystem = GetGameInstance()->GetSubsystem<UEOSSubsystem>();
	if (EOSSubsystem)
	{
		EOSSubsystem->OnWriteUserFileComplete.AddUObject(this, &ThisClass::OnWriteFileComplete);
	}

	// 如果本地Setting存档存在
	if (UGameplayStatics::DoesSaveGameExist(SlotSetting, UserIndex))
	{
		USaveGameSetting* SaveGameSetting = Cast<USaveGameSetting>(UGameplayStatics::LoadGameFromSlot(SlotSetting, UserIndex));
		const USaveGameSetting* DefaultSaveGameSetting = GetDefault<USaveGameSetting>();
		// 如果版本号一致，把Setting放到缓存里
		if (SaveGameSetting && DefaultSaveGameSetting && SaveGameSetting->Version == DefaultSaveGameSetting->Version)
		{
			CacheSetting = SaveGameSetting;

			// 应用Setting数据
			ApplySetting();
		}
		// 创建Setting
		else
		{
			CreateSaveGameSetting();
		}
	}
	// 创建Setting
	else
	{
		CreateSaveGameSetting();
	}
	
	// 如果本地Loadout存档存在
	if (UGameplayStatics::DoesSaveGameExist(SlotLoadout, UserIndex))
	{
		USaveGameLoadout* SaveGameLoadout = Cast<USaveGameLoadout>(UGameplayStatics::LoadGameFromSlot(SlotLoadout, UserIndex));
		const USaveGameLoadout* DefaultSaveGameLoadout = GetDefault<USaveGameLoadout>();
		// 如果版本号一致，把Loadout放到缓存里
		if (SaveGameLoadout && DefaultSaveGameLoadout && SaveGameLoadout->Version == DefaultSaveGameLoadout->Version)
		{
			CacheLoadout = SaveGameLoadout;
		}
		// 创建Loadout
		else
		{
			CreateSaveGameLoadout();
		}
	}
	// 创建Loadout
	else
	{
		CreateSaveGameLoadout();
	}
}

// 创建Setting
void UStorageSubsystem::CreateSaveGameSetting()
{
	if (USaveGameSetting* SaveGameSetting = Cast<USaveGameSetting>(UGameplayStatics::CreateSaveGameObject(USaveGameSetting::StaticClass())))
	{
		// 保存Setting至缓存
		CacheSetting = SaveGameSetting;

		// 保存Setting至本地存档
		UGameplayStatics::SaveGameToSlot(CacheSetting, SlotSetting, UserIndex);

		// 应用Setting数据
		ApplySetting();
	}
}

// 创建Loadout
void UStorageSubsystem::CreateSaveGameLoadout()
{
	if (USaveGameLoadout* SaveGameLoadout = Cast<USaveGameLoadout>(UGameplayStatics::CreateSaveGameObject(USaveGameLoadout::StaticClass())))
	{
		// 保存Loadout至缓存
		CacheLoadout = SaveGameLoadout;

		// 保存Loadout至本地存档
		UGameplayStatics::SaveGameToSlot(CacheLoadout, SlotLoadout, UserIndex);
	}
}

// 保存Setting
void UStorageSubsystem::SaveSetting()
{
	if (CacheSetting)
	{
		// Save to disk
		UGameplayStatics::AsyncSaveGameToSlot(CacheSetting, SlotSetting, UserIndex);

		// Throttle save to Cloud
		GetWorld()->GetTimerManager().SetTimer(SaveSettingToCloudTimerHandle, this, &ThisClass::SaveSettingToCloud, 5.f);
	}
}

// 保存Setting到云端
void UStorageSubsystem::SaveSettingToCloud()
{
	if (EOSSubsystem == nullptr) EOSSubsystem = GetGameInstance()->GetSubsystem<UEOSSubsystem>();
	if (EOSSubsystem && CacheSetting)
	{
		TArray<uint8> FileContents;
		FMemoryWriter MemoryWriter(FileContents, true);
		FObjectAndNameAsStringProxyArchive Ar(MemoryWriter, false);
		Ar.ArIsSaveGame = false; // 无论属性设否设置了UPROPERTY(SaveGame)，都将进行序列化
		Ar.ArNoDelta = true;
		CacheSetting->Serialize(Ar);
	
		// 将本地存档保存到云端
		EOSSubsystem->WriteUserFile(SlotSetting, FileContents);
	}
}

// 保存Loadout
void UStorageSubsystem::SaveLoadouts()
{
	if (CacheLoadout)
	{
		// Save to disk
		UGameplayStatics::AsyncSaveGameToSlot(CacheLoadout, SlotLoadout, UserIndex);

		// Throttle save to Cloud
		GetWorld()->GetTimerManager().SetTimer(SaveLoadoutToCloudTimerHandle, this, &ThisClass::SaveLoadoutToCloud, 5.f);
	}
}

// 保存Loadout到云端
void UStorageSubsystem::SaveLoadoutToCloud()
{
	if (EOSSubsystem == nullptr) EOSSubsystem = GetGameInstance()->GetSubsystem<UEOSSubsystem>();
	if (EOSSubsystem && CacheLoadout)
	{
		TArray<uint8> FileContents;
		FMemoryWriter MemoryWriter(FileContents, true);
		FObjectAndNameAsStringProxyArchive Ar(MemoryWriter, false);
		Ar.ArIsSaveGame = false; // 无论属性设否设置了UPROPERTY(SaveGame)，都将进行序列化
		Ar.ArNoDelta = true;
		CacheLoadout->Serialize(Ar);
	
		// 将本地存档保存到云端
		EOSSubsystem->WriteUserFile(SlotLoadout, FileContents);
	}
}

// 保存到云端完成
void UStorageSubsystem::OnWriteFileComplete(bool bWasSuccessful)
{
	UE_LOG(LogTemp, Warning, TEXT("OnWriteFileComplete: %d"), bWasSuccessful);
}

// 应用设置
void UStorageSubsystem::ApplySetting()
{
	if (CacheSetting == nullptr) return;
	
	// 设置语言
	FInternationalization::Get().SetCurrentCulture(CacheSetting->Language);
	
	// 设置亮度
	GEngine->DisplayGamma = CacheSetting->Brightness;
	
	// 设置音量
	SetAudio(CacheSetting->MasterVolume, ESoundClassType::Master);
	SetAudio(CacheSetting->EffectsVolume, ESoundClassType::Effects);
	SetAudio(CacheSetting->MusicVolume, ESoundClassType::Music);
	SetAudio(CacheSetting->DialogueVolume, ESoundClassType::Dialogue);
}

// 设置音量（PIE是无效的，需要在独立进程条件下测试。）
void UStorageSubsystem::SetAudio(float Value, ESoundClassType SoundClassType)
{
	if (AssetSubsystem == nullptr) AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
	if (AssetSubsystem && AssetSubsystem->CommonAsset)
	{
		USoundClass* SoundClass;
		switch (SoundClassType)
		{
		case ESoundClassType::Master:
			SoundClass = AssetSubsystem->CommonAsset->MasterSound;
			break;
		case ESoundClassType::Effects:
			SoundClass = AssetSubsystem->CommonAsset->EffectsSound;
			break;
		case ESoundClassType::Music:
			SoundClass = AssetSubsystem->CommonAsset->MusicSound;
			break;
		case ESoundClassType::Dialogue:
			SoundClass = AssetSubsystem->CommonAsset->DialogueSound;
			break;
		default:
			SoundClass = AssetSubsystem->CommonAsset->MasterSound;
			break;
		}

		UGameplayStatics::SetSoundMixClassOverride(
			this,
			AssetSubsystem->CommonAsset->SoundMix,
			SoundClass,
			Value,
			1.f,
			0.2f,
			true
		);
	}
}
