#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Storage/ConfigType.h"
#include "DevSetting.generated.h"

enum class EEquipmentName : uint8;

UCLASS(Config = EditorPerProjectUserSettings)
class MUTATEARENA_API UDevSetting : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditAnywhere, Category = "EditorMaps", meta=(AllowedClasses="/Script/Engine.World"))
	TArray<FSoftObjectPath> EditorMaps;

	UPROPERTY(Config, EditAnywhere, Category = "Mutation")
	bool bUseMutationSettings = true;
	UPROPERTY(Config, EditAnywhere, Category = "Mutation")
	int32 TotalRound = 3;
	UPROPERTY(Config, EditAnywhere, Category = "Mutation")
	float WarmupTime = 3.f;
	UPROPERTY(Config, EditAnywhere, Category = "Mutation")
	float RoundTime = 100.f;
	UPROPERTY(Config, EditAnywhere, Category = "Mutation")
	float MutateTime = 5.f;
	UPROPERTY(Config, EditAnywhere, Category = "Mutation")
	float PostRoundTime = 3.f;
	UPROPERTY(Config, EditAnywhere, Category = "Mutation")
	float CooldownTime = 3.f;
	UPROPERTY(Config, EditAnywhere, Category = "Mutation")
	int32 MutateClientIndex = 2;

	UPROPERTY(Config, EditAnywhere, Category = "Equipment")
	bool bIsUseCustomEquipment = true;
	UPROPERTY(Config, EditAnywhere, Category = "Equipment")
	EEquipmentName PrimaryEquipment = EEquipmentName::AK47;
	UPROPERTY(Config, EditAnywhere, Category = "Equipment")
	EEquipmentName SecondaryEquipment = EEquipmentName::Glock17;
	UPROPERTY(Config, EditAnywhere, Category = "Equipment")
	EEquipmentName MeleeEquipment = EEquipmentName::Kukri;
	UPROPERTY(Config, EditAnywhere, Category = "Equipment")
	EEquipmentName ThrowingEquipment = EEquipmentName::Grenade;
	UPROPERTY(Config, EditAnywhere, Category = "Equipment")
	bool bIsAdjustEquipmentSocketTransform = false;

	UPROPERTY(Config, EditAnywhere, Category = "Dev")
	bool bKeepInMap = true;

};
