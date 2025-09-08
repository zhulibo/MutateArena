#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "SaveGameSetting.generated.h"

enum class EMutantCharacterName : uint8;
enum class EHumanCharacterName : uint8;

UCLASS()
class MUTATEARENA_API USaveGameSetting : public USaveGame
{
	GENERATED_BODY()

public:
	USaveGameSetting();

	UPROPERTY()
	int32 Version = 1;

	UPROPERTY()
	FString Language;
	UPROPERTY()
	bool bHideSkins;
	UPROPERTY()
	bool bObfuscateName;
	UPROPERTY()
	bool bObfuscateTextChat;

	UPROPERTY()
	float MouseSensitivity;
	UPROPERTY()
	float ControllerSensitivity;

	EWindowMode::Type WindowMode;
	UPROPERTY()
	FIntPoint ScreenResolution;
	UPROPERTY()
	float Brightness;

	UPROPERTY()
	float MasterVolume;
	UPROPERTY()
	float EffectsVolume;
	UPROPERTY()
	float MusicVolume;
	UPROPERTY()
	float DialogueVolume;

};
