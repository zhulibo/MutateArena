#pragma once
#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "RadialMenuItem.generated.h"

struct FRadialMenuItemData;

UCLASS()
class MUTATEARENA_API URadialMenuItem : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	void InitData(const FRadialMenuItemData& Data);

	void SetSelectionState(bool bIsSelected);

protected:
	UPROPERTY(meta = (BindWidget))
	class UCommonTextBlock* ItemName;

	UPROPERTY(meta = (BindWidget))
	class UCommonLazyImage* ItemImage;
	
};
