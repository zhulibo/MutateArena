#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MutateArena/Characters/Data/CharacterType.h"
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
	
	UPROPERTY(Config, EditAnywhere, Category = "Dev")
	bool bShowMiniMap = false;

	UPROPERTY(Config, EditAnywhere, Category = "Mutation")
	bool bUseMutationSettings = true;
	UPROPERTY(Config, EditAnywhere, Category = "Mutation")
	int32 TotalRound = 12;
	UPROPERTY(Config, EditAnywhere, Category = "Mutation")
	int32 WarmupTime = 1;
	UPROPERTY(Config, EditAnywhere, Category = "Mutation")
	int32 RoundTime = 180;
	UPROPERTY(Config, EditAnywhere, Category = "Mutation")
	int32 MutateTime = 5;
	UPROPERTY(Config, EditAnywhere, Category = "Mutation")
	int32 PostRoundTime = 7;
	UPROPERTY(Config, EditAnywhere, Category = "Mutation")
	int32 CooldownTime = 5;
	UPROPERTY(Config, EditAnywhere, Category = "Mutation")
	int32 MutateClientIndex = 2;
	
	UPROPERTY(Config, EditAnywhere, Category = "AFK")
	int32 AFKHostingTime = 300;
	
	UPROPERTY(Config, EditAnywhere, Category = "DNA")
	EDNA2 DNA1 = EDNA2::None;
	UPROPERTY(Config, EditAnywhere, Category = "DNA")
	EDNA2 DNA2 = EDNA2::None;
	
	UPROPERTY(Config, EditAnywhere, Category = "EditorMaps", meta=(AllowedClasses="/Script/Engine.World"))
	TArray<FSoftObjectPath> EditorMaps;

#if WITH_EDITOR
	virtual FText GetSectionText() const override;
#endif
	
};
