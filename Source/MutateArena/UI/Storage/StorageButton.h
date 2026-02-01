#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "StorageButton.generated.h"

enum class EHumanCharacterName : uint8;
enum class EEquipmentName : uint8;
enum class EEquipmentType : uint8;

UCLASS()
class MUTATEARENA_API UStorageButton : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	class UCommonTextBlock* TranslatedShowName;
	UPROPERTY()
	FString ShowName;

	UPROPERTY(meta = (BindWidget))
	class UCommonLazyImage* ShowImg;
	// Equipment img
	UPROPERTY()
	TSoftObjectPtr<UTexture2D> ShowImgPath;
	// Character full body img
	UPROPERTY()
	TSoftObjectPtr<UTexture2D> ShowImgFullBodyPath;
	
	UPROPERTY()
	EHumanCharacterName HumanCharacterName;

	UPROPERTY()
	EEquipmentName EquipmentName;
	UPROPERTY()
	EEquipmentType EquipmentType;

};
