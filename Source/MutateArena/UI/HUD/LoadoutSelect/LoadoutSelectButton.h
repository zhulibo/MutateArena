#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "LoadoutSelectButton.generated.h"

UCLASS()
class MUTATEARENA_API ULoadoutSelectButton : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	class UCommonTextBlock* Slot1EquipmentText;
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* Slot2EquipmentText;
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* Slot3EquipmentText;
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* Slot4EquipmentText;

};
