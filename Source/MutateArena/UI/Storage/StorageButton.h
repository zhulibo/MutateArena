#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "StorageButton.generated.h"

enum class EHumanName : uint8;
enum class EEquipmentName : uint8;
enum class EEquipmentSlot : uint8;

UCLASS()
class MUTATEARENA_API UStorageButton : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	class UCommonTextBlock* ShowName;
	UPROPERTY()
	FText Name;

	UPROPERTY(meta = (BindWidget))
	class UCommonLazyImage* ShowImg;
	// Equipment img
	UPROPERTY()
	TSoftObjectPtr<UTexture2D> ShowImgPath;
	// Character full body img
	UPROPERTY()
	TSoftObjectPtr<UTexture2D> ShowImgFullBodyPath;
	
	UPROPERTY()
	EHumanName HumanName;

	UPROPERTY()
	EEquipmentName EquipmentName;
	UPROPERTY()
	EEquipmentSlot EquipmentSlot;

};
