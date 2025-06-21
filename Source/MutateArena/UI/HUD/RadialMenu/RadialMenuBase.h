#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "RadialMenuBase.generated.h"

UCLASS()
class MUTATEARENA_API URadialMenuBase : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	class UCommonTextBlock* Item1;
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* Item2;
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* Item3;
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* Item4;
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* Item5;
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* Item6;
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* Item7;
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* Item8;

	UPROPERTY()
	int32 SelectedItemIndex;
	UPROPERTY()
	int32 LastSelectedItemIndex;
	UFUNCTION()
	void SetSelectedItem(int32 Index);

};
