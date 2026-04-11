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
	
	UDevSetting();
	
	UPROPERTY(Config, EditAnywhere, Category = "Equipment")
	bool bIsUseCustomEquipment = true;
	UPROPERTY(Config, EditAnywhere, Category = "Equipment")
	bool bIsAdjustEquipmentSocketTransform = false;
	UPROPERTY(Config, EditAnywhere, Category = "Equipment")
	EEquipmentName PrimaryEquipment = EEquipmentName::None;
	UPROPERTY(Config, EditAnywhere, Category = "Equipment")
	EEquipmentName SecondaryEquipment = EEquipmentName::None;
	UPROPERTY(Config, EditAnywhere, Category = "Equipment")
	EEquipmentName MeleeEquipment = EEquipmentName::None;
	UPROPERTY(Config, EditAnywhere, Category = "Equipment")
	EEquipmentName ThrowingEquipment = EEquipmentName::None;

	UPROPERTY(Config, EditAnywhere, Category = "Dev")
	bool bKeepInMap = true;

	UPROPERTY(Config, EditAnywhere, Category = "Mutation")
	bool bUseMutationSettings = true;
	UPROPERTY(Config, EditAnywhere, Category = "Mutation")
	int32 TotalRound = 3;
	UPROPERTY(Config, EditAnywhere, Category = "Mutation")
	int32 WarmupTime = 2;
	UPROPERTY(Config, EditAnywhere, Category = "Mutation")
	int32 RoundTime = 100;
	UPROPERTY(Config, EditAnywhere, Category = "Mutation")
	int32 MutateTime = 5;
	UPROPERTY(Config, EditAnywhere, Category = "Mutation")
	int32 PostRoundTime = 3;
	UPROPERTY(Config, EditAnywhere, Category = "Mutation")
	int32 CooldownTime = 3;
	UPROPERTY(Config, EditAnywhere, Category = "Mutation")
	int32 MutateClientIndex = 2;
	
	UPROPERTY(Config, EditAnywhere, Category = "AFK")
	int32 AFKHostingTime = 30;
	
	UPROPERTY(Config, EditAnywhere, Category = "EditorMaps", meta=(AllowedClasses="/Script/Engine.World"))
	TArray<FSoftObjectPath> EditorMaps;

#if WITH_EDITOR
	virtual FText GetSectionText() const override;
#endif
	
};
