#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "LoadoutItem.generated.h"

enum class EEquipmentName : uint8;

UCLASS()
class MUTATEARENA_API ULoadoutItem : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	class UCommonTextBlock* TranslatedShowName;
	UPROPERTY()
	FString ShowName;

	UPROPERTY(meta = (BindWidget))
	class UCommonLazyImage* ShowImg;

	UPROPERTY()
	EEquipmentName EquipmentName;
	
};
