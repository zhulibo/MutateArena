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
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "EditorMaps", meta=(AllowedClasses="/Script/Engine.World"))
	TArray<FSoftObjectPath> EditorMaps;
	
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Mutation", meta=(AllowedClasses="/Script/Engine.World"))
	bool bUseMutationSettings = true;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Mutation", meta=(AllowedClasses="/Script/Engine.World"))
	int32 TotalRound = 3;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Mutation", meta=(AllowedClasses="/Script/Engine.World"))
	float WarmupTime = 3.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Mutation", meta=(AllowedClasses="/Script/Engine.World"))
	float RoundTime = 60.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Mutation", meta=(AllowedClasses="/Script/Engine.World"))
	float MutateTime = 5.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Mutation", meta=(AllowedClasses="/Script/Engine.World"))
	float PostRoundTime = 3.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Mutation", meta=(AllowedClasses="/Script/Engine.World"))
	float CooldownTime = 3.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Mutation", meta=(AllowedClasses="/Script/Engine.World"))
	int32 MutateClientIndex = 2;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Equipment", meta=(AllowedClasses="/Script/Engine.World"))
	bool bUseEquipmentSettings = true;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Equipment", meta=(AllowedClasses="/Script/Engine.World"))
	EEquipmentName PrimaryEquipment = EEquipmentName::AK47;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Equipment", meta=(AllowedClasses="/Script/Engine.World"))
	EEquipmentName SecondaryEquipment = EEquipmentName::Glock17;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Equipment", meta=(AllowedClasses="/Script/Engine.World"))
	EEquipmentName MeleeEquipment = EEquipmentName::Kukri;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Equipment", meta=(AllowedClasses="/Script/Engine.World"))
	EEquipmentName ThrowingEquipment = EEquipmentName::Grenade;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Dev", meta=(AllowedClasses="/Script/Engine.World"))
	bool bKeepInMap = true;
	
};
