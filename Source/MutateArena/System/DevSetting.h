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
	float WarmupTime = 2.f;
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
	
	UPROPERTY(Config, EditAnywhere, Category = "AFK")
	float AFKHostingTime = 30.f;
	
	UPROPERTY(Config, EditAnywhere, Category = "EditorMaps", meta=(AllowedClasses="/Script/Engine.World"))
	TArray<FSoftObjectPath> EditorMaps;

#if WITH_EDITOR
	virtual FText GetSectionText() const override;
#endif
	
};
