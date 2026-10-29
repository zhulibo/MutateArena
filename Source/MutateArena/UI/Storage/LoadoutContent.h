#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "LoadoutContent.generated.h"

UCLASS()
class MUTATEARENA_API ULoadoutContent : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	class ULoadoutItem* Slot1Item;
	UPROPERTY(meta = (BindWidget))
	ULoadoutItem* Slot2Item;
	UPROPERTY(meta = (BindWidget))
	ULoadoutItem* Slot3Item;
	UPROPERTY(meta = (BindWidget))
	ULoadoutItem* Slot4Item;
	UPROPERTY(meta = (BindWidget))
	ULoadoutItem* Slot5Item;
	UPROPERTY(meta = (BindWidget))
	ULoadoutItem* Slot6Item;

protected:
	virtual void NativeOnInitialized() override;
	
};
