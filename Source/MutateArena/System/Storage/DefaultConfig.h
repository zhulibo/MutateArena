#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Characters/Data/CharacterType.h"
#include "ConfigType.h"
#include "MutateArena/Equipments/Data/EquipmentType.h"
#include "DefaultConfig.generated.h"

enum class EEquipmentName : uint8;

UCLASS()
class MUTATEARENA_API UDefaultConfig : public UObject
{
	GENERATED_BODY()

public:
	UDefaultConfig();

	UPROPERTY()
	EEquipmentName Primary = EEquipmentName::AK47;
	UPROPERTY()
	EEquipmentName Secondary = EEquipmentName::Glock17;
	UPROPERTY()
	EEquipmentName Melee = EEquipmentName::Kukri;
	UPROPERTY()
	EEquipmentName Throwing = EEquipmentName::Grenade;
	UPROPERTY()
	TArray<FLoadout> Loadouts;
	int32 LoadoutIndex = 0;

	UPROPERTY()
	EHumanCharacterName HumanCharacterName = EHumanCharacterName::Solider;
	UPROPERTY()
	EMutantCharacterName MutantCharacterName = EMutantCharacterName::Tank;

	UPROPERTY()
	FString Language;
	UPROPERTY()
	bool ObfuscatePlayerName = false;
	UPROPERTY()
	bool ObfuscateTextChat = false;

	UPROPERTY()
	float MouseSensitivity = 0.8f;
	UPROPERTY()
	bool MouseAimAssistSteering = false;
	UPROPERTY()
	bool MouseAimAssistSlowdown = false;
	UPROPERTY()
	float ControllerSensitivity = 3.f;
	UPROPERTY()
	bool ControllerAimAssistSteering = false;
	UPROPERTY()
	bool ControllerAimAssistSlowdown = false;

	EWindowMode::Type WindowMode = EWindowMode::Windowed;
	UPROPERTY()
	FIntPoint ScreenResolution = FIntPoint(1280, 720);
	UPROPERTY()
	float Brightness = 2.2f;

	UPROPERTY()
	float MasterVolume = 0.8f;
	UPROPERTY()
	float EffectsVolume = 1.f;
	UPROPERTY()
	float MusicVolume = 1.f;
	UPROPERTY()
	float DialogueVolume = 1.f;

};
