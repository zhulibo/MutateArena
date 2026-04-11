#pragma once

#include "CoreMinimal.h"
#include "CommonTextBlock.h"
#include "CommonUserWidget.h"
#include "ComboBoxItem.generated.h"

UCLASS()
class MUTATEARENA_API UComboBoxItem : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* ItemText;
	
};
