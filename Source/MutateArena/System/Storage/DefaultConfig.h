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
	bool bHideSkins = false;
	UPROPERTY()
	bool bObfuscateName = false;
	UPROPERTY()
	bool bObfuscateTextChat = false;

	UPROPERTY()
	float MouseSensitivity = 1.2f;
	UPROPERTY()
	float MouseSensitivityMaxMul = 4.f;

	UPROPERTY()
	float ControllerSensitivity = 3.4f;
	UPROPERTY()
	float ControllerSensitivityMaxMul = 3.f;

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
